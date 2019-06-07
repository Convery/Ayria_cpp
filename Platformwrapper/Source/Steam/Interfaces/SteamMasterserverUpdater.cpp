/*
    Initial author: Convery (tcn@ayria.se)
    Started: 10-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamMasterServerUpdater
    {
        void SetActive(bool bActive)
        {
            Traceprint();
        }
        void SetHeartbeatInterval(int iHeartbeatInterval)
        {
            Traceprint();
        }
        bool HandleIncomingPacket(const void *pData, int cbData, uint32_t srcIP, uint16_t srcPort)
        {
            Traceprint();
            return {};
        }
        int  GetNextOutgoingPacket(void *pOut, int cbMaxOut, uint32_t *pNetAdr, uint16_t *pPort)
        {
            Traceprint();
            return {};
        }
        void SetBasicServerData(unsigned short nProtocolVersion, bool bDedicatedServer, const char *pRegionName, const char *pProductName, unsigned short nMaxReportedClients, bool bPasswordProtected, const char *pGameDescription)
        {
            Debugprint(va("Update Steam-gameserver:\n> Dedicated: %s\n> Passwordprotected: %s\n> Product: %s\n> Description: %s\n> Maxplayers: %u",
                          bDedicatedServer ? "TRUE" : "FALSE", bPasswordProtected ? "TRUE" : "FALSE", pProductName, pGameDescription, nMaxReportedClients));

            Matchmaking::Localserver.Gamedata["Protocolversion"] = nProtocolVersion;
            Matchmaking::Localserver.Gamedata["needsPassword"] = bPasswordProtected;
            Matchmaking::Localserver.Gamedata["Gamedescription"] = pGameDescription;
            Matchmaking::Localserver.Gamedata["Playerlimit"] = nMaxReportedClients;
            Matchmaking::Localserver.Gamedata["isDedicated"] = bDedicatedServer;
            Matchmaking::Localserver.Gamedata["Productname"] = pProductName;
            Matchmaking::Localserver.Gamedata["Serverregion"] = pRegionName;
            Matchmaking::Updateserver();
        }
        void ClearAllKeyValues()
        {
            Traceprint();
        }
        void SetKeyValue(const char *pKey, const char *pValue)
        {
            Debugprint(va("%s - Key: \"%s\" - Value: \"%s\"", __FUNCTION__, pKey, pValue));
        }
        void NotifyShutdown()
        {
            Traceprint();
        }
        bool WasRestartRequested()
        {
            Traceprint();
            return {};
        }
        void ForceHeartbeat()
        {
            Traceprint();
        }
        bool AddMasterServer(const char *pServerAddress)
        {
            Traceprint();
            return {};
        }
        bool RemoveMasterServer(const char *pServerAddress)
        {
            Traceprint();
            return {};
        }
        int  GetNumMasterServers()
        {
            // NOTE(tcn): Some games uses this for an 'is online' check.
            return 1;
        }
        int  GetMasterServerAddress(int iServer, char *pOut, int outBufferSize)
        {
            // TODO(tcn): Returns bytes written, investigate this when needed.
            Traceprint();
            return 0;
        }
    };

    struct SteamMasterserverupdater001 : Interface_t
    {
        SteamMasterserverupdater001()
        {
            Createmethod(0, SteamMasterServerUpdater, SetActive);
            Createmethod(1, SteamMasterServerUpdater, SetHeartbeatInterval);
            Createmethod(2, SteamMasterServerUpdater, HandleIncomingPacket);
            Createmethod(3, SteamMasterServerUpdater, GetNextOutgoingPacket);
            Createmethod(4, SteamMasterServerUpdater, SetBasicServerData);
            Createmethod(5, SteamMasterServerUpdater, ClearAllKeyValues);
            Createmethod(6, SteamMasterServerUpdater, SetKeyValue);
            Createmethod(7, SteamMasterServerUpdater, NotifyShutdown);
            Createmethod(8, SteamMasterServerUpdater, WasRestartRequested);
            Createmethod(9, SteamMasterServerUpdater, ForceHeartbeat);
            Createmethod(10, SteamMasterServerUpdater, AddMasterServer);
            Createmethod(11, SteamMasterServerUpdater, RemoveMasterServer);
            Createmethod(12, SteamMasterServerUpdater, GetNumMasterServers);
            Createmethod(13, SteamMasterServerUpdater, GetMasterServerAddress);
        }
    };

    struct SteamMasterserverupdaterloader
    {
        SteamMasterserverupdaterloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::MASTERSERVERUPDATER, "SteamMasterserverupdater001", SteamMasterserverupdater001);
        }
    };
    static SteamMasterserverupdaterloader Interfaceloader{};
}
