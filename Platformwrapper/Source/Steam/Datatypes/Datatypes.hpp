/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-02
    License: MIT
*/

#pragma once
#include "Resultcodes.hpp"
#include "Callbackdata.hpp"

namespace Steam
{
    struct SteamID_t
    {
        enum class Accounttype_t : uint8_t
        {
            Invalid = 0,
            Individual = 1,
            Gameserver = 3,
            Anonserver = 4,
            Contentserver = 6,
            Clanaccount = 7,
            Chataccount = 8,
            Consoleuser = 9,
            Anonymous = 10
        };
        enum class Environment_t : uint8_t
        {
            Invalid = 0,
            Public = 1,
            Beta = 2,
            Dev = 4
        };

        union
        {
            uint64_t FullID;
            struct
            {
                uint64_t
                    Universe : 8,
                    Accounttype : 4,
                    SessionID : 20,
                    UserID : 32;
            };
        };
    };
    struct GameID_t
    {
        enum class Gametype_t : uint8_t
        {
            Application = 0,
            Gamemod = 1,
            Shortcut = 2,
            P2P = 3
        };

        union
        {
            uint64_t FullID;
            struct
            {
                uint64_t
                    ModID : 32,
                    Type : 8,
                    AppID : 24;
            };
        };
    };

    using HServerListRequest = void *;

    using PublishedFileUpdateHandle_t = uint64_t;
    using SteamLeaderboardEntries_t = uint64_t;
    using SteamLeaderboard_t = uint64_t;
    using PublishedFileId_t = uint64_t;
    using PartyBeaconID_t = uint64_t;
    using SteamAPICall_t = uint64_t;
    using ManifestID_t = uint64_t;
    using UGCHandle_t = uint64_t;
    using GID_t = uint64_t;

    using ScreenshotHandle = uint32_t;
    using AccountID_t = uint32_t;
    using PartnerID_t = uint32_t;
    using HAuthTicket = uint32_t;
    using PackageId_t = uint32_t;
    using DepotID_t = uint32_t;
    using AppID_t = uint32_t;

    using HServerQuery = int32_t;
    using HSteamPipe = int32_t;
    using HSteamUser = int32_t;
    using HSteamCall = int32_t;

    using FriendsGroupID_t = int16_t;
}
