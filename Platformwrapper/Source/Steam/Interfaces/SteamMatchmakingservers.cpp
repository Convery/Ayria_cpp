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
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    // Callbacks.
    struct MatchmakingKV { char Key[256], Value[256]; };
    struct ISteamMatchmakingServerListResponse001
    {
        virtual void ServerResponded(int iServer) = 0;
        virtual void ServerFailedToRespond(int iServer) = 0;
        virtual void RefreshComplete(uint32_t response) = 0;
    };
    struct ISteamMatchmakingServerListResponse002
    {
        virtual void ServerResponded(void *hRequest, int iServer) = 0;
        virtual void ServerFailedToRespond(void *hRequest, int iServer) = 0;
        virtual void RefreshComplete(void *hRequest, uint32_t response) = 0;
    };
    struct ISteamMatchmakingPingResponse
    {
        virtual void ServerResponded(void *server) = 0;
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

    // Save the callback between calls.
    ISteamMatchmakingServerListResponse001 *Responsecallback1{};
    ISteamMatchmakingServerListResponse002 *Responsecallback2{};

    struct SteamMatchmakingServers
    {
        // TODO(tcn): Actually filter the servers.
        void *RequestInternetServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void *RequestLANServerList1(uint32_t iApp, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void *RequestFriendsServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void *RequestFavoritesServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void *RequestHistoryServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void *RequestSpectatorServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse)
        {
            Responsecallback2 = pRequestServersResponse;
            return this;
        }
        void ReleaseRequest(void *hServerListRequest) { Traceprint(); };

        void RequestInternetServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse; }
        void RequestLANServerList0(uint32_t iApp, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse;  }
        void RequestFriendsServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse;  }
        void RequestFavoritesServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse;  }
        void RequestHistoryServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse;  }
        void RequestSpectatorServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Responsecallback1 = pRequestServersResponse;  }

        Callbacks::gameserveritem_t *GetServerDetails1(uint32_t eType, int iServer) { Traceprint(); return {}; };
        Callbacks::gameserveritem_t *GetServerDetails2(void *hRequest, int iServer) const
        {
            auto Serialized = new Callbacks::gameserveritem_t();
            const auto Servers = Matchmaking::Externalservers();
            if (Servers.size() <= iServer) return 0;
            auto Server = Servers[iServer];

            // Address in host-order.
            Serialized->m_NetAdr.m_unIP = ntohl(Server->Core->Address);
            Serialized->m_NetAdr.m_usQueryPort = Server->Get("Network.Queryport", uint16_t());
            Serialized->m_NetAdr.m_usConnectionPort = Server->Get("Network.Gameport", uint16_t());

            // To make it a little more readable.
            #define Copy(x, y) lCopy(x, sizeof(x), Server->Get(y, std::string()));
            auto lCopy = [](char *Dst, size_t Max, std::string &&Src) -> void
            {
                std::memcpy(Dst, Src.c_str(), std::min(Max, Src.size()));
            };

            // String-properties.
            Copy(Serialized->m_szGameDescription, "Steam.Productdescription");
            Copy(Serialized->m_szGameDir, "Steam.Gamedirectory");
            Copy(Serialized->m_szServerName, "Server.Hostname");
            Copy(Serialized->m_szGameTags, "Session.Gametags");
            Copy(Serialized->m_szMap, "Session.Mapname");

            // TODO(tcn): Get some real information.
            Serialized->m_bSecure = Serialized->m_bPassword = Server->Get("Security.Anticheat", false);
            Serialized->m_bPassword = Server->Get("Security.Passwordprotected", false);
            Serialized->m_nServerVersion = Server->Get("Server.Version", 1001);
            Serialized->m_steamID = Server->Get("Server.XUID", uint64_t());
            Serialized->m_nBotPlayers = Server->Get("Players.Bots", 0);
            Serialized->m_nPlayers = Server->Get("Players.Current", 0);
            Serialized->m_nMaxPlayers = Server->Get("Players.Max", 0);
            Serialized->m_nAppID = Global.ApplicationID;
            Serialized->m_bHadSuccessfulResponse = true;
            Serialized->m_bDoNotRefresh = false;
            Serialized->m_ulTimeLastPlayed = 0;
            Serialized->m_nPing = 33;

            return Serialized;
        }
        void CancelQuery(uint32_t eType) { Traceprint(); };
        void RefreshQuery(uint32_t eType) { Traceprint(); };
        bool IsRefreshing(uint32_t eType) { Traceprint(); return {}; };
        int GetServerCount(uint32_t eType)
        {
            // Complete the listing as we did it in the background.
            const auto Servers = Matchmaking::Externalservers();
            for(size_t i = 0; i < Servers.size(); ++i)
            {
                if (Responsecallback1) Responsecallback1->ServerResponded(i);
                if (Responsecallback2) Responsecallback2->ServerResponded(this, i);
            }

            if (Responsecallback2) Responsecallback2->RefreshComplete(this, 0);
            if (Responsecallback1) Responsecallback1->RefreshComplete(0);

            return Servers.size();
        }
        void RefreshServer(uint32_t eType, int iServer) { Traceprint(); };
        int PingServer(uint32_t unIP, uint16_t usPort, ISteamMatchmakingPingResponse *pRequestServersResponse)
        {
            return 0;
        }
        int PlayerDetails(uint32_t unIP, uint16_t usPort, ISteamMatchmakingPlayersResponse *pRequestServersResponse) { Traceprint(); return {}; };
        int ServerRules(uint32_t unIP, uint16_t usPort, ISteamMatchmakingRulesResponse *pRequestServersResponse) { Traceprint(); return {}; };
        void CancelServerQuery(int hServerQuery) { Traceprint(); };
    };

    struct SteamMatchmakingservers001 : Interface_t
    {
        SteamMatchmakingservers001()
        {
            Createmethod(0, SteamMatchmakingServers, RequestInternetServerList0);
            Createmethod(1, SteamMatchmakingServers, RequestLANServerList0);
            Createmethod(2, SteamMatchmakingServers, RequestFriendsServerList0);
            Createmethod(3, SteamMatchmakingServers, RequestFavoritesServerList0);
            Createmethod(4, SteamMatchmakingServers, RequestHistoryServerList0);
            Createmethod(5, SteamMatchmakingServers, RequestSpectatorServerList0);
            Createmethod(6, SteamMatchmakingServers, GetServerDetails1);
            Createmethod(7, SteamMatchmakingServers, CancelQuery);
            Createmethod(8, SteamMatchmakingServers, RefreshQuery);
            Createmethod(9, SteamMatchmakingServers, IsRefreshing);
            Createmethod(10, SteamMatchmakingServers, GetServerCount);
            Createmethod(11, SteamMatchmakingServers, RefreshServer);
            Createmethod(12, SteamMatchmakingServers, PingServer);
            Createmethod(13, SteamMatchmakingServers, PlayerDetails);
            Createmethod(14, SteamMatchmakingServers, ServerRules);
            Createmethod(15, SteamMatchmakingServers, CancelServerQuery);
        };
    };
    struct SteamMatchmakingservers002 : Interface_t
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
            Createmethod(7, SteamMatchmakingServers, GetServerDetails2);
            Createmethod(8, SteamMatchmakingServers, CancelQuery);
            Createmethod(9, SteamMatchmakingServers, RefreshQuery);
            Createmethod(10, SteamMatchmakingServers, IsRefreshing);
            Createmethod(11, SteamMatchmakingServers, GetServerCount);
            Createmethod(12, SteamMatchmakingServers, RefreshServer);
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
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::MATCHMAKINGSERVERS, "SteamMatchmakingservers001", SteamMatchmakingservers001);
            Register(Interfacetype_t::MATCHMAKINGSERVERS, "SteamMatchmakingservers002", SteamMatchmakingservers002);
        }
    };
    static Steammatchmakingserverloader Interfaceloader{};
}
