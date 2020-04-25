/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamGameserver
    {
        void LogOn0()
        {
        }
        void LogOff()
        {
            Traceprint();
        }
        bool BLoggedOn()
        {
            Traceprint();
            return true;
        }
        void SetSpawnCount(uint32_t ucSpawn)
        {
            Traceprint();
        }
        bool GetSteam2GetEncryptionKeyToSendToNewClient(void *pvEncryptionKey, uint32_t *pcbEncryptionKey, uint32_t cbMaxEncryptionKey)
        {
            Traceprint();
            return false;
        }
        bool SendSteam2UserConnect(uint32_t unUserID, const void *pvRawKey, uint32_t unKeyLen, uint32_t unIPPublic, uint16_t usPort, const void *pvCookie, uint32_t cubCookie)
        {
            Traceprint();
            return false;
        }
        bool SendSteam3UserConnect(CSteamID steamID, uint32_t unIPPublic, const void *pvCookie, uint32_t cubCookie)
        {
            Traceprint();
            return false;
        }
        bool RemoveUserConnect(uint32_t unUserID)
        {
            Traceprint();
            return false;
        }
        bool SendUserDisconnect0(CSteamID steamID, uint32_t unUserID)
        {
            Traceprint();
            return false;
        }
        bool SendUserStatusResponse(CSteamID steamID, int nSecondsConnected, int nSecondsSinceLast)
        {
            Traceprint();
            return false;
        }
        bool Obsolete_GSSetStatus(int32_t nAppIdServed, uint32_t unServerFlags, int cPlayers, int cPlayersMax, int cBotPlayers, int unGamePort, const char *pchServerName, const char *pchGameDir, const char *pchMapName, const char *pchVersion)
        {
            Traceprint();
            return false;
        }
        bool UpdateStatus0(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pchMapName)
        {
            auto Localserver = Matchmaking::Localserver();
            Localserver->Set("Players.Max", cPlayersMax);
            Localserver->Set("Players.Bots", cBotPlayers);
            Localserver->Set("Players.Current", cPlayers);
            Localserver->Set("Session.Mapname", pchMapName ? pchMapName : "");
            Localserver->Set("Server.Hostname", pchServerName ? pchServerName : "");
            // We do not force an update as this may be called multiple times with the same information.
            return true;
        }
        bool BSecure()
        {
            Traceprint();

            const auto Request = new Callbacks::GSPolicyResponse_t();
            const auto RequestID = Callbacks::Createrequest();
            Request->m_bSecure = true;

            Callbacks::Completerequest(RequestID, Callbacks::k_iSteamUserCallbacks + 15, Request);

            return true;
        }
        CSteamID GetSteamID()
        {
            auto ID = CSteamID(Global.UserID);
            ID.Set(ID.GetAccountID(), ID.GetEUniverse(), k_EAccountTypeGameServer);
            return ID;
        }
        bool SetServerType0(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint32_t unGamePort, const char *pchGameDir, const char *pchVersion)
        {
            Traceprint();
            return false;
        }
        bool SetServerType1(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Traceprint();
            return false;
        }
        bool UpdateStatus1(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName)
        {
            auto Localserver = Matchmaking::Localserver();
            Localserver->Set("Players.Max", cPlayersMax);
            Localserver->Set("Players.Bots", cBotPlayers);
            Localserver->Set("Players.Current", cPlayers);
            Localserver->Set("Session.Mapname", pchMapName ? pchMapName : "");
            Localserver->Set("Server.Hostname", pchServerName ? pchServerName : "");
            Localserver->Set("Server.Friendlyname", pSpectatorServerName ? pSpectatorServerName : "");
            // We do not force an update as this may be called multiple times with the same information.
            return true;
        }
        bool CreateUnauthenticatedUser(CSteamID *pSteamID)
        {
            Traceprint();
            return false;
        }
        bool SetUserData(CSteamID steamIDUser, const char *pchPlayerName, uint32_t uScore)
        {
            Traceprint();
            return false;
        }
        void UpdateSpectatorPort(uint16_t unSpectatorPort)
        {
            Traceprint();
        }
        void SetGameType(const char *pchGameType)
        {
            Traceprint();
        }
        bool SendUserConnect(uint32_t, uint32_t, uint16_t, const void *, uint32_t)
        {
            Traceprint();
            return false;
        }
        bool GetUserAchievementStatus(CSteamID steamID, const char *pchAchievementName)
        {
            Traceprint();
            return false;
        }
        bool SendUserConnectAndAuthenticate0(CSteamID steamIDUser, uint32_t, void *, uint32_t)
        {
            Traceprint();
            return false;
        }
        CSteamID CreateUnauthenticatedUserConnection()
        {
            Traceprint();
            return CSteamID(Global.UserID);
        }
        void SendUserDisconnect1(CSteamID steamIDUser)
        {
            Traceprint();
        }
        bool BUpdateUserData(CSteamID steamIDUser, const char *pchPlayerName, uint32_t uScore)
        {
            Traceprint();
            return false;
        }
        bool BSetServerType0(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Traceprint();
            return false;
        }
        bool BGetUserAchievementStatus(CSteamID steamID, const char *pchAchievementName)
        {
            Traceprint();
            return false;
        }
        bool SendUserConnectAndAuthenticate1(uint32_t unIPClient, const void *pvAuthBlob, uint32_t cubAuthBlobSize, CSteamID *pSteamIDUser)
        {
            Traceprint();

            const auto Request = new Callbacks::GSClientApprove_t();
            *pSteamIDUser = *(CSteamID *)pvAuthBlob;
            Request->m_OwnerSteamID = *pSteamIDUser;
            Request->m_SteamID = *pSteamIDUser;

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::k_iSteamGameServerCallbacks + 1, Request);
            return true;
        }
        bool BSetServerType1(uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t unSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Traceprint();
            return false;
        }
        void UpdateServerStatus(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName)
        {
            auto Localserver = Matchmaking::Localserver();
            Localserver->Set("Players.Max", cPlayersMax);
            Localserver->Set("Players.Bots", cBotPlayers);
            Localserver->Set("Players.Current", cPlayers);
            Localserver->Set("Session.Mapname", pchMapName ? pchMapName : "");
            Localserver->Set("Server.Hostname", pchServerName ? pchServerName : "");
            Localserver->Set("Server.Friendlyname", pSpectatorServerName ? pSpectatorServerName : "");
            // We do not force an update as this may be called multiple times with the same information.
        }
        void GetGameplayStats()
        {
            Traceprint();
        }
        bool RequestUserGroupStatus(CSteamID steamIDUser, CSteamID steamIDGroup)
        {
            Traceprint();
            return false;
        }
        uint32_t GetPublicIP()
        {
            Traceprint();
            return ntohl(inet_addr("240.0.0.1"));
        }
        uint32_t UserHasLicenseForApp(CSteamID steamID, uint32_t appID)
        {
            Traceprint();
            return 0;
        }
        void SetGameTags(const char *pchGameTags)
        {
            Matchmaking::Localserver()->Set("Session.Gametags", pchGameTags);
            Matchmaking::Announceupdate();
        }
        uint64_t GetServerReputation()
        {
            Traceprint();
            return 0;
        }
        uint32_t GetAuthSessionTicket(void *pTicket, int cbMaxTicket, uint32_t *pcbTicket)
        {
            const auto Request = new Callbacks::GetAuthSessionTicketResponse_t();
            Request->m_eResult = EResult::k_EResultOK;
            Request->m_hAuthTicket = 1337;
            *pcbTicket = cbMaxTicket;
            Traceprint();

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::k_iSteamUserCallbacks + 63, Request);
            return Request->m_hAuthTicket;
        }
        uint32_t BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, CSteamID steamID) const
        {
            Debugprint(va("%s for 0x%llx", __func__, steamID.ConvertToUint64()));
            const auto Request = new Callbacks::ValidateAuthTicketResponse_t();
            Request->m_eAuthSessionResponse = 0; // k_EBeginAuthSessionResultOK
            Request->m_OwnerSteamID = steamID;
            Request->m_SteamID = steamID;

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::k_iSteamUserCallbacks + 43, Request);
            return 0; // k_EBeginAuthSessionResultOK
        }
        void EndAuthSession(CSteamID steamID)
        {
            Traceprint();
        }
        void CancelAuthTicket(uint32_t hAuthTicket)
        {
            Traceprint();
        }
        bool InitGameServer(uint32_t unGameIP, uint16_t unGamePort, uint16_t usQueryPort, uint32_t unServerFlags, uint32_t nAppID, const char *pchVersion)
        {
            Traceprint();
            return true;
        }
        void SetProduct(const char *pchProductName)
        {
            Traceprint();
        }
        void SetGameDescription(const char *pchGameDescription)
        {
            Traceprint();
        }
        void SetModDir(const char *pchModDir)
        {
            Traceprint();
        }
        void SetDedicatedServer(bool bDedicatedServer)
        {
            Traceprint();
        }
        void LogOn1(const char *pszAccountName, const char *pszPassword)
        {
        }
        void LogOnAnonymous()
        {
        }
        bool WasRestartRequested()
        {
            Traceprint();
            return false;
        }
        void SetMaxPlayerCount(int cPlayersMax)
        {
            Traceprint();
        }
        void SetBotPlayerCount(int cBotPlayers)
        {
            Traceprint();
        }
        void SetServerName(const char *pszServerName)
        {
            Traceprint();
        }
        void SetMapName(const char *pszMapName)
        {
            Traceprint();
        }
        void SetPasswordProtected(bool bPasswordProtected)
        {
            Traceprint();
        }
        void SetSpectatorPort(uint16_t unSpectatorPort)
        {
            Traceprint();
        }
        void SetSpectatorServerName(const char *pszSpectatorServerName)
        {
            Traceprint();
        }
        void ClearAllKeyValues()
        {
            Traceprint();
        }
        void SetKeyValue(const char *pKey, const char *pValue)
        {
            Traceprint();
        }
        void SetGameData(const char *pchGameData)
        {
            Traceprint();
        }
        void SetRegion(const char *pchRegionName)
        {
            Traceprint();
        }
        int SendUserConnectAndAuthenticate2(uint32_t unIPClient, const void *pvAuthBlob, uint32_t cubAuthBlobSize, CSteamID *pSteamIDUser)
        {
            Traceprint();
            return 0;
        }
        bool HandleIncomingPacket(const void *pData, int cbData, uint32_t srcIP, uint16_t srcPort)
        {
            Traceprint();
            return false;
        }
        int GetNextOutgoingPacket(void *pOut, int cbMaxOut, uint32_t *pNetAdr, uint16_t *pPort)
        {
            Traceprint();
            return 0;
        }
        void EnableHeartbeats(bool bActive)
        {
            Traceprint();
        }
        void SetHeartbeatInterval(int iHeartbeatInterval)
        {
            Traceprint();
        }
        void ForceHeartbeat()
        {
            Traceprint();
        }
        uint64_t AssociateWithClan(CSteamID clanID)
        {
            Traceprint();
            return 0;
        }
        uint64_t ComputeNewPlayerCompatibility(CSteamID steamID)
        {
            Traceprint();
            return 0;
        }
        void LogOn2(const char *pszUnk)
        {
            Traceprint();
        }
        void LogOn3(const char *pszAccountName, const char *pszPassword)
        {
            Traceprint();
        }
    };

    struct SteamGameserver001 : Interface_t
    {
        SteamGameserver001()
        {
            /* No steamworks SDK info */
        };
    };
    struct SteamGameserver002 : Interface_t
    {
        SteamGameserver002()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, SetSpawnCount);
            Createmethod(4, SteamGameserver, GetSteam2GetEncryptionKeyToSendToNewClient);
            Createmethod(5, SteamGameserver, SendSteam2UserConnect);
            Createmethod(6, SteamGameserver, SendSteam3UserConnect);
            Createmethod(7, SteamGameserver, RemoveUserConnect);
            Createmethod(8, SteamGameserver, SendUserDisconnect0);
            Createmethod(9, SteamGameserver, SendUserStatusResponse);
            Createmethod(10, SteamGameserver, Obsolete_GSSetStatus);
            Createmethod(11, SteamGameserver, UpdateStatus0);
            Createmethod(12, SteamGameserver, BSecure);
            Createmethod(13, SteamGameserver, GetSteamID);
            Createmethod(14, SteamGameserver, SetServerType0);
            Createmethod(15, SteamGameserver, SetServerType1);
            Createmethod(16, SteamGameserver, UpdateStatus1);
            Createmethod(17, SteamGameserver, CreateUnauthenticatedUser);
            Createmethod(18, SteamGameserver, SetUserData);
            Createmethod(19, SteamGameserver, UpdateSpectatorPort);
            Createmethod(20, SteamGameserver, SetGameType);
        };
    };
    struct SteamGameserver003 : Interface_t
    {
        SteamGameserver003()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, GetSteam2GetEncryptionKeyToSendToNewClient);
            Createmethod(6, SteamGameserver, SendUserConnect);
            Createmethod(7, SteamGameserver, RemoveUserConnect);
            Createmethod(8, SteamGameserver, SendUserDisconnect0);
            Createmethod(9, SteamGameserver, SetSpawnCount);
            Createmethod(10, SteamGameserver, SetServerType1);
            Createmethod(11, SteamGameserver, UpdateStatus1);
            Createmethod(12, SteamGameserver, CreateUnauthenticatedUser);
            Createmethod(13, SteamGameserver, SetUserData);
            Createmethod(14, SteamGameserver, UpdateSpectatorPort);
            Createmethod(15, SteamGameserver, SetGameType);
            Createmethod(16, SteamGameserver, GetUserAchievementStatus);
        };
    };
    struct SteamGameserver004 : Interface_t
    {
        SteamGameserver004()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate0);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType0);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
        };
    };
    struct SteamGameserver005 : Interface_t
    {
        SteamGameserver005()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
        };
    };
    struct SteamGameserver006 : Interface_t
    {
        SteamGameserver006()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
        };
    };
    struct SteamGameserver007 : Interface_t
    {
        SteamGameserver007()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
        };
    };
    struct SteamGameserver008 : Interface_t
    {
        SteamGameserver008()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP);
        };
    };
    struct SteamGameserver009 : Interface_t
    {
        SteamGameserver009()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP);
            Createmethod(17, SteamGameserver, SetGameData);
            Createmethod(18, SteamGameserver, UserHasLicenseForApp);

        };
    };
    struct SteamGameserver010 : Interface_t
    {
        SteamGameserver010()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameTags);
            Createmethod(13, SteamGameserver, GetGameplayStats);
            Createmethod(14, SteamGameserver, GetServerReputation);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP);
            Createmethod(17, SteamGameserver, SetGameData);
            Createmethod(18, SteamGameserver, UserHasLicenseForApp);
            Createmethod(19, SteamGameserver, GetAuthSessionTicket);
            Createmethod(20, SteamGameserver, BeginAuthSession);
            Createmethod(21, SteamGameserver, EndAuthSession);
            Createmethod(22, SteamGameserver, CancelAuthTicket);
        };
    };
    struct SteamGameserver011 : Interface_t
    {
        SteamGameserver011()
        {
            Createmethod(0, SteamGameserver, InitGameServer);
            Createmethod(1, SteamGameserver, SetProduct);
            Createmethod(2, SteamGameserver, SetGameDescription);
            Createmethod(3, SteamGameserver, SetModDir);
            Createmethod(4, SteamGameserver, SetDedicatedServer);
            Createmethod(5, SteamGameserver, LogOn2);
            Createmethod(6, SteamGameserver, LogOnAnonymous);
            Createmethod(7, SteamGameserver, LogOff);
            Createmethod(8, SteamGameserver, BLoggedOn);
            Createmethod(9, SteamGameserver, BSecure);
            Createmethod(10, SteamGameserver, GetSteamID);
            Createmethod(11, SteamGameserver, WasRestartRequested);
            Createmethod(12, SteamGameserver, SetMaxPlayerCount);
            Createmethod(13, SteamGameserver, SetBotPlayerCount);
            Createmethod(14, SteamGameserver, SetServerName);
            Createmethod(15, SteamGameserver, SetMapName);
            Createmethod(16, SteamGameserver, SetPasswordProtected);
            Createmethod(17, SteamGameserver, SetSpectatorPort);
            Createmethod(18, SteamGameserver, SetSpectatorServerName);
            Createmethod(19, SteamGameserver, ClearAllKeyValues);
            Createmethod(20, SteamGameserver, SetKeyValue);
            Createmethod(21, SteamGameserver, SetGameTags);
            Createmethod(22, SteamGameserver, SetGameData);
            Createmethod(23, SteamGameserver, SetRegion);
            Createmethod(24, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(25, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(26, SteamGameserver, SendUserDisconnect1);
            Createmethod(27, SteamGameserver, BUpdateUserData);
            Createmethod(28, SteamGameserver, GetAuthSessionTicket);
            Createmethod(29, SteamGameserver, BeginAuthSession);
            Createmethod(30, SteamGameserver, EndAuthSession);
            Createmethod(31, SteamGameserver, CancelAuthTicket);
            Createmethod(32, SteamGameserver, UserHasLicenseForApp);
            Createmethod(33, SteamGameserver, RequestUserGroupStatus);
            Createmethod(34, SteamGameserver, GetGameplayStats);
            Createmethod(35, SteamGameserver, GetServerReputation);
            Createmethod(36, SteamGameserver, GetPublicIP);
            Createmethod(37, SteamGameserver, HandleIncomingPacket);
            Createmethod(38, SteamGameserver, GetNextOutgoingPacket);
            Createmethod(39, SteamGameserver, EnableHeartbeats);
            Createmethod(40, SteamGameserver, SetHeartbeatInterval);
            Createmethod(41, SteamGameserver, ForceHeartbeat);
            Createmethod(42, SteamGameserver, AssociateWithClan);
            Createmethod(43, SteamGameserver, ComputeNewPlayerCompatibility);
        };
    };
    struct SteamGameserver012 : Interface_t
    {
        SteamGameserver012()
        {
            Createmethod(0, SteamGameserver, InitGameServer);
            Createmethod(1, SteamGameserver, SetProduct);
            Createmethod(2, SteamGameserver, SetGameDescription);
            Createmethod(3, SteamGameserver, SetModDir);
            Createmethod(4, SteamGameserver, SetDedicatedServer);
            Createmethod(5, SteamGameserver, LogOn3);
            Createmethod(6, SteamGameserver, LogOnAnonymous);
            Createmethod(7, SteamGameserver, LogOff);
            Createmethod(8, SteamGameserver, BLoggedOn);
            Createmethod(9, SteamGameserver, BSecure);
            Createmethod(10, SteamGameserver, GetSteamID);
            Createmethod(11, SteamGameserver, WasRestartRequested);
            Createmethod(12, SteamGameserver, SetMaxPlayerCount);
            Createmethod(13, SteamGameserver, SetBotPlayerCount);
            Createmethod(14, SteamGameserver, SetServerName);
            Createmethod(15, SteamGameserver, SetMapName);
            Createmethod(16, SteamGameserver, SetPasswordProtected);
            Createmethod(17, SteamGameserver, SetSpectatorPort);
            Createmethod(18, SteamGameserver, SetSpectatorServerName);
            Createmethod(19, SteamGameserver, ClearAllKeyValues);
            Createmethod(20, SteamGameserver, SetKeyValue);
            Createmethod(21, SteamGameserver, SetGameTags);
            Createmethod(22, SteamGameserver, SetGameData);
            Createmethod(23, SteamGameserver, SetRegion);
            Createmethod(24, SteamGameserver, SendUserConnectAndAuthenticate2);
            Createmethod(25, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(26, SteamGameserver, SendUserDisconnect1);
            Createmethod(27, SteamGameserver, BUpdateUserData);
            Createmethod(28, SteamGameserver, GetAuthSessionTicket);
            Createmethod(29, SteamGameserver, BeginAuthSession);
            Createmethod(30, SteamGameserver, EndAuthSession);
            Createmethod(31, SteamGameserver, CancelAuthTicket);
            Createmethod(32, SteamGameserver, UserHasLicenseForApp);
            Createmethod(33, SteamGameserver, RequestUserGroupStatus);
            Createmethod(34, SteamGameserver, GetGameplayStats);
            Createmethod(35, SteamGameserver, GetServerReputation);
            Createmethod(36, SteamGameserver, GetPublicIP);
            Createmethod(37, SteamGameserver, HandleIncomingPacket);
            Createmethod(38, SteamGameserver, GetNextOutgoingPacket);
            Createmethod(39, SteamGameserver, EnableHeartbeats);
            Createmethod(40, SteamGameserver, SetHeartbeatInterval);
            Createmethod(41, SteamGameserver, ForceHeartbeat);
            Createmethod(42, SteamGameserver, AssociateWithClan);
            Createmethod(43, SteamGameserver, ComputeNewPlayerCompatibility);
        };
    };

    struct Steamgameserverloader
    {
        Steamgameserverloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver001", SteamGameserver001);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver002", SteamGameserver002);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver003", SteamGameserver003);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver004", SteamGameserver004);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver005", SteamGameserver005);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver006", SteamGameserver006);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver007", SteamGameserver007);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver008", SteamGameserver008);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver009", SteamGameserver009);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver010", SteamGameserver010);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver011", SteamGameserver011);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver012", SteamGameserver012);
        }
    };
    static Steamgameserverloader Interfaceloader{};
}
