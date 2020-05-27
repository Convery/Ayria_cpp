/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-25
    License: MIT
*/

#include "../../Global.hpp"

namespace Clients
{
    std::string Clientticket;
    Client_t Localclient;
    RSA *Localkeypair;


    Client_t getSelf()
    {
        return Localclient;
    }

    void onStartup()
    {
        Clientticket = "";
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
        Sendclientinfo();

        // Listen for updates.
        Networking::Core::Registerhandler("Ayria::Clientinfo", onClientinfo);
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl getClientID(unsigned int *ClientID)
        {
            if (ClientID) *ClientID = Clients::Localclient.ClientID;
        }
        EXPORT_ATTR void __cdecl getClientname(const char **Clientname)
        {
            if (Clientname) *Clientname = Clients::Localclient.Clientname.c_str();
        }
        EXPORT_ATTR void __cdecl getClientticket(const char **Clientticket)
        {
            if (Clientticket) *Clientticket = Clients::Clientticket.c_str();
        }
        EXPORT_ATTR void __cdecl getLocalplayers(const char **Playerlist)
        {
            static std::string JSONString{ "{}" };
            auto Array = nlohmann::json::array();

            for (const auto &Localclient : *Clients::getLocalclients())
            {
                auto Object = nlohmann::json::object();
                Object["Clientname"] = Localclient.Clientname;
                Object["Publickey"] = Localclient.Publickey;
                Object["ClientID"] = Localclient.ClientID;

                Array += Object;
            }

            JSONString = Array.dump();
            *Playerlist = JSONString.c_str();
        }
    }
}
