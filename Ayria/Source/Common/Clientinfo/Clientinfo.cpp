/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-25
    License: MIT
*/

#include "../../Global.hpp"

namespace Client
{
    std::vector<Clientinfo_t> Remoteclients;
    Clientinfo_t Localclient;
    RSA *Localkeypair;

    namespace Messages
    {
        void Sendclientinfo()
        {
            auto Object = nlohmann::json::object();
            Object["Clientticket"] = Localclient.Clientticket;
            Object["Status"] = uint8_t(Localclient.Status);
            Object["Clientname"] = Localclient.Clientname;
            Object["ClientID"] = Localclient.ClientID;
            Object["Avatar"] = Localclient.Avatar;

            Object["Publickey"] = Base64::Encode(PK_RSA::getPublickey(Localkeypair));
            Networking::Core::Sendmessage("Ayria::Clientinfo", Object.dump());
        }

        std::unordered_set<sockaddr_in, decltype(FNV::Hash), decltype(FNV::Equal)> Greetednodes;
        void onClientinfo(sockaddr_in Client, const char *Content)
        {
            // Greet the node if we haven't.
            if (!Greetednodes.contains(Client)) [[unlikely]]
            {
                Greetednodes.insert(Client);
                Sendclientinfo();
            }

            try
            {
                Clientinfo_t Newclient;

                const auto Object = nlohmann::json::parse(Content);
                Newclient.Clientticket = Object.value("Clientticket", std::string());
                Newclient.Clientname = Object.value("Clientname", std::string());
                Newclient.Publickey = Object.value("Publickey", std::string());
                Newclient.ClientID = Object.value("ClientID", uint32_t());
                Newclient.Avatar = Object.value("Avatar", std::string());

                Newclient.Status = (decltype(Clientinfo_t::Status))(Object.value("Status", uint8_t()));
                if (Newclient.ClientID == 0 || Newclient.Clientname.empty()) return;

                const auto Inputhash = Hash::FNV1a_64(&Newclient, sizeof(Newclient));
                std::erase_if(Remoteclients, [=](const auto &Item)
                    {
                        return Hash::FNV1a_64(&Item, sizeof(Item)) == Inputhash;
                    });
                Remoteclients.emplace_back(Newclient);

            } catch (...) {}
        }
    }

    void onStartup()
    {
        Localclient.Avatar = {};
        Localclient.ClientID = 0;
        Localkeypair = PK_RSA::Createkeypair(512);
        Localclient.Clientname = va("Ayria_%d", GetCurrentProcessId());
        Localclient.ClientID = Hash::FNV1_32(PK_RSA::getPublickey(Localkeypair));

        // DEV(tcn): Load the clients name and ID from disk to override the defaults.
        {
            if (const auto Filebuffer = FS::Readfile("./Ayria/" + "Username.txt"s); !Filebuffer.empty())
            {
                Localclient.Clientname = { Filebuffer.begin(), Filebuffer.end() };
            }
            if (const auto Filebuffer = FS::Readfile("./Ayria/" + "UserID.txt"s); !Filebuffer.empty())
            {
                Localclient.ClientID = std::strtoul((char *)Filebuffer.c_str(), nullptr, 16);
            }
        }

        // Connect to some backend to get unique info.
        if constexpr (false)
        {
            // TODO(tcn): This.
        }

        // Notify other clients about startup.
        Messages::Sendclientinfo();

        // Listen for updates.
        Networking::Core::Registerhandler("Ayria::Clientinfo", Messages::onClientinfo);
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl getClientID(unsigned int *ClientID)
        {
            if (ClientID) *ClientID = Client::Localclient.ClientID;
        }
        EXPORT_ATTR void __cdecl getClientname(const char **Clientname)
        {
            if (Clientname) *Clientname = Client::Localclient.Clientname.c_str();
        }
        EXPORT_ATTR void __cdecl getClientticket(const char **Clientticket)
        {
            if (Clientticket) *Clientticket = Client::Localclient.Clientticket.c_str();
        }
        EXPORT_ATTR void __cdecl getLocalplayers(const char **Playerlist)
        {
            static std::string JSONString{"{}"};
            auto Parent = nlohmann::json::array();

            for (const auto &Localclient : Client::Remoteclients)
            {
                auto Object = nlohmann::json::object();
                Object["Clientticket"] = Localclient.Clientticket;
                Object["Status"] = uint8_t(Localclient.Status);
                Object["Clientname"] = Localclient.Clientname;
                Object["Publickey"] = Localclient.Publickey;
                Object["ClientID"] = Localclient.ClientID;
                Object["Avatar"] = Localclient.Avatar;

                Parent += Object;
            }

            JSONString = Parent.dump();
            *Playerlist = JSONString.c_str();
        }
    }
}
