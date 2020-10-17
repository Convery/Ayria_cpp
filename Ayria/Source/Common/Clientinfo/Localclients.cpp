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
    std::vector<Ayriaclient> Networkclients;

    // Backend access.
    Ayriaclient *getLocalclient() { return &Localclient; }
    std::vector<Ayriaclient> *getNetworkclients() { return &Networkclients; }

    // Internal helpers.
    static void Sendclientinfo()
    {
        const auto String = Accountinfo(nullptr);
        Backend::Sendmessage(Hash::FNV1_32("Clientdiscovery"), String);
    }
    static void __cdecl Discoveryhandler(uint32_t, const char *JSONString)
    {
        Ayriaclient Newclient{};
        const auto Object = ParseJSON(JSONString);
        Newclient.ClientID = Object.value("ClientID", uint32_t());

        if (const auto Locale = Object.value("Locale", std::string()); !Locale.empty())
            std::strncpy(Newclient.Locale, Locale.c_str(), 7);

        if (const auto Username = Object.value("Username", std::string()); !Username.empty())
            std::strncpy(Newclient.Username, Username.c_str(), 19);

        if (Newclient.ClientID && Newclient.Username[0])
        {
            std::erase_if(Networkclients, [&](const auto &Item) { return Newclient.ClientID == Item.ClientID; });
            Networkclients.emplace_back(std::move(Newclient));
        }
    }

    // Initialize and update.
    void Initialize()
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
        Backend::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);
    }

    static uint32_t Lastupdate{};
    void doFrame()
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
}
