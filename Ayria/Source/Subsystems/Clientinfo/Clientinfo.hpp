/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Clientinfo
{
    struct Ayriaclient
    {
        uint32_t ClientID;
        char Username[20];
        char Locale[8];
    };

    // Backend access.
    Ayriaclient *getLocalclient();
    std::vector<Ayriaclient> *getNetworkclients();

    // Initialize and update.
    void Initialize();
    void doFrame();

    // Add API handlers.
    inline std::string __cdecl Accountinfo(const char *)
    {
        const auto Localclient = getLocalclient();

        auto Object = nlohmann::json::object();
        Object["Locale"] = Localclient->Locale;
        Object["ClientID"] = Localclient->ClientID;
        Object["Username"] = Localclient->Username;

        return Object.dump();
    }
    inline std::string __cdecl LANClients(const char *)
    {
        const auto Localnetwork = *getNetworkclients();
        auto Object = nlohmann::json::object();

        for (const auto &Client : Localnetwork)
        {
            Object["Locale"] = Client.Locale;
            Object["ClientID"] = Client.ClientID;
            Object["Username"] = Client.Username;
        }

        return Object.dump();
    }
    inline void API_Initialize()
    {
        API::Registerhandler_Client("Accountinfo", Accountinfo);
        API::Registerhandler_Network("LANClients", LANClients);
    }
}

    //struct Networkclient
    //{
    //    uint32_t ClientID;
    //    uint32_t IPAddress;
    //    std::wstring Username;
    //};
    //struct Networkpool
    //{
    //    uint16_t Groupport;
    //    uint16_t Maxmembers;
    //    uint32_t Groupaddress;
    //    std::vector<Networkclient> Members;
    //};
    //struct Matchmakingsession : Networkpool
    //{
    //    union
    //    {
    //        uint8_t Flags;
    //        struct
    //        {
    //            uint8_t
    //                isHost : 1,
    //                isPublic : 1,
    //                isServer : 1,
    //                isRunning : 1,
    //                Compressedinfo : 1,
    //                RESERVED1 : 1,
    //                RESERVED2 : 1,
    //                RESERVED3 : 1;
    //        };
    //    };
    //    uint32_t Platform;
    //    std::wstring Servername;
    //    std::string Sessiondata;
    //};
