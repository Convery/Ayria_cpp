/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamUser
    {
        public:
        uint32_t GetHSteamUser()
        {
            Traceprint();
            return 0;
        }
        void LogOn(CSteamID steamID)
        {
            Traceprint();
            return;
        }
        void LogOff()
        {
            Traceprint();
            return;
        }
        bool BLoggedOn()
        {
            return true;
        }
        uint32_t GetLogonState()
        {
            Traceprint();
            return 0;
        }
        bool BConnected()
        {
            Traceprint();
            return true;
        }
        CSteamID GetSteamID()
        {
            // Cache marker
            return CSteamID(Global.UserID);
        }
        bool IsVACBanned(uint32_t eVACBan)
        {
            Traceprint();
            return false;
        }
        bool RequireShowVACBannedMessage(uint32_t eVACBan)
        {
            Traceprint();
            return false;
        }
        void AcknowledgeVACBanning(uint32_t eVACBan)
        {
            Traceprint();
        }
        int NClientGameIDAdd(int nGameID)
        {
            Traceprint();
            return 0;
        }
        void RemoveClientGame(int nClientGameID)
        {
            Traceprint();
        }
        void SetClientGameServer(int nClientGameID, uint32_t unIPServer, uint16_t usPortServer)
        {
            Traceprint();
        }
        void SetSteam2Ticket(uint8_t *pubTicket, int cubTicket)
        {
            Traceprint();
        }
        void AddServerNetAddress(uint32_t unIP, uint16_t unPort)
        {
            Traceprint();
        }
        bool SetEmail(const char *pchEmail)
        {
            Traceprint();
            return false;
        }
        int GetSteamGameConnectToken(void *pBlob, int cbMaxBlob)
        {
            Traceprint();
            return 0;
        }
        bool SetRegistryString(uint32_t eRegistrySubTree, const char *pchKey, const char *pchValue)
        {
            Traceprint();
            return false;
        }
        bool GetRegistryString(uint32_t eRegistrySubTree, const char *pchKey, char *pchValue, int cbValue)
        {
            Traceprint();
            return false;
        }
        bool SetRegistryInt(uint32_t eRegistrySubTree, const char *pchKey, int iValue)
        {
            Traceprint();
            return false;
        }
        bool GetRegistryInt(uint32_t eRegistrySubTree, const char *pchKey, int *piValue)
        {
            Traceprint();
            return false;
        }
        int InitiateGameConnection0(void *pBlob, int cbMaxBlob, CSteamID steamID, int nGameAppID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            Traceprint();
            return 0;
        }
        void TerminateGameConnection(uint32_t unIPServer, uint16_t usPortServer)
        {
            Traceprint();
        }
        void SetSelfAsPrimaryChatDestination()
        {
            Traceprint();
        }
        bool IsPrimaryChatDestination()
        {
            Traceprint();
            return false;
        }
        void RequestLegacyCDKey(uint32_t iAppID)
        {
            Traceprint();
        }
        int InitiateGameConnection1(void *pBlob, int cbMaxBlob, CSteamID steamID, CGameID nGameAppID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            Traceprint();
            return 0;
        }
        bool SendGuestPassByEmail(const char *pchEmailAccount, uint64_t gidGuestPassID, bool bResending)
        {
            Traceprint();
            return false;
        }
        bool SendGuestPassByAccountID(uint32_t uAccountID, uint64_t gidGuestPassID, bool bResending)
        {
            Traceprint();
            return false;
        }
        bool AckGuestPass(const char *pchGuestPassCode)
        {
            Traceprint();
            return false;
        }
        bool RedeemGuestPass(const char *pchGuestPassCode)
        {
            Traceprint();
            return false;
        }
        uint32_t GetGuestPassToGiveCount()
        {
            Traceprint();
            return 0;
        }
        uint32_t GetGuestPassToRedeemCount()
        {
            Traceprint();
            return 0;
        }
        uint32_t GetGuestPassLastUpdateTime()
        {
            Traceprint();
            return 0;
        }
        bool GetGuestPassToGiveInfo(uint32_t nPassIndex, uint64_t *pgidGuestPassID, int* pnPackageID, uint32_t* pRTime32Created, uint32_t* pRTime32Expiration, uint32_t* pRTime32Sent, uint32_t* pRTime32Redeemed, char * pchRecipientAddress, int cRecipientAddressSize)
        {
            Traceprint();
            return false;
        }
        bool GetGuestPassToRedeemInfo(uint32_t nPassIndex, uint64_t *pgidGuestPassID, int* pnPackageID, uint32_t* pRTime32Created, uint32_t* pRTime32Expiration, uint32_t* pRTime32Sent, uint32_t* pRTime32Redeemed)
        {
            Traceprint();
            return false;
        }
        bool GetGuestPassToRedeemSenderAddress(uint32_t nPassIndex, char* pchSenderAddress, int cSenderAddressSize)
        {
            Traceprint();
            return false;
        }
        bool GetGuestPassToRedeemSenderName(uint32_t nPassIndex, char* pchSenderName, int cSenderNameSize)
        {
            Traceprint();
            return false;
        }
        void AcknowledgeMessageByGID(const char *pchMessageGID)
        {
            Traceprint();
        }
        bool SetLanguage(const char *pchLanguage)
        {
            Traceprint();
            Global.Language = pchLanguage;
            return true;
        }
        void TrackAppUsageEvent0(CGameID gameID, uint32_t eAppUsageEvent, const char *pchExtraInfo = "")
        {
            Traceprint();
        }
        void SetAccountName(const char* pchAccountName)
        {
            Traceprint();
        }
        void SetPassword(const char* pchPassword)
        {
            Traceprint();
        }
        void SetAccountCreationTime(uint32_t rtime32Time)
        {
            Traceprint();
        }
        int InitiateGameConnection2(void *pBlob, int cbMaxBlob, CSteamID steamID, CGameID nGameAppID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure, void *pvSteam2GetEncryptionKey, int cbSteam2GetEncryptionKey)
        {
            Traceprint();
            return 0;
        }
        void RefreshSteam2Login()
        {
            Traceprint();
        }
        int InitiateGameConnection3(void *pBlob, int cbMaxBlob, CSteamID steamID, CGameID gameID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure, void *pvSteam2GetEncryptionKey, int cbSteam2GetEncryptionKey)
        {
            Traceprint();
            return 0;
        }
        int InitiateGameConnection4(void *pBlob, int cbMaxBlob, CSteamID steamID, CGameID gameID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            Traceprint();
            return 0;
        }
        int InitiateGameConnection5(void *pBlob, int cbMaxBlob, CSteamID steamID, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            Traceprint();
            return 0;
        }
        int InitiateGameConnection6(void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32_t unIPServer, uint16_t usPortServer, bool bSecure)
        {
            Traceprint();
            return 0;
        }
        bool GetUserDataFolder(char *pchBuffer, int cubBuffer)
        {
            Traceprint();
            return false;
        }
        void StartVoiceRecording()
        {
            Traceprint();
        }
        void StopVoiceRecording()
        {
            Traceprint();
        }
        uint32_t GetCompressedVoice(void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten)
        {
            Traceprint();
            return 0;
        }
        uint32_t DecompressVoice0(void *pCompressed, uint32_t cbCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetAuthSessionTicket(void *pTicket, int cbMaxTicket, uint32_t *pcbTicket)
        {
            Traceprint();

            // k_HAuthTicketInvalid
            return 0;
        }
        uint32_t BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, CSteamID steamID)
        {
            Debugprint(va("%s for 0x%llx", __func__, steamID.ConvertToUint64()));

            return 0;
        }
        void EndAuthSession(CSteamID steamID)
        {
            Traceprint();
        }
        void CancelAuthTicket(uint32_t hAuthTicket)
        {
            Traceprint();
        }
        uint32_t UserHasLicenseForApp(CSteamID steamID, uint32_t appID)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetAvailableVoice0(uint32_t *pcbCompressed, uint32_t *pcbUncompressed)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetVoice0(bool bWantCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t *nBytesWritten, bool bWantUncompressed, void *pUncompressedDestBuffer, uint32_t cbUncompressedDestBufferSize, uint32_t *nUncompressBytesWritten)
        {
            Traceprint();
            return 0;
        }
        bool BIsBehindNAT()
        {
            Traceprint();
            return false;
        }
        void AdvertiseGame(CSteamID steamIDGameServer, uint32_t unIPServer, uint16_t usPortServer)
        {
            Traceprint();
        }
        uint64_t RequestEncryptedAppTicket(void *pDataToInclude, unsigned int cbDataToInclude)
        {
            return 0;
        }
        bool GetEncryptedAppTicket(void *pTicket, unsigned int cbMaxTicket, uint32_t * pcbTicket)
        {
            Traceprint();
            return false;
        }
        uint32_t DecompressVoice1(void *pCompressed, uint32_t cbCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t * nBytesWritten, uint32_t nSamplesPerSec)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetVoiceOptimalSampleRate()
        {
            Traceprint();
            return 0;
        }
        uint32_t GetAvailableVoice1(uint32_t * pcbCompressed, uint32_t * pcbUncompressed, uint32_t nUncompressedVoiceDesiredSampleRate)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetVoice1(bool bWantCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t * nBytesWritten, bool bWantUncompressed, void *pUncompressedDestBuffer, uint32_t cbUncompressedDestBufferSize, uint32_t * nUncompressBytesWritten, uint32_t nUncompressedVoiceDesiredSampleRate)
        {
            // k_EVoiceResultNoData
            return 3;
        }
        uint32_t DecompressVoice2(const void *pCompressed, uint32_t cbCompressed, void *pDestBuffer, uint32_t cbDestBufferSize, uint32_t * nBytesWritten, uint32_t nDesiredSampleRate)
        {
            Traceprint();
            return 0;
        }
        int GetGameBadgeLevel(int nSeries, bool bFoil)
        {
            Traceprint();
            return 0;
        }
        int GetPlayerSteamLevel()
        {
            Traceprint();
            return 0;
        }
        void TrackAppUsageEvent1(CGameID gameID, int eAppUsageEvent, const char *pchExtraInfo = "")
        {
            Traceprint();
        }
        uint64_t RequestStoreAuthURL(const char *pchRedirectURL)
        {
            Traceprint();
            return 0;
        }
    };

    struct SteamUser001 : Interface_t
    {
        SteamUser001()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser002 : Interface_t
    {
        SteamUser002()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser003 : Interface_t
    {
        SteamUser003()
        {
            // Missing SDK info.
        };
    };
    struct SteamUser004 : Interface_t
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
    struct SteamUser005 : Interface_t
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
            Createmethod(17, SteamUser, InitiateGameConnection1);
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
            Createmethod(35, SteamUser, TrackAppUsageEvent0);
            Createmethod(36, SteamUser, SetAccountName);
            Createmethod(37, SteamUser, SetPassword);
            Createmethod(38, SteamUser, SetAccountCreationTime);
        };
    };
    struct SteamUser006 : Interface_t
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
            Createmethod(9, SteamUser, InitiateGameConnection1);
            Createmethod(10, SteamUser, TerminateGameConnection);
            Createmethod(11, SteamUser, TrackAppUsageEvent0);
        };
    };
    struct SteamUser007 : Interface_t
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
            Createmethod(9, SteamUser, InitiateGameConnection2);
            Createmethod(10, SteamUser, TerminateGameConnection);
            Createmethod(11, SteamUser, TrackAppUsageEvent0);
            Createmethod(12, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser008 : Interface_t
    {
        SteamUser008()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection3);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser009 : Interface_t
    {
        SteamUser009()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection4);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, RefreshSteam2Login);
        };
    };
    struct SteamUser010 : Interface_t
    {
        SteamUser010()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection5);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
        };
    };
    struct SteamUser011 : Interface_t
    {
        SteamUser011()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetCompressedVoice);
            Createmethod(10, SteamUser, DecompressVoice0);
        };
    };
    struct SteamUser012 : Interface_t
    {
        SteamUser012()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
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
    struct SteamUser013 : Interface_t
    {
        SteamUser013()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
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
    struct SteamUser014 : Interface_t
    {
        SteamUser014()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
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
    struct SteamUser015 : Interface_t
    {
        SteamUser015()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice0);
            Createmethod(10, SteamUser, GetVoice0);
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
    struct SteamUser016 : Interface_t
    {
        SteamUser016()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice2);
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
    struct SteamUser017 : Interface_t
    {
        SteamUser017()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent0);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice2);
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
    struct SteamUser018 : Interface_t
    {
        SteamUser018()
        {
            Createmethod(0, SteamUser, GetHSteamUser);
            Createmethod(1, SteamUser, BLoggedOn);
            Createmethod(2, SteamUser, GetSteamID);
            Createmethod(3, SteamUser, InitiateGameConnection6);
            Createmethod(4, SteamUser, TerminateGameConnection);
            Createmethod(5, SteamUser, TrackAppUsageEvent1);
            Createmethod(6, SteamUser, GetUserDataFolder);
            Createmethod(7, SteamUser, StartVoiceRecording);
            Createmethod(8, SteamUser, StopVoiceRecording);
            Createmethod(9, SteamUser, GetAvailableVoice1);
            Createmethod(10, SteamUser, GetVoice1);
            Createmethod(11, SteamUser, DecompressVoice2);
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

    struct Steamuserloader
    {
        Steamuserloader()
        {
            Registerinterface(Interfacetype_t::USER, "SteamUser001", new SteamUser001());
            Registerinterface(Interfacetype_t::USER, "SteamUser002", new SteamUser002());
            Registerinterface(Interfacetype_t::USER, "SteamUser003", new SteamUser003());
            Registerinterface(Interfacetype_t::USER, "SteamUser004", new SteamUser004());
            Registerinterface(Interfacetype_t::USER, "SteamUser005", new SteamUser005());
            Registerinterface(Interfacetype_t::USER, "SteamUser006", new SteamUser006());
            Registerinterface(Interfacetype_t::USER, "SteamUser007", new SteamUser007());
            Registerinterface(Interfacetype_t::USER, "SteamUser008", new SteamUser008());
            Registerinterface(Interfacetype_t::USER, "SteamUser009", new SteamUser009());
            Registerinterface(Interfacetype_t::USER, "SteamUser010", new SteamUser010());
            Registerinterface(Interfacetype_t::USER, "SteamUser011", new SteamUser011());
            Registerinterface(Interfacetype_t::USER, "SteamUser012", new SteamUser012());
            Registerinterface(Interfacetype_t::USER, "SteamUser013", new SteamUser013());
            Registerinterface(Interfacetype_t::USER, "SteamUser014", new SteamUser014());
            Registerinterface(Interfacetype_t::USER, "SteamUser015", new SteamUser015());
            Registerinterface(Interfacetype_t::USER, "SteamUser016", new SteamUser016());
            Registerinterface(Interfacetype_t::USER, "SteamUser017", new SteamUser017());
            Registerinterface(Interfacetype_t::USER, "SteamUser018", new SteamUser018());
        }
    };
    static Steamuserloader Interfaceloader{};
}
