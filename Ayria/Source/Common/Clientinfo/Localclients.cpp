/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Clientinfo
{
    Account_t Localclient{ {.AccountID = 0xDEADC0DE }, u8"English"s, u8"Ayria"s };
    std::vector<Networkclient_t> Networkclients;

    // Client core information.
    Account_t *getLocalclient()
    {
        return &Localclient;
    }
    bool isClientonline(uint32_t ClientID)
    {
        for (const auto &Client : Networkclients)
            if (Client.AccountID.AccountID == ClientID)
                return true;
        return false;
    }

    const std::vector<Networkclient_t> *getNetworkclients()
    {
        return &Networkclients;
    }
    const Networkclient_t *getNetworkclient(uint32_t NodeID)
    {
        for (auto it = Networkclients.cbegin(); it != Networkclients.cend(); ++it)
            if (it->NodeID == NodeID)
                return &*it;

        return {};
    }

    // Internal helpers.
    static void __cdecl Sendclientinfo()
    {
        const auto String = Accountinfo(nullptr);
        Backend::Sendmessage(Hash::FNV1_32("Clientdiscovery"), String);
    }
    static void __cdecl Discoveryhandler(uint32_t NodeID, const char *JSONString)
    {
        Networkclient_t Newclient{ NodeID };
        const auto Object = JSON::Parse(JSONString);
        const auto AccountID = Object.value("AccountID", uint64_t());
        const auto Username = Object.value("Username", std::u8string());

        if (!AccountID || Username.empty()) return;
        Newclient.AccountID.Raw = Object.value("AccountID", uint64_t());
        std::memcpy(Newclient.Username, Username.data(), std::min(Username.size(), size_t(31)));

        // MSVC does not like custom unordered_set shared across translation-units -.-'
        std::erase_if(Networkclients, [&](const auto &Item) { return Item.NodeID == NodeID; });

        // Verify their relation to us.
        for (const auto &Relation : Social::Relations::Get())
        {
            if (Social::Relationflags_t{ Relation->Flags }.isBlocked) [[unlikely]]
            {
                if (Relation->AccountID == Newclient.AccountID.AccountID) [[unlikely]]
                {
                    Backend::Blockclient(NodeID);
                    return;
                }
            }
        }

        Networkclients.push_back(std::move(Newclient));
    }

    // Initialize the subsystems.
    void Initialize_client()
    {
        if (const auto Filebuffer = FS::Readfile<char>("./Ayria/Clientinfo.json"); !Filebuffer.empty())
        {
            const auto Object = JSON::Parse(Filebuffer);
            const auto Locale = Object.value("Locale", std::u8string());
            const auto Username = Object.value("Username", std::u8string());
            const auto AccountID = Object.value("AccountID", Localclient.ID.AccountID);

            // Warn the user about bad configurations.
            if (!Object.contains("AccountID") || !Object.contains("Username"))
                Warningprint("./Ayria/Clientinfo.json is misconfigured. Missing UserID or Username.");

            Localclient.ID.AccountID = AccountID;
            if (!Locale.empty()) Localclient.Locale = Locale;
            if (!Username.empty()) Localclient.Username = Username;
        }

        // Listen for new clients.
        Backend::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);

        // Register a background event.
        Backend::Enqueuetask(5000, Sendclientinfo);
    }
}
