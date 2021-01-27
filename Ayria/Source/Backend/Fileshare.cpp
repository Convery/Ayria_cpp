/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend::Fileshare
{
    static Inlinedvector<Fileshare_t, 4> Mappedfiles{};
    static Hashmap<size_t, Fileshare_t *> Socketmap{};
    static Hashset<std::wstring> Storednames{};
    static bool Initialized = false;
    static FD_SET Activesockets{};

    [[noreturn]] static DWORD __stdcall Sharethread(void *)
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        constexpr timeval Defaulttimeout{ NULL, 100000 };

        while (true)
        {
            FD_SET ReadFD{ Activesockets};
            auto Timeout{ Defaulttimeout };
            const auto Count = Activesockets.fd_count;

            if (select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[unlikely]]
            {
                for (const auto &[Socket, Share] : Socketmap)
                {
                    if (FD_ISSET(Socket, &ReadFD))
                    {
                        const auto Newsocket = accept(Socket, nullptr, nullptr);
                        const auto Filepath = va(L"./Ayria/Storage/%08X/%ls", Share->OwnerID, Share->Filename);

                        if (const auto Filebuffer = FS::Readfile<char>(Filepath); !Filebuffer.empty())
                        {
                            send(Newsocket, Filebuffer.data(), int(Filebuffer.size()), NULL);
                        }
                        closesocket(Newsocket);
                    }
                }
            }
        }
    }
    static Blob Recv(std::string Address, uint16_t Port, uint32_t Filesize)
    {
        const sockaddr_in Host{ AF_INET, Port, {{.S_addr = inet_addr(Address.data())}} };

        const auto Socket = socket(AF_INET, SOCK_STREAM, 0);
        connect(Socket, (const sockaddr *)&Host, sizeof(Host));

        Blob Storage; Storage.reserve(Filesize);
        while (Storage.size() < Filesize)
        {
            uint8_t Temp[1024];
            const auto Size = recv(Socket, (char *)Temp, 1024, NULL);
            if (Size < NULL) [[unlikely]] return {};
            if (Size == NULL) [[unlikely]] break;

            Storage.append(Temp, Size);
        }

        closesocket(Socket);
        return Storage;
    }

    std::future<Blob> Download(std::string_view Address, uint16_t Port, uint32_t Filesize)
    {
        return std::async(std::launch::async, Recv, std::string(Address), Port, Filesize);
    }
    Fileshare_t *Upload(std::wstring_view Filepath)
    {
        const auto Filename = Filepath.substr(std::max(Filepath.find_last_of('/'), Filepath.find_last_of('\\')));

        for (auto &Mappedfile : Mappedfiles)
            if (Hash::WW32(Filename) == Hash::WW32(Mappedfile.Filename))
                return &Mappedfile;

        if (const auto Filebuffer = FS::Readfile(Filepath); !Filebuffer.empty())
        {
            sockaddr_in Localhost{ AF_INET, NULL, {{.S_addr = htonl(INADDR_ANY)}} };
            const auto Stored = Storednames.insert(std::wstring(Filename)).first;
            const auto Socket = socket(AF_INET, SOCK_STREAM, 0);
            const auto Checksum = Hash::WW32(Filebuffer);
            int Socksize = sizeof(Localhost);

            bind(Socket, (sockaddr *)&Localhost, sizeof(Localhost));
            getsockname(Socket, (sockaddr *)&Localhost, &Socksize);
            FD_SET(Socket, &Activesockets);
            listen(Socket, 10);

            const auto Share = &Mappedfiles.emplace_back(uint32_t(Filebuffer.size()), Checksum, Global.UserID, Stored->c_str(), Localhost.sin_port);
            Socketmap[Socket] = Share;

            if (!Initialized)
            {
                Initialized = true;
                CreateThread(NULL, NULL, Sharethread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
            }

            return Share;
        }

        return nullptr;
    }
    std::vector<Fileshare_t *> Listshares()
    {
        std::vector<Fileshare_t *> Result; Result.reserve(Mappedfiles.size());
        for (auto &Mappedfile : Mappedfiles)
            Result.push_back(&Mappedfile);
        return Result;
    }

    // API exports.
    static std::string __cdecl Uploadfile(const char *JSONString)
    {
        const auto Request = JSON::Parse(JSONString);
        const auto Filename = Request.value<std::wstring>("Filename");

        const auto Cleanname = Filename.substr(std::max(Filename.find_last_of('/'), Filename.find_last_of('\\')));
        const auto Filepath = va(L"./Ayria/Storage/%08X/%l*s", Global.UserID, Cleanname.size(), Cleanname.data());
        const auto Share = Upload(Filepath);

        auto Response = JSON::Object_t({ { "Resultcode", Share ? 200 : 404 } });
        if (Share)
        {
            Response["Filesize"] = Share->Filesize;
            Response["Checksum"] = Share->Checksum;
            Response["OwnerID"] = Share->OwnerID;
            Response["Port"] = Share->Port;
        }

        return JSON::Dump(Response);
    }
    static std::string __cdecl Downloadfile(const char *JSONString)
    {
        const auto Request = JSON::Parse(JSONString);
        const auto Filename = Request.value<std::string>("Filename");
        const auto Address = Request.value<std::string>("Address");
        const auto OwnerID = Request.value<size_t>("OwnerID");
        const auto Port = Request.value<uint16_t>("Port");
        const auto Size = Request.value<uint32_t>("Size");

        const auto Cleanname = Filename.substr(std::max(Filename.find_last_of('/'), Filename.find_last_of('\\')));
        const auto Filepath = va(L"./Ayria/Storage/%08X/%l*s", OwnerID, Cleanname.size(), Cleanname.data());

        std::thread([=]() -> void
        {
            Backend::setThreadname("Fileshare::Downloadfile");

            const auto Blob = Recv(Address, Port, Size);
            if (!Blob.empty()) FS::Writefile(Filepath, Blob);
        }).detach();

        return "{}";
    }

    void Initialize()
    {
        API::addHandler("Fileshare::Uploadfile", Uploadfile);
        API::addHandler("Fileshare::Downloadfile", Downloadfile);
    }
}
