//==========================  Open Steamworks  ================================
//
// This file is part of the Open Steamworks project. All individuals associated
// with this project do not claim ownership of the contents
//
// The code, comments, and all related files, projects, resources,
// redistributables included with this project are Copyright Valve Corporation.
// Additionally, Valve, the Valve logo, Half-Life, the Half-Life logo, the
// Lambda logo, Steam, the Steam logo, Team Fortress, the Team Fortress logo,
// Opposing Force, Day of Defeat, the Day of Defeat logo, Counter-Strike, the
// Counter-Strike logo, Source, the Source logo, and Counter-Strike Condition
// Zero are trademarks and or registered trademarks of Valve Corporation.
// All other trademarks are property of their respective owners.
//
//=============================================================================

#ifndef CSTEAMID_H
#define CSTEAMID_H
#define NO_CSTEAMID_STL

#include "../Steam.hpp"

#ifdef _S4N_
#define sprintf(...)
#else
#include <cstdio>
#ifndef NO_CSTEAMID_STL
#include <iostream>
#endif //NO_CSTEAMID_STL
#endif
#include <cstdint>

enum EResult
{
    k_EResultOK = 1,                            // success
    k_EResultFail = 2,                          // generic failure
    k_EResultNoConnection = 3,                  // no/failed network connection
    k_EResultNoConnectionRetry = 4,             // OBSOLETE - removed
    k_EResultInvalidPassword = 5,               // password/ticket is invalid
    k_EResultLoggedInElsewhere = 6,             // same user logged in elsewhere
    k_EResultInvalidProtocolVer = 7,            // protocol version is incorrect
    k_EResultInvalidParam = 8,                  // a parameter is incorrect
    k_EResultFileNotFound = 9,                  // file was not found
    k_EResultBusy = 10,                         // called method busy - action not taken
    k_EResultInvalidState = 11,                 // called object was in an invalid state
    k_EResultInvalidName = 12,                  // name is invalid
    k_EResultInvalidEmail = 13,                 // email is invalid
    k_EResultDuplicateName = 14,                // name is not unique
    k_EResultAccessDenied = 15,                 // access is denied
    k_EResultTimeout = 16,                      // operation timed out
    k_EResultBanned = 17,                       // VAC2 banned
    k_EResultAccountNotFound = 18,              // account not found
    k_EResultInvalidSteamID = 19,               // steamID is invalid
    k_EResultServiceUnavailable = 20,           // The requested service is currently unavailable
    k_EResultNotLoggedOn = 21,                  // The user is not logged on
    k_EResultPending = 22,                      // Request is pending (may be in process, or waiting on third party)
    k_EResultEncryptionFailure = 23,            // Encryption or Decryption failed
    k_EResultInsufficientPrivilege = 24,        // Insufficient privilege
    k_EResultLimitExceeded = 25,                // Too much of a good thing
    k_EResultRevoked = 26,                      // Access has been revoked (used for revoked guest passes)
    k_EResultExpired = 27,                      // License/Guest pass the user is trying to access is expired
    k_EResultAlreadyRedeemed = 28,              // Guest pass has already been redeemed by account, cannot be acked again
    k_EResultDuplicateRequest = 29,             // The request is a duplicate and the action has already occurred in the past, ignored this time
    k_EResultAlreadyOwned = 30,                 // All the games in this guest pass redemption request are already owned by the user
    k_EResultIPNotFound = 31,                   // IP address not found
    k_EResultPersistFailed = 32,                // failed to write change to the data store
    k_EResultLockingFailed = 33,                // failed to acquire access lock for this operation
    k_EResultLogonSessionReplaced = 34,
    k_EResultConnectFailed = 35,
    k_EResultHandshakeFailed = 36,
    k_EResultIOFailure = 37,
    k_EResultRemoteDisconnect = 38,
    k_EResultShoppingCartNotFound = 39,         // failed to find the shopping cart requested
    k_EResultBlocked = 40,                      // a user didn't allow it
    k_EResultIgnored = 41,                      // target is ignoring sender
    k_EResultNoMatch = 42,                      // nothing matching the request found
    k_EResultAccountDisabled = 43,
    k_EResultServiceReadOnly = 44,              // this service is not accepting content changes right now
    k_EResultAccountNotFeatured = 45,           // account doesn't have value, so this feature isn't available
    k_EResultAdministratorOK = 46,              // allowed to take this action, but only because requester is admin
    k_EResultContentVersion = 47,               // A Version mismatch in content transmitted within the Steam protocol.
    k_EResultTryAnotherCM = 48,                 // The current CM can't service the user making a request, user should try another.
    k_EResultPasswordRequiredToKickSession = 49,// You are already logged in elsewhere, this cached credential login has failed.
    k_EResultAlreadyLoggedInElsewhere = 50,     // You are already logged in elsewhere, you must wait
    k_EResultSuspended = 51,                    // Long running operation (content download) suspended/paused
    k_EResultCancelled = 52,                    // Operation canceled (typically by user: content download)
    k_EResultDataCorruption = 53,               // Operation canceled because data is ill formed or unrecoverable
    k_EResultDiskFull = 54,                     // Operation canceled - not enough disk space.
    k_EResultRemoteCallFailed = 55,             // an remote call or IPC call failed
    k_EResultPasswordUnset = 56,                // Password could not be verified as it's unset server side
    k_EResultPSNAccountUnlinked = 57,           // Attempt to logon from a PS3 failed because the PSN online id is not linked to a Steam account
    k_EResultPSNTicketInvalid = 58,             // PSN ticket was invalid
    k_EResultPSNAccountAlreadyLinked = 59,      // PSN account is already linked to some other account, must explicitly request to replace/delete the link first
    k_EResultRemoteFileConflict = 60,           // The sync cannot resume due to a conflict between the local and remote files
    k_EResultIllegalPassword = 61,              // The requested new password is not legal
    k_EResultSameAsPreviousValue = 62,          // new value is the same as the old one ( secret question and answer )
    k_EResultAccountLogonDenied = 63,           // account login denied due to 2nd factor authentication failure
    k_EResultCannotUseOldPassword = 64,         // The requested new password is not legal
    k_EResultInvalidLoginAuthCode = 65,         // account login denied due to auth code invalid
    k_EResultAccountLogonDeniedNoMail = 66,     // account login denied due to 2nd factor auth failure - and no mail has been sent
    k_EResultHardwareNotCapableOfIPT = 67,
    k_EResultIPTInitError = 68,
    k_EResultParentalControlRestrictions = 69,  // Operation failed due to parental control restrictions for current user
    k_EResultFacebookQueryError = 70,           // Facebook query returned an error
    k_EResultExpiredLoginAuthCode = 71,         // Expired Login Auth Code
    k_EResultIPLoginRestrictionFailed = 72,     // IP Login Restriction Failed
    k_EResultAccountLockedDown = 73,            // Account Locked Down
    k_EResultAccountLogonDeniedVerifiedEmailRequired = 74, // Account Logon Denied Verified Email Required
    k_EResultNoMatchingURL = 75,                // No matching URL
    k_EResultBadResponse = 76,                  // parse failure, missing field, etc.
    k_EResultRequirePasswordReEntry = 77,       // The user cannot complete the action until they re-enter their password
    k_EResultValueOutOfRange = 78,              // the value entered is outside the acceptable range
    k_EResultUnexpectedError = 79,              //
    k_EResultFeatureDisabled = 80,              //
    k_EResultInvalidCEGSubmission = 81,
    k_EResultRestrictedDevice = 82,
    k_EResultRegionLocked = 83,
    k_EResultRateLimitExceeded = 84,
};

