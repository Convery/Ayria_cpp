/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include "../Global.hpp"

namespace Client
{
    std::unordered_set<Client_t, decltype(FNV::Hash), decltype(FNV::Equal)> Clients{};
    std::string Clientname{};
    Blob Clientticket{};
    uint32_t ClientID{};
    RSA *Clientkey{};

    std::vector<Client_t> getClients()
    {
        std::vector<Client_t> Result; Result.reserve(Clients.size());
        for(const auto &Item : Clients)
        {
            Result.emplace_back(Item);
        }

        return Result;
    }

    void onGreeting(const char *Content)
    {
        if (!Base64::isValid(Content)) return;
        Client_t Newclient{};

        try
        {
            const auto Object = nlohmann::json::parse(Base64::Decode(Content));
            Newclient.Clientname = Object.value("Clientname", std::string());
            Newclient.Publickey = Object.value("Publickey", std::string());
            Newclient.ClientID = Object.value("ClientID", uint32_t());

            if (!Base64::isValid(Newclient.Publickey)) return;
            Clients.insert(Newclient);
        }
        catch(...){}
    }
    bool onStartup()
    {
        Clientname = "Ayria_offline";
        Clientkey = PK_RSA::Createkeypair(512);
        ClientID = Hash::FNV1_32(PK_RSA::getPublickey(Clientkey));

        // DEV(tcn): Load the clients name and ID from disk to override the defaults.
        {
            if (const auto Filebuffer = FS::Readfile("./Ayria/" + "Username.txt"s); !Filebuffer.empty())
            {
                Clientname = { Filebuffer.begin(), Filebuffer.end() };
            }
            if (const auto Filebuffer = FS::Readfile("./Ayria/" + "UserID.txt"s); !Filebuffer.empty())
            {
                ClientID = std::strtoul((char *)Filebuffer.c_str(), nullptr, 16);
            }
        }

        // Connect to some backend to get unique info.
        if constexpr (false)
        {
            // TODO(tcn): This.
        }

        // Handle new clients.
        Networking::addHandler("", onGreeting);

        // Greet the other nodes when found.
        auto Object = nlohmann::json::object();
        Object["Publickey"] = Base64::Encode(PK_RSA::getPublickey(Clientkey));
        Object["Clientname"] = Clientname;
        Object["ClientID"] = ClientID;
        Networking::addGreeting("Ayria", Object.dump());

        return ClientID != 0;
    }


    extern "C"
    {
        EXPORT_ATTR unsigned long __cdecl getClientID() { return ClientID; }
        EXPORT_ATTR const char * __cdecl getClientname() { return Clientname.c_str(); }
        EXPORT_ATTR short __cdecl getClientticket(unsigned short Bufferlength, unsigned char *Buffer)
        {
            if (Clientticket.empty()) return int16_t(0);
            if (Bufferlength < Clientticket.size()) return int16_t(Bufferlength - Clientticket.size());

            std::memcpy(Buffer, Clientticket.data(), Clientticket.size());
            return int16_t(Clientticket.size());
        }
    }
}
