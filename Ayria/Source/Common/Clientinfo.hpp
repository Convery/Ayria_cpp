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

    // Helpers for creating AccountIDs.
    inline AccountID_t Createaccount(Accountflags_t Type)
    {
        AccountID_t Client{};

        const auto Time = time(NULL); const auto UTC = gmtime(&Time);
        Client.Creationdate = (UTC->tm_year << 8) | (UTC->tm_mon << 4) | (UTC->tm_mday - 1);
        Client.AccountID = getLocalclient()->ID.AccountID;
        Client.Accounttype = Type;
        return Client;
    }
    inline AccountID_t Createserver()
    {
        Accountflags_t Flags{}; Flags.isServer = true;
        return Createaccount(Flags);
    }
    inline AccountID_t Creategroup()
    {
        Accountflags_t Flags{}; Flags.isGroup = true;
        return Createaccount(Flags);
    }

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
    inline void API_Initialize()
    {
        Initialize_client();
        Initialize_crypto();

        API::Registerhandler_Client("Accountinfo", Accountinfo);
        API::Registerhandler_Network("LANClients", LANClients);
    }
}
