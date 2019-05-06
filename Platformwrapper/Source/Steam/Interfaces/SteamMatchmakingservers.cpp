/*
    Initial author: Convery (tcn@ayria.se)
    Started: 05-05-2019
    License: MIT
*/

#include "../Steam.hpp"

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

    struct SteamMatchmakingServers
    {
        void *RequestInternetServerList1(uint32_t iApp,MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void *RequestLANServerList1(uint32_t iApp, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void *RequestFriendsServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void *RequestFavoritesServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void *RequestHistoryServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void *RequestSpectatorServerList1(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse002 *pRequestServersResponse) { Traceprint(); return {}; };
        void ReleaseRequest(void *hServerListRequest) { Traceprint(); };

        void RequestInternetServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };
        void RequestLANServerList0(uint32_t iApp, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };
        void RequestFriendsServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };
        void RequestFavoritesServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };
        void RequestHistoryServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };
        void RequestSpectatorServerList0(uint32_t iApp, MatchmakingKV **ppchFilters, uint32_t nFilters, ISteamMatchmakingServerListResponse001 *pRequestServersResponse) { Traceprint(); };

        void *GetServerDetails(uint32_t eType, int iServer) { Traceprint(); return {}; };
        void CancelQuery(uint32_t eType) { Traceprint(); };
        void RefreshQuery(uint32_t eType) { Traceprint(); };
        bool IsRefreshing(uint32_t eType) { Traceprint(); return {}; };
        int GetServerCount(uint32_t eType) { Traceprint(); return {}; };
        void RefreshServer(uint32_t eType, int iServer) { Traceprint(); };
        int PingServer(uint32_t unIP, uint16_t usPort, ISteamMatchmakingPingResponse *pRequestServersResponse) { Traceprint(); return {}; };
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
            Createmethod(6, SteamMatchmakingServers, ReleaseRequest);

            Createmethod(7, SteamMatchmakingServers, GetServerDetails);
            Createmethod(8, SteamMatchmakingServers, CancelQuery);
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

            Createmethod(6, SteamMatchmakingServers, GetServerDetails);
            Createmethod(7, SteamMatchmakingServers, CancelQuery);
            Createmethod(8, SteamMatchmakingServers, IsRefreshing);
            Createmethod(9, SteamMatchmakingServers, GetServerCount);
            Createmethod(10, SteamMatchmakingServers, RefreshServer);
            Createmethod(11, SteamMatchmakingServers, PingServer);
            Createmethod(12, SteamMatchmakingServers, PlayerDetails);
            Createmethod(13, SteamMatchmakingServers, ServerRules);
            Createmethod(14, SteamMatchmakingServers, CancelServerQuery);
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
