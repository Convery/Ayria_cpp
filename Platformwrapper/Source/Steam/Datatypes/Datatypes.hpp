/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Steam
{
    struct SteamID_t
    {
        enum class Accounttype_t : uint64_t
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
        enum class Universe_t : uint64_t
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
                Universe_t Universe : 8;
                Accounttype_t Accounttype : 4;

                uint64_t isClan : 1;
                uint64_t isLobby : 1;
                uint64_t isMMSLobby : 1;
                uint64_t RESERVED : 4;
                uint64_t SessionID : 12;

                uint64_t UserID : 32;
            };
        };

        SteamID_t toChatID() const
        {
            if (Accounttype == Accounttype_t::Clanaccount)
            {
                return SteamID_t
                {
                    .Universe = Universe, .Accounttype = Accounttype_t::Chataccount, .isClan = true,
                    .SessionID = SessionID, .UserID = UserID
                };
            }
            if (Accounttype == Accounttype_t::Chataccount)
            {
                return *this;
            }

            return {};
        }
        SteamID_t toClanID() const
        {
            if (Accounttype == Accounttype_t::Chataccount && isClan)
            {
                return SteamID_t
                {
                    .Universe = Universe, .Accounttype = Accounttype_t::Clanaccount,
                    .SessionID = SessionID, .UserID = UserID
                };
            }
            if(Accounttype == Accounttype_t::Clanaccount)
            {
                return *this;
            }

            return {};
        }
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

    using HServerListRequest = size_t; // void *

    using PublishedFileUpdateHandle_t = uint64_t;
    using UGCFileWriteStreamHandle_t = uint64_t;
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

#include "Resultcodes.hpp"
#include "Taskresults.hpp"
