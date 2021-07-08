/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-05
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Fileshare
{
    static Hashmap<uint32_t, std::vector<Fileinfo_t>> Remotefiles{};
    static std::vector<std::future<bool>> Pendingoperations{};
    static Hashmap<uint32_t, uint16_t> Remoteports{};
    static std::vector<Fileinfo_t> Localfiles{};
    static uint16_t Listenport{};
    static size_t Listensocket{};
    static std::string Seed{};

    // Helpers to manage the strings.
    static std::pair<std::string, std::string> Parsepath(const std::string &Relativepath)
    {
        std::smatch Result{};
        if (const auto Regex = std::regex("(.*)\\/(.+)", std::regex_constants::optimize); !std::regex_match(Relativepath, Result, Regex))
            return {};

        if (Result.size() < 3)
            return {};

        // Clean up the path.
        auto Tmp = std::make_pair(Result[1].str(), Result[2].str());
        if (Tmp.first.find("Ayria/Storage/") != std::string::npos) Tmp.first.erase(0, Tmp.first.find("Ayria/Storage/"));
        while (Tmp.first.front() == '.' || Tmp.first.front() == '/' || Tmp.first.front() == '\\') Tmp.first.erase(0, 1);
        while (Tmp.second.back() == '/' || Tmp.second.back() == '\\') Tmp.second.pop_back();

        return Tmp;
    }

    // For cleaner code.
    inline bool isEnabled()
    {
        return Global.Settings.enableFileshare;
    }

    // All paths are relative to the Ayria/Storage directory.
    const Fileinfo_t *addFile(const std::string &Relativepath, std::unordered_map<uint64_t, uint64_t> &&Tags)
    {
        Fileinfo_t File{};

        std::tie(File.Directory, File.Filename) = Parsepath(Relativepath);
        if (File.Directory.empty() || File.Filename.empty()) [[unlikely]] return {};

        const auto Path = va("./Ayria/Storage/%s/%s", File.Directory.c_str(), File.Filename.c_str());
        if (!FS::Fileexists(Path)) [[unlikely]] return {};

        File.Checksum = Hash::WW32(FS::Readfile(Path));
        File.FileID = Hash::WW32(Path + Seed);
        File.Filetags = std::move(Tags);
        File.Size = FS::Filesize(Path);

        // TODO(tcn): Insert into DB.

        return &Localfiles.emplace_back(File);
    }
    void remFile(const std::string &Relativepath)
    {
        const auto [Directory, Filename] = Parsepath(Relativepath);
        if (Directory.empty() || Filename.empty()) [[unlikely]] return;

        std::erase_if(Localfiles, [&](const auto &Item)
        {
            return Item.Filename == Filename && Item.Directory == Directory;
        });
    }
    void remFile(uint32_t FileID)
    {
        std::erase_if(Localfiles, [=](const auto &Item) { return Item.FileID == FileID; });
    }

    // Get a listing for another client or the ones we currently share.
    const std::vector<Fileinfo_t> *getListing(uint32_t AccountID)
    {
        if (!Remotefiles.contains(AccountID)) return {};
        return &Remotefiles[AccountID];
    }
    const std::vector<Fileinfo_t> &getCurrentlist()
    {
        return Localfiles;
    }

    // Initialize the filesharing and get our port.
    static uint16_t getLocalport()
    {
        // The system has been disabled by the user.
        if (!isEnabled()) return {};

        // Return an initialized port.
        if (Listenport) [[likely]] return Listenport;

        // Winsock 2 is needed for shutdown().
        WSADATA Unused; WSAStartup(MAKEWORD(2, 2), &Unused);

        // Standard TCP, nothing fancy.
        const auto Socket = socket(AF_INET, SOCK_STREAM, 0);

        // Bind to a random port.
        sockaddr_in Localhost{ AF_INET, NULL, {{.S_addr = htonl(INADDR_ANY)}} };
        auto Error = bind(Socket, (SOCKADDR *)&Localhost, sizeof(Localhost));

        // Fetch the bound address.
        int Size{ sizeof(Localhost) };
        Error |= getsockname(Socket, (SOCKADDR *)&Localhost, &Size);

        // Enable incoming connections.
        Error |= listen(Socket, 5);

        // Can't do much if there were any errors along the way.
        if (Error) { closesocket(Socket); return {}; }

        // Complete.
        Listenport = Localhost.sin_port;
        Listensocket = Socket;
        return Listenport;
    }

    // Accept requests from other clients.
    static void __cdecl Pollnetwork()
    {
        // Sanity checking, let the hardware branch-prediction deal with it.
        if (!isEnabled()) return;

        constexpr timeval Defaulttimeout{ NULL, 1 };
        auto Timeout{ Defaulttimeout };
        auto Count{ 1 };

        // Check incoming connections.
        FD_SET ReadFD{}; FD_SET(Listensocket, &ReadFD);
        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;

        // Send the socket for processing in another thread.
        const auto Socket = accept(Listensocket, NULL, NULL);
        Pendingoperations.emplace_back(Uploadfile(Socket));

        // Prune the operations storage.
        std::erase_if(Pendingoperations, [](const auto &Item) { return !Item.valid(); });
    }

    // Share information with the other clients.
    static void Sharelist()
    {
        // Need a port to share our files.
        const auto Shareport = getLocalport();
        if (!Shareport) return;

        auto Array = JSON::Array_t();
        Array.resize(Localfiles.size());

        for (const auto &Item : Localfiles)
        {
            auto Tags = JSON::Array_t();
            Tags.resize(Item.Filetags.size() * 2);
            for (const auto &Tag : lz::flatten(Item.Filetags))
                Tags.emplace_back(Tag);

            Array.emplace_back(JSON::Object_t({
                { "Directory", Item.Directory },
                { "Filename", Item.Filename },
                { "Checksum", Item.Checksum },
                { "FileID", Item.FileID },
                { "Size", Item.Size },
                { "Tags", Tags }
            }));
        }

        const auto Request = JSON::Dump(JSON::Object_t({
            { "Shareport", Shareport },
            { "Files", Array }
        }));
        Network::LAN::Publish("AYRIA::Filelisting", Request);
    }
    static void Querypeers(std::string_view Filename, std::pair<uint64_t, uint64_t> Tag)
    {
        const auto Request = JSON::Dump(JSON::Object_t({
            { "Secondarytag", Tag.second },
            { "Primarytag", Tag.first },
            { "Filename", Filename }
        }));
        Network::LAN::Publish("AYRIA::Filequery", Request);
    }

    // Callbacks for the LAN messages.
    static void __cdecl Filequery(unsigned int, const char *Message, unsigned int Length)
    {
        // Sanity checking, let the hardware branch-prediction deal with it.
        if (!isEnabled()) return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Secondarytag = Request.value<uint64_t>("Secondarytag");
        const auto Primarytag = Request.value<uint64_t>("Primarytag");
        const auto Filename = Request.value<std::string>("Filename");

        const auto Result = std::ranges::any_of(Localfiles, [&](Fileinfo_t &File)
        {
            if (File.Filetags.contains(Primarytag) && File.Filetags[Primarytag] == Secondarytag) return true;
            if (std::strstr(File.Filename.c_str(), Filename.c_str())) return true;
            return false;
        });

        if (Result) Sharelist();
    }
    static void __cdecl Filelisting(unsigned int AccountID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Shareport = Request.value<uint16_t>("Shareport");
        const auto Files = Request.value<JSON::Array_t>("Files");

        if (!Shareport) return;
        if (Files.empty()) return;
        Remoteports[AccountID] = Shareport;

        for (JSON::Value_t Item : Files)
        {
            if (!Item.contains("Directory")) continue;
            if (!Item.contains("Filename")) continue;
            if (!Item.contains("Checksum")) continue;
            if (!Item.contains("FileID")) continue;
            if (!Item.contains("Size")) continue;

            Fileinfo_t File;
            File.Directory = (std::string)Item["Directory"];
            File.Filename = (std::string)Item["Filename"];
            File.Checksum = Item["Checksum"];
            File.FileID = Item["FileID"];
            File.Size = Item["Size"];

            // Escape any odd paths.
            std::tie(File.Directory, File.Filename) = Parsepath(va("%s/%s", File.Directory.c_str(), File.Filename.c_str()));
            if (File.Directory.empty() || File.Filename.empty()) [[unlikely]] continue;

            const auto Array = Item.value<JSON::Array_t>("Tags");
            for (auto Chunked = lz::chunks(Array, 2); auto &&Chunk : Chunked)
            {
                File.Filetags[*Chunk.begin()] = *Chunk.end();
            }

            std::erase_if(Remotefiles[AccountID], [&](const auto &Tmp) { return Tmp.FileID == File.FileID; });
            Remotefiles[AccountID].emplace_back(File);
        }
    }

    // Download/upload a single file from/to another user.
    struct Request_t { uint32_t FileID, Size; };
    static std::future<bool> Uploadfile(size_t Socket)
    {
        const auto Lambda = [](size_t Socket) -> bool
        {
            Request_t Request{};
            auto Result = recv(Socket, (char *)&Request, sizeof(Request), NULL);
            if (Result < static_cast<int>(sizeof(Request))) { closesocket(Socket); return false; }
            shutdown(Socket, SD_RECEIVE);

            for (const auto &File : Localfiles)
            {
                if (File.FileID == Request.FileID && File.Size == Request.Size)
                {
                    const auto Path = va("./Ayria/Storage/%s/%s", File.Directory.c_str(), File.Filename.c_str());
                    const auto Handle = std::fopen(Path.c_str(), "rb");
                    if (!Handle) break;

                    const auto Buffer = static_cast<char *>(alloca(4096));
                    while (true)
                    {
                        const auto Size = std::fread(Buffer, 1, 4096, Handle);
                        if (Size == 0) break;

                        const auto Sent = send(Socket, Buffer, static_cast<int>(Size), NULL);
                        if (Sent == SOCKET_ERROR) break;

                        // Rewind if we couldn't send the whole buffer.
                        if (Sent < static_cast<int>(Size))
                            std::fseek(Handle, (Sent - Size), SEEK_CUR);
                    }

                    shutdown(Socket, SD_SEND);
                    closesocket(Socket);
                    std::fclose(Handle);
                    return true;
                }
            }

            closesocket(Socket);
            return false;
        };

        return std::async(std::launch::async, Lambda, Socket);
    }
    static std::future<bool> Downloadfile(uint32_t AccountID, uint32_t FileID)
    {
        const auto Lambda = [](uint32_t AccountID, uint32_t FileID) -> bool
        {
            if (!Remoteports.contains(AccountID)) return false;
            if (!Remotefiles.contains(AccountID)) return false;

            const auto Port = Remoteports[AccountID];
            const auto Address = Network::LAN::getPeer(Clientinfo::getSessionID(AccountID));
            const sockaddr_in IPAddress{ AF_INET, Port, {{.S_addr = Address.s_addr}} };

            for (const auto &File : Remotefiles[AccountID])
            {
                if (File.FileID == FileID)
                {
                    const auto Socket = socket(AF_INET, SOCK_STREAM, 0);
                    if (Socket == INVALID_SOCKET) return false;

                    auto Result = connect(Socket, (sockaddr *)&IPAddress, sizeof(IPAddress));
                    if (Result == SOCKET_ERROR) { closesocket(Socket); return false; }

                    const Request_t Request{ File.FileID, File.Size };
                    Result = send(Socket, reinterpret_cast<const char *>(&Request), sizeof(Request), NULL);
                    if (Result < static_cast<int>(sizeof(Request))) { closesocket(Socket); return false; }
                    shutdown(Socket, SD_SEND);

                    const auto Path = va("./Ayria/Storage/Remotefiles/%08X/%s/%s", AccountID, File.Directory.c_str(), File.Filename.c_str());
                    std::filesystem::create_directories(Path);

                    const auto Handle = std::fopen((Path + ".part").c_str(), "wb");
                    if (!Handle) { closesocket(Socket); return false; }

                    const auto Buffer = static_cast<char *>(alloca(4096));
                    size_t Writtensize = 0;

                    while (true)
                    {
                        const auto Length = recv(Socket, Buffer, 4096, NULL);
                        if (Length == SOCKET_ERROR) break;

                        std::fwrite(Buffer, Length, 1, Handle);
                        Writtensize += Length;

                        if (Writtensize >= File.Size) break;
                    }

                    shutdown(Socket, SD_RECEIVE);
                    closesocket(Socket);

                    std::fclose(Handle);
                    std::filesystem::rename((Path + ".part"), Path);

                    if (const auto Checksum = Hash::WW32(FS::Readfile(Path)); Checksum != File.Checksum)
                    {
                        Errorprint(va("Fileshare download from %08X (\"%s\") was corrupted.", AccountID, File.Filename.c_str()));
                        std::remove(Path.c_str());
                        return false;
                    }

                    return true;
                }
            }

            return false;
        };

        return std::async(std::launch::async, Lambda, AccountID, FileID);
    }

    // JSON API for sharing files with others.
    namespace API
    {
        static std::string __cdecl Sharefile(JSON::Value_t &&Request)
        {
            const auto Relativepath = Request.value<std::string>("Relativepath");
            const auto Filetags = Request.value<JSON::Array_t>("Filetags");

            if (Relativepath.empty()) [[unlikely]]
                return R"({ "Error" : "Missing Relativepath value" })";

            std::unordered_map<uint64_t, uint64_t> Tags{};
            for (auto Chunked = lz::chunks(Filetags, 2); auto &&Chunk : Chunked)
                Tags[*Chunk.begin()] = *Chunk.end();

            const auto Pointer = addFile(Relativepath, std::move(Tags));
            if (!Pointer) [[unlikely]]
                return R"({ "Error" : "Could not add file." })";

            return va(R"({ "FileID" : %u })", Pointer->FileID);
        }
        static std::string __cdecl Unlistfile(JSON::Value_t &&Request)
        {
            const auto Relativepath = Request.value<std::string>("Relativepath");
            const auto FileID = Request.value<uint32_t>("FileID");

            // Being able to identify the file is important..
            if (Relativepath.empty() && !FileID) [[unlikely]]
                return R"({ "Error" : "Missing Relativepath or FileID" })";

            if (FileID) remFile(FileID);
            else remFile(Relativepath);

            return "{}";
        }
        static std::string __cdecl Downloadfile(JSON::Value_t &&Request)
        {
            const auto AccountID = Request.value<uint32_t>("AccountID");
            const auto FileID = Request.value<uint32_t>("FileID");

            if (!AccountID || !FileID) [[unlikely]]
                return R"({ "Error" : "Missing AccountID or FileID" })";

            Pendingoperations.emplace_back(Fileshare::Downloadfile(AccountID, FileID));
            return "{}";
        }

        static std::string __cdecl Listlocalfiles(JSON::Value_t &&)
        {
            auto Array = JSON::Array_t();
            Array.resize(Localfiles.size());

            for (const auto &Item : Localfiles)
            {
                auto Tags = JSON::Array_t();
                Tags.resize(Item.Filetags.size() * 2);
                for (const auto &Tag : lz::flatten(Item.Filetags))
                    Tags.emplace_back(Tag);

                Array.emplace_back(JSON::Object_t({
                    { "Directory", Item.Directory },
                    { "Filename", Item.Filename },
                    { "Checksum", Item.Checksum },
                    { "FileID", Item.FileID },
                    { "Size", Item.Size },
                    { "Tags", Tags }
                }));
            }

            return JSON::Dump(Array);
        }
        static std::string __cdecl Listremotefiles(JSON::Value_t &&)
        {
            auto Results = JSON::Array_t();
            Results.resize(Remotefiles.size());

            for (const auto &[AccountID, Filelist] : Remotefiles)
            {
                auto Array = JSON::Array_t();
                Array.resize(Filelist.size());

                for (const auto &Item : Filelist)
                {
                    auto Tags = JSON::Array_t();
                    Tags.resize(Item.Filetags.size() * 2);
                    for (const auto &Tag : lz::flatten(Item.Filetags))
                        Tags.emplace_back(Tag);

                    Array.emplace_back(JSON::Object_t({
                        { "Directory", Item.Directory },
                        { "Filename", Item.Filename },
                        { "Checksum", Item.Checksum },
                        { "FileID", Item.FileID },
                        { "Size", Item.Size },
                        { "Tags", Tags }
                    }));
                }

                Results.emplace_back(JSON::Object_t({
                    { "AccountID", AccountID },
                    { "Files", Array }
                }));
            }

            return JSON::Dump(Results);
        }

        static std::string __cdecl Searchfileshare(JSON::Value_t &&Request)
        {
            const auto Secondarytag = Request.value<uint64_t>("Secondarytag");
            const auto Primarytag = Request.value<uint64_t>("Primarytag");
            const auto Filename = Request.value<std::string>("Filename");

            // Need something to identify the peer by.
            if (Filename.empty() && !Primarytag) [[unlikely]]
                return R"({ "Error" : "Missing Filename or Primarytag" })";

            Querypeers(Filename, { Primarytag, Secondarytag });
            return "{}";
        }
    }

    // Set up the service.
    void Initialize()
    {
        // Create a unique seed for this computer.
        Seed = Hash::Tiger192(Clientinfo::GenerateHWID());

        // Register the LAN callbacks.
        Network::LAN::Register("AYRIA::Filequery", Filequery);
        Network::LAN::Register("AYRIA::Filelisting", Filelisting);

        // Register the JSON endpoints.
        Backend::API::addEndpoint("Sharefile", API::Sharefile);
        Backend::API::addEndpoint("Unlistfile", API::Unlistfile);
        Backend::API::addEndpoint("Downloadfile", API::Downloadfile);
        Backend::API::addEndpoint("Listlocalfiles", API::Listlocalfiles);
        Backend::API::addEndpoint("Listremotefiles", API::Listremotefiles);
        Backend::API::addEndpoint("Searchfileshare", API::Searchfileshare);

        // Set up a periodic task to poll.
        Backend::Enqueuetask(250, Pollnetwork);
    }
}
