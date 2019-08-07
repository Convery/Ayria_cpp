/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#pragma once
#include "Stdinclude.hpp"
#include "Auxiliary/CSteamID.hpp"
#include "../Common/Matchmaking.hpp"

namespace Steam
{
    // Keep the global state together.
    struct Globalstate_t
    {
        uint64_t UserID;
        std::string Path;
        std::string Username;
        std::string Language;
        uint32_t ApplicationID;
        uint64_t Startuptimestamp;
    };
    extern Globalstate_t Global;

    // A Steam interface is a class that proxies calls to their backend.
    // As such we can create a generic interface with just callbacks.
    using Interface_t = struct { void *VTABLE[70]; };

    // The types of interfaces provided as of writing.
    enum class Interfacetype_t
    {
        UGC,
        APPS,
        USER,
        HTTP,
        MUSIC,
        UTILS,
        VIDEO,
        CLIENT,
        FRIENDS,
        APPLIST,
        INVENTORY,
        USERSTATS,
        CONTROLLER,
        GAMESERVER,
        NETWORKING,
        HTMLSURFACE,
        MUSICREMOTE,
        SCREENSHOTS,
        MATCHMAKING,
        REMOTESTORAGE,
        CONTENTSERVER,
        UNIFIEDMESSAGES,
        GAMESERVERSTATS,
        MATCHMAKINGSERVERS,
        MASTERSERVERUPDATER,
        MAX,
        INVALID
    };

