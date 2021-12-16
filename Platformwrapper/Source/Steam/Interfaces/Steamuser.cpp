/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-12-16
    License: MIT
*/

#include <Steam.hpp>

// We use for loops for results that return 0 or 1 values, ignore warnings.
#pragma warning(disable: 4702)

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    // First 32 bytes are for our own use.
    Blob Authticket;

    struct SteamUser
    {
        enum class EBeginAuthSessionResult
        {
            k_EBeginAuthSessionResultOK,						    // Ticket is valid for this game and this steamID.
            k_EBeginAuthSessionResultInvalidTicket = 1,				// Ticket is not valid.
            k_EBeginAuthSessionResultDuplicateRequest = 2,			// A ticket has already been submitted for this steamID
            k_EBeginAuthSessionResultInvalidVersion = 3,			// Ticket is from an incompatible interface version
            k_EBeginAuthSessionResultGameMismatch = 4,				// Ticket is not for this game
            k_EBeginAuthSessionResultExpiredTicket = 5,				// Ticket has expired
        };
        enum class EUserHasLicenseForAppResult
        {
            k_EUserHasLicenseResultHasLicense,					    // User has a license for specified app
            k_EUserHasLicenseResultDoesNotHaveLicense = 1,			// User does not have a license for the specified app
            k_EUserHasLicenseResultNoAuth = 2,						// User has not been authenticated
        };
        enum class EVoiceResult
        {
            k_EVoiceResultOK,
            k_EVoiceResultNotInitialized = 1,
            k_EVoiceResultNotRecording = 2,
            k_EVoiceResultNoData = 3,
            k_EVoiceResultBufferTooSmall = 4,
            k_EVoiceResultDataCorrupted = 5,
            k_EVoiceResultRestricted = 6,
            k_EVoiceResultUnsupportedCodec = 7,
            k_EVoiceResultReceiverOutOfDate = 8,
            k_EVoiceResultReceiverDidNotAnswer = 9,
        };
        enum class EDurationControlOnlineState
        {
            k_EDurationControlOnlineState_Invalid,				    // nil value
            k_EDurationControlOnlineState_Offline = 1,				// currently in offline play - single-player, offline co-op, etc.
            k_EDurationControlOnlineState_Online = 2,				// currently in online play
            k_EDurationControlOnlineState_OnlineHighPri = 3,		// currently in online play and requests not to be interrupted
        };
        enum class ELogonState
        {
            k_ELogonStateNotLoggedOn,
            k_ELogonStateLoggingOn = 1,
            k_ELogonStateLoggingOff = 2,
            k_ELogonStateLoggedOn = 3
        };
        enum class EAppUsageEvent
        {
            k_EAppUsageEventGameLaunch = 1,
            k_EAppUsageEventGameLaunchTrial = 2,
            k_EAppUsageEventMedia = 3,
            k_EAppUsageEventPreloadStart = 4,
            k_EAppUsageEventPreloadFinish = 5,
            k_EAppUsageEventMarketingMessageView = 6,	// deprecated, do not use
            k_EAppUsageEventInGameAdViewed = 7,
            k_EAppUsageEventGameLaunchFreeWeekend = 8,
        };
        enum class ERegistrySubTree
        {
            k_ERegistrySubTreeNews,
            k_ERegistrySubTreeApps = 1,
            k_ERegistrySubTreeSubscriptions = 2,
            k_ERegistrySubTreeGameServers = 3,
            k_ERegistrySubTreeFriends = 4,
            k_ERegistrySubTreeSystem = 5,
            k_ERegistrySubTreeAppOwnershipTickets = 6,
            k_ERegistrySubTreeLegacyCDKeys = 7,
        };
        enum class EVACBan
        {
            k_EVACBanGoldsrc,
            k_EVACBanSource,
            k_EVACBanDayOfDefeatSource,
        };

        EBeginAuthSessionResult BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, SteamID_t steamID)
        {
            Debugprint(va("%s for 0x%llx", __func__, steamID.FullID));
            const auto Request = new Tasks::ValidateAuthTicketResponse_t();
            Request->m_eAuthSessionResponse = 0; // k_EBeginAuthSessionResultOK
            Request->m_OwnerSteamID = steamID;
            Request->m_SteamID = steamID;

            Tasks::Completerequest(Tasks::Createrequest(), Tasks::ECallbackType::ValidateAuthTicketResponse_t, Request);
            return EBeginAuthSessionResult::k_EBeginAuthSessionResultOK;
        }
        HAuthTicket GetAuthSessionTicket(void *pTicket, int cbMaxTicket, uint32_t *pcbTicket)
        {
            const auto Request = new Tasks::GetAuthSessionTicketResponse_t();
            Request->m_eResult = EResult::k_EResultOK;
            Request->m_hAuthTicket = 1337;
            *pcbTicket = cbMaxTicket;
            Traceprint();

            std::memcpy(pTicket, "Ticket", 7);
            *pcbTicket = 7;

            Tasks::Completerequest(Tasks::Createrequest(), Tasks::ECallbackType::GetAuthSessionTicketResponse_t, Request);
            return Request->m_hAuthTicket;
        }
        void CancelAuthTicket(HAuthTicket hAuthTicket) {}
        void EndAuthSession(SteamID_t steamID) {}

        SteamAPICall_t RequestEncryptedAppTicket(void *pDataToInclude, int cbDataToInclude)
        {
            // Append game data.
            Authticket.resize(cbDataToInclude + 32);
            if (cbDataToInclude > 0) std::memcpy(Authticket.data() + 32, pDataToInclude, cbDataToInclude);
            Infoprint(va("Creating an \"encrypted\" ticket with %d bytes of game-data.", cbDataToInclude));

            // Notify the game that we are ready.
            const auto Request = new Tasks::EncryptedAppTicketResponse_t();
            const auto RequestID = Tasks::Createrequest();
            Request->m_eResult = EResult::k_EResultOK;

            Tasks::Completerequest(RequestID, Tasks::ECallbackType::EncryptedAppTicketResponse_t, Request);
            return RequestID;
        }
        bool GetEncryptedAppTicket(void *pTicket, int cbMaxTicket, uint32_t *pcbTicket)
        {
            std::memcpy(pTicket, Authticket.data(), std::min((int)Authticket.size(), cbMaxTicket));
            *pcbTicket = std::min((int)Authticket.size(), cbMaxTicket);
            return true;
        }
        void SetSteam2Ticket(uint8_t *pubTicket, int cubTicket)
        {
            // Never seen this used in a game..
            Traceprint();
        }

        void LogOn(SteamID_t steamID) {}
        bool BConnected() { return true; }
        bool BLoggedOn() { return true; }
        void LogOff() {}

        ELogonState GetLogonState()
        {
            return ELogonState::k_ELogonStateLoggedOn;
        }
        HSteamUser GetHSteamUser()
        {
            return 1;
        }
        SteamID_t GetSteamID()
        {
            return Global.XUID;
        }

        EVoiceResult GetVoice1(bool bWantCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten, bool bWantUncompressed, void *pUncompressedDestBuffer, uint32_t cbUncompressedDestBufferSize, uint32_t *nUncompressBytesWritten, uint32_t nUncompressedVoiceDesiredSampleRate) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult GetVoice0(bool bWantCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten, bool bWantUncompressed, void *pUncompressedDestBuffer, uint32_t cbUncompressedDestBufferSize, uint32_t *nUncompressBytesWritten) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult DecompressVoice1(const void *pCompressed, uint32_t cbCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten, uint32_t nDesiredSampleRate) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult DecompressVoice0(const void *pCompressed, uint32_t cbCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult GetAvailableVoice1(uint32_t *pcbCompressed, uint32_t *pcbUncompressed, uint32_t nUncompressedVoiceDesiredSampleRate) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult GetCompressedVoice(void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten) { return EVoiceResult::k_EVoiceResultNoData; }
        EVoiceResult GetAvailableVoice0(uint32_t *pcbCompressed, uint32_t *pcbUncompressed) { return EVoiceResult::k_EVoiceResultNoData; }
        uint32_t GetVoiceOptimalSampleRate() { return 48000; }
        void StartVoiceRecording() {}
        void StopVoiceRecording() {}

        EUserHasLicenseForAppResult UserHasLicenseForApp(SteamID_t steamID, AppID_t appID)
        {
            return EUserHasLicenseForAppResult::k_EUserHasLicenseResultHasLicense;
        }
        SteamAPICall_t RequestStoreAuthURL(const char *pchRedirectURL)
        {
            const auto Request = new Tasks::StoreAuthURLResponse_t();
            const auto RequestID = Tasks::Createrequest();

            Infoprint(va("%s: %s", __FUNCTION__, pchRedirectURL));
            std::strncpy(Request->m_szURL, "ayria.se", 512);

            Tasks::Completerequest(RequestID, Tasks::ECallbackType::StoreAuthURLResponse_t, Request);
            return RequestID;
        }
        SteamAPICall_t GetMarketEligibility()
        {
            const auto Request = new Tasks::MarketEligibilityResponse_t();
            const auto RequestID = Tasks::Createrequest();
            Request->m_bAllowed = true;

            Tasks::Completerequest(RequestID, Tasks::ECallbackType::MarketEligibilityResponse_t, Request);
            return RequestID;
        }

        bool GetGuestPassToGiveInfo(uint32_t nPassIndex, GID_t *pgidGuestPassID, PackageId_t *pnPackageID, uint32_t *pRTime32Created, uint32_t *pRTime32Expiration, uint32_t *pRTime32Sent, uint32_t *pRTime32Redeemed, char *pchRecipientAddress, int cRecipientAddressSize) { return false; }
        bool GetGuestPassToRedeemInfo(uint32_t nPassIndex, GID_t *pgidGuestPassID, PackageId_t *pnPackageID, uint32_t *pRTime32Created, uint32_t *pRTime32Expiration, uint32_t *pRTime32Sent, uint32_t *pRTime32Redeemed) { return false; }
        bool GetGuestPassToRedeemSenderAddress(uint32_t nPassIndex, char *pchSenderAddress, int cSenderAddressSize) { return false; }
        bool GetGuestPassToRedeemSenderName(uint32_t nPassIndex, char *pchSenderName, int cSenderNameSize) { return false; }
        bool SendGuestPassByEmail(const char *pchEmailAccount, GID_t gidGuestPassID, bool bResending) { return false; }
        bool SendGuestPassByAccountID(uint32_t uAccountID, GID_t gidGuestPassID, bool bResending) { return false; }
        bool RedeemGuestPass(const char *pchGuestPassCode) { return true; }
        bool AckGuestPass(const char *pchGuestPassCode) { return true; }
        uint32_t GetGuestPassLastUpdateTime() { return uint32_t(uint32_t(time(NULL))); }
        uint32_t GetGuestPassToRedeemCount() { return 0; }
        uint32_t GetGuestPassToGiveCount() { return 0; }

        bool BIsPhoneRequiringVerification() { return false; }
        bool BIsPhoneIdentifying() { return false; }
        bool BIsTwoFactorEnabled() { return false; }
        bool BIsPhoneVerified() { return true; }
        bool BIsBehindNAT() { return false; }

        SteamAPICall_t GetDurationControl()
        {
            // Parental control system, not needed.
            const auto Request = new Tasks::DurationControl_t();
            const auto RequestID = Tasks::Createrequest();
            Request->m_eResult = EResult::k_EResultOK;
            Request->m_appid = Global.AppID;
            Request->m_bApplicable = false;

            Tasks::Completerequest(RequestID, Tasks::ECallbackType::DurationControl_t, Request);
            return RequestID;
        }
        bool BSetDurationControlOnlineState(EDurationControlOnlineState eNewState)
        { return true; }

        bool SetEmail(const char *pchEmail)
        {
            return true;
        }
        bool SetLanguage(const char *pchLanguage)
        {
            *Global.Locale = pchLanguage;
            return true;
        }
        bool GetUserDataFolder(char *pchBuffer, int cubBuffer)
        {
            constexpr auto Dir = "./Ayria/Storage";
            if (cubBuffer < 16) return false;

            std::memcpy(pchBuffer, Dir, 16);
            return true;
        }
        bool SetRegistryInt(ERegistrySubTree eRegistrySubTree, const char *pchKey, int iValue) { Traceprint(); return false; }
        bool GetRegistryInt(ERegistrySubTree eRegistrySubTree, const char *pchKey, int *piValue) { Traceprint(); return false; }
        bool SetRegistryString(ERegistrySubTree eRegistrySubTree, const char *pchKey, const char *pchValue) { Traceprint(); return false; }
        bool GetRegistryString(ERegistrySubTree eRegistrySubTree, const char *pchKey, char *pchValue, int cbValue) { Traceprint(); return false; }

        bool IsPrimaryChatDestination() { return true; }
        void SetSelfAsPrimaryChatDestination() {}

        void AcknowledgeMessageByGID(const char *pchMessageGID) {}
        bool RequireShowVACBannedMessage(EVACBan eVACBan)
        {
            return false;
        }
        void AcknowledgeVACBanning(EVACBan eVACBan) {}
        bool IsVACBanned(EVACBan eVACBan)
        {
            return false;
        }

        int GetGameBadgeLevel(int nSeries, bool bFoil)
        {
            return 1;
        }
        int GetPlayerSteamLevel()
        {
            return 1;
        }

        int InitiateGameConnection1(void *pAuthBlob, int cbMaxAuthBlob, SteamID_t steamIDGameServer, GameID_t nGameAppID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure, void *pvSteam2GetEncryptionKey, int cbSteam2GetEncryptionKey)
        {
            std::memcpy(pAuthBlob, "Cookie", std::min(cbMaxAuthBlob, 7));
            return std::min(cbMaxAuthBlob, 7);
        }
        int InitiateGameConnection0(void *pAuthBlob, int cbMaxAuthBlob, SteamID_t steamIDGameServer, GameID_t nGameAppID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            std::memcpy(pAuthBlob, "Cookie", std::min(cbMaxAuthBlob, 7));
            return std::min(cbMaxAuthBlob, 7);
        }
        int InitiateGameConnection2(void *pAuthBlob, int cbMaxAuthBlob, SteamID_t steamIDGameServer, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            std::memcpy(pAuthBlob, "Cookie", std::min(cbMaxAuthBlob, 7));
            return std::min(cbMaxAuthBlob, 7);
        }
        void TerminateGameConnection(uint32_t unIPServer, uint16_t usPortServer) {}
        int GetSteamGameConnectToken(void *pAuthBlob, int cbMaxAuthBlob)
        {
            std::memcpy(pAuthBlob, "Cookie", std::min(cbMaxAuthBlob, 7));
            return std::min(cbMaxAuthBlob, 7);
        }

        void AdvertiseGame(SteamID_t steamIDGameServer, uint32_t unIPServer, uint16_t usPortServer)
        {
            // TODO(tcn): Trigger the toaster notification in Steam?
        }
        void SetClientGameServer(int nClientGameID, uint32_t unIPServer, uint16_t usPortServer) { Traceprint(); }
        void AddServerNetAddress(uint32_t unIP, uint16_t unPort) {}
        void RemoveClientGame(int nClientGameID) {}
        int NClientGameIDAdd(int nGameID)
        {
            // Deprecated.
            return 0;
        }

        void SetAccountCreationTime(uint32_t rtime32Time) {}
        void SetAccountName(const char *pchAccountName) {}
        void SetPassword(const char *pchPassword) {}
        void RequestLegacyCDKey(AppID_t iAppID) {}
        void RefreshSteam2Login() {}

        void TrackAppUsageEvent(GameID_t gameID, EAppUsageEvent eAppUsageEvent, const char *pchExtraInfo) {}
    };

    struct SteamUser001 : Interface_t<>
    {
        SteamUser001()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser002 : Interface_t<>
    {
        SteamUser002()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser003 : Interface_t<>
    {
        SteamUser003()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser004 : Interface_t<26>
    {
        SteamUser004()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, LogOn);
            Createmethod(2, SteamUser, LogOff);
            Createmethod(3, SteamUser, BLoggedOn);
            Createmethod(4, SteamUser, GetLogonState);
            Createmethod(5, SteamUser, BConnected);
            Createmethod(6, SteamUser, GetSteamID);
            Createmethod(7, SteamUser, IsVACBanned);
            Createmethod(8, SteamUser, RequireShowVACBannedMessage);
            Createmethod(9, SteamUser, AcknowledgeVACBanning);
            Createmethod(10, SteamUser, NClientGameIDAdd);
            Createmethod(11, SteamUser, RemoveClientGame);
            Createmethod(12, SteamUser, SetClientGameServer);
            Createmethod(13, SteamUser, SetSteam2Ticket);
            Createmethod(14, SteamUser, AddServerNetAddress);
            Createmethod(15, SteamUser, SetEmail);
            Createmethod(16, SteamUser, GetSteamGameConnectToken);
            Createmethod(17, SteamUser, SetRegistryString);
            Createmethod(18, SteamUser, GetRegistryString);
            Createmethod(19, SteamUser, SetRegistryInt);
            Createmethod(20, SteamUser, GetRegistryInt);
            Createmethod(21, SteamUser, InitiateGameConnection0);
            Createmethod(22, SteamUser, TerminateGameConnection);
            Createmethod(23, SteamUser, SetSelfAsPrimaryChatDestination);
            Createmethod(24, SteamUser, IsPrimaryChatDestination);
            Createmethod(25, SteamUser, RequestLegacyCDKey);
        };
    };
    struct SteamUser005 : Interface_t<39>
    {
        SteamUser005()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, LogOn);
            Createmethod(2, SteamUser, LogOff);
            Createmethod(3, SteamUser, BLoggedOn);
            Createmethod(4, SteamUser, GetLogonState);
            Createmethod(5, SteamUser, BConnected);
            Createmethod(6, SteamUser, GetSteamID);
            Createmethod(7, SteamUser, IsVACBanned);
            Createmethod(8, SteamUser, RequireShowVACBannedMessage);
            Createmethod(9, SteamUser, AcknowledgeVACBanning);
            Createmethod(10, SteamUser, SetSteam2Ticket);
            Createmethod(11, SteamUser, AddServerNetAddress);
            Createmethod(12, SteamUser, SetEmail);
            Createmethod(13, SteamUser, SetRegistryString);
            Createmethod(14, SteamUser, GetRegistryString);
            Createmethod(15, SteamUser, SetRegistryInt);
            Createmethod(16, SteamUser, GetRegistryInt);
            Createmethod(17, SteamUser, InitiateGameConnection0);
            Createmethod(18, SteamUser, TerminateGameConnection);
            Createmethod(19, SteamUser, SetSelfAsPrimaryChatDestination);
            Createmethod(20, SteamUser, IsPrimaryChatDestination);
            Createmethod(21, SteamUser, RequestLegacyCDKey);
            Createmethod(22, SteamUser, SendGuestPassByEmail);
            Createmethod(23, SteamUser, SendGuestPassByAccountID);
            Createmethod(24, SteamUser, AckGuestPass);
            Createmethod(25, SteamUser, RedeemGuestPass);
            Createmethod(26, SteamUser, GetGuestPassToGiveCount);
            Createmethod(27, SteamUser, GetGuestPassToRedeemCount);
            Createmethod(28, SteamUser, GetGuestPassLastUpdateTime);
            Createmethod(29, SteamUser, GetGuestPassToGiveInfo);
            Createmethod(30, SteamUser, GetGuestPassToRedeemInfo);
            Createmethod(31, SteamUser, GetGuestPassToRedeemSenderAddress);
            Createmethod(32, SteamUser, GetGuestPassToRedeemSenderName);
            Createmethod(33, SteamUser, AcknowledgeMessageByGID);
            Createmethod(34, SteamUser, SetLanguage);
            Createmethod(35, SteamUser, TrackAppUsageEvent);
            Createmethod(36, SteamUser, SetAccountName);
            Createmethod(37, SteamUser, SetPassword);
            Createmethod(38, SteamUser, SetAccountCreationTime);
        };
    };
    struct SteamUser006 : Interface_t<12>
    {
        SteamUser006()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, LogOn);
            Createmethod(2, SteamUser, LogOff);
            Createmethod(3, SteamUser, BLoggedOn);
            Createmethod(4, SteamUser, GetSteamID);
            Createmethod(5, SteamUser, SetRegistryString);
            Createmethod(6, SteamUser, GetRegistryString);
            Createmethod(7, SteamUser, SetRegistryInt);
            Createmethod(8, SteamUser, GetRegistryInt);
            Createmethod(9, SteamUser, InitiateGameConnection0);
            Createmethod(10, SteamUser, TerminateGameConnection);
            Createmethod(11, SteamUser, TrackAppUsageEvent);
        };
    };
    struct SteamUser007 : Interface_t<13>
    {
        SteamUser007()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, LogOn);
            Createmethod(2, SteamUser, LogOff);
            Createmethod(3, SteamUser, BLoggedOn);
            Createmethod(4, SteamUser, GetSteamID);
            Createmethod(5, SteamUser, SetRegistryString);
            Createmethod(6, SteamUser, GetRegistryString);
            Createmethod(7, SteamUser, SetRegistryInt);
            Createmethod(8, SteamUser, GetRegistryInt);
            Createmethod(9, SteamUser, InitiateGameConnection1);
            Createmethod(10, SteamUser, TerminateGameConnection);
            Createmethod(11, SteamUser, TrackAppUsageEvent);
            Createmethod(12, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser008 : Interface_t<7>
    {
        SteamUser008()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection1);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser009 : Interface_t<7>
    {
        SteamUser009()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection0);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser010 : Interface_t<6>
    {
        SteamUser010()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
        };
    };
    struct SteamUser011 : Interface_t<11>
    {
        SteamUser011()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetCompressedVoice);
            Createmethod(10, SteamUser, DecompressVoice0);
        };
    };
    struct SteamUser012 : Interface_t<16>
    {
        SteamUser012()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetCompressedVoice);
            Createmethod(10, SteamUser, DecompressVoice0);
            Createmethod(11, SteamUser, GetAuthSessionTicket);
            Createmethod(12, SteamUser, BeginAuthSession);
            Createmethod(13, SteamUser, EndAuthSession);
            Createmethod(14, SteamUser, CancelAuthTicket);
            Createmethod(15, SteamUser, UserHasLicenseForApp);
        };
    };
    struct SteamUser013 : Interface_t<18>
    {
        SteamUser013()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice0);
            Createmethod(10, SteamUser, GetVoice0);
            Createmethod(11, SteamUser, DecompressVoice0);
            Createmethod(12, SteamUser, GetAuthSessionTicket);
            Createmethod(13, SteamUser, BeginAuthSession);
            Createmethod(14, SteamUser, EndAuthSession);
            Createmethod(15, SteamUser, CancelAuthTicket);
            Createmethod(16, SteamUser, UserHasLicenseForApp);
            Createmethod(17, SteamUser, BIsBehindNAT);
        };
    };
    struct SteamUser014 : Interface_t<21>
    {
        SteamUser014()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice0);
            Createmethod(10, SteamUser, GetVoice0);
            Createmethod(11, SteamUser, DecompressVoice0);
            Createmethod(12, SteamUser, GetAuthSessionTicket);
            Createmethod(13, SteamUser, BeginAuthSession);
            Createmethod(14, SteamUser, EndAuthSession);
            Createmethod(15, SteamUser, CancelAuthTicket);
            Createmethod(16, SteamUser, UserHasLicenseForApp);
            Createmethod(17, SteamUser, BIsBehindNAT);
            Createmethod(18, SteamUser, AdvertiseGame);
            Createmethod(19, SteamUser, RequestEncryptedAppTicket);
            Createmethod(20, SteamUser, GetEncryptedAppTicket);
        };
    };
    struct SteamUser015 : Interface_t<22>
    {
        SteamUser015()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice0);
            Createmethod(10, SteamUser, GetVoice0);
            Createmethod(11, SteamUser, DecompressVoice0);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
        };
    };
    struct SteamUser016 : Interface_t<22>
    {
        SteamUser016()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
        };
    };
    struct SteamUser017 : Interface_t<25>
    {
        SteamUser017()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
            Createmethod(23, SteamUser, GetGameBadgeLevel);
            Createmethod(24, SteamUser, GetPlayerSteamLevel);
        };
    };
    struct SteamUser018 : Interface_t<25>
    {
        SteamUser018()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
            Createmethod(22, SteamUser, GetGameBadgeLevel);
            Createmethod(23, SteamUser, GetPlayerSteamLevel);
            Createmethod(24, SteamUser, RequestStoreAuthURL);
        };
    };
    struct SteamUser019 : Interface_t<29>
    {
        SteamUser019()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
            Createmethod(22, SteamUser, GetGameBadgeLevel);
            Createmethod(23, SteamUser, GetPlayerSteamLevel);
            Createmethod(24, SteamUser, RequestStoreAuthURL);
            Createmethod(25, SteamUser, BIsPhoneVerified);
            Createmethod(26, SteamUser, BIsTwoFactorEnabled);
            Createmethod(27, SteamUser, BIsPhoneIdentifying);
            Createmethod(28, SteamUser, BIsPhoneRequiringVerification);
        };
    };
    struct SteamUser020 : Interface_t<31>
    {
        SteamUser020()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
            Createmethod(22, SteamUser, GetGameBadgeLevel);
            Createmethod(23, SteamUser, GetPlayerSteamLevel);
            Createmethod(24, SteamUser, RequestStoreAuthURL);
            Createmethod(25, SteamUser, BIsPhoneVerified);
            Createmethod(26, SteamUser, BIsTwoFactorEnabled);
            Createmethod(27, SteamUser, BIsPhoneIdentifying);
            Createmethod(28, SteamUser, BIsPhoneRequiringVerification);
            Createmethod(29, SteamUser, GetMarketEligibility);
            Createmethod(30, SteamUser, GetDurationControl);
        };
    };
    struct SteamUser021 : Interface_t<32>
    {
        SteamUser021()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection2);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice1);
            Createmethod(12, SteamUser, GetVoiceOptimalSampleRate);
            Createmethod(13, SteamUser, GetAuthSessionTicket);
            Createmethod(14, SteamUser, BeginAuthSession);
            Createmethod(15, SteamUser, EndAuthSession);
            Createmethod(16, SteamUser, CancelAuthTicket);
            Createmethod(17, SteamUser, UserHasLicenseForApp);
            Createmethod(18, SteamUser, BIsBehindNAT);
            Createmethod(19, SteamUser, AdvertiseGame);
            Createmethod(20, SteamUser, RequestEncryptedAppTicket);
            Createmethod(21, SteamUser, GetEncryptedAppTicket);
            Createmethod(22, SteamUser, GetGameBadgeLevel);
            Createmethod(23, SteamUser, GetPlayerSteamLevel);
            Createmethod(24, SteamUser, RequestStoreAuthURL);
            Createmethod(25, SteamUser, BIsPhoneVerified);
            Createmethod(26, SteamUser, BIsTwoFactorEnabled);
            Createmethod(27, SteamUser, BIsPhoneIdentifying);
            Createmethod(28, SteamUser, BIsPhoneRequiringVerification);
            Createmethod(29, SteamUser, GetMarketEligibility);
            Createmethod(30, SteamUser, GetDurationControl);
            Createmethod(31, SteamUser, BSetDurationControlOnlineState);
        };
    };

    struct Steamuserloader
    {
        Steamuserloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::USER, "SteamUser001", SteamUser001);
            Register(Interfacetype_t::USER, "SteamUser002", SteamUser002);
            Register(Interfacetype_t::USER, "SteamUser003", SteamUser003);
            Register(Interfacetype_t::USER, "SteamUser004", SteamUser004);
            Register(Interfacetype_t::USER, "SteamUser005", SteamUser005);
            Register(Interfacetype_t::USER, "SteamUser006", SteamUser006);
            Register(Interfacetype_t::USER, "SteamUser007", SteamUser007);
            Register(Interfacetype_t::USER, "SteamUser008", SteamUser008);
            Register(Interfacetype_t::USER, "SteamUser009", SteamUser009);
            Register(Interfacetype_t::USER, "SteamUser010", SteamUser010);
            Register(Interfacetype_t::USER, "SteamUser011", SteamUser011);
            Register(Interfacetype_t::USER, "SteamUser012", SteamUser012);
            Register(Interfacetype_t::USER, "SteamUser013", SteamUser013);
            Register(Interfacetype_t::USER, "SteamUser014", SteamUser014);
            Register(Interfacetype_t::USER, "SteamUser015", SteamUser015);
            Register(Interfacetype_t::USER, "SteamUser016", SteamUser016);
            Register(Interfacetype_t::USER, "SteamUser017", SteamUser017);
            Register(Interfacetype_t::USER, "SteamUser018", SteamUser018);
            Register(Interfacetype_t::USER, "SteamUser019", SteamUser019);
            Register(Interfacetype_t::USER, "SteamUser020", SteamUser020);
            Register(Interfacetype_t::USER, "SteamUser021", SteamUser021);
        }
    };
    static Steamuserloader Interfaceloader{};
}
