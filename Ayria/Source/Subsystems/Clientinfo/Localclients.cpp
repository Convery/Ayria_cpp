/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Clientinfo
{
    Ayriaclient Localclient{ 0xDEADC0DE, "Ayria", "English" };
    std::unordered_set<Ayriaclient, decltype(FNV::Hash), decltype(FNV::Equal)> Localnetwork;

    // Fetch backend info.
    const Ayriaclient *getLocalclient()
    {
        return &Localclient;
    }

    namespace Internal
    {
        void Sendclientinfo()
        {
            const auto String = API::getLocalclient();
            Auxiliary::Sendmessage(Hash::FNV1_32("Clientdiscovery"), String);
        }
        void __cdecl Discoveryhandler(const char *JSONString)
        {
            Ayriaclient Newclient{};
            const auto Object = ParseJSON(JSONString);
            Newclient.ClientID = Object.value("ClientID", uint32_t());

            if (const auto Locale = Object.value("Locale", std::string()); !Locale.empty())
                std::strncpy(Newclient.Locale, Locale.c_str(), 7);

            if (const auto Username = Object.value("Username", std::string()); !Username.empty())
                std::strncpy(Newclient.Username, Username.c_str(), 19);

            if (Newclient.ClientID && Newclient.Username[0])
                Localnetwork.insert(std::move(Newclient));
        }

        static uint32_t Lastupdate{};
        void UpdateLocalclient()
        {
            const auto Currenttime = GetTickCount();
            if (Currenttime > (Lastupdate + 5000))
            {
                Lastupdate = Currenttime;

                // Announce our presence.
                Sendclientinfo();

                // TODO(tcn): Poll a server.
            }
        }
        void InitLocalclient()
        {
            if (const auto Filebuffer = FS::Readfile("./Ayria/Clientinfo.json"); !Filebuffer.empty())
            {
                const auto Object = ParseJSON(B2S(Filebuffer));
                Localclient.ClientID = Object.value("ClientID", Localclient.ClientID);

                if (const auto Locale = Object.value("Locale", std::string()); !Locale.empty())
                    std::strncpy(Localclient.Locale, Locale.c_str(), 7);

                if (const auto Username = Object.value("Username", std::string()); !Username.empty())
                    std::strncpy(Localclient.Username, Username.c_str(), 19);

                // Warn the user about bad configurations.
                if (!Object.contains("ClientID") || !Object.contains("Username"))
                    Warningprint("./Ayria/Clientinfo.json is misconfigured. Missing UserID or Username.");
            }

            // Listen for new clients.
            Auxiliary::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);
        }
    }

    namespace API
    {
        static std::string JSON_local;
        extern "C" EXPORT_ATTR const char *__cdecl getLocalclient()
        {
            auto Object = nlohmann::json::object();
            Object["Locale"] = Localclient.Locale;
            Object["ClientID"] = Localclient.ClientID;
            Object["Username"] = Localclient.Username;

            JSON_local = Object.dump();
            return JSON_local.c_str();
        }

        static std::string JSON_network;
        extern "C" EXPORT_ATTR const char *__cdecl getNetworkclients()
        {
            auto Object = nlohmann::json::object();

            for (const auto &Client : Localnetwork)
            {
                Object["Locale"] = Client.Locale;
                Object["ClientID"] = Client.ClientID;
                Object["Username"] = Client.Username;
            }

            JSON_network = Object.dump();
            return JSON_network.c_str();
        }
    }
}