    // Return a specific version of the interface by name or the latest by their category / type.
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t *Interface);
    Interface_t **Fetchinterface(std::string_view Name);
    Interface_t **Fetchinterface(Interfacetype_t Type);
    bool Scanforinterfaces(std::string_view Filename);
    size_t Getinterfaceversion(Interfacetype_t Type);

    // Block and wait for Steams IPC initialization event as some games need it.
    // Also redirect module lookups for legacy compatibility.
    void Redirectmodulehandle();
    void InitializeIPC();

    // Async replies.
    namespace Callbacks
    {
        void Completerequest(uint64_t RequestID, int32_t Callbacktype, void *Databuffer);
        void Registercallback(void *Callback, int32_t Callbacktype);
        uint64_t Createrequest();
        void Runcallbacks();

        #pragma region Callbackstructs
        enum ECallbackType
        {
            k_iSteamUserCallbacks = 100,
            k_iSteamGameServerCallbacks = 200,
            k_iSteamFriendsCallbacks = 300,
            k_iSteamBillingCallbacks = 400,
            k_iSteamMatchmakingCallbacks = 500,
            k_iSteamContentServerCallbacks = 600,
            k_iSteamUtilsCallbacks = 700,
            k_iClientFriendsCallbacks = 800,
            k_iClientUserCallbacks = 900,
            k_iSteamAppsCallbacks = 1000,
            k_iSteamUserStatsCallbacks = 1100,
            k_iSteamNetworkingCallbacks = 1200,
            k_iClientRemoteStorageCallbacks = 1300,
            k_iSteamUserItemsCallbacks = 1400,
            k_iSteamGameServerItemsCallbacks = 1500,
            k_iClientUtilsCallbacks = 1600,
            k_iSteamGameCoordinatorCallbacks = 1700,
            k_iSteamGameServerStatsCallbacks = 1800,
            k_iSteam2AsyncCallbacks = 1900,
            k_iSteamGameStatsCallbacks = 2000,
            k_iClientHTTPCallbacks = 2100,
            k_iClientScreenshotsCallbacks = 2200,
            k_iSteamScreenshotsCallbacks = 2300,
            k_iClientAudioCallbacks = 2400,
            k_iClientUnifiedMessagesCallbacks = 2500,
            k_iSteamStreamLauncherCallbacks = 2600,
            k_iClientControllerCallbacks = 2700,
            k_iSteamControllerCallbacks = 2800,
            k_iClientParentalSettingsCallbacks = 2900,
            k_iClientDeviceAuthCallbacks = 3000,
            k_iClientNetworkDeviceManagerCallbacks = 3100,
            k_iClientMusicCallbacks = 3200,
            k_iClientRemoteClientManagerCallbacks = 3300,
            k_iClientUGCCallbacks = 3400,
            k_iSteamStreamClientCallbacks = 3500,
            k_IClientProductBuilderCallbacks = 3600,
            k_iClientShortcutsCallbacks = 3700,
            k_iClientRemoteControlManagerCallbacks = 3800,
            k_iSteamAppListCallbacks = 3900,
            k_iSteamMusicCallbacks = 4000,
            k_iSteamMusicRemoteCallbacks = 4100,
            k_iClientVRCallbacks = 4200,
            k_iClientReservedCallbacks = 4300,
            k_iSteamReservedCallbacks = 4400,
            k_iSteamHTMLSurfaceCallbacks = 4500,
            k_iClientVideoCallbacks = 4600,
            k_iClientInventoryCallbacks = 4700,
        };

        struct CallbackMsg_t { int32_t m_hSteamUser; int m_iCallback; uint8_t *m_pubParam; int m_cubParam; };
        struct SteamServerConnectFailure_t { EResult m_eResult; bool m_bStillRetrying; };
        struct SteamServersDisconnected_t { EResult m_eResult; };
        struct ClientGameServerDeny_t { uint32_t m_uAppID; uint32_t m_unGameServerIP; uint16_t m_usGameServerPort; uint16_t m_bSecure; uint32_t m_uReason; };
        struct ValidateAuthTicketResponse_t { CSteamID m_SteamID; uint32_t m_eAuthSessionResponse; CSteamID m_OwnerSteamID; };
        struct MicroTxnAuthorizationResponse_t { uint32_t m_unAppID; uint64_t m_ulOrderID; uint8_t m_bAuthorized; };
        struct EncryptedAppTicketResponse_t { EResult m_eResult; };
        struct GetAuthSessionTicketResponse_t { uint32_t m_hAuthTicket; EResult m_eResult; };
        struct GameWebCallback_t { char m_szURL[256]; };
        struct StoreAuthURLResponse_t { char m_szURL[512]; };
        struct FriendGameInfo_t { CGameID m_gameID; uint32_t m_unGameIP; uint16_t m_usGamePort; uint16_t m_usQueryPort; CSteamID m_steamIDLobby; };
        struct FriendSessionStateInfo_t { uint32_t m_uiOnlineSessionInstances; uint8_t m_uiPublishedToFriendsSessionInstance; };
        struct PersonaStateChange_t { uint64_t m_ulSteamID; int m_nChangeFlags; };
        struct GameOverlayActivated_t { uint8_t m_bActive; };
        struct GameServerChangeRequested_t { char m_rgchServer[64]; char m_rgchPassword[64]; };
        struct GameLobbyJoinRequested_t { CSteamID m_steamIDLobby; CSteamID m_steamIDFriend; };
        struct AvatarImageLoaded_t { CSteamID m_steamID; int m_iImage; int m_iWide; int m_iTall; };
        struct ClanOfficerListResponse_t { CSteamID m_steamIDClan; int m_cOfficers; uint8_t m_bSuccess; };
        struct FriendRichPresenceUpdate_t { CSteamID m_steamIDFriend; uint32_t m_nAppID; };
        struct GameRichPresenceJoinRequested_t { CSteamID m_steamIDFriend; char m_rgchConnect[256]; };
        struct GameConnectedClanChatMsg_t { CSteamID m_steamIDClanChat; CSteamID m_steamIDUser; int m_iMessageID; };
        struct GameConnectedChatJoin_t { CSteamID m_steamIDClanChat; CSteamID m_steamIDUser; };
        struct GameConnectedChatLeave_t { CSteamID m_steamIDClanChat; CSteamID m_steamIDUser; bool m_bKicked; bool m_bDropped; };
        struct DownloadClanActivityCountsResult_t { bool m_bSuccess; };
        struct JoinClanChatRoomCompletionResult_t { CSteamID m_steamIDClanChat; enum EChatRoomEnterResponse m_eChatRoomEnterResponse; };
        struct GameConnectedFriendChatMsg_t { CSteamID m_steamIDUser; int m_iMessageID; };
        struct FriendsGetFollowerCount_t { EResult m_eResult; CSteamID m_steamID; int m_nCount; };
        struct FriendsIsFollowing_t { EResult m_eResult; CSteamID m_steamID; bool m_bIsFollowing; };
        struct FriendsEnumerateFollowingList_t { EResult m_eResult; CSteamID m_rgSteamID[50]; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; };
        struct SetPersonaNameResponse_t { bool m_bSuccess; bool m_bLocalSuccess; EResult m_result; };
        struct LowBatteryPower_t { uint8_t m_nMinutesBatteryLeft; };
        struct SteamAPICallCompleted_t { uint64_t m_hAsyncCall; int m_iCallback; uint32_t m_cubParam; };
        struct CheckFileSignature_t { enum ECheckFileSignature m_eCheckFileSignature; };
        struct GamepadTextInputDismissed_t { bool m_bSubmitted; uint32_t m_unSubmittedText; };
        struct MatchMakingKeyValuePair_t { char m_szKey[256]; char m_szValue[256]; };
        struct servernetadr_t { uint16_t m_usConnectionPort; uint16_t m_usQueryPort; uint32_t m_unIP; };
        struct gameserveritem_t { servernetadr_t m_NetAdr; int m_nPing; bool m_bHadSuccessfulResponse; bool m_bDoNotRefresh; char m_szGameDir[32]; char m_szMap[32]; char m_szGameDescription[64]; uint32_t m_nAppID; int m_nPlayers; int m_nMaxPlayers; int m_nBotPlayers; bool m_bPassword; bool m_bSecure; uint32_t m_ulTimeLastPlayed; int m_nServerVersion; char m_szServerName[64]; char m_szGameTags[128]; CSteamID m_steamID; };
        struct FavoritesListChanged_t { uint32_t m_nIP; uint32_t m_nQueryPort; uint32_t m_nConnPort; uint32_t m_nAppID; uint32_t m_nFlags; bool m_bAdd; uint32_t m_unAccountId; };
        struct LobbyInvite_t { uint64_t m_ulSteamIDUser; uint64_t m_ulSteamIDLobby; uint64_t m_ulGameID; };
        struct LobbyEnter_t { uint64_t m_ulSteamIDLobby; uint32_t m_rgfChatPermissions; bool m_bLocked; uint32_t m_EChatRoomEnterResponse; };
        struct LobbyDataUpdate_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDMember; uint8_t m_bSuccess; };
        struct LobbyChatUpdate_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDUserChanged; uint64_t m_ulSteamIDMakingChange; uint32_t m_rgfChatMemberStateChange; };
        struct LobbyChatMsg_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDUser; uint8_t m_eChatEntryType; uint32_t m_iChatID; };
        struct LobbyGameCreated_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDGameServer; uint32_t m_unIP; uint16_t m_usPort; };
        struct LobbyMatchList_t { uint32_t m_nLobbiesMatching; };
        struct LobbyKicked_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDAdmin; uint8_t m_bKickedDueToDisconnect; };
        struct LobbyCreated_t { EResult m_eResult; uint64_t m_ulSteamIDLobby; };
        struct PSNGameBootInviteResult_t { bool m_bGameBootInviteExists; CSteamID m_steamIDLobby; };
        struct FavoritesListAccountsUpdated_t { EResult m_eResult; };
        struct SteamParamStringArray_t { const char **m_ppStrings; int32_t m_nNumStrings; };
        struct RemoteStorageAppSyncedClient_t { uint32_t m_nAppID; EResult m_eResult; int m_unNumDownloads; };
        struct RemoteStorageAppSyncedServer_t { uint32_t m_nAppID; EResult m_eResult; int m_unNumUploads; };
        struct RemoteStorageAppSyncProgress_t { char m_rgchCurrentFile[260]; uint32_t m_nAppID; uint32_t m_uBytesTransferredThisChunk; double m_dAppPercentComplete; bool m_bUploading; };
        struct RemoteStorageAppSyncStatusCheck_t { uint32_t m_nAppID; EResult m_eResult; };
        struct RemoteStorageFileShareResult_t { EResult m_eResult; uint64_t m_hFile; char m_rgchFilename[260]; };
        struct RemoteStoragePublishFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
        struct RemoteStorageDeletePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
        struct RemoteStorageEnumerateUserPublishedFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; };
        struct RemoteStorageSubscribePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
        struct RemoteStorageEnumerateUserSubscribedFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; uint32_t m_rgRTimeSubscribed[50]; };
        struct RemoteStorageUnsubscribePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
        struct RemoteStorageUpdatePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
        struct RemoteStorageDownloadUGCResult_t { EResult m_eResult; uint64_t m_hFile; uint32_t m_nAppID; int32_t m_nSizeInBytes; char m_pchFileName[260]; uint64_t m_ulSteamIDOwner; };
        struct RemoteStorageGetPublishedFileDetailsResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; uint32_t m_nCreatorAppID; uint32_t m_nConsumerAppID; char m_rgchTitle[129]; char m_rgchDescription[8000]; uint64_t m_hFile; uint64_t m_hPreviewFile; uint64_t m_ulSteamIDOwner; uint32_t m_rtimeCreated; uint32_t m_rtimeUpdated; enum ERemoteStoragePublishedFileVisibility m_eVisibility; bool m_bBanned; char m_rgchTags[1025]; bool m_bTagsTruncated; char m_pchFileName[260]; int32_t m_nFileSize; int32_t m_nPreviewFileSize; char m_rgchURL[256]; enum EWorkshopFileType m_eFileType; bool m_bAcceptedForUse; };
        struct RemoteStorageEnumerateWorkshopFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; float m_rgScore[50]; uint32_t m_nAppId; uint32_t m_unStartIndex; };
        struct RemoteStorageGetPublishedItemVoteDetailsResult_t { EResult m_eResult; uint64_t m_unPublishedFileId; int32_t m_nVotesFor; int32_t m_nVotesAgainst; int32_t m_nReports; float m_fScore; };
        struct RemoteStoragePublishedFileSubscribed_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
        struct RemoteStoragePublishedFileUnsubscribed_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
        struct RemoteStoragePublishedFileDeleted_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
        struct RemoteStorageUpdateUserPublishedItemVoteResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
        struct RemoteStorageUserVoteDetails_t { EResult m_eResult; uint64_t m_nPublishedFileId; enum EWorkshopVote m_eVote; };
        struct RemoteStorageEnumerateUserSharedWorkshopFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; };
        struct RemoteStorageSetUserPublishedFileActionResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; enum EWorkshopFileAction m_eAction; };
        struct RemoteStorageEnumeratePublishedFilesByUserActionResult_t { EResult m_eResult; enum EWorkshopFileAction m_eAction; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; uint32_t m_rgRTimeUpdated[50]; };
        struct RemoteStoragePublishFileProgress_t { double m_dPercentFile; bool m_bPreview; };
        struct RemoteStoragePublishedFileUpdated_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; uint64_t m_ulUnused; };
        struct RemoteStorageFileWriteAsyncComplete_t { EResult m_eResult; };
        struct RemoteStorageFileReadAsyncComplete_t { uint64_t m_hFileReadAsync; EResult m_eResult; uint32_t m_nOffset; uint32_t m_cubRead; };
        struct LeaderboardEntry_t { CSteamID m_steamIDUser; int32_t m_nGlobalRank; int32_t m_nScore; int32_t m_cDetails; uint64_t m_hUGC; };
        struct UserStatsReceived_t { uint64_t m_nGameID; EResult m_eResult; CSteamID m_steamIDUser; };
        struct UserStatsStored_t { uint64_t m_nGameID; EResult m_eResult; };
        struct UserAchievementStored_t { uint64_t m_nGameID; bool m_bGroupAchievement; char m_rgchAchievementName[128]; uint32_t m_nCurProgress; uint32_t m_nMaxProgress; };
        struct LeaderboardFindResult_t { uint64_t m_hSteamLeaderboard; uint8_t m_bLeaderboardFound; };
        struct LeaderboardScoresDownloaded_t { uint64_t m_hSteamLeaderboard; uint64_t m_hSteamLeaderboardEntries; int m_cEntryCount; };
        struct LeaderboardScoreUploaded_t { uint8_t m_bSuccess; uint64_t m_hSteamLeaderboard; int32_t m_nScore; uint8_t m_bScoreChanged; int m_nGlobalRankNew; int m_nGlobalRankPrevious; };
        struct NumberOfCurrentPlayers_t { uint8_t m_bSuccess; int32_t m_cPlayers; };
        struct UserStatsUnloaded_t { CSteamID m_steamIDUser; };
        struct UserAchievementIconFetched_t { CGameID m_nGameID; char m_rgchAchievementName[128]; bool m_bAchieved; int m_nIconHandle; };
        struct GlobalAchievementPercentagesReady_t { uint64_t m_nGameID; EResult m_eResult; };
        struct LeaderboardUGCSet_t { EResult m_eResult; uint64_t m_hSteamLeaderboard; };
        struct PS3TrophiesInstalled_t { uint64_t m_nGameID; EResult m_eResult; uint64_t m_ulRequiredDiskSpace; };
        struct GlobalStatsReceived_t { uint64_t m_nGameID; EResult m_eResult; };
        struct DlcInstalled_t { uint32_t m_nAppID; };
        struct RegisterActivationCodeResponse_t { enum ERegisterActivationCodeResult m_eResult; uint32_t m_unPackageRegistered; };
        struct AppProofOfPurchaseKeyResponse_t { EResult m_eResult; uint32_t m_nAppID; uint32_t m_cchKeyLength; char m_rgchKey[240]; };
        struct FileDetailsResult_t { EResult m_eResult; uint64_t m_ulFileSize; uint8_t m_FileSHA[20]; uint32_t m_unFlags; };
        struct P2PSessionState_t { uint8_t m_bConnectionActive; uint8_t m_bConnecting; uint8_t m_eP2PSessionError; uint8_t m_bUsingRelay; int32_t m_nBytesQueuedForSend; int32_t m_nPacketsQueuedForSend; uint32_t m_nRemoteIP; uint16_t m_nRemotePort; };
        struct P2PSessionRequest_t { CSteamID m_steamIDRemote; };
        struct P2PSessionConnectFail_t { CSteamID m_steamIDRemote; uint8_t m_eP2PSessionError; };
        struct SocketStatusCallback_t { uint32_t m_hSocket; uint32_t m_hListenSocket; CSteamID m_steamIDRemote; int m_eSNetSocketState; };
        struct ScreenshotReady_t { uint32_t m_hLocal; EResult m_eResult; };
        struct VolumeHasChanged_t { float m_flNewVolume; };
        struct MusicPlayerWantsShuffled_t { bool m_bShuffled; };
        struct MusicPlayerWantsLooped_t { bool m_bLooped; };
        struct MusicPlayerWantsVolume_t { float m_flNewVolume; };
        struct MusicPlayerSelectsQueueEntry_t { int nID; };
        struct MusicPlayerSelectsPlaylistEntry_t { int nID; };
        struct MusicPlayerWantsPlayingRepeatStatus_t { int m_nPlayingRepeatStatus; };
        struct HTTPRequestCompleted_t { uint32_t m_hRequest; uint64_t m_ulContextValue; bool m_bRequestSuccessful; enum EHTTPStatusCode m_eStatusCode; uint32_t m_unBodySize; };
        struct HTTPRequestHeadersReceived_t { uint32_t m_hRequest; uint64_t m_ulContextValue; };
        struct HTTPRequestDataReceived_t { uint32_t m_hRequest; uint64_t m_ulContextValue; uint32_t m_cOffset; uint32_t m_cBytesReceived; };
        struct SteamUnifiedMessagesSendMethodResult_t { uint64_t m_hHandle; uint64_t m_unContext; EResult m_eResult; uint32_t m_unResponseSize; };
        struct ControllerAnalogActionData_t { enum EControllerSourceMode eMode; float x; float y; bool bActive; };
        struct ControllerDigitalActionData_t { bool bState; bool bActive; };
        struct ControllerMotionData_t { float rotQuatX; float rotQuatY; float rotQuatZ; float rotQuatW; float posAccelX; float posAccelY; float posAccelZ; float rotVelX; float rotVelY; float rotVelZ; };
        struct SteamUGCDetails_t { uint64_t m_nPublishedFileId; EResult m_eResult; enum EWorkshopFileType m_eFileType; uint32_t m_nCreatorAppID; uint32_t m_nConsumerAppID; char m_rgchTitle[129]; char m_rgchDescription[8000]; uint64_t m_ulSteamIDOwner; uint32_t m_rtimeCreated; uint32_t m_rtimeUpdated; uint32_t m_rtimeAddedToUserList; enum ERemoteStoragePublishedFileVisibility m_eVisibility; bool m_bBanned; bool m_bAcceptedForUse; bool m_bTagsTruncated; char m_rgchTags[1025]; uint64_t m_hFile; uint64_t m_hPreviewFile; char m_pchFileName[260]; int32_t m_nFileSize; int32_t m_nPreviewFileSize; char m_rgchURL[256]; uint32_t m_unVotesUp; uint32_t m_unVotesDown; float m_flScore; uint32_t m_unNumChildren; };
        struct SteamUGCQueryCompleted_t { uint64_t m_handle; EResult m_eResult; uint32_t m_unNumResultsReturned; uint32_t m_unTotalMatchingResults; bool m_bCachedData; };
        struct SteamUGCRequestUGCDetailsResult_t { struct SteamUGCDetails_t m_details; bool m_bCachedData; };
        struct CreateItemResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
        struct SubmitItemUpdateResult_t { EResult m_eResult; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
        struct DownloadItemResult_t { uint32_t m_unAppID; uint64_t m_nPublishedFileId; EResult m_eResult; };
        struct UserFavoriteItemsListChanged_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bWasAddRequest; };
        struct SetUserItemVoteResult_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bVoteUp; };
        struct GetUserItemVoteResult_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bVotedUp; bool m_bVotedDown; bool m_bVoteSkipped; };
        struct StartPlaytimeTrackingResult_t { EResult m_eResult; };
        struct StopPlaytimeTrackingResult_t { EResult m_eResult; };
        struct SteamAppInstalled_t { uint32_t m_nAppID; };
        struct SteamAppUninstalled_t { uint32_t m_nAppID; };
        struct HTML_BrowserReady_t { uint32_t unBrowserHandle; };
        struct HTML_NeedsPaint_t { uint32_t unBrowserHandle; const char *pBGRA; uint32_t unWide; uint32_t unTall; uint32_t unUpdateX; uint32_t unUpdateY; uint32_t unUpdateWide; uint32_t unUpdateTall; uint32_t unScrollX; uint32_t unScrollY; float flPageScale; uint32_t unPageSerial; };
        struct HTML_StartRequest_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchTarget; const char *pchPostData; bool bIsRedirect; };
        struct HTML_CloseBrowser_t { uint32_t unBrowserHandle; };
        struct HTML_URLChanged_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchPostData; bool bIsRedirect; const char *pchPageTitle; bool bNewNavigation; };
        struct HTML_FinishedRequest_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchPageTitle; };
        struct HTML_OpenLinkInNewTab_t { uint32_t unBrowserHandle; const char *pchURL; };
        struct HTML_ChangedTitle_t { uint32_t unBrowserHandle; const char *pchTitle; };
        struct HTML_SearchResults_t { uint32_t unBrowserHandle; uint32_t unResults; uint32_t unCurrentMatch; };
        struct HTML_CanGoBackAndForward_t { uint32_t unBrowserHandle; bool bCanGoBack; bool bCanGoForward; };
        struct HTML_HorizontalScroll_t { uint32_t unBrowserHandle; uint32_t unScrollMax; uint32_t unScrollCurrent; float flPageScale; bool bVisible; uint32_t unPageSize; };
        struct HTML_VerticalScroll_t { uint32_t unBrowserHandle; uint32_t unScrollMax; uint32_t unScrollCurrent; float flPageScale; bool bVisible; uint32_t unPageSize; };
        struct HTML_LinkAtPosition_t { uint32_t unBrowserHandle; uint32_t x; uint32_t y; const char *pchURL; bool bInput; bool bLiveLink; };
        struct HTML_JSAlert_t { uint32_t unBrowserHandle; const char *pchMessage; };
        struct HTML_JSConfirm_t { uint32_t unBrowserHandle; const char *pchMessage; };
        struct HTML_FileOpenDialog_t { uint32_t unBrowserHandle; const char *pchTitle; const char *pchInitialFile; };
        struct HTML_NewWindow_t { uint32_t unBrowserHandle; const char *pchURL; uint32_t unX; uint32_t unY; uint32_t unWide; uint32_t unTall; uint32_t unNewWindow_BrowserHandle; };
        struct HTML_SetCursor_t { uint32_t unBrowserHandle; uint32_t eMouseCursor; };
        struct HTML_StatusText_t { uint32_t unBrowserHandle; const char *pchMsg; };
        struct HTML_ShowToolTip_t { uint32_t unBrowserHandle; const char *pchMsg; };
        struct HTML_UpdateToolTip_t { uint32_t unBrowserHandle; const char *pchMsg; };
        struct HTML_HideToolTip_t { uint32_t unBrowserHandle; };
        struct SteamItemDetails_t { uint64_t m_itemId; int32_t m_iDefinition; uint16_t m_unQuantity; uint16_t m_unFlags; };
        struct SteamInventoryResultReady_t { int32_t m_handle; EResult m_result; };
        struct SteamInventoryFullUpdate_t { int32_t m_handle; };
        struct SteamInventoryEligiblePromoItemDefIDs_t { EResult m_result; CSteamID m_steamID; int m_numEligiblePromoItemDefs; bool m_bCachedData; };
        struct BroadcastUploadStop_t { enum EBroadcastUploadResult m_eResult; };
        struct GetVideoURLResult_t { EResult m_eResult; uint32_t m_unVideoAppID; char m_rgchURL[256]; };
        struct GSClientApprove_t { CSteamID m_SteamID; CSteamID m_OwnerSteamID; };
        struct GSClientDeny_t { CSteamID m_SteamID; enum EDenyReason m_eDenyReason; char m_rgchOptionalText[128]; };
        struct GSClientKick_t { CSteamID m_SteamID; enum EDenyReason m_eDenyReason; };
        struct GSClientAchievementStatus_t { uint64_t m_SteamID; char m_pchAchievement[128]; bool m_bUnlocked; };
        struct GSPolicyResponse_t { uint8_t m_bSecure; };
        struct GSGameplayStats_t { EResult m_eResult; int32_t m_nRank; uint32_t m_unTotalConnects; uint32_t m_unTotalMinutesPlayed; };
        struct GSClientGroupStatus_t { CSteamID m_SteamIDUser; CSteamID m_SteamIDGroup; bool m_bMember; bool m_bOfficer; };
        struct GSReputation_t { EResult m_eResult; uint32_t m_unReputationScore; bool m_bBanned; uint32_t m_unBannedIP; uint16_t m_usBannedPort; uint64_t m_ulBannedGameID; uint32_t m_unBanExpires; };
        struct AssociateWithClanResult_t { EResult m_eResult; };
        struct ComputeNewPlayerCompatibilityResult_t { EResult m_eResult; int m_cPlayersThatDontLikeCandidate; int m_cPlayersThatCandidateDoesntLike; int m_cClanPlayersThatDontLikeCandidate; CSteamID m_SteamIDCandidate; };
        struct GSStatsReceived_t { EResult m_eResult; CSteamID m_steamIDUser; };
        struct GSStatsStored_t { EResult m_eResult; CSteamID m_steamIDUser; };
        struct GSStatsUnloaded_t { CSteamID m_steamIDUser; };
        #pragma endregion
    }
}

