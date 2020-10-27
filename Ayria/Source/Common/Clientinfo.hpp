/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Clientinfo
{
    #pragma region Datatypes
    #pragma pack(push, 1)
    struct Networkclient_t
    {
        uint32_t NodeID;    // Ephemeral identifier.
        AccountID_t AccountID;
        char8_t Username[32];
    };
    #pragma pack(pop)
    #pragma endregion

    // Client core information.
    Account_t *getLocalclient();
    bool isClientonline(uint32_t ClientID);
    const std::vector<Networkclient_t> *getNetworkclients();
    const Networkclient_t *getNetworkclient(uint32_t NodeID);

    // Client crypto information.
    std::string_view getPublickey(uint32_t ClientID);
    std::string_view getHardwarekey();
    RSA *getSessionkey();

    // Initialize the subsystems.
    void Initialize_client();
    void Initialize_crypto();

    // JSON API handlers.
    inline std::string __cdecl Accountinfo(const char *)
    {
        const auto Localclient = getLocalclient();

        auto Object = nlohmann::json::object();
        Object["AccountID"] = Localclient->ID.Raw;
        Object["Locale"] = Localclient->Locale.asUTF8();
        Object["Username"] = Localclient->Username.asUTF8();

        return DumpJSON(Object);
    }
    inline std::string __cdecl LANClients(const char *)
    {
        const auto Localnetwork = *getNetworkclients();
        auto Array = nlohmann::json::array();

        for (const auto &Client : Localnetwork)
        {
            Array += { { "Username", Client.Username }, { "AccountID", Client.AccountID.Raw } };
        }

        return DumpJSON(Array);
    }
    inline std::string __cdecl getClient(const char *JSONString)
    {
        const auto Localnetwork = *getNetworkclients();
        const auto Request = ParseJSON(JSONString);
        auto Object = nlohmann::json::object();

        if (Request.contains("UserID"))
        {
            const uint32_t UserID = Request["UserID"];
            for (const auto &Client : Localnetwork)
            {
                if (Client.AccountID.AccountID == UserID)
                {
                    Object = { { "Username", Client.Username }, { "AccountID", Client.AccountID.Raw } };
                    break;
                }
            }
        }
        if (Object.empty() && Request.contains("Username"))
        {
            const std::u8string Username = Request["Username"];
            for (const auto &Client : Localnetwork)
            {
                if (Client.Username == Username)
                {
                    Object = { { "Username", Client.Username }, { "AccountID", Client.AccountID.Raw } };
                    break;
                }
            }
        }

        return DumpJSON(Object);
    }
    inline void API_Initialize()
    {
        Initialize_client();
        Initialize_crypto();

        API::Registerhandler_Client("Accountinfo", Accountinfo);
        API::Registerhandler_Client("LANClients", LANClients);
        API::Registerhandler_Client("getClient", getClient);
    }
}
