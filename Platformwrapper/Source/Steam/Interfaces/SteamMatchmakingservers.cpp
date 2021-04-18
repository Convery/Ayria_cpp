/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-05-05
    License: MIT
*/

#include "../Steam.hpp"
#include <WinSock2.h>
#pragma warning(disable : 4100)

namespace Steam
{
    enum EMatchMakingServerResponse
    {
        eServerResponded = 0,
        eServerFailedToRespond,
        eNoServersListedOnMasterServer // for the Internet query type, returned in response callback if no servers of this type match
    };
    enum EMatchMakingType
    {
        eInternetServer = 0,
        eLANServer,
        eFriendsServer,
        eFavoritesServer,
        eHistoryServer,
        eSpectatorServer,
        eInvalidServer
    };

    // Callbacks.
    struct ISteamMatchmakingServerListResponse001
    {
        virtual void ServerResponded(int iServer) = 0;
        virtual void ServerFailedToRespond(int iServer) = 0;
        virtual void RefreshComplete(EMatchMakingServerResponse response) = 0;
    };
    struct ISteamMatchmakingServerListResponse002
    {
        virtual void ServerResponded(HServerListRequest hRequest, int iServer) = 0;
        virtual void ServerFailedToRespond(HServerListRequest hRequest, int iServer) = 0;
        virtual void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) = 0;
    };
    struct ISteamMatchmakingPingResponse
    {
        virtual void ServerResponded(gameserveritem_t *server) = 0;
        virtual void ServerFailedToRespond() = 0;
    };
    struct ISteamMatchmakingPlayersResponse
    {
        virtual void AddPlayerToList(const char *pchName, int nScore, float flTimePlayed) = 0;
        virtual void PlayersFailedToRespond() = 0;
        virtual void PlayersRefreshComplete() = 0;
    };
    struct ISteamMatchmakingRulesResponse
    {
        virtual void RulesResponded(const char *pchRule, const char *pchValue) = 0;
        virtual void RulesFailedToRespond() = 0;
        virtual void RulesRefreshComplete() = 0;
    };

    // SQL <-> Steam management.
    static Hashmap<uint32_t, gameserveritem_t, decltype(WW::Hash), decltype(WW::Equal)> Gameservers{};
    static sqlite::database Database()
    {
        try
        {
            sqlite::sqlite_config Config{};
            Config.flags = sqlite::OpenFlags::CREATE | sqlite::OpenFlags::READWRITE | sqlite::OpenFlags::FULLMUTEX;
            return sqlite::database("./Ayria/Client.db", Config);
        }
        catch (std::exception &e)
        {
            Errorprint(va("Could not connect to the database: %s", e.what()));
            return sqlite::database(":memory:");
        }
    }
    static void Pollservers()
    {
        static auto Lastupdate = GetTickCount();
        const auto Currenttime = GetTickCount();

        // Only update once in a while.
        if (Currenttime - Lastupdate < 5000) return;

        Gameservers.clear();

        Database() << "SELECT * FROM Matchmakingsessions WHERE ProviderID = ?;" << Hash::WW32("Steam")
            >> [](uint32_t ProviderID, uint32_t HostID, uint32_t Lastupdate, const std::string &B64Gamedata, const std::string &Servername,
                  uint32_t GameID, const std::string &Mapname, uint32_t IPv4, uint16_t Port)
        {
            // If the server has timed out..
            if (time(NULL) - Lastupdate > 30) [[unlikely]]
                return;

            const auto Object = JSON::Parse(Base64::Decode(B64Gamedata));
            gameserveritem_t Server{};

            Server.m_bPassword = Object.value<bool>("isPasswordprotected");
            Server.m_nMaxPlayers = Object.value<uint32_t>("Playermax");
            Server.m_nBotPlayers = Object.value<uint32_t>("Botcount");
            Server.m_nPlayers = Object.value<uint32_t>("Playercount");
            Server.m_bSecure = Object.value<bool>("isSecure");
            Server.m_bHadSuccessfulResponse = true;
            Server.m_bDoNotRefresh = true;
            Server.m_nAppID = GameID;
            Server.m_nPing = 33;

            Server.m_NetAdr.m_usConnectionPort = Object.value<uint16_t>("Gameport");
            Server.m_NetAdr.m_usQueryPort = Object.value<uint16_t>("Queryport");
            Server.m_NetAdr.m_unIP = IPv4;

            const auto Versionstring = Object.value<std::string>("Version");
            if (!Versionstring.empty())
            {
                uint8_t A{}, B{}, C{}, D{};
                std::sscanf(Versionstring.c_str(), "%hhu.%hhu.%hhu.%hhu", &A, &B, &C, &D);
                Server.m_nServerVersion |= A << 24;
                Server.m_nServerVersion |= B << 16;
                Server.m_nServerVersion |= C << 8;
                Server.m_nServerVersion |= D << 0;
            }

            Server.m_steamID.Accounttype = SteamID_t::Accounttype_t::Gameserver;
            Server.m_steamID.Universe = SteamID_t::Universe_t::Public;
            Server.m_steamID.UserID = HostID;

            std::strncpy(Server.m_szGameDescription, Object.value<std::string>("Gamedescription").c_str(), sizeof(Server.m_szGameDescription));
            std::strncpy(Server.m_szGameTags, Object.value<std::string>("Gametags").c_str(), sizeof(Server.m_szGameTags));
            std::strncpy(Server.m_szGameDir, Object.value<std::string>("Gamedir").c_str(), sizeof(Server.m_szGameDir));
            std::strncpy(Server.m_szServerName, Servername.c_str(), sizeof(Server.m_szServerName));
            std::strncpy(Server.m_szMap, Mapname.c_str(), sizeof(Server.m_szMap));

            Gameservers.emplace(HostID, Server);
        };
    }

    struct SteamMatchmakingServers
    {
        static Hashmap<HServerListRequest, EMatchMakingType> Requests;

        HServerListRequest RequestFavoritesServerList1(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(0, eNoServersListedOnMasterServer);
            return 0;
        }
        HServerListRequest RequestFriendsServerList1(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(0, eNoServersListedOnMasterServer);
            return 0;
        }
        HServerListRequest RequestHistoryServerList1(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(0, eNoServersListedOnMasterServer);
            return 0;
        }
        HServerListRequest RequestInternetServerList1(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            size_t Handle = GetTickCount();
            Requests.emplace(Handle, eInternetServer);

            Pollservers();
            int Serverindex{-1};

            for (const auto &[HostID, Server] : Gameservers)
            {
                const auto Octet = Server.m_NetAdr.m_unIP & 0xFF000000;
                Serverindex++;

                // Only want WAN servers.
                if (Octet == 192 || Octet == 10)
                    continue;

                // And servers for this game.
                if (Server.m_nAppID != iApp)
                    continue;

                pRequestServersResponse->ServerResponded(Handle, Serverindex);
            }

            pRequestServersResponse->RefreshComplete(Handle, Gameservers.empty() ? eNoServersListedOnMasterServer : eServerResponded);
            return Handle;
        }
        HServerListRequest RequestSpectatorServerList1(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(0, eNoServersListedOnMasterServer);
            return 0;
        }
        HServerListRequest RequestLANServerList1(AppID_t iApp, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            size_t Handle = GetTickCount();
            Requests.emplace(Handle, eLANServer);

            Pollservers();
            int Serverindex{-1};

            for (const auto &[HostID, Server] : Gameservers)
            {
                const auto Octet = Server.m_NetAdr.m_unIP & 0xFF000000;
                Serverindex++;

                // Only want LAN servers.
                if (Octet != 192 && Octet != 10)
                    continue;

                // And servers for this game.
                if (Server.m_nAppID != iApp)
                    continue;

                pRequestServersResponse->ServerResponded(Handle, Serverindex);
            }

            pRequestServersResponse->RefreshComplete(Handle, Gameservers.empty() ? eNoServersListedOnMasterServer : eServerResponded);
            return Handle;
        }

        HServerQuery PingServer(uint32_t unIP, uint16_t usPort, ISteamMatchmakingPingResponse *pRequestServersResponse)
        {
            for (auto It = Gameservers.begin(); It != Gameservers.end(); ++It)
            {
                if (It->second.m_NetAdr.m_unIP == unIP && It->second.m_NetAdr.m_usQueryPort == usPort)
                {
                    pRequestServersResponse->ServerResponded(&It->second);
                    return GetTickCount();
                }
            }

            pRequestServersResponse->ServerFailedToRespond();
            return GetTickCount();
        }
        HServerQuery PlayerDetails(uint32_t unIP, uint16_t usPort, ISteamMatchmakingPlayersResponse *pRequestServersResponse)
        {
            std::string B64Gamedata;

            Database() << "SELECT B64Gamedata FROM Matchmakingsessions WHERE ProviderID = ? AND IPv4 = ? AND Port = ?;"
                << Hash::WW32("Steam") << unIP << usPort >> B64Gamedata;

            // No such entry in the DB.
            if (B64Gamedata.empty())
            {
                pRequestServersResponse->PlayersFailedToRespond();
                return GetTickCount();
            }

            const auto Object = JSON::Parse(Base64::Decode(B64Gamedata));
            const auto Userdata = Object.value<JSON::Array_t>("Userdata");

            for (const auto &Item : Userdata)
            {
                const auto Username = Item.value<std::string>("Username");
                const auto Userscore = Item.value<uint32_t>("Userscore");

                pRequestServersResponse->AddPlayerToList(Username.c_str(), Userscore, 60.0f);
            }

            pRequestServersResponse->PlayersRefreshComplete();
            return GetTickCount();
        }
        HServerQuery ServerRules(uint32_t unIP, uint16_t usPort, ISteamMatchmakingRulesResponse *pRequestServersResponse)
        {
            std::string B64Gamedata;

            Database() << "SELECT B64Gamedata FROM Matchmakingsessions WHERE ProviderID = ? AND IPv4 = ? AND Port = ?;"
                       << Hash::WW32("Steam") << unIP << usPort >> B64Gamedata;

            // No such entry in the DB.
            if (B64Gamedata.empty())
            {
                pRequestServersResponse->RulesFailedToRespond();
                return GetTickCount();
            }

            const auto Object = JSON::Parse(Base64::Decode(B64Gamedata));
            const auto Keyvalues = Object.value<JSON::Object_t>("Keyvalues");

            for (const auto &[Key, Value] : Keyvalues)
            {
                pRequestServersResponse->RulesResponded(Key.c_str(), Value.get<std::string>().c_str());
            }

            pRequestServersResponse->RulesRefreshComplete();
            return GetTickCount();
        }

        bool IsRefreshing0(EMatchMakingType eType)
        {
            return false;
        }
        bool IsRefreshing1(HServerListRequest hRequest)
        {
            return false;
        }

        gameserveritem_t *GetServerDetails0(EMatchMakingType eType, int iServer)
        {
            for(auto It = Gameservers.begin(); It != Gameservers.end(); ++It)
            {
                const auto Octet = It->second.m_NetAdr.m_unIP & 0xFF000000;
                if (eType == eLANServer && (Octet == 192 || Octet == 10)) iServer--;
                if (eType == eInternetServer && (Octet != 192 && Octet != 10)) iServer--;

                if (iServer == -1) return &It->second;
            }

            return nullptr;
        }
        gameserveritem_t *GetServerDetails1(HServerListRequest hRequest, int iServer)
        {
            return GetServerDetails0(Requests[hRequest], iServer);
        }

        int GetServerCount0(EMatchMakingType eType)
        {
            Pollservers();
            int Servercount{};

            for (const auto &[HostID, Server] : Gameservers)
            {
                const auto Octet = Server.m_NetAdr.m_unIP & 0xFF000000;
                if (eType == eLANServer && (Octet == 192 || Octet == 10)) Servercount++;
                if (eType == eInternetServer && (Octet != 192 && Octet != 10)) Servercount++;
            }

            return Servercount;
        }
        int GetServerCount1(HServerListRequest hRequest)
        {
            return GetServerCount0(Requests[hRequest]);
        }

        void CancelQuery0(EMatchMakingType eType)
        {
        }
        void CancelQuery1(HServerListRequest hRequest)
        {
        }
        void CancelServerQuery(HServerQuery hServerQuery)
        {
        }
        void RefreshQuery0(EMatchMakingType eType)
        {
            Pollservers();
        }
        void RefreshQuery1(HServerListRequest hRequest)
        {
            Pollservers();
        }
        void RefreshServer0(EMatchMakingType eType, int iServer)
        {}
        void RefreshServer1(HServerListRequest hRequest, int iServer)
        {}
        void ReleaseRequest(HServerListRequest hServerListRequest)
        {
        }

        void RequestFavoritesServerList0(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(eNoServersListedOnMasterServer);
        }
        void RequestFriendsServerList0(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(eNoServersListedOnMasterServer);
        }
        void RequestHistoryServerList0(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(eNoServersListedOnMasterServer);
        }
        void RequestInternetServerList0(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            Pollservers();
            int Serverindex{-1};

            for (const auto &[HostID, Server] : Gameservers)
            {
                const auto Octet = Server.m_NetAdr.m_unIP & 0xFF000000;
                Serverindex++;

                // Only want WAN servers.
                if (Octet == 192 || Octet == 10)
                    continue;

                // And servers for this game.
                if (Server.m_nAppID != iApp)
                    continue;

                pRequestServersResponse->ServerResponded(Serverindex);
            }

            pRequestServersResponse->RefreshComplete(Gameservers.empty() ? eNoServersListedOnMasterServer : eServerResponded);
        }
        void RequestSpectatorServerList0(AppID_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            pRequestServersResponse->RefreshComplete(eNoServersListedOnMasterServer);
        }
        void RequestLANServerList0(AppID_t iApp, ISteamMatchmakingServerListResponse001 *pRequestServersResponse)
        {
            Pollservers();
            int Serverindex{-1};

            for (const auto &[HostID, Server] : Gameservers)
            {
                const auto Octet = Server.m_NetAdr.m_unIP & 0xFF000000;
                Serverindex++;

                // Only want LAN servers.
                if (Octet != 192 && Octet != 10)
                    continue;

                // And servers for this game.
                if (Server.m_nAppID != iApp)
                    continue;

                pRequestServersResponse->ServerResponded(Serverindex);
            }

            pRequestServersResponse->RefreshComplete(Gameservers.empty() ? eNoServersListedOnMasterServer : eServerResponded);
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamMatchmakingservers001 : Interface_t<16>
    {
        SteamMatchmakingservers001()
        {
            Createmethod(0, SteamMatchmakingServers, RequestInternetServerList0);
            Createmethod(1, SteamMatchmakingServers, RequestLANServerList0);
            Createmethod(2, SteamMatchmakingServers, RequestFriendsServerList0);
            Createmethod(3, SteamMatchmakingServers, RequestFavoritesServerList0);
            Createmethod(4, SteamMatchmakingServers, RequestHistoryServerList0);
            Createmethod(5, SteamMatchmakingServers, RequestSpectatorServerList0);
            Createmethod(6, SteamMatchmakingServers, GetServerDetails0);
            Createmethod(7, SteamMatchmakingServers, CancelQuery0);
            Createmethod(8, SteamMatchmakingServers, RefreshQuery0);
            Createmethod(9, SteamMatchmakingServers, IsRefreshing0);
            Createmethod(10, SteamMatchmakingServers, GetServerCount0);
            Createmethod(11, SteamMatchmakingServers, RefreshServer0);
            Createmethod(12, SteamMatchmakingServers, PingServer);
            Createmethod(13, SteamMatchmakingServers, PlayerDetails);
            Createmethod(14, SteamMatchmakingServers, ServerRules);
            Createmethod(15, SteamMatchmakingServers, CancelServerQuery);
        };
    };
    struct SteamMatchmakingservers002 : Interface_t<17>
    {
        SteamMatchmakingservers002()
        {
            Createmethod(0, SteamMatchmakingServers, RequestInternetServerList1);
            Createmethod(1, SteamMatchmakingServers, RequestLANServerList1);
            Createmethod(2, SteamMatchmakingServers, RequestFriendsServerList1);
            Createmethod(3, SteamMatchmakingServers, RequestFavoritesServerList1);
            Createmethod(4, SteamMatchmakingServers, RequestHistoryServerList1);
            Createmethod(5, SteamMatchmakingServers, RequestSpectatorServerList1);
            Createmethod(6, SteamMatchmakingServers, ReleaseRequest);
            Createmethod(7, SteamMatchmakingServers, GetServerDetails1);
            Createmethod(8, SteamMatchmakingServers, CancelQuery1);
            Createmethod(9, SteamMatchmakingServers, RefreshQuery1);
            Createmethod(10, SteamMatchmakingServers, IsRefreshing1);
            Createmethod(11, SteamMatchmakingServers, GetServerCount1);
            Createmethod(12, SteamMatchmakingServers, RefreshServer1);
            Createmethod(13, SteamMatchmakingServers, PingServer);
            Createmethod(14, SteamMatchmakingServers, PlayerDetails);
            Createmethod(15, SteamMatchmakingServers, ServerRules);
            Createmethod(16, SteamMatchmakingServers, CancelServerQuery);
        };
    };

    struct Steammatchmakingserverloader
    {
        Steammatchmakingserverloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::MATCHMAKINGSERVERS, "SteamMatchmakingservers001", SteamMatchmakingservers001);
            Register(Interfacetype_t::MATCHMAKINGSERVERS, "SteamMatchmakingservers002", SteamMatchmakingservers002);
        }
    };
    static Steammatchmakingserverloader Interfaceloader{};
}