namespace Callbacks = Steam::Callbacks;

// Interface exports.
extern "C"
{
    // Initialization and shutdown.
    EXPORT_ATTR bool SteamAPI_Init();
    EXPORT_ATTR bool SteamAPI_InitSafe();
    EXPORT_ATTR void SteamAPI_Shutdown();
    EXPORT_ATTR bool SteamAPI_IsSteamRunning();
    EXPORT_ATTR const char *SteamAPI_GetSteamInstallPath();
    EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID);

    // Callback management.
    EXPORT_ATTR void SteamAPI_RunCallbacks();
    EXPORT_ATTR void SteamAPI_RegisterCallback(void *pCallback, int iCallback);
    EXPORT_ATTR void SteamAPI_UnregisterCallback(void *pCallback, int iCallback);
    EXPORT_ATTR void SteamAPI_RegisterCallResult(void *pCallback, uint64_t hAPICall);
    EXPORT_ATTR void SteamAPI_UnregisterCallResult(void *pCallback, uint64_t hAPICall);

    // Steam proxy.
    EXPORT_ATTR int32_t SteamAPI_GetHSteamUser();
    EXPORT_ATTR int32_t SteamAPI_GetHSteamPipe();
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamUser();
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamPipe();
    EXPORT_ATTR bool SteamGameServer_BSecure();
    EXPORT_ATTR void SteamGameServer_Shutdown();
    EXPORT_ATTR void SteamGameServer_RunCallbacks();
    EXPORT_ATTR uint64_t SteamGameServer_GetSteamID();
    EXPORT_ATTR bool SteamGameServer_Init(uint32_t unIP, uint16_t usPort, uint16_t usGamePort, ...);
    // TODO(tcn): Replace with vararg versions.
    EXPORT_ATTR bool SteamGameServer_InitSafe(uint32_t unIP, uint16_t usPort, uint16_t usGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchGameDir, const char *pchVersionString);
    EXPORT_ATTR bool SteamInternal_GameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersionString);

    // Interface access.
    EXPORT_ATTR void *SteamAppList();
    EXPORT_ATTR void *SteamApps();
    EXPORT_ATTR void *SteamClient();
    EXPORT_ATTR void *SteamController();
    EXPORT_ATTR void *SteamFriends();
    EXPORT_ATTR void *SteamGameServer();
    EXPORT_ATTR void *SteamGameServerHTTP();
    EXPORT_ATTR void *SteamGameServerInventory();
    EXPORT_ATTR void *SteamGameServerNetworking();
    EXPORT_ATTR void *SteamGameServerStats();
    EXPORT_ATTR void *SteamGameServerUGC();
    EXPORT_ATTR void *SteamGameServerUtils();
    EXPORT_ATTR void *SteamHTMLSurface();
    EXPORT_ATTR void *SteamHTTP();
    EXPORT_ATTR void *SteamInventory();
    EXPORT_ATTR void *SteamMatchmaking();
    EXPORT_ATTR void *SteamMatchmakingServers();
    EXPORT_ATTR void *SteamMusic();
    EXPORT_ATTR void *SteamMusicRemote();
    EXPORT_ATTR void *SteamNetworking();
    EXPORT_ATTR void *SteamRemoteStorage();
    EXPORT_ATTR void *SteamScreenshots();
    EXPORT_ATTR void *SteamUnifiedMessages();
    EXPORT_ATTR void *SteamUGC();
    EXPORT_ATTR void *SteamUser();
    EXPORT_ATTR void *SteamUserStats();
    EXPORT_ATTR void *SteamUtils();
    EXPORT_ATTR void *SteamVideo();
    EXPORT_ATTR void *SteamMasterServerUpdater();
    EXPORT_ATTR void *SteamInternal_CreateInterface(const char *Interfacename);
}