enum EAccountType
{
    k_EAccountTypeInvalid = 0,
    k_EAccountTypeIndividual = 1,       // single user account
    k_EAccountTypeMultiseat = 2,        // multiseat (e.g. cybercafe) account
    k_EAccountTypeGameServer = 3,       // game server account
    k_EAccountTypeAnonGameServer = 4,   // anonymous game server account
    k_EAccountTypePending = 5,          // pending
    k_EAccountTypeContentServer = 6,    // content server
    k_EAccountTypeClan = 7,
    k_EAccountTypeChat = 8,
    k_EAccountTypeConsoleUser = 9,      // Fake SteamID for local PSN account on PS3 or Live account on 360, etc.
    k_EAccountTypeAnonUser = 10,

    // Max of 16 items in this field
    k_EAccountTypeMax
};

enum EChatSteamIDInstanceFlags
{
    k_EChatAccountInstanceMask = 4095,
    k_EChatInstanceFlagClan = 524288,
    k_EChatInstanceFlagLobby = 262144,
    k_EChatInstanceFlagMMSLobby = 131072,
};

#pragma pack( push, 1 )

// Steam ID structure (64 bits total)
class CSteamID
{
    public:

    //-----------------------------------------------------------------------------
    // Purpose: Constructor
    //-----------------------------------------------------------------------------
    CSteamID()
    {
        m_steamid.m_comp.m_unAccountID = 0;
        m_steamid.m_comp.m_EAccountType = k_EAccountTypeInvalid;
        m_steamid.m_comp.m_EUniverse = 0;
        m_steamid.m_comp.m_unAccountInstance = 0;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Constructor
    // Input  : unAccountID - 32-bit account ID
    //   eUniverse -  Universe this account belongs to
    //   eAccountType - Type of account
    //-----------------------------------------------------------------------------
    CSteamID(uint32_t unAccountID, uint32_t eUniverse, EAccountType eAccountType)
    {
        Set(unAccountID, eUniverse, eAccountType);
    }


    //-----------------------------------------------------------------------------
    // Purpose: Constructor
    // Input  : unAccountID - 32-bit account ID
    //   unAccountInstance - instance
    //   eUniverse -  Universe this account belongs to
    //   eAccountType - Type of account
    //-----------------------------------------------------------------------------
    CSteamID(uint32_t unAccountID, unsigned int unAccountInstance, uint32_t eUniverse, EAccountType eAccountType)
    {
        #if defined(_SERVER) && defined(Assert)
        Assert(!((k_EAccountTypeIndividual == eAccountType) && (unAccountInstance > k_unSteamUserWebInstance))); // enforce that for individual accounts, instance is always 1
        #endif // _SERVER
        InstancedSet(unAccountID, unAccountInstance, eUniverse, eAccountType);
    }


    //-----------------------------------------------------------------------------
    // Purpose: Constructor
    // Input  : ulSteamID -  64-bit representation of a Steam ID
    // Note: Will not accept a uint32 or int32 as input, as that is a probable mistake.
    //   See the stubbed out overloads in the private: section for more info.
    //-----------------------------------------------------------------------------
    CSteamID(uint64_t ulSteamID)
    {
        SetFromUint64(ulSteamID);
    }


    //-----------------------------------------------------------------------------
    // Purpose: Sets parameters for steam ID
    // Input  : unAccountID - 32-bit account ID
    //   eUniverse -  Universe this account belongs to
    //   eAccountType - Type of account
    //-----------------------------------------------------------------------------
    void Set(uint32_t unAccountID, uint32_t eUniverse, EAccountType eAccountType)
    {
        m_steamid.m_comp.m_unAccountID = unAccountID;
        m_steamid.m_comp.m_EUniverse = eUniverse;
        m_steamid.m_comp.m_EAccountType = eAccountType;

        if(eAccountType == k_EAccountTypeClan)
        {
            m_steamid.m_comp.m_unAccountInstance = 0;
        }
        else
        {
            // by default we pick the desktop instance
            m_steamid.m_comp.m_unAccountInstance = 1;
        }
    }


    //-----------------------------------------------------------------------------
    // Purpose: Sets parameters for steam ID
    // Input  : unAccountID - 32-bit account ID
    //   eUniverse -  Universe this account belongs to
    //   eAccountType - Type of account
    //-----------------------------------------------------------------------------
    void InstancedSet(uint32_t unAccountID, uint32_t unInstance, uint32_t eUniverse, EAccountType eAccountType)
    {
        m_steamid.m_comp.m_unAccountID = unAccountID;
        m_steamid.m_comp.m_EUniverse = eUniverse;
        m_steamid.m_comp.m_EAccountType = eAccountType;
        m_steamid.m_comp.m_unAccountInstance = unInstance;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Initializes a steam ID from its 52 bit parts and universe/type
    // Input  : ulIdentifier - 52 bits of goodness
    //-----------------------------------------------------------------------------
    void FullSet(uint64_t ulIdentifier, uint32_t eUniverse, EAccountType eAccountType)
    {
        m_steamid.m_comp.m_unAccountID = (ulIdentifier & 0xFFFFFFFF);                   // account ID is low 32 bits
        m_steamid.m_comp.m_unAccountInstance = ((ulIdentifier >> 32) & 0x000FFFFF);     // account instance is next 20 bits
        m_steamid.m_comp.m_EUniverse = eUniverse;
        m_steamid.m_comp.m_EAccountType = eAccountType;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Initializes a steam ID from its 64-bit representation
    // Input  : ulSteamID -  64-bit representation of a Steam ID
    //-----------------------------------------------------------------------------
    void SetFromUint64(uint64_t ulSteamID)
    {
        m_steamid.m_unAll64Bits = ulSteamID;
    }

    #ifdef TSTEAMGLOBALUSERID_H // NO_STEAM would not include TSteamGlobalUserID.h, thus we lose this facet of CSteamID
    //-----------------------------------------------------------------------------
    // Purpose: Initializes a steam ID from a Steam2 ID structure
    // Input: pTSteamGlobalUserID - Steam2 ID to convert
    //   eUniverse -    universe this ID belongs to
    //-----------------------------------------------------------------------------
    void SetFromSteam2(TSteamGlobalUserID *pTSteamGlobalUserID, EUniverse eUniverse)
    {
        m_steamid.m_comp.m_unAccountID = pTSteamGlobalUserID->m_SteamLocalUserID.Split.Low32bits * 2 +
            pTSteamGlobalUserID->m_SteamLocalUserID.Split.High32bits;
        m_steamid.m_comp.m_EUniverse = eUniverse;  // set the universe
        m_steamid.m_comp.m_EAccountType = k_EAccountTypeIndividual; // Steam 2 accounts always map to account type of individual
        m_steamid.m_comp.m_unAccountInstance = k_unSteamUserDesktopInstance; // Steam2 only knew desktop instances
    }

    //-----------------------------------------------------------------------------
    // Purpose: Fills out a Steam2 ID structure
    // Input: pTSteamGlobalUserID - Steam2 ID to write to
    //-----------------------------------------------------------------------------
    void ConvertToSteam2(TSteamGlobalUserID *pTSteamGlobalUserID) const
    {
        // only individual accounts have any meaning in Steam 2, only they can be mapped
        // Assert( m_steamid.m_comp.m_EAccountType == k_EAccountTypeIndividual );

        pTSteamGlobalUserID->m_SteamInstanceID = 0;
        pTSteamGlobalUserID->m_SteamLocalUserID.Split.High32bits = m_steamid.m_comp.m_unAccountID % 2;
        pTSteamGlobalUserID->m_SteamLocalUserID.Split.Low32bits = m_steamid.m_comp.m_unAccountID / 2;
    }
    #endif // TSTEAMGLOBALUSERID_H

    //-----------------------------------------------------------------------------
    // Purpose: Converts steam ID to its 64-bit representation
    // Output : 64-bit representation of a Steam ID
    //-----------------------------------------------------------------------------
    long long unsigned int ConvertToUint64() const
    {
        return m_steamid.m_unAll64Bits;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Converts the static parts of a steam ID to a 64-bit representation.
    //   For multiseat accounts, all instances of that account will have the
    //   same static account key, so they can be grouped together by the static
    //   account key.
    // Output : 64-bit static account key
    //-----------------------------------------------------------------------------
    uint64_t GetStaticAccountKey() const
    {
        // note we do NOT include the account instance (which is a dynamic property) in the static account key
        return (uint64_t)((((uint64_t)m_steamid.m_comp.m_EUniverse) << 56) + ((uint64_t)m_steamid.m_comp.m_EAccountType << 52) + m_steamid.m_comp.m_unAccountID);
    }


    //-----------------------------------------------------------------------------
    // Purpose: create an anonymous game server login to be filled in by the AM
    //-----------------------------------------------------------------------------
    void CreateBlankAnonLogon(uint32_t eUniverse)
    {
        m_steamid.m_comp.m_unAccountID = 0;
        m_steamid.m_comp.m_EAccountType = k_EAccountTypeAnonGameServer;
        m_steamid.m_comp.m_EUniverse = eUniverse;
        m_steamid.m_comp.m_unAccountInstance = 0;
    }


    //-----------------------------------------------------------------------------
    // Purpose: create an anonymous game server login to be filled in by the AM
    //-----------------------------------------------------------------------------
    void CreateBlankAnonUserLogon(uint32_t eUniverse)
    {
        m_steamid.m_comp.m_unAccountID = 0;
        m_steamid.m_comp.m_EAccountType = k_EAccountTypeAnonUser;
        m_steamid.m_comp.m_EUniverse = eUniverse;
        m_steamid.m_comp.m_unAccountInstance = 0;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this an anonymous game server login that will be filled in?
    //-----------------------------------------------------------------------------
    bool BBlankAnonAccount() const
    {
        return m_steamid.m_comp.m_unAccountID == 0 && BAnonAccount() && m_steamid.m_comp.m_unAccountInstance == 0;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this a game server account id?
    //-----------------------------------------------------------------------------
    bool BGameServerAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeGameServer || m_steamid.m_comp.m_EAccountType == k_EAccountTypeAnonGameServer;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this a content server account id?
    //-----------------------------------------------------------------------------
    bool BContentServerAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeContentServer;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Is this a clan account id?
    //-----------------------------------------------------------------------------
    bool BClanAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeClan;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Is this a chat account id?
    //-----------------------------------------------------------------------------
    bool BChatAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeChat;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this a chat account id?
    //-----------------------------------------------------------------------------
    bool IsLobby() const
    {
        return (m_steamid.m_comp.m_EAccountType == k_EAccountTypeChat)
            && (m_steamid.m_comp.m_unAccountInstance & ((0x000FFFFF + 1) >> 2));
    }


    //-----------------------------------------------------------------------------
    // Purpose: Is this an individual user account id?
    //-----------------------------------------------------------------------------
    bool BIndividualAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeIndividual || m_steamid.m_comp.m_EAccountType == k_EAccountTypeConsoleUser;
    }


    //-----------------------------------------------------------------------------
    // Purpose: Is this an anonymous account?
    //-----------------------------------------------------------------------------
    bool BAnonAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeAnonUser || m_steamid.m_comp.m_EAccountType == k_EAccountTypeAnonGameServer;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this an anonymous user account? ( used to create an account or reset a password )
    //-----------------------------------------------------------------------------
    bool BAnonUserAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeAnonUser;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Is this a faked up Steam ID for a PSN friend account?
    //-----------------------------------------------------------------------------
    bool BConsoleUserAccount() const
    {
        return m_steamid.m_comp.m_EAccountType == k_EAccountTypeConsoleUser;
    }


    // simple accessors
    void SetAccountID(uint32_t unAccountID) { m_steamid.m_comp.m_unAccountID = unAccountID; }
    void SetAccountInstance(uint32_t unInstance) { m_steamid.m_comp.m_unAccountInstance = unInstance; }
    void ClearIndividualInstance() { if(BIndividualAccount()) m_steamid.m_comp.m_unAccountInstance = 0; }
    bool HasNoIndividualInstance() const { return BIndividualAccount() && (m_steamid.m_comp.m_unAccountInstance == 0); }
    uint32_t GetAccountID() const { return m_steamid.m_comp.m_unAccountID; }
    uint32_t GetUnAccountInstance() const { return m_steamid.m_comp.m_unAccountInstance; }
    EAccountType GetEAccountType() const { return (EAccountType)m_steamid.m_comp.m_EAccountType; }
    uint32_t GetEUniverse() const { return m_steamid.m_comp.m_EUniverse; }
    void SetEUniverse(uint32_t eUniverse) { m_steamid.m_comp.m_EUniverse = eUniverse; }
    bool IsValid() const;

    // this set of functions is hidden, will be moved out of class
    explicit CSteamID(const char *pchSteamID, uint32_t eDefaultUniverse = 0);

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996)
    #endif

    const char *Render() const    // renders this steam ID to string
    {
        const int k_cBufLen = 30;
        const int k_cBufs = 4;
        char *pchBuf;

        static char rgchBuf[k_cBufs][k_cBufLen];
        static int nBuf = 0;

        pchBuf = rgchBuf[nBuf++];
        nBuf %= k_cBufs;

        switch(m_steamid.m_comp.m_EAccountType)
        {
            case k_EAccountTypeInvalid:
            case k_EAccountTypeIndividual:
                sprintf(pchBuf, "STEAM_0:%u:%u", (m_steamid.m_comp.m_unAccountID % 2) ? 1 : 0, (uint32_t)m_steamid.m_comp.m_unAccountID / 2);
                break;
            default:
                sprintf(pchBuf, "%llu", ConvertToUint64());
        }
        return pchBuf;
    }
    static const char *Render(uint64_t ulSteamID) // static method to render a uint64 representation of a steam ID to a string
    {
        return CSteamID(ulSteamID).Render();
    }

    const char *SteamRender() const // renders this steam ID to string using the new rendering style
    {
        const int k_cBufLen = 37;
        const int k_cBufs = 4;
        char *pchBuf;

        static char rgchBuf[k_cBufs][k_cBufLen];
        static int nBuf = 0;

        pchBuf = rgchBuf[nBuf++];
        nBuf %= k_cBufs;

        switch(m_steamid.m_comp.m_EAccountType)
        {
            case k_EAccountTypeAnonGameServer:
                sprintf(pchBuf, "[A:%u:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
                break;
            case k_EAccountTypeGameServer:
                sprintf(pchBuf, "[G:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            case k_EAccountTypeMultiseat:
                sprintf(pchBuf, "[M:%u:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
                break;
            case k_EAccountTypePending:
                sprintf(pchBuf, "[P:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            case k_EAccountTypeContentServer:
                sprintf(pchBuf, "[C:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            case k_EAccountTypeClan:
                sprintf(pchBuf, "[g:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            case k_EAccountTypeChat:
                switch(m_steamid.m_comp.m_unAccountInstance & ~0x00000FFF)
                {
                    case ((0x000FFFFF + 1) >> 1):
                        sprintf(pchBuf, "[c:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                        break;
                    case ((0x000FFFFF + 1) >> 2):
                        sprintf(pchBuf, "[L:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                        break;
                    default:
                        sprintf(pchBuf, "[T:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                        break;
                }
                break;
            case k_EAccountTypeInvalid:
                sprintf(pchBuf, "[I:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            case k_EAccountTypeIndividual:
                sprintf(pchBuf, "[U:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
            default:
                sprintf(pchBuf, "[i:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
                break;
        }

        return pchBuf;
    }

    static const char *SteamRender(uint64_t ulSteamID) // static method to render a uint64 representation of a steam ID to a string
    {
        return CSteamID(ulSteamID).SteamRender();
    }

    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    void SetFromString(const char *pchSteamID, uint32_t eDefaultUniverse);
    bool SetFromSteam2String(const char *pchSteam2ID, uint32_t eUniverse);

    bool operator==(const CSteamID &val) const { return m_steamid.m_unAll64Bits == val.m_steamid.m_unAll64Bits; }
    bool operator!=(const CSteamID &val) const { return !operator==(val); }
    bool operator<(const CSteamID &val) const { return m_steamid.m_unAll64Bits < val.m_steamid.m_unAll64Bits; }
    bool operator>(const CSteamID &val) const { return m_steamid.m_unAll64Bits > val.m_steamid.m_unAll64Bits; }

    // DEBUG function
    bool BValidExternalSteamID() const;

    private:
    // These are defined here to prevent accidental implicit conversion of a u32AccountID to a CSteamID.
    // If you get a compiler error about an ambiguous constructor/function then it may be because you're
    // passing a 32-bit int to a function that takes a CSteamID. You should explicitly create the SteamID
    // using the correct Universe and account Type/Instance values.
    CSteamID(uint32_t);
    CSteamID(int32_t);

    // 64 bits total
    union SteamID_t
    {
        struct SteamIDComponent_t
        {
            #ifdef VALVE_BIG_ENDIAN
            EUniverse   m_EUniverse : 8; // universe this account belongs to
            unsigned int  m_EAccountType : 4;   // type of account - can't show as EAccountType, due to signed / unsigned difference
            unsigned int  m_unAccountInstance : 20; // dynamic instance ID
            uint32    m_unAccountID : 32;   // unique account identifier
            #else
            uint32_t   m_unAccountID : 32;   // unique account identifier
            unsigned int  m_unAccountInstance : 20; // dynamic instance ID
            unsigned int  m_EAccountType : 4;   // type of account - can't show as EAccountType, due to signed / unsigned difference
            uint32_t   m_EUniverse : 8; // universe this account belongs to
            #endif
        } m_comp;

        uint64_t m_unAll64Bits;
    } m_steamid;
};

#ifndef NO_CSTEAMID_STL
inline std::ostream &operator<<(std::ostream &out, const CSteamID &s)
{
    out << s.Render();
    return out;
}
#endif //NO_CSTEAMID_STL

inline bool CSteamID::IsValid() const
{
    if(m_steamid.m_comp.m_EAccountType <= k_EAccountTypeInvalid || m_steamid.m_comp.m_EAccountType >= k_EAccountTypeMax)
        return false;

    if(m_steamid.m_comp.m_EUniverse <= 0 || m_steamid.m_comp.m_EUniverse >= 11)
        return false;

    if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeIndividual)
    {
        if(m_steamid.m_comp.m_unAccountID == 0 || m_steamid.m_comp.m_unAccountInstance > 4)
            return false;
    }

    if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeClan)
    {
        if(m_steamid.m_comp.m_unAccountID == 0 || m_steamid.m_comp.m_unAccountInstance != 0)
            return false;
    }

    if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeGameServer)
    {
        if(m_steamid.m_comp.m_unAccountID == 0)
            return false;
        // Any limit on instances?  We use them for local users and bots
    }
    return true;
}

// generic invalid CSteamID
#define k_steamIDNil CSteamID()

// This steamID comes from a user game connection to an out of date GS that hasnt implemented the protocol
// to provide its steamID
#define k_steamIDOutofDateGS CSteamID( 0, 0, k_EUniverseInvalid, k_EAccountTypeInvalid )
// This steamID comes from a user game connection to an sv_lan GS
#define k_steamIDLanModeGS CSteamID( 0, 0, k_EUniversePublic, k_EAccountTypeInvalid )
// This steamID can come from a user game connection to a GS that has just booted but hasnt yet even initialized
// its steam3 component and started logging on.
#define k_steamIDNotInitYetGS CSteamID( 1, 0, k_EUniverseInvalid, k_EAccountTypeInvalid )
// This steamID can come from a user game connection to a GS that isn't using the steam authentication system but still
// wants to support the "Join Game" option in the friends list
#define k_steamIDNonSteamGS CSteamID( 2, 0, k_EUniverseInvalid, k_EAccountTypeInvalid )


// Returns the matching chat steamID, with the default instance of 0
// If the steamID passed in is already of type k_EAccountTypeChat it will be returned with the same instance
inline CSteamID ChatIDFromSteamID(const CSteamID &steamID)
{
    if(steamID.GetEAccountType() == k_EAccountTypeClan)
    {
        return CSteamID(steamID.GetAccountID(), k_EChatInstanceFlagClan, steamID.GetEUniverse(), k_EAccountTypeChat);
    }
    else if(steamID.GetEAccountType() == k_EAccountTypeChat)
    {
        return steamID;
    }
    else
    {
        return k_steamIDNil;
    }
}

// Returns the matching clan steamID, with the default instance of 0
// If the steamID passed in is already of type k_EAccountTypeClan it will be returned with the same instance
inline CSteamID ClanIDFromSteamID(const CSteamID &steamID)
{
    if(steamID.GetEAccountType() == k_EAccountTypeChat && steamID.GetUnAccountInstance() & k_EChatInstanceFlagClan)
    {
        return CSteamID(steamID.GetAccountID(), 0, steamID.GetEUniverse(), k_EAccountTypeClan);
    }
    else if(steamID.GetEAccountType() == k_EAccountTypeClan)
    {
        return steamID;
    }
    else
    {
        return k_steamIDNil;
    }
}

// Asserts steamID type before conversion
CSteamID ChatIDFromClanID(const CSteamID &steamIDClan);
// Asserts steamID type before conversion
CSteamID ClanIDFromChatID(const CSteamID &steamIDChat);




#pragma pack( pop )

#pragma pack( push, 1 )

//-----------------------------------------------------------------------------
// Purpose: encapsulates an appID/modID pair
//-----------------------------------------------------------------------------
class CGameID
{
    public:

    CGameID()
    {
        m_gameID.m_nType = k_EGameIDTypeApp;
        m_gameID.m_nAppID = 0;
        m_gameID.m_nModID = 0;
    }

    explicit CGameID(uint64_t ulGameID)
    {
        m_ulGameID = ulGameID;
    }

    explicit CGameID(int32_t nAppID)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = nAppID;
    }

    explicit CGameID(uint32_t nAppID)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = nAppID;
    }

    CGameID(uint32_t nAppID, uint32_t nModID)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = nAppID;
        m_gameID.m_nModID = nModID;
        m_gameID.m_nType = k_EGameIDTypeGameMod;
    }

    // Hidden functions used only by Steam
    explicit CGameID(const char *pchGameID);
    const char *Render() const;     // render this Game ID to string
    static const char *Render(uint64_t ulGameID);  // static method to render a uint64 representation of a Game ID to a string

                                                        // must include checksum_crc.h first to get this functionality
    #if defined( CHECKSUM_CRC_H )
    CGameID(uint32 nAppID, const char *pchModPath)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = nAppID;
        m_gameID.m_nType = k_EGameIDTypeGameMod;

        char rgchModDir[MAX_PATH];
        Q_FileBase(pchModPath, rgchModDir, sizeof(rgchModDir));
        CRC32_t crc32;
        CRC32_Init(&crc32);
        CRC32_ProcessBuffer(&crc32, rgchModDir, Q_strlen(rgchModDir));
        CRC32_Final(&crc32);

        // set the high-bit on the mod-id
        // reduces crc32 to 31bits, but lets us use the modID as a guaranteed unique
        // replacement for appID's
        m_gameID.m_nModID = crc32 | (0x80000000);
    }

    CGameID(const char *pchExePath, const char *pchAppName)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = k_uAppIdInvalid;
        m_gameID.m_nType = k_EGameIDTypeShortcut;

        CRC32_t crc32;
        CRC32_Init(&crc32);
        CRC32_ProcessBuffer(&crc32, pchExePath, Q_strlen(pchExePath));
        CRC32_ProcessBuffer(&crc32, pchAppName, Q_strlen(pchAppName));
        CRC32_Final(&crc32);

        // set the high-bit on the mod-id
        // reduces crc32 to 31bits, but lets us use the modID as a guaranteed unique
        // replacement for appID's
        m_gameID.m_nModID = crc32 | (0x80000000);
    }

    #if defined( VSTFILEID_H )

    CGameID(VstFileID vstFileID)
    {
        m_ulGameID = 0;
        m_gameID.m_nAppID = k_uAppIdInvalid;
        m_gameID.m_nType = k_EGameIDTypeP2P;

        CRC32_t crc32;
        CRC32_Init(&crc32);
        const char *pchFileId = vstFileID.Render();
        CRC32_ProcessBuffer(&crc32, pchFileId, Q_strlen(pchFileId));
        CRC32_Final(&crc32);

        // set the high-bit on the mod-id
        // reduces crc32 to 31bits, but lets us use the modID as a guaranteed unique
        // replacement for appID's
        m_gameID.m_nModID = crc32 | (0x80000000);
    }

    #endif /* VSTFILEID_H */

    #endif /* CHECKSUM_CRC_H */


    uint64_t ToUint64() const
    {
        return m_ulGameID;
    }

    uint64_t *GetUint64Ptr()
    {
        return &m_ulGameID;
    }

    bool IsMod() const
    {
        return (m_gameID.m_nType == k_EGameIDTypeGameMod);
    }

    bool IsShortcut() const
    {
        return (m_gameID.m_nType == k_EGameIDTypeShortcut);
    }

    bool IsP2PFile() const
    {
        return (m_gameID.m_nType == k_EGameIDTypeP2P);
    }

    bool IsSteamApp() const
    {
        return (m_gameID.m_nType == k_EGameIDTypeApp);
    }

    uint32_t ModID() const
    {
        return m_gameID.m_nModID;
    }

    uint32_t AppID() const
    {
        return m_gameID.m_nAppID;
    }

    bool operator == (const CGameID &rhs) const
    {
        return m_ulGameID == rhs.m_ulGameID;
    }

    bool operator != (const CGameID &rhs) const
    {
        return !(*this == rhs);
    }

    bool operator < (const CGameID &rhs) const
    {
        return (m_ulGameID < rhs.m_ulGameID);
    }

    bool IsValid() const
    {
        // each type has it's own invalid fixed point:
        switch(m_gameID.m_nType)
        {
            case k_EGameIDTypeApp:
                return m_gameID.m_nAppID != 0;
                break;
            case k_EGameIDTypeGameMod:
                return m_gameID.m_nAppID != 0 && m_gameID.m_nModID & 0x80000000;
                break;
            case k_EGameIDTypeShortcut:
                return (m_gameID.m_nModID & 0x80000000) != 0;
                break;
            case k_EGameIDTypeP2P:
                return m_gameID.m_nAppID == 0 && m_gameID.m_nModID & 0x80000000;
                break;
            default:
                #if defined(Assert)
                Assert(false);
                #endif
                return false;
        }

    }

    void Reset()
    {
        m_ulGameID = 0;
    }



    private:

    enum EGameIDType
    {
        k_EGameIDTypeApp = 0,
        k_EGameIDTypeGameMod = 1,
        k_EGameIDTypeShortcut = 2,
        k_EGameIDTypeP2P = 3,
    };

    struct GameID_t
    {
        #ifdef VALVE_BIG_ENDIAN
        unsigned int m_nModID : 32;
        unsigned int m_nType : 8;
        unsigned int m_nAppID : 24;
        #else
        unsigned int m_nAppID : 24;
        unsigned int m_nType : 8;
        unsigned int m_nModID : 32;
        #endif
    };

    union
    {
        uint64_t m_ulGameID;
        GameID_t m_gameID;
    };
};

#pragma pack( pop )

#endif // CSTEAMID_H
