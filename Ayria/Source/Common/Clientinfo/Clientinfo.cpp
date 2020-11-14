/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-07
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Clientinfo
{
    std::unordered_set<Client_t, decltype(FNV::Hash), decltype(FNV::Equal)> LANClients, WANClients;
    std::unordered_map<uint32_t, std::u8string> Usernames;
    std::unordered_map<uint32_t, std::string> Sharedkeys;
    bool hasDirtyclients{ true };

    // Client information.
    bool isOnline(AyriaID_t ClientID)
    {
        return isOnline(ClientID.AccountID);
    }
    bool isOnline(uint32_t AccountID)
    {
        if (std::any_of(std::execution::par_unseq, LANClients.begin(), LANClients.end(),
            [AccountID](const Client_t &Client) { return AccountID == Client.UserID.AccountID; }))
            return true;

        if (std::any_of(std::execution::par_unseq, WANClients.begin(), WANClients.end(),
            [AccountID](const Client_t &Client) { return AccountID == Client.UserID.AccountID; }))
            return true;

        return false;
    }
    const Client_t *getClient(AyriaID_t ClientID)
    {
        return getClient(ClientID.AccountID);
    }
    const Client_t *getClient(uint32_t AccountID)
    {
        for (const auto &Client : getNetworkclients())
            if (Client->UserID.AccountID == AccountID)
                return Client;

        return nullptr;
    }
    std::vector<const Client_t *> getNetworkclients()
    {
        static std::vector<const Client_t *> Result;
        if (!hasDirtyclients) return Result;

        Result.clear();
        Result.reserve(LANClients.size() + WANClients.size());

        for (auto it = LANClients.cbegin(); it != LANClients.cend(); ++it)
            Result.push_back(&*it);

        for (auto it = WANClients.cbegin(); it != WANClients.cend(); ++it)
            Result.push_back(&*it);

        hasDirtyclients = false;
        return Result;
    }
    const Client_t *getNetworkclient(uint32_t NetworkID)
    {
        for (const auto &Client : getNetworkclients())
            if (Client->NetworkID == NetworkID)
                return Client;

        return nullptr;
    }

    // API export.
    static void __cdecl Discoveryhandler(uint32_t NodeID, const char *JSONString)
    {
        Client_t Newclient{ 0, NodeID };
        const auto Object = JSON::Parse(JSONString);
        const auto UserID = Object.value("UserID", uint64_t());
        const auto Username = Object.value("Username", std::u8string());
        const auto Sharedkey = Object.value("Sharedkey", std::string());

        // WTF?
        if (!UserID || Username.empty()) return;

        Newclient.B64Sharedkey = Sharedkeys.emplace(Newclient.UserID.AccountID, Sharedkey).first->second.c_str();
        Newclient.Username = Usernames.emplace(Newclient.UserID.AccountID, Username).first->second.c_str();
        Newclient.UserID.Raw = UserID;

        // Check if the client is blocked.
        for (const auto &Relation : Social::Relationships::All())
        {
            if (Relation->UserID == Newclient.UserID.AccountID) [[unlikely]]
            {
                if (Relation->Flags.isBlocked) [[unlikely]]
                {
                    Backend::Blockclient(NodeID);
                    return;
                }
            }
        }

        LANClients.insert(std::move(Newclient));
        hasDirtyclients = true;
    }

    // Find remote clients.
    static void __cdecl Discoverremote()
    {
        // TODO(tcn): Poll a server and update WANClients.
    }

    void Initialize()
    {
        // Register the messagehandler.
        Backend::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);

        // Register a background event.
        Backend::Enqueuetask(5000, Discoverremote);
    }
}
