/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Userinfo
{
    std::string_view getB64HardwareID();
    std::string_view getB64Privatekey();
    std::string_view getB64Sharedkey();
    Account_t *getAccount();
    void Initialize();

    // API export.
    std::string __cdecl Accountinfo(const char *);
}

namespace Clientinfo
{
    #pragma pack(push, 1)
    struct Client_t
    {
        AccountID_t AccountID;
        uint32_t NetworkID; // Ephemeral identifier.
        const char *B64Publickey;
        const char8_t *Username;
    };
    #pragma pack(pop)

    // Client information.
    const Client_t *getNetworkclient(uint32_t NetworkID);
    std::vector<const Client_t *> getNetworkclients();
    const Client_t *getClient(uint32_t ClientID);
    bool isOnline(uint32_t ClientID);
    void Initialize();

    // JSON API.
    inline std::string __cdecl getClients(const char *)
    {
        JSON::Array_t Array;

        const auto Clients = getNetworkclients();
        Array.reserve(Clients.size());

        for (const auto &Client : Clients)
        {
            Array.push_back(JSON::Object_t(
            {
                { "AccountID", Client->AccountID.Raw },
                { "Username", Client->Username },
                { "Sharedkey", Client->B64Publickey }
            }));
        }

        return JSON::Dump(Array);
    }

    inline void API_Initialize()
    {
        Clientinfo::Initialize();
        Userinfo::Initialize();

        API::Registerhandler_Client("Accountinfo", Userinfo::Accountinfo);
        API::Registerhandler_Client("getClients", getClients);
    }
}
