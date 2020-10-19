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
        uint32_t NodeID;    // Ephemeral identifier.
        uint32_t ClientID;
        char Username[16];
        char Locale[8];
    };

    // Backend access.
    Ayriaclient *getLocalclient();
    std::vector<Ayriaclient> *getNetworkclients();

    // Initialize and update.
    void Initialize();
    void doFrame();

    // Helper for network messages.
    inline uint32_t toClientID(uint32_t NodeID)
    {
        for (const auto &Client : *getNetworkclients())
            if (Client.NodeID == NodeID)
                return Client.ClientID;
        return 0;
    }

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

        Initialize();
    }
}
