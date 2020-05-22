/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include "../Global.hpp"

namespace Client
{
    std::vector<Client_t> Knownclients;
    std::string Clientticket{};
    std::string Clientname{};
    uint32_t ClientID{};
    RSA *Clientkey{};

    std::vector<Client_t> getClients()
    {
        return Knownclients;
    }
    void Clienthandler(const char *Content)
    {
        try
        {
            Traceprint();

            Client_t Newclient;

            const auto Object = nlohmann::json::parse(Content);
            Newclient.ClientID = Object.value("ClientID", uint32_t());
            Newclient.Publickey = Object.value("Publickey", std::string());
            Newclient.Clientname = Object.value("Clientname", std::string());
            if (Newclient.ClientID == 0 || Newclient.Clientname.empty()) return;

            const auto Inputhash = Hash::FNV1a_64(&Newclient, sizeof(Newclient));
            std::erase_if(Knownclients, [=](const auto &Item)
            {
                return Hash::FNV1a_64(&Item, sizeof(Item)) == Inputhash;
            });

            Social::onNewclient(Newclient);
            Knownclients.emplace_back(Newclient);

        } catch (...) {}
    }

    void onStartup()
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

        // Greet the other nodes when found.
        auto Object = nlohmann::json::object();
        Object["Publickey"] = Base64::Encode(PK_RSA::getPublickey(Clientkey));
        Object["Clientname"] = Clientname;
        Object["ClientID"] = ClientID;

        Network::addHandler("Client", Clienthandler);
        Network::addGreeting("Client", Object.dump());
        Network::addBroadcast("Client", Object.dump());
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl getClientID(unsigned int *ClientID)
        {
            if (ClientID) *ClientID = Client::ClientID;
        }
        EXPORT_ATTR void __cdecl getClientname(const char **Clientname)
        {
            if (Clientname) *Clientname = Client::Clientname.c_str();
        }
        EXPORT_ATTR void __cdecl getClientticket(const char **Clientticket)
        {
            if (Clientticket) *Clientticket = Client::Clientticket.c_str();
        }
    }
}
