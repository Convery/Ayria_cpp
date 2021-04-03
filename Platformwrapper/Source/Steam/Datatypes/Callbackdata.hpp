/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Datatypes.hpp"

// Asynchronous tasks.
namespace Steam::Callbacks
{
    #pragma region Callbackstructs
    struct AppProofOfPurchaseKeyResponse_t { EResult m_eResult; uint32_t m_nAppID; uint32_t m_cchKeyLength; char m_rgchKey[240]; };
    struct AssociateWithClanResult_t { EResult m_eResult; };
    struct AvatarImageLoaded_t { SteamID_t m_steamID; int m_iImage; int m_iWide; int m_iTall; };
    struct BroadcastUploadStop_t { enum EBroadcastUploadResult : uint32_t m_eResult; };
    struct CallbackMsg_t { int32_t m_hSteamUser; int m_iCallback; uint8_t *m_pubParam; int m_cubParam; };
    struct CheckFileSignature_t { uint32_t m_eCheckFileSignature; };
    struct ClanOfficerListResponse_t { SteamID_t m_steamIDClan; int m_cOfficers; uint8_t m_bSuccess; };
    struct ClientAppNewsItemUpdate_t { uint8_t m_eNewsUpdateType; uint32_t m_uNewsID; uint32_t m_uAppID; };
    struct ClientGameServerDeny_t { uint32_t m_uAppID; uint32_t m_unGameServerIP; uint16_t m_usGameServerPort; uint16_t m_bSecure; uint32_t m_uReason; };
    struct ClientSteamNewsItemUpdate_t { uint8_t m_eNewsUpdateType; uint32_t m_uNewsID, m_uHaveSubID, m_uNotHaveSubID, m_uHaveAppID, m_uNotHaveAppID, m_uHaveAppIDInstalled, m_uHavePlayedAppID; };
    struct ComputeNewPlayerCompatibilityResult_t { EResult m_eResult; int m_cPlayersThatDontLikeCandidate; int m_cPlayersThatCandidateDoesntLike; int m_cClanPlayersThatDontLikeCandidate; SteamID_t m_SteamIDCandidate; };
    struct ControllerAnalogActionData_t { enum EControllerSourceMode : uint32_t eMode; float x; float y; bool bActive; };
    struct ControllerDigitalActionData_t { bool bState; bool bActive; };
    struct ControllerMotionData_t { float rotQuatX; float rotQuatY; float rotQuatZ; float rotQuatW; float posAccelX; float posAccelY; float posAccelZ; float rotVelX; float rotVelY; float rotVelZ; };
    struct CreateItemResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
    struct DlcInstalled_t { uint32_t m_nAppID; };
    struct DownloadClanActivityCountsResult_t { bool m_bSuccess; };
    struct DownloadItemResult_t { uint32_t m_unAppID; uint64_t m_nPublishedFileId; EResult m_eResult; };
    struct DurationControl_t { EResult m_eResult; AppID_t m_appid; bool m_bApplicable; int32_t m_csecsLast5h; uint32_t m_progress; uint32_t m_notification; int32_t m_csecsToday, m_csecsRemaining; };
    struct EncryptedAppTicketResponse_t { EResult m_eResult; };
    struct FavoritesListAccountsUpdated_t { EResult m_eResult; };
    struct FavoritesListChanged_t { uint32_t m_nIP; uint32_t m_nQueryPort; uint32_t m_nConnPort; uint32_t m_nAppID; uint32_t m_nFlags; bool m_bAdd; uint32_t m_unAccountId; };
    struct FileDetailsResult_t { EResult m_eResult; uint64_t m_ulFileSize; uint8_t m_FileSHA[20]; uint32_t m_unFlags; };
    struct FriendAdded_t { EResult m_eResult; uint64_t m_ulSteamID; };
    struct FriendGameInfo_t { GameID_t m_gameID; uint32_t m_unGameIP; uint16_t m_usGamePort; uint16_t m_usQueryPort; SteamID_t m_steamIDLobby; };
    struct FriendRichPresenceUpdate_t { SteamID_t m_steamIDFriend; uint32_t m_nAppID; };
    struct FriendSessionStateInfo_t { uint32_t m_uiOnlineSessionInstances; uint8_t m_uiPublishedToFriendsSessionInstance; };
    struct FriendsEnumerateFollowingList_t { EResult m_eResult; SteamID_t m_rgSteamID[50]; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; };
    struct FriendsGetFollowerCount_t { EResult m_eResult; SteamID_t m_steamID; int m_nCount; };
    struct FriendsIsFollowing_t { EResult m_eResult; SteamID_t m_steamID; bool m_bIsFollowing; };
    struct GSClientAchievementStatus_t { uint64_t m_SteamID; char m_pchAchievement[128]; bool m_bUnlocked; };
    struct GSClientApprove_t { SteamID_t m_SteamID; SteamID_t m_OwnerSteamID; };
    struct GSClientDeny_t { SteamID_t m_SteamID; enum EDenyReason : uint32_t m_eDenyReason; char m_rgchOptionalText[128]; };
    struct GSClientGroupStatus_t { SteamID_t m_SteamIDUser; SteamID_t m_SteamIDGroup; bool m_bMember; bool m_bOfficer; };
    struct GSClientKick_t { SteamID_t m_SteamID; enum EDenyReason : uint32_t m_eDenyReason; };
    struct GSGameplayStats_t { EResult m_eResult; int32_t m_nRank; uint32_t m_unTotalConnects; uint32_t m_unTotalMinutesPlayed; };
    struct GSPolicyResponse_t { uint8_t m_bSecure; };
    struct GSReputation_t { EResult m_eResult; uint32_t m_unReputationScore; bool m_bBanned; uint32_t m_unBannedIP; uint16_t m_usBannedPort; uint64_t m_ulBannedGameID; uint32_t m_unBanExpires; };
    struct GSStatsReceived_t { EResult m_eResult; SteamID_t m_steamIDUser; };
    struct GSStatsStored_t { EResult m_eResult; SteamID_t m_steamIDUser; };
    struct GSStatsUnloaded_t { SteamID_t m_steamIDUser; };
    struct GameConnectedChatJoin_t { SteamID_t m_steamIDClanChat; SteamID_t m_steamIDUser; };
    struct GameConnectedChatLeave_t { SteamID_t m_steamIDClanChat; SteamID_t m_steamIDUser; bool m_bKicked; bool m_bDropped; };
    struct GameConnectedClanChatMsg_t { SteamID_t m_steamIDClanChat; SteamID_t m_steamIDUser; int m_iMessageID; };
    struct GameConnectedFriendChatMsg_t { SteamID_t m_steamIDUser; int m_iMessageID; };
    struct GameLobbyJoinRequested_t { SteamID_t m_steamIDLobby; SteamID_t m_steamIDFriend; };
    struct GameOverlayActivated_t { uint8_t m_bActive; };
    struct GameRichPresenceJoinRequested_t { SteamID_t m_steamIDFriend; char m_rgchConnect[256]; };
    struct GameServerChangeRequested_t { char m_rgchServer[64]; char m_rgchPassword[64]; };
    struct GameWebCallback_t { char m_szURL[256]; };
    struct GamepadTextInputDismissed_t { bool m_bSubmitted; uint32_t m_unSubmittedText; };
    struct GetAuthSessionTicketResponse_t { uint32_t m_hAuthTicket; EResult m_eResult; };
    struct GetUserItemVoteResult_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bVotedUp; bool m_bVotedDown; bool m_bVoteSkipped; };
    struct GetVideoURLResult_t { EResult m_eResult; uint32_t m_unVideoAppID; char m_rgchURL[256]; };
    struct GlobalAchievementPercentagesReady_t { uint64_t m_nGameID; EResult m_eResult; };
    struct GlobalStatsReceived_t { uint64_t m_nGameID; EResult m_eResult; };
    struct HTML_BrowserReady_t { uint32_t unBrowserHandle; };
    struct HTML_CanGoBackAndForward_t { uint32_t unBrowserHandle; bool bCanGoBack; bool bCanGoForward; };
    struct HTML_ChangedTitle_t { uint32_t unBrowserHandle; const char *pchTitle; };
    struct HTML_CloseBrowser_t { uint32_t unBrowserHandle; };
    struct HTML_FileOpenDialog_t { uint32_t unBrowserHandle; const char *pchTitle; const char *pchInitialFile; };
    struct HTML_FinishedRequest_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchPageTitle; };
    struct HTML_HideToolTip_t { uint32_t unBrowserHandle; };
    struct HTML_HorizontalScroll_t { uint32_t unBrowserHandle; uint32_t unScrollMax; uint32_t unScrollCurrent; float flPageScale; bool bVisible; uint32_t unPageSize; };
    struct HTML_JSAlert_t { uint32_t unBrowserHandle; const char *pchMessage; };
    struct HTML_JSConfirm_t { uint32_t unBrowserHandle; const char *pchMessage; };
    struct HTML_LinkAtPosition_t { uint32_t unBrowserHandle; uint32_t x; uint32_t y; const char *pchURL; bool bInput; bool bLiveLink; };
    struct HTML_NeedsPaint_t { uint32_t unBrowserHandle; const char *pBGRA; uint32_t unWide; uint32_t unTall; uint32_t unUpdateX; uint32_t unUpdateY; uint32_t unUpdateWide; uint32_t unUpdateTall; uint32_t unScrollX; uint32_t unScrollY; float flPageScale; uint32_t unPageSerial; };
    struct HTML_NewWindow_t { uint32_t unBrowserHandle; const char *pchURL; uint32_t unX; uint32_t unY; uint32_t unWide; uint32_t unTall; uint32_t unNewWindow_BrowserHandle; };
    struct HTML_OpenLinkInNewTab_t { uint32_t unBrowserHandle; const char *pchURL; };
    struct HTML_SearchResults_t { uint32_t unBrowserHandle; uint32_t unResults; uint32_t unCurrentMatch; };
    struct HTML_SetCursor_t { uint32_t unBrowserHandle; uint32_t eMouseCursor; };
    struct HTML_ShowToolTip_t { uint32_t unBrowserHandle; const char *pchMsg; };
    struct HTML_StartRequest_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchTarget; const char *pchPostData; bool bIsRedirect; };
    struct HTML_StatusText_t { uint32_t unBrowserHandle; const char *pchMsg; };
    struct HTML_URLChanged_t { uint32_t unBrowserHandle; const char *pchURL; const char *pchPostData; bool bIsRedirect; const char *pchPageTitle; bool bNewNavigation; };
    struct HTML_UpdateToolTip_t { uint32_t unBrowserHandle; const char *pchMsg; };
    struct HTML_VerticalScroll_t { uint32_t unBrowserHandle; uint32_t unScrollMax; uint32_t unScrollCurrent; float flPageScale; bool bVisible; uint32_t unPageSize; };
    struct HTTPRequestCompleted_t { uint32_t m_hRequest; uint64_t m_ulContextValue; bool m_bRequestSuccessful; enum EHTTPStatusCode : uint32_t m_eStatusCode; uint32_t m_unBodySize; };
    struct HTTPRequestDataReceived_t { uint32_t m_hRequest; uint64_t m_ulContextValue; uint32_t m_cOffset; uint32_t m_cBytesReceived; };
    struct HTTPRequestHeadersReceived_t { uint32_t m_hRequest; uint64_t m_ulContextValue; };
    struct JoinClanChatRoomCompletionResult_t { SteamID_t m_steamIDClanChat; uint32_t m_eChatRoomEnterResponse; };
    struct LeaderboardEntry_t { SteamID_t m_steamIDUser; int32_t m_nGlobalRank; int32_t m_nScore; int32_t m_cDetails; uint64_t m_hUGC; };
    struct LeaderboardFindResult_t { uint64_t m_hSteamLeaderboard; uint8_t m_bLeaderboardFound; };
    struct LeaderboardScoreUploaded_t { uint8_t m_bSuccess; uint64_t m_hSteamLeaderboard; int32_t m_nScore; uint8_t m_bScoreChanged; int m_nGlobalRankNew; int m_nGlobalRankPrevious; };
    struct LeaderboardScoresDownloaded_t { uint64_t m_hSteamLeaderboard; uint64_t m_hSteamLeaderboardEntries; int m_cEntryCount; };
    struct LeaderboardUGCSet_t { EResult m_eResult; uint64_t m_hSteamLeaderboard; };
    struct LobbyChatMsg_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDUser; uint8_t m_eChatEntryType; uint32_t m_iChatID; };
    struct LobbyChatUpdate_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDUserChanged; uint64_t m_ulSteamIDMakingChange; uint32_t m_rgfChatMemberStateChange; };
    struct LobbyCreated_t { EResult m_eResult; uint64_t m_ulSteamIDLobby; };
    struct LobbyDataUpdate_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDMember; uint8_t m_bSuccess; };
    struct LobbyEnter_t { uint64_t m_ulSteamIDLobby; uint32_t m_rgfChatPermissions; bool m_bLocked; uint32_t m_EChatRoomEnterResponse; };
    struct LobbyGameCreated_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDGameServer; uint32_t m_unIP; uint16_t m_usPort; };
    struct LobbyInvite_t { uint64_t m_ulSteamIDUser; uint64_t m_ulSteamIDLobby; uint64_t m_ulGameID; };
    struct LobbyKicked_t { uint64_t m_ulSteamIDLobby; uint64_t m_ulSteamIDAdmin; uint8_t m_bKickedDueToDisconnect; };
    struct LobbyMatchList_t { uint32_t m_nLobbiesMatching; };
    struct LoggedOff_t { EResult m_eResult; };
    struct LogonFailure_t { EResult m_eResult; };
    struct LowBatteryPower_t { uint8_t m_nMinutesBatteryLeft; };
    struct MarketEligibilityResponse_t { bool m_bAllowed; uint32_t m_eNotAllowedReason, m_rtAllowedAtTime; int32_t m_cdaySteamGuardRequiredDays, m_cdayNewDeviceCooldown; };
    struct MatchMakingKeyValuePair_t { char m_szKey[256]; char m_szValue[256]; };
    struct MicroTxnAuthorizationResponse_t { uint32_t m_unAppID; uint64_t m_ulOrderID; uint8_t m_bAuthorized; };
    struct MusicPlayerSelectsPlaylistEntry_t { int nID; };
    struct MusicPlayerSelectsQueueEntry_t { int nID; };
    struct MusicPlayerWantsLooped_t { bool m_bLooped; };
    struct MusicPlayerWantsPlayingRepeatStatus_t { int m_nPlayingRepeatStatus; };
    struct MusicPlayerWantsShuffled_t { bool m_bShuffled; };
    struct MusicPlayerWantsVolume_t { float m_flNewVolume; };
    struct NumberOfCurrentPlayers_t { uint8_t m_bSuccess; int32_t m_cPlayers; };
    struct P2PSessionConnectFail_t { SteamID_t m_steamIDRemote; uint8_t m_eP2PSessionError; };
    struct P2PSessionRequest_t { SteamID_t m_steamIDRemote; };
    struct P2PSessionState_t { uint8_t m_bConnectionActive; uint8_t m_bConnecting; uint8_t m_eP2PSessionError; uint8_t m_bUsingRelay; int32_t m_nBytesQueuedForSend; int32_t m_nPacketsQueuedForSend; uint32_t m_nRemoteIP; uint16_t m_nRemotePort; };
    struct PS3TrophiesInstalled_t { uint64_t m_nGameID; EResult m_eResult; uint64_t m_ulRequiredDiskSpace; };
    struct PSNGameBootInviteResult_t { bool m_bGameBootInviteExists; SteamID_t m_steamIDLobby; };
    struct PersonaStateChange_t { uint64_t m_ulSteamID; int m_nChangeFlags; };
    struct RegisterActivationCodeResponse_t { uint32_t m_eResult; uint32_t m_unPackageRegistered; };
    struct RemoteStorageAppSyncProgress_t { char m_rgchCurrentFile[260]; uint32_t m_nAppID; uint32_t m_uBytesTransferredThisChunk; double m_dAppPercentComplete; bool m_bUploading; };
    struct RemoteStorageAppSyncStatusCheck_t { uint32_t m_nAppID; EResult m_eResult; };
    struct RemoteStorageAppSyncedClient_t { uint32_t m_nAppID; EResult m_eResult; int m_unNumDownloads; };
    struct RemoteStorageAppSyncedServer_t { uint32_t m_nAppID; EResult m_eResult; int m_unNumUploads; };
    struct RemoteStorageDeletePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
    struct RemoteStorageDownloadUGCResult_t { EResult m_eResult; uint64_t m_hFile; uint32_t m_nAppID; int32_t m_nSizeInBytes; char m_pchFileName[260]; uint64_t m_ulSteamIDOwner; };
    struct RemoteStorageEnumeratePublishedFilesByUserActionResult_t { EResult m_eResult; enum EWorkshopFileAction : uint32_t m_eAction; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; uint32_t m_rgRTimeUpdated[50]; };
    struct RemoteStorageEnumerateUserPublishedFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; };
    struct RemoteStorageEnumerateUserSharedWorkshopFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; };
    struct RemoteStorageEnumerateUserSubscribedFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; uint32_t m_rgRTimeSubscribed[50]; };
    struct RemoteStorageEnumerateWorkshopFilesResult_t { EResult m_eResult; int32_t m_nResultsReturned; int32_t m_nTotalResultCount; uint64_t m_rgPublishedFileId[50]; float m_rgScore[50]; uint32_t m_nAppId; uint32_t m_unStartIndex; };
    struct RemoteStorageFileReadAsyncComplete_t { uint64_t m_hFileReadAsync; EResult m_eResult; uint32_t m_nOffset; uint32_t m_cubRead; };
    struct RemoteStorageFileShareResult_t { EResult m_eResult; uint64_t m_hFile; char m_rgchFilename[260]; };
    struct RemoteStorageFileWriteAsyncComplete_t { EResult m_eResult; };
    struct RemoteStorageGetPublishedFileDetailsResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; uint32_t m_nCreatorAppID; uint32_t m_nConsumerAppID; char m_rgchTitle[129]; char m_rgchDescription[8000]; uint64_t m_hFile; uint64_t m_hPreviewFile; uint64_t m_ulSteamIDOwner; uint32_t m_rtimeCreated; uint32_t m_rtimeUpdated; enum ERemoteStoragePublishedFileVisibility : uint32_t m_eVisibility; bool m_bBanned; char m_rgchTags[1025]; bool m_bTagsTruncated; char m_pchFileName[260]; int32_t m_nFileSize; int32_t m_nPreviewFileSize; char m_rgchURL[256]; enum EWorkshopFileType : uint32_t m_eFileType; bool m_bAcceptedForUse; };
    struct RemoteStorageGetPublishedItemVoteDetailsResult_t { EResult m_eResult; uint64_t m_unPublishedFileId; int32_t m_nVotesFor; int32_t m_nVotesAgainst; int32_t m_nReports; float m_fScore; };
    struct RemoteStoragePublishFileProgress_t { double m_dPercentFile; bool m_bPreview; };
    struct RemoteStoragePublishFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
    struct RemoteStoragePublishedFileDeleted_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
    struct RemoteStoragePublishedFileSubscribed_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
    struct RemoteStoragePublishedFileUnsubscribed_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; };
    struct RemoteStoragePublishedFileUpdated_t { uint64_t m_nPublishedFileId; uint32_t m_nAppID; uint64_t m_ulUnused; };
    struct RemoteStorageSetUserPublishedFileActionResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; enum EWorkshopFileAction : uint32_t m_eAction; };
    struct RemoteStorageSubscribePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
    struct RemoteStorageUnsubscribePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
    struct RemoteStorageUpdatePublishedFileResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
    struct RemoteStorageUpdateUserPublishedItemVoteResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; };
    struct RemoteStorageUserVoteDetails_t { EResult m_eResult; uint64_t m_nPublishedFileId; enum EWorkshopVote : uint32_t m_eVote; };
    struct ScreenshotReady_t { uint32_t m_hLocal; EResult m_eResult; };
    struct SetPersonaNameResponse_t { bool m_bSuccess; bool m_bLocalSuccess; EResult m_result; };
    struct SetUserItemVoteResult_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bVoteUp; };
    struct SocketStatusCallback_t { uint32_t m_hSocket; uint32_t m_hListenSocket; SteamID_t m_steamIDRemote; int m_eSNetSocketState; };
    struct StartPlaytimeTrackingResult_t { EResult m_eResult; };
    struct SteamAPICallCompleted_t { uint64_t m_hAsyncCall; int m_iCallback; uint32_t m_cubParam; };
    struct SteamAppInstalled_t { uint32_t m_nAppID; };
    struct SteamAppUninstalled_t { uint32_t m_nAppID; };
    struct SteamInventoryEligiblePromoItemDefIDs_t { EResult m_result; SteamID_t m_steamID; int m_numEligiblePromoItemDefs; bool m_bCachedData; };
    struct SteamInventoryFullUpdate_t { int32_t m_handle; };
    struct SteamInventoryResultReady_t { int32_t m_handle; EResult m_result; };
    struct SteamItemDetails_t { uint64_t m_itemId; int32_t m_iDefinition; uint16_t m_unQuantity; uint16_t m_unFlags; };
    struct SteamParamStringArray_t { const char **m_ppStrings; int32_t m_nNumStrings; };
    struct SteamServerConnectFailure_t { EResult m_eResult; bool m_bStillRetrying; };
    struct SteamServersDisconnected_t { EResult m_eResult; };
    struct SteamUGCDetails_t { uint64_t m_nPublishedFileId; EResult m_eResult; enum EWorkshopFileType : uint32_t m_eFileType; uint32_t m_nCreatorAppID; uint32_t m_nConsumerAppID; char m_rgchTitle[129]; char m_rgchDescription[8000]; uint64_t m_ulSteamIDOwner; uint32_t m_rtimeCreated; uint32_t m_rtimeUpdated; uint32_t m_rtimeAddedToUserList; enum ERemoteStoragePublishedFileVisibility : uint32_t m_eVisibility; bool m_bBanned; bool m_bAcceptedForUse; bool m_bTagsTruncated; char m_rgchTags[1025]; uint64_t m_hFile; uint64_t m_hPreviewFile; char m_pchFileName[260]; int32_t m_nFileSize; int32_t m_nPreviewFileSize; char m_rgchURL[256]; uint32_t m_unVotesUp; uint32_t m_unVotesDown; float m_flScore; uint32_t m_unNumChildren; };
    struct SteamUGCQueryCompleted_t { uint64_t m_handle; EResult m_eResult; uint32_t m_unNumResultsReturned; uint32_t m_unTotalMatchingResults; bool m_bCachedData; };
    struct SteamUGCRequestUGCDetailsResult_t { struct SteamUGCDetails_t m_details; bool m_bCachedData; };
    struct SteamUnifiedMessagesSendMethodResult_t { uint64_t m_hHandle; uint64_t m_unContext; EResult m_eResult; uint32_t m_unResponseSize; };
    struct StopPlaytimeTrackingResult_t { EResult m_eResult; };
    struct StoreAuthURLResponse_t { char m_szURL[512]; };
    struct SubmitItemUpdateResult_t { EResult m_eResult; bool m_bUserNeedsToAcceptWorkshopLegalAgreement; };
    struct UserAchievementIconFetched_t { GameID_t m_nGameID; char m_rgchAchievementName[128]; bool m_bAchieved; int m_nIconHandle; };
    struct UserAchievementStored_t { uint64_t m_nGameID; bool m_bGroupAchievement; char m_rgchAchievementName[128]; uint32_t m_nCurProgress; uint32_t m_nMaxProgress; };
    struct UserFavoriteItemsListChanged_t { uint64_t m_nPublishedFileId; EResult m_eResult; bool m_bWasAddRequest; };
    struct UserStatsReceived_t { uint64_t m_nGameID; EResult m_eResult; SteamID_t m_steamIDUser; };
    struct UserStatsStored_t { uint64_t m_nGameID; EResult m_eResult; };
    struct UserStatsUnloaded_t { SteamID_t m_steamIDUser; };
    struct ValidateAuthTicketResponse_t { SteamID_t m_SteamID; uint32_t m_eAuthSessionResponse; SteamID_t m_OwnerSteamID; };
    struct VolumeHasChanged_t { float m_flNewVolume; };
    struct servernetadr_t { uint16_t m_usConnectionPort; uint16_t m_usQueryPort; uint32_t m_unIP; };
    struct gameserveritem_t { servernetadr_t m_NetAdr; int m_nPing; bool m_bHadSuccessfulResponse; bool m_bDoNotRefresh; char m_szGameDir[32]; char m_szMap[32]; char m_szGameDescription[64]; uint32_t m_nAppID; int m_nPlayers; int m_nMaxPlayers; int m_nBotPlayers; bool m_bPassword; bool m_bSecure; uint32_t m_ulTimeLastPlayed; int m_nServerVersion; char m_szServerName[64]; char m_szGameTags[128]; SteamID_t m_steamID; };
    #pragma endregion

    #pragma region Callbacktypes
    namespace Types
    {
        enum ECallbackType : int32_t
        {
            k_iSteamUserCallbacks = 100,
            SteamServersConnected_t = k_iSteamUserCallbacks + 1,
            SteamServerConnectFailure_t = k_iSteamUserCallbacks + 2,
            SteamServersDisconnected_t = k_iSteamUserCallbacks + 3,
            BeginLogonRetry_t = k_iSteamUserCallbacks + 4,
            ClientGameServerDeny_t = k_iSteamUserCallbacks + 13,
            PrimaryChatDestinationSetOld_t = k_iSteamUserCallbacks + 14,
            GSPolicyResponse_t = k_iSteamUserCallbacks + 15,
            IPCFailure_t = k_iSteamUserCallbacks + 17,
            LicensesUpdated_t = k_iSteamUserCallbacks + 25,
            AppLifetimeNotice_t = k_iSteamUserCallbacks + 30,
            DRMSDKFileTransferResult_t = k_iSteamUserCallbacks + 41,
            ValidateAuthTicketResponse_t = k_iSteamUserCallbacks + 43,
            MicroTxnAuthorizationResponse_t = k_iSteamUserCallbacks + 52,
            EncryptedAppTicketResponse_t = k_iSteamUserCallbacks + 54,
            GetAuthSessionTicketResponse_t = k_iSteamUserCallbacks + 63,
            GameWebCallback_t = k_iSteamUserCallbacks + 64,
            StoreAuthURLResponse_t = k_iSteamUserCallbacks + 65,
            MarketEligibilityResponse_t = k_iSteamUserCallbacks + 66,
            DurationControl_t = k_iSteamUserCallbacks + 67,

            k_iSteamGameServerCallbacks = 200,
            GSClientApprove_t = k_iSteamGameServerCallbacks + 1,
            GSClientDeny_t = k_iSteamGameServerCallbacks + 2,
            GSClientKick_t = k_iSteamGameServerCallbacks + 3,
            GSClientSteam2Deny_t = k_iSteamGameServerCallbacks + 4,
            GSClientSteam2Accept_t = k_iSteamGameServerCallbacks + 5,
            GSClientAchievementStatus_t = k_iSteamGameServerCallbacks + 6,
            GSGameplayStats_t = k_iSteamGameServerCallbacks + 7,
            GSClientGroupStatus_t = k_iSteamGameServerCallbacks + 8,
            GSReputation_t = k_iSteamGameServerCallbacks + 9,
            AssociateWithClanResult_t = k_iSteamGameServerCallbacks + 10,
            ComputeNewPlayerCompatibilityResult_t = k_iSteamGameServerCallbacks + 11,

            k_iSteamFriendsCallbacks = 300,
            PersonaStateChange_t = k_iSteamFriendsCallbacks + 4,
            GameOverlayActivated_t = k_iSteamFriendsCallbacks + 31,
            GameServerChangeRequested_t = k_iSteamFriendsCallbacks + 32,
            GameLobbyJoinRequested_t = k_iSteamFriendsCallbacks + 33,
            AvatarImageLoaded_t = k_iSteamFriendsCallbacks + 34,
            ClanOfficerListResponse_t = k_iSteamFriendsCallbacks + 35,
            FriendRichPresenceUpdate_t = k_iSteamFriendsCallbacks + 36,
            GameRichPresenceJoinRequested_t = k_iSteamFriendsCallbacks + 37,
            GameConnectedClanChatMsg_t = k_iSteamFriendsCallbacks + 38,
            GameConnectedChatJoin_t = k_iSteamFriendsCallbacks + 39,
            GameConnectedChatLeave_t = k_iSteamFriendsCallbacks + 40,
            DownloadClanActivityCountsResult_t = k_iSteamFriendsCallbacks + 41,
            JoinClanChatRoomCompletionResult_t = k_iSteamFriendsCallbacks + 42,
            GameConnectedFriendChatMsg_t = k_iSteamFriendsCallbacks + 43,
            FriendsGetFollowerCount_t = k_iSteamFriendsCallbacks + 44,
            FriendsIsFollowing_t = k_iSteamFriendsCallbacks + 45,
            FriendsEnumerateFollowingList_t = k_iSteamFriendsCallbacks + 46,
            SetPersonaNameResponse_t = k_iSteamFriendsCallbacks + 47,
            UnreadChatMessagesChanged_t = k_iSteamFriendsCallbacks + 48,
            OverlayBrowserProtocolNavigation_t = k_iSteamFriendsCallbacks + 49,

            k_iSteamBillingCallbacks = 400,
            FinalPriceMsg_t = k_iSteamBillingCallbacks + 1,
            PurchaseMsg_t = k_iSteamBillingCallbacks + 2,

            k_iSteamMatchmakingCallbacks = 500,
            FavoritesListChangedOld_t = k_iSteamMatchmakingCallbacks + 1,
            FavoritesListChanged_t = k_iSteamMatchmakingCallbacks + 2,
            LobbyInvite_t = k_iSteamMatchmakingCallbacks + 3,
            LobbyEnter_t = k_iSteamMatchmakingCallbacks + 4,
            LobbyDataUpdate_t = k_iSteamMatchmakingCallbacks + 5,
            LobbyChatUpdate_t = k_iSteamMatchmakingCallbacks + 6,
            LobbyChatMsg_t = k_iSteamMatchmakingCallbacks + 7,
            LobbyAdminChange_t = k_iSteamMatchmakingCallbacks + 8,
            LobbyGameCreated_t = k_iSteamMatchmakingCallbacks + 9,
            LobbyMatchList_t = k_iSteamMatchmakingCallbacks + 10,
            LobbyClosing_t = k_iSteamMatchmakingCallbacks + 11,
            LobbyKicked_t = k_iSteamMatchmakingCallbacks + 12,
            LobbyCreated_t = k_iSteamMatchmakingCallbacks + 13,
            RequestFriendsLobbiesResponse_t = k_iSteamMatchmakingCallbacks + 14,
            PSNGameBootInviteResult_t = k_iSteamMatchmakingCallbacks + 15,
            FavoritesListAccountsUpdated_t = k_iSteamMatchmakingCallbacks + 16,

            k_iSteamContentServerCallbacks = 600,
            CSClientApprove_t = k_iSteamContentServerCallbacks + 1,
            CSClientDeny_t = k_iSteamContentServerCallbacks + 2,

            k_iSteamUtilsCallbacks = 700,
            IPCountry_t = k_iSteamUtilsCallbacks + 1,
            LowBatteryPower_t = k_iSteamUtilsCallbacks + 2,
            SteamAPICallCompleted_t = k_iSteamUtilsCallbacks + 3,
            SteamShutdown_t = k_iSteamUtilsCallbacks + 4,
            CheckFileSignature_t = k_iSteamUtilsCallbacks + 5,
            SteamConfigStoreChanged_t = k_iSteamUtilsCallbacks + 11,
            GamepadTextInputDismissed_t = k_iSteamUtilsCallbacks + 14,

            k_iClientFriendsCallbacks = 800,
            GameOverlayActivateRequested_t = k_iClientFriendsCallbacks + 1,
            ClanEventReceived_t = k_iClientFriendsCallbacks + 2,
            FriendAdded_t = k_iClientFriendsCallbacks + 3,
            UserRequestingFriendship_t = k_iClientFriendsCallbacks + 4,
            FriendChatMsg_t = k_iClientFriendsCallbacks + 5,
            FriendInvited_t = k_iClientFriendsCallbacks + 6,
            ChatRoomInvite_t = k_iClientFriendsCallbacks + 7,
            ChatRoomEnter_t = k_iClientFriendsCallbacks + 8,
            ChatMemberStateChange_t = k_iClientFriendsCallbacks + 9,
            ChatRoomMsg_t = k_iClientFriendsCallbacks + 10,
            ChatRoomDlgClose_t = k_iClientFriendsCallbacks + 11,
            ChatRoomClosing_t = k_iClientFriendsCallbacks + 12,
            ChatRoomKicking_t = k_iClientFriendsCallbacks + 13,
            ChatRoomBanning_t = k_iClientFriendsCallbacks + 14,
            ChatRoomCreate_t = k_iClientFriendsCallbacks + 15,
            OpenChatDialog_t = k_iClientFriendsCallbacks + 16,
            ChatRoomActionResult_t = k_iClientFriendsCallbacks + 17,
            ChatRoomDlgSerialized_t = k_iClientFriendsCallbacks + 18,
            ClanInfoChanged_t = k_iClientFriendsCallbacks + 19,
            ChatMemberInfoChanged_t = k_iClientFriendsCallbacks + 20,
            ChatRoomInfoChanged_t = k_iClientFriendsCallbacks + 21,
            SteamRackBouncing_t = k_iClientFriendsCallbacks + 22,
            ChatRoomSpeakChanged_t = k_iClientFriendsCallbacks + 23,
            NotifyIncomingCall_t = k_iClientFriendsCallbacks + 24,
            NotifyHangup_t = k_iClientFriendsCallbacks + 25,
            NotifyRequestResume_t = k_iClientFriendsCallbacks + 26,
            NotifyChatRoomVoiceStateChanged_t = k_iClientFriendsCallbacks + 27,
            ChatRoomDlgUIChange_t = k_iClientFriendsCallbacks + 28,
            VoiceCallInitiated_t = k_iClientFriendsCallbacks + 29,
            FriendIgnored_t = k_iClientFriendsCallbacks + 30,
            VoiceInputDeviceChanged_t = k_iClientFriendsCallbacks + 31,
            VoiceEnabledStateChanged_t = k_iClientFriendsCallbacks + 32,
            FriendsWhoPlayGameUpdate_t = k_iClientFriendsCallbacks + 33,
            FriendProfileInfoResponse_t = k_iClientFriendsCallbacks + 34,
            RichInviteReceived_t = k_iClientFriendsCallbacks + 35,
            FriendsMenuChange_t = k_iClientFriendsCallbacks + 36,
            TradeInviteReceived_t = k_iClientFriendsCallbacks + 37,
            TradeInviteResponse_t = k_iClientFriendsCallbacks + 38,
            TradeStartSession_t = k_iClientFriendsCallbacks + 39,
            TradeInviteCanceled_t = k_iClientFriendsCallbacks + 40,
            GameUsingVoice_t = k_iClientFriendsCallbacks + 41,
            FriendsGroupCreated_t = k_iClientFriendsCallbacks + 42,
            FriendsGroupDeleted_t = k_iClientFriendsCallbacks + 43,
            FriendsGroupRenamed_t = k_iClientFriendsCallbacks + 44,
            FriendsGroupMemberAdded_t = k_iClientFriendsCallbacks + 45,
            FriendsGroupMemberRemoved_t = k_iClientFriendsCallbacks + 46,
            NameHistoryResponse_t = k_iClientFriendsCallbacks + 47,

            k_iClientUserCallbacks = 900,
            SystemIM_t = k_iClientUserCallbacks + 1,
            GuestPassGiftTarget_t = k_iClientUserCallbacks + 2,
            PrimaryChatDestinationSet_t = k_iClientUserCallbacks + 3,
            LicenseChanged_t = k_iClientUserCallbacks + 5,
            RequestClientAppListInfo_t = k_iClientUserCallbacks + 6,
            SetClientAppUpdateState_t = k_iClientUserCallbacks + 7,
            InstallClientApp_t = k_iClientUserCallbacks + 8,
            UninstallClientApp_t = k_iClientUserCallbacks + 9,
            Steam2TicketChanged_t = k_iClientUserCallbacks + 10,
            ClientAppNewsItemUpdate_t = k_iClientUserCallbacks + 11,
            ClientSteamNewsItemUpdate_t = k_iClientUserCallbacks + 12,
            ClientSteamNewsClientUpdate_t = k_iClientUserCallbacks + 13,
            LegacyCDKeyRegistered_t = k_iClientUserCallbacks + 14,
            AccountInformationUpdated_t = k_iClientUserCallbacks + 15,
            GuestPassSent_t = k_iClientUserCallbacks + 16,
            GuestPassAcked_t = k_iClientUserCallbacks + 17,
            GuestPassRedeemed_t = k_iClientUserCallbacks + 18,
            UpdateGuestPasses_t = k_iClientUserCallbacks + 19,
            LogOnCredentialsChanged_t = k_iClientUserCallbacks + 20,
            CheckPasswordResponse_t = k_iClientUserCallbacks + 22,
            ResetPasswordResponse_t = k_iClientUserCallbacks + 23,
            DRMDataRequest_t = k_iClientUserCallbacks + 24,
            DRMDataResponse_t = k_iClientUserCallbacks + 25,
            DRMFailureResponse_t = k_iClientUserCallbacks + 26,
            AppOwnershipTicketReceived_t = k_iClientUserCallbacks + 28,
            PasswordChangeResponse_t = k_iClientUserCallbacks + 29,
            EmailChangeResponse_t = k_iClientUserCallbacks + 30,
            SecretQAChangeResponse_t = k_iClientUserCallbacks + 31,
            CreateAccountResponse_t = k_iClientUserCallbacks + 32,
            SendForgottonPasswordEmailResponse_t = k_iClientUserCallbacks + 33,
            ResetForgottonPasswordResponse_t = k_iClientUserCallbacks + 34,
            CreateAccountInformSteam3Response_t = k_iClientUserCallbacks + 35,
            DownloadFromDFSResponse_t = k_iClientUserCallbacks + 36,
            ClientMarketingMessageUpdate_t = k_iClientUserCallbacks + 37,
            ValidateEmailResponse_t = k_iClientUserCallbacks + 38,
            RequestChangeEmailResponse_t = k_iClientUserCallbacks + 39,
            VerifyPasswordResponse_t = k_iClientUserCallbacks + 40,
            Steam2LoginResponse_t = k_iClientUserCallbacks + 41,
            WebAuthRequestCallback_t = k_iClientUserCallbacks + 42,
            MicroTxnAuthRequestCallback_t = k_iClientUserCallbacks + 44,
            MicroTxnAuthResponse_t = k_iClientUserCallbacks + 45,
            AppMinutesPlayedDataNotice_t = k_iClientUserCallbacks + 46,
            MicroTxnInfoUpdated_t = k_iClientUserCallbacks + 47,
            WalletBalanceUpdated_t = k_iClientUserCallbacks + 48,
            EnableMachineLockingResponse_t = k_iClientUserCallbacks + 49,
            MachineLockProgressResponse_t = k_iClientUserCallbacks + 50,
            Steam3ExtraLoginProgress_t = k_iClientUserCallbacks + 51,
            RequestAccountDataResult_t = k_iClientUserCallbacks + 52,
            IsAccountNameInUseResult_t = k_iClientUserCallbacks + 53,
            LoginInformationChanged_t = k_iClientUserCallbacks + 55,
            RequestSpecialSurveyResult_t = k_iClientUserCallbacks + 56,
            SendSpecialSurveyResponseResult_t = k_iClientUserCallbacks + 57,
            UpdateItemAnnouncement_t = k_iClientUserCallbacks + 58,
            ChangeSteamGuardOptionsResponse_t = k_iClientUserCallbacks + 59,
            UpdateCommentNotification_t = k_iClientUserCallbacks + 60,
            FriendUserStatusPublished_t = k_iClientUserCallbacks + 61,
            UpdateOfflineMessageNotification_t = k_iClientUserCallbacks + 62,
            FriendMessageHistoryChatLog_t = k_iClientUserCallbacks + 63,
            TestAvailablePasswordResponse_t = k_iClientUserCallbacks + 64,
            GetSteamGuardDetailsResponse_t = k_iClientUserCallbacks + 66,

            k_iSteamAppsCallbacks = 1000,
            AppDataChanged_t = k_iSteamAppsCallbacks + 1,
            RequestAppCallbacksComplete_t = k_iSteamAppsCallbacks + 2,
            AppInfoUpdateComplete_t = k_iSteamAppsCallbacks + 3,
            AppEventTriggered_t = k_iSteamAppsCallbacks + 4,
            DlcInstalled_t = k_iSteamAppsCallbacks + 5,
            AppEventStateChange_t = k_iSteamAppsCallbacks + 6,
            AppValidationComplete_t = k_iSteamAppsCallbacks + 7,
            RegisterActivationCodeResponse_t = k_iSteamAppsCallbacks + 8,
            DownloadScheduleChanged_t = k_iSteamAppsCallbacks + 9,
            DlcInstallRequest_t = k_iSteamAppsCallbacks + 10,
            AppLaunchTenFootOverlay_t = k_iSteamAppsCallbacks + 11,
            AppBackupStatus_t = k_iSteamAppsCallbacks + 12,
            RequestAppProofOfPurchaseKeyResponse_t = k_iSteamAppsCallbacks + 13,
            NewUrlLaunchParameters_t = k_iSteamAppsCallbacks + 14,
            AppProofOfPurchaseKeyResponse_t = k_iSteamAppsCallbacks + 21,
            FileDetailsResult_t = k_iSteamAppsCallbacks + 23,
            TimedTrialStatus_t = k_iSteamAppsCallbacks + 30,

            k_iSteamUserStatsCallbacks = 1100,
            UserStatsReceived_t = k_iSteamUserStatsCallbacks + 1,
            UserStatsStored_t = k_iSteamUserStatsCallbacks + 2,
            UserAchievementStored_t = k_iSteamUserStatsCallbacks + 3,
            LeaderboardFindResult_t = k_iSteamUserStatsCallbacks + 4,
            LeaderboardScoresDownloaded_t = k_iSteamUserStatsCallbacks + 5,
            LeaderboardScoreUploaded_t = k_iSteamUserStatsCallbacks + 6,
            NumberOfCurrentPlayers_t = k_iSteamUserStatsCallbacks + 7,
            UserStatsUnloaded_t = k_iSteamUserStatsCallbacks + 8,
            UserAchievementIconFetched_t = k_iSteamUserStatsCallbacks + 9,
            GlobalAchievementPercentagesReady_t = k_iSteamUserStatsCallbacks + 10,
            LeaderboardUGCSet_t = k_iSteamUserStatsCallbacks + 11,
            GlobalStatsReceived_t = k_iSteamUserStatsCallbacks + 12,

            k_iSteamNetworkingCallbacks = 1200,
            SocketStatusCallback_t = k_iSteamNetworkingCallbacks + 1,
            P2PSessionRequest_t = k_iSteamNetworkingCallbacks + 2,
            P2PSessionConnectFail_t = k_iSteamNetworkingCallbacks + 3,
            SteamNetworkingSocketsConfigUpdated_t = k_iSteamNetworkingCallbacks + 95,
            SteamNetworkingSocketsCert_t = k_iSteamNetworkingCallbacks + 96,
            SteamNetworkingSocketsRecvP2PFailure_t = k_iSteamNetworkingCallbacks + 97,
            SteamNetworkingSocketsRecvP2PRendezvous_t = k_iSteamNetworkingCallbacks + 98,

            k_iSteamNetworkingSocketsCallbacks = 1220,
            SteamNetConnectionStatusChangedCallback_t = k_iSteamNetworkingSocketsCallbacks + 1,
            SteamNetAuthenticationStatus_t = k_iSteamNetworkingSocketsCallbacks + 2,

            k_iSteamNetworkingMessagesCallbacks = 1250,
            SteamNetworkingMessagesSessionRequest_t = k_iSteamNetworkingMessagesCallbacks + 1,
            SteamNetworkingMessagesSessionFailed_t = k_iSteamNetworkingMessagesCallbacks + 2,

            k_iSteamNetworkingUtilsCallbacks = 1280,
            SteamRelayNetworkStatus_t = k_iSteamNetworkingUtilsCallbacks + 1,

            k_iClientRemoteStorageCallbacks = 1300,
            RemoteStorageAppSyncedClient_t = k_iClientRemoteStorageCallbacks + 1,
            RemoteStorageAppSyncedServer_t = k_iClientRemoteStorageCallbacks + 2,
            RemoteStorageAppSyncProgress_t = k_iClientRemoteStorageCallbacks + 3,
            RemoteStorageAppInfoLoaded_t = k_iClientRemoteStorageCallbacks + 4,
            RemoteStorageAppSyncStatusCheck_t = k_iClientRemoteStorageCallbacks + 5,
            RemoteStorageConflictResolution_t = k_iClientRemoteStorageCallbacks + 6,
            RemoteStorageFileShareResult_t = k_iClientRemoteStorageCallbacks + 7,
            RemoteStorageDownloadUGCResultold_t = k_iClientRemoteStorageCallbacks + 8,
            RemoteStoragePublishFileResult_t = k_iClientRemoteStorageCallbacks + 9,
            RemoteStorageGetPublishedFileDetailsResultold_t = k_iClientRemoteStorageCallbacks + 10,
            RemoteStorageDeletePublishedFileResult_t = k_iClientRemoteStorageCallbacks + 11,
            RemoteStorageEnumerateUserPublishedFilesResult_t = k_iClientRemoteStorageCallbacks + 12,
            RemoteStorageSubscribePublishedFileResult_t = k_iClientRemoteStorageCallbacks + 13,
            RemoteStorageEnumerateUserSubscribedFilesResult_t = k_iClientRemoteStorageCallbacks + 14,
            RemoteStorageUnsubscribePublishedFileResult_t = k_iClientRemoteStorageCallbacks + 15,
            RemoteStorageUpdatePublishedFileResult_t = k_iClientRemoteStorageCallbacks + 16,
            RemoteStorageDownloadUGCResult_t = k_iClientRemoteStorageCallbacks + 17,
            RemoteStorageGetPublishedFileDetailsResult_t = k_iClientRemoteStorageCallbacks + 18,
            RemoteStorageEnumerateWorkshopFilesResult_t = k_iClientRemoteStorageCallbacks + 19,
            RemoteStorageGetPublishedItemVoteDetailsResult_t = k_iClientRemoteStorageCallbacks + 20,
            RemoteStoragePublishedFileSubscribed_t = k_iClientRemoteStorageCallbacks + 21,
            RemoteStoragePublishedFileUnsubscribed_t = k_iClientRemoteStorageCallbacks + 22,
            RemoteStoragePublishedFileDeleted_t = k_iClientRemoteStorageCallbacks + 23,
            RemoteStorageUpdateUserPublishedItemVoteResult_t = k_iClientRemoteStorageCallbacks + 24,
            RemoteStorageUserVoteDetails_t = k_iClientRemoteStorageCallbacks + 25,
            RemoteStorageEnumerateUserSharedWorkshopFilesResult_t = k_iClientRemoteStorageCallbacks + 26,
            RemoteStorageSetUserPublishedFileActionResult_t = k_iClientRemoteStorageCallbacks + 27,
            RemoteStorageEnumeratePublishedFilesByUserActionResult_t = k_iClientRemoteStorageCallbacks + 28,
            RemoteStoragePublishFileProgress_t = k_iClientRemoteStorageCallbacks + 29,
            RemoteStoragePublishedFileUpdated_t = k_iClientRemoteStorageCallbacks + 30,
            RemoteStorageFileWriteAsyncComplete_t = k_iClientRemoteStorageCallbacks + 31,
            RemoteStorageFileReadAsyncComplete_t = k_iClientRemoteStorageCallbacks + 32,

            k_iClientDepotBuilderCallbacks = 1400,

            k_iSteamUserItemsCallbacks = 1400,

            k_iSteamGameServerItemsCallbacks = 1500,

            k_iClientUtilsCallbacks = 1600,
            CellIDChanged_t = k_iClientUtilsCallbacks + 3,

            k_iSteamGameCoordinatorCallbacks = 1700,
            GCMessageAvailable_t = k_iSteamGameCoordinatorCallbacks + 1,
            GCMessageFailed_t = k_iSteamGameCoordinatorCallbacks + 2,

            k_iSteamGameServerStatsCallbacks = 1800,
            GSStatsReceived_t = k_iSteamGameServerStatsCallbacks + 0,
            GSStatsStored_t = k_iSteamGameServerStatsCallbacks + 1,

            k_iSteam2AsyncCallbacks = 1900,

            k_iSteamGameStatsCallbacks = 2000,
            GameStatsSessionIssued_t = k_iSteamGameStatsCallbacks + 1,
            GameStatsSessionClosed_t = k_iSteamGameStatsCallbacks + 2,

            k_iClientHTTPCallbacks = 2100,
            HTTPRequestCompleted_t = k_iClientHTTPCallbacks + 1,
            HTTPRequestHeadersReceived_t = k_iClientHTTPCallbacks + 2,
            HTTPRequestDataReceived_t = k_iClientHTTPCallbacks + 3,

            k_iClientScreenshotsCallbacks = 2200,
            ScreenshotUploadProgress_t = k_iClientScreenshotsCallbacks + 1,
            ScreenshotWritten_t = k_iClientScreenshotsCallbacks + 2,
            ScreenshotUploaded_t = k_iClientScreenshotsCallbacks + 3,
            ScreenshotBatchComplete_t = k_iClientScreenshotsCallbacks + 4,
            ScreenshotDeleted_t = k_iClientScreenshotsCallbacks + 5,
            ScreenshotTriggered_t = k_iClientScreenshotsCallbacks + 6,

            k_iSteamScreenshotsCallbacks = 2300,
            ScreenshotReady_t = k_iSteamScreenshotsCallbacks + 1,
            ScreenshotRequested_t = k_iSteamScreenshotsCallbacks + 2,

            k_iClientAudioCallbacks = 2400,

            k_iClientUnifiedMessagesCallbacks = 2500,
            SteamUnifiedMessagesSendMethodResult_t = k_iClientUnifiedMessagesCallbacks + 1,

            k_iSteamStreamLauncherCallbacks = 2600,

            k_iClientControllerCallbacks = 2700,

            k_iSteamControllerCallbacks = 2800,

            k_iClientParentalSettingsCallbacks = 2900,

            k_iClientDeviceAuthCallbacks = 3000,

            k_iClientNetworkDeviceManagerCallbacks = 3100,

            k_iClientMusicCallbacks = 3200,

            k_iClientRemoteClientManagerCallbacks = 3300,

            k_iClientUGCCallbacks = 3400,
            SteamUGCQueryCompleted_t = k_iClientUGCCallbacks + 1,
            SteamUGCRequestUGCDetailsResult_t = k_iClientUGCCallbacks + 2,
            CreateItemResult_t = k_iClientUGCCallbacks + 3,
            SubmitItemUpdateResult_t = k_iClientUGCCallbacks + 4,
            ItemInstalled_t = k_iClientUGCCallbacks + 5,
            DownloadItemResult_t = k_iClientUGCCallbacks + 6,
            UserFavoriteItemsListChanged_t = k_iClientUGCCallbacks + 7,
            SetUserItemVoteResult_t = k_iClientUGCCallbacks + 8,
            GetUserItemVoteResult_t = k_iClientUGCCallbacks + 9,
            StartPlaytimeTrackingResult_t = k_iClientUGCCallbacks + 10,
            StopPlaytimeTrackingResult_t = k_iClientUGCCallbacks + 11,
            AddUGCDependencyResult_t = k_iClientUGCCallbacks + 12,
            RemoveUGCDependencyResult_t = k_iClientUGCCallbacks + 13,
            AddAppDependencyResult_t = k_iClientUGCCallbacks + 14,
            RemoveAppDependencyResult_t = k_iClientUGCCallbacks + 15,
            GetAppDependenciesResult_t = k_iClientUGCCallbacks + 16,
            DeleteItemResult_t = k_iClientUGCCallbacks + 17,

            k_iSteamStreamClientCallbacks = 3500,

            k_IClientProductBuilderCallbacks = 3600,

            k_iClientShortcutsCallbacks = 3700,

            k_iClientRemoteControlManagerCallbacks = 3800,

            k_iSteamAppListCallbacks = 3900,
            SteamAppInstalled_t = k_iSteamAppListCallbacks + 1,
            SteamAppUninstalled_t = k_iSteamAppListCallbacks + 2,

            k_iSteamMusicCallbacks = 4000,
            PlaybackStatusHasChanged_t = k_iSteamMusicCallbacks + 1,
            VolumeHasChanged_t = k_iSteamMusicCallbacks + 2,
            MusicPlayerWantsVolume_t = k_iSteamMusicCallbacks + 11,
            MusicPlayerSelectsQueueEntry_t = k_iSteamMusicCallbacks + 12,
            MusicPlayerSelectsPlaylistEntry_t = k_iSteamMusicCallbacks + 13,

            k_iSteamMusicRemoteCallbacks = 4100,
            MusicPlayerRemoteWillActivate_t = k_iSteamMusicRemoteCallbacks + 1,
            MusicPlayerRemoteWillDeactivate_t = k_iSteamMusicRemoteCallbacks + 2,
            MusicPlayerRemoteToFront_t = k_iSteamMusicRemoteCallbacks + 3,
            MusicPlayerWillQuit_t = k_iSteamMusicRemoteCallbacks + 4,
            MusicPlayerWantsPlay_t = k_iSteamMusicRemoteCallbacks + 5,
            MusicPlayerWantsPause_t = k_iSteamMusicRemoteCallbacks + 6,
            MusicPlayerWantsPlayPrevious_t = k_iSteamMusicRemoteCallbacks + 7,
            MusicPlayerWantsPlayNext_t = k_iSteamMusicRemoteCallbacks + 8,
            MusicPlayerWantsShuffled_t = k_iSteamMusicRemoteCallbacks + 9,
            MusicPlayerWantsLooped_t = k_iSteamMusicRemoteCallbacks + 10,
            MusicPlayerWantsPlayingRepeatStatus_t = k_iSteamMusicRemoteCallbacks + 14,

            k_iClientVRCallbacks = 4200,

            k_iClientGameNotificationCallbacks = 4300,

            k_iClientReservedCallbacks = 4300,

            k_iSteamGameNotificationCallbacks = 4400,

            k_iSteamReservedCallbacks = 4400,

            k_iSteamHTMLSurfaceCallbacks = 4500,
            HTML_BrowserReady_t = k_iSteamHTMLSurfaceCallbacks + 1,
            HTML_NeedsPaint_t = k_iSteamHTMLSurfaceCallbacks + 2,
            HTML_StartRequest_t = k_iSteamHTMLSurfaceCallbacks + 3,
            HTML_CloseBrowser_t = k_iSteamHTMLSurfaceCallbacks + 4,
            HTML_URLChanged_t = k_iSteamHTMLSurfaceCallbacks + 5,
            HTML_FinishedRequest_t = k_iSteamHTMLSurfaceCallbacks + 6,
            HTML_OpenLinkInNewTab_t = k_iSteamHTMLSurfaceCallbacks + 7,
            HTML_ChangedTitle_t = k_iSteamHTMLSurfaceCallbacks + 8,
            HTML_SearchResults_t = k_iSteamHTMLSurfaceCallbacks + 9,
            HTML_CanGoBackAndForward_t = k_iSteamHTMLSurfaceCallbacks + 10,
            HTML_HorizontalScroll_t = k_iSteamHTMLSurfaceCallbacks + 11,
            HTML_VerticalScroll_t = k_iSteamHTMLSurfaceCallbacks + 12,
            HTML_LinkAtPosition_t = k_iSteamHTMLSurfaceCallbacks + 13,
            HTML_JSAlert_t = k_iSteamHTMLSurfaceCallbacks + 14,
            HTML_JSConfirm_t = k_iSteamHTMLSurfaceCallbacks + 15,
            HTML_FileOpenDialog_t = k_iSteamHTMLSurfaceCallbacks + 16,
            HTML_NewWindow_t = k_iSteamHTMLSurfaceCallbacks + 21,
            HTML_SetCursor_t = k_iSteamHTMLSurfaceCallbacks + 22,
            HTML_StatusText_t = k_iSteamHTMLSurfaceCallbacks + 23,
            HTML_ShowToolTip_t = k_iSteamHTMLSurfaceCallbacks + 24,
            HTML_UpdateToolTip_t = k_iSteamHTMLSurfaceCallbacks + 25,
            HTML_HideToolTip_t = k_iSteamHTMLSurfaceCallbacks + 26,
            HTML_BrowserRestarted_t = k_iSteamHTMLSurfaceCallbacks + 27,

            k_iClientVideoCallbacks = 4600,
            BroadcastUploadStart_t = k_iClientVideoCallbacks + 4,
            BroadcastUploadStop_t = k_iClientVideoCallbacks + 5,
            GetVideoURLResult_t = k_iClientVideoCallbacks + 11,
            GetOPFSettingsResult_t = k_iClientVideoCallbacks + 24,

            k_iClientInventoryCallbacks = 4700,
            SteamInventoryResultReady_t = k_iClientInventoryCallbacks + 0,
            SteamInventoryFullUpdate_t = k_iClientInventoryCallbacks + 1,
            SteamInventoryDefinitionUpdate_t = k_iClientInventoryCallbacks + 2,
            SteamInventoryEligiblePromoItemDefIDs_t = k_iClientInventoryCallbacks + 3,
            SteamInventoryStartPurchaseResult_t = k_iClientInventoryCallbacks + 4,
            SteamInventoryRequestPricesResult_t = k_iClientInventoryCallbacks + 5,

            k_iClientBluetoothManagerCallbacks = 4800,

            k_iClientSharedConnectionCallbacks = 4900,

            k_ISteamParentalSettingsCallbacks = 5000,
            SteamParentalSettingsChanged_t = k_ISteamParentalSettingsCallbacks + 1,

            k_iClientShaderCallbacks = 5100,

            k_iSteamGameSearchCallbacks = 5200,
            SearchForGameProgressCallback_t = k_iSteamGameSearchCallbacks + 1,
            SearchForGameResultCallback_t = k_iSteamGameSearchCallbacks + 2,
            RequestPlayersForGameProgressCallback_t = k_iSteamGameSearchCallbacks + 11,
            RequestPlayersForGameResultCallback_t = k_iSteamGameSearchCallbacks + 12,
            RequestPlayersForGameFinalResultCallback_t = k_iSteamGameSearchCallbacks + 13,
            SubmitPlayerResultResultCallback_t = k_iSteamGameSearchCallbacks + 14,
            EndGameResultCallback_t = k_iSteamGameSearchCallbacks + 15,

            k_iSteamPartiesCallbacks = 5300,
            JoinPartyCallback_t = k_iSteamPartiesCallbacks + 1,
            CreateBeaconCallback_t = k_iSteamPartiesCallbacks + 2,
            ReservationNotificationCallback_t = k_iSteamPartiesCallbacks + 3,
            ChangeNumOpenSlotsCallback_t = k_iSteamPartiesCallbacks + 4,
            AvailableBeaconLocationsUpdated_t = k_iSteamPartiesCallbacks + 5,
            ActiveBeaconsUpdated_t = k_iSteamPartiesCallbacks + 6,

            k_iClientPartiesCallbacks = 5400,

            k_iSteamSTARCallbacks = 5500,

            k_iClientSTARCallbacks = 5600,

            k_iSteamRemotePlayCallbacks = 5700,
            SteamRemotePlaySessionConnected_t = k_iSteamRemotePlayCallbacks + 1,
            SteamRemotePlaySessionDisconnected_t = k_iSteamRemotePlayCallbacks + 2,

            k_iClientCompatCallbacks = 5800,

            k_iSteamChatCallbacks = 5900,
        };

        constexpr const char *asString(ECallbackType Code)
        {
            switch (Code)
            {
                case k_iSteamUserCallbacks: return "k_iSteamUserCallbacks";
                case SteamServersConnected_t: return "SteamServersConnected_t";
                case SteamServerConnectFailure_t: return "SteamServerConnectFailure_t";
                case SteamServersDisconnected_t: return "SteamServersDisconnected_t";
                case BeginLogonRetry_t: return "BeginLogonRetry_t";
                case ClientGameServerDeny_t: return "ClientGameServerDeny_t";
                case PrimaryChatDestinationSetOld_t: return "PrimaryChatDestinationSetOld_t";
                case GSPolicyResponse_t: return "GSPolicyResponse_t";
                case IPCFailure_t: return "IPCFailure_t";
                case LicensesUpdated_t: return "LicensesUpdated_t";
                case AppLifetimeNotice_t: return "AppLifetimeNotice_t";
                case DRMSDKFileTransferResult_t: return "DRMSDKFileTransferResult_t";
                case ValidateAuthTicketResponse_t: return "ValidateAuthTicketResponse_t";
                case MicroTxnAuthorizationResponse_t: return "MicroTxnAuthorizationResponse_t";
                case EncryptedAppTicketResponse_t: return "EncryptedAppTicketResponse_t";
                case GetAuthSessionTicketResponse_t: return "GetAuthSessionTicketResponse_t";
                case GameWebCallback_t: return "GameWebCallback_t";
                case StoreAuthURLResponse_t: return "StoreAuthURLResponse_t";
                case MarketEligibilityResponse_t: return "MarketEligibilityResponse_t";
                case DurationControl_t: return "DurationControl_t";
                case k_iSteamGameServerCallbacks: return "k_iSteamGameServerCallbacks";
                case GSClientApprove_t: return "GSClientApprove_t";
                case GSClientDeny_t: return "GSClientDeny_t";
                case GSClientKick_t: return "GSClientKick_t";
                case GSClientSteam2Deny_t: return "GSClientSteam2Deny_t";
                case GSClientSteam2Accept_t: return "GSClientSteam2Accept_t";
                case GSClientAchievementStatus_t: return "GSClientAchievementStatus_t";
                case GSGameplayStats_t: return "GSGameplayStats_t";
                case GSClientGroupStatus_t: return "GSClientGroupStatus_t";
                case GSReputation_t: return "GSReputation_t";
                case AssociateWithClanResult_t: return "AssociateWithClanResult_t";
                case ComputeNewPlayerCompatibilityResult_t: return "ComputeNewPlayerCompatibilityResult_t";
                case k_iSteamFriendsCallbacks: return "k_iSteamFriendsCallbacks";
                case PersonaStateChange_t: return "PersonaStateChange_t";
                case GameOverlayActivated_t: return "GameOverlayActivated_t";
                case GameServerChangeRequested_t: return "GameServerChangeRequested_t";
                case GameLobbyJoinRequested_t: return "GameLobbyJoinRequested_t";
                case AvatarImageLoaded_t: return "AvatarImageLoaded_t";
                case ClanOfficerListResponse_t: return "ClanOfficerListResponse_t";
                case FriendRichPresenceUpdate_t: return "FriendRichPresenceUpdate_t";
                case GameRichPresenceJoinRequested_t: return "GameRichPresenceJoinRequested_t";
                case GameConnectedClanChatMsg_t: return "GameConnectedClanChatMsg_t";
                case GameConnectedChatJoin_t: return "GameConnectedChatJoin_t";
                case GameConnectedChatLeave_t: return "GameConnectedChatLeave_t";
                case DownloadClanActivityCountsResult_t: return "DownloadClanActivityCountsResult_t";
                case JoinClanChatRoomCompletionResult_t: return "JoinClanChatRoomCompletionResult_t";
                case GameConnectedFriendChatMsg_t: return "GameConnectedFriendChatMsg_t";
                case FriendsGetFollowerCount_t: return "FriendsGetFollowerCount_t";
                case FriendsIsFollowing_t: return "FriendsIsFollowing_t";
                case FriendsEnumerateFollowingList_t: return "FriendsEnumerateFollowingList_t";
                case SetPersonaNameResponse_t: return "SetPersonaNameResponse_t";
                case UnreadChatMessagesChanged_t: return "UnreadChatMessagesChanged_t";
                case OverlayBrowserProtocolNavigation_t: return "OverlayBrowserProtocolNavigation_t";
                case k_iSteamBillingCallbacks: return "k_iSteamBillingCallbacks";
                case FinalPriceMsg_t: return "FinalPriceMsg_t";
                case PurchaseMsg_t: return "PurchaseMsg_t";
                case k_iSteamMatchmakingCallbacks: return "k_iSteamMatchmakingCallbacks";
                case FavoritesListChangedOld_t: return "FavoritesListChangedOld_t";
                case FavoritesListChanged_t: return "FavoritesListChanged_t";
                case LobbyInvite_t: return "LobbyInvite_t";
                case LobbyEnter_t: return "LobbyEnter_t";
                case LobbyDataUpdate_t: return "LobbyDataUpdate_t";
                case LobbyChatUpdate_t: return "LobbyChatUpdate_t";
                case LobbyChatMsg_t: return "LobbyChatMsg_t";
                case LobbyAdminChange_t: return "LobbyAdminChange_t";
                case LobbyGameCreated_t: return "LobbyGameCreated_t";
                case LobbyMatchList_t: return "LobbyMatchList_t";
                case LobbyClosing_t: return "LobbyClosing_t";
                case LobbyKicked_t: return "LobbyKicked_t";
                case LobbyCreated_t: return "LobbyCreated_t";
                case RequestFriendsLobbiesResponse_t: return "RequestFriendsLobbiesResponse_t";
                case PSNGameBootInviteResult_t: return "PSNGameBootInviteResult_t";
                case FavoritesListAccountsUpdated_t: return "FavoritesListAccountsUpdated_t";
                case k_iSteamContentServerCallbacks: return "k_iSteamContentServerCallbacks";
                case CSClientApprove_t: return "CSClientApprove_t";
                case CSClientDeny_t: return "CSClientDeny_t";
                case k_iSteamUtilsCallbacks: return "k_iSteamUtilsCallbacks";
                case IPCountry_t: return "IPCountry_t";
                case LowBatteryPower_t: return "LowBatteryPower_t";
                case SteamAPICallCompleted_t: return "SteamAPICallCompleted_t";
                case SteamShutdown_t: return "SteamShutdown_t";
                case CheckFileSignature_t: return "CheckFileSignature_t";
                case SteamConfigStoreChanged_t: return "SteamConfigStoreChanged_t";
                case GamepadTextInputDismissed_t: return "GamepadTextInputDismissed_t";
                case k_iClientFriendsCallbacks: return "k_iClientFriendsCallbacks";
                case GameOverlayActivateRequested_t: return "GameOverlayActivateRequested_t";
                case ClanEventReceived_t: return "ClanEventReceived_t";
                case FriendAdded_t: return "FriendAdded_t";
                case UserRequestingFriendship_t: return "UserRequestingFriendship_t";
                case FriendChatMsg_t: return "FriendChatMsg_t";
                case FriendInvited_t: return "FriendInvited_t";
                case ChatRoomInvite_t: return "ChatRoomInvite_t";
                case ChatRoomEnter_t: return "ChatRoomEnter_t";
                case ChatMemberStateChange_t: return "ChatMemberStateChange_t";
                case ChatRoomMsg_t: return "ChatRoomMsg_t";
                case ChatRoomDlgClose_t: return "ChatRoomDlgClose_t";
                case ChatRoomClosing_t: return "ChatRoomClosing_t";
                case ChatRoomKicking_t: return "ChatRoomKicking_t";
                case ChatRoomBanning_t: return "ChatRoomBanning_t";
                case ChatRoomCreate_t: return "ChatRoomCreate_t";
                case OpenChatDialog_t: return "OpenChatDialog_t";
                case ChatRoomActionResult_t: return "ChatRoomActionResult_t";
                case ChatRoomDlgSerialized_t: return "ChatRoomDlgSerialized_t";
                case ClanInfoChanged_t: return "ClanInfoChanged_t";
                case ChatMemberInfoChanged_t: return "ChatMemberInfoChanged_t";
                case ChatRoomInfoChanged_t: return "ChatRoomInfoChanged_t";
                case SteamRackBouncing_t: return "SteamRackBouncing_t";
                case ChatRoomSpeakChanged_t: return "ChatRoomSpeakChanged_t";
                case NotifyIncomingCall_t: return "NotifyIncomingCall_t";
                case NotifyHangup_t: return "NotifyHangup_t";
                case NotifyRequestResume_t: return "NotifyRequestResume_t";
                case NotifyChatRoomVoiceStateChanged_t: return "NotifyChatRoomVoiceStateChanged_t";
                case ChatRoomDlgUIChange_t: return "ChatRoomDlgUIChange_t";
                case VoiceCallInitiated_t: return "VoiceCallInitiated_t";
                case FriendIgnored_t: return "FriendIgnored_t";
                case VoiceInputDeviceChanged_t: return "VoiceInputDeviceChanged_t";
                case VoiceEnabledStateChanged_t: return "VoiceEnabledStateChanged_t";
                case FriendsWhoPlayGameUpdate_t: return "FriendsWhoPlayGameUpdate_t";
                case FriendProfileInfoResponse_t: return "FriendProfileInfoResponse_t";
                case RichInviteReceived_t: return "RichInviteReceived_t";
                case FriendsMenuChange_t: return "FriendsMenuChange_t";
                case TradeInviteReceived_t: return "TradeInviteReceived_t";
                case TradeInviteResponse_t: return "TradeInviteResponse_t";
                case TradeStartSession_t: return "TradeStartSession_t";
                case TradeInviteCanceled_t: return "TradeInviteCanceled_t";
                case GameUsingVoice_t: return "GameUsingVoice_t";
                case FriendsGroupCreated_t: return "FriendsGroupCreated_t";
                case FriendsGroupDeleted_t: return "FriendsGroupDeleted_t";
                case FriendsGroupRenamed_t: return "FriendsGroupRenamed_t";
                case FriendsGroupMemberAdded_t: return "FriendsGroupMemberAdded_t";
                case FriendsGroupMemberRemoved_t: return "FriendsGroupMemberRemoved_t";
                case NameHistoryResponse_t: return "NameHistoryResponse_t";
                case k_iClientUserCallbacks: return "k_iClientUserCallbacks";
                case SystemIM_t: return "SystemIM_t";
                case GuestPassGiftTarget_t: return "GuestPassGiftTarget_t";
                case PrimaryChatDestinationSet_t: return "PrimaryChatDestinationSet_t";
                case LicenseChanged_t: return "LicenseChanged_t";
                case RequestClientAppListInfo_t: return "RequestClientAppListInfo_t";
                case SetClientAppUpdateState_t: return "SetClientAppUpdateState_t";
                case InstallClientApp_t: return "InstallClientApp_t";
                case UninstallClientApp_t: return "UninstallClientApp_t";
                case Steam2TicketChanged_t: return "Steam2TicketChanged_t";
                case ClientAppNewsItemUpdate_t: return "ClientAppNewsItemUpdate_t";
                case ClientSteamNewsItemUpdate_t: return "ClientSteamNewsItemUpdate_t";
                case ClientSteamNewsClientUpdate_t: return "ClientSteamNewsClientUpdate_t";
                case LegacyCDKeyRegistered_t: return "LegacyCDKeyRegistered_t";
                case AccountInformationUpdated_t: return "AccountInformationUpdated_t";
                case GuestPassSent_t: return "GuestPassSent_t";
                case GuestPassAcked_t: return "GuestPassAcked_t";
                case GuestPassRedeemed_t: return "GuestPassRedeemed_t";
                case UpdateGuestPasses_t: return "UpdateGuestPasses_t";
                case LogOnCredentialsChanged_t: return "LogOnCredentialsChanged_t";
                case CheckPasswordResponse_t: return "CheckPasswordResponse_t";
                case ResetPasswordResponse_t: return "ResetPasswordResponse_t";
                case DRMDataRequest_t: return "DRMDataRequest_t";
                case DRMDataResponse_t: return "DRMDataResponse_t";
                case DRMFailureResponse_t: return "DRMFailureResponse_t";
                case AppOwnershipTicketReceived_t: return "AppOwnershipTicketReceived_t";
                case PasswordChangeResponse_t: return "PasswordChangeResponse_t";
                case EmailChangeResponse_t: return "EmailChangeResponse_t";
                case SecretQAChangeResponse_t: return "SecretQAChangeResponse_t";
                case CreateAccountResponse_t: return "CreateAccountResponse_t";
                case SendForgottonPasswordEmailResponse_t: return "SendForgottonPasswordEmailResponse_t";
                case ResetForgottonPasswordResponse_t: return "ResetForgottonPasswordResponse_t";
                case CreateAccountInformSteam3Response_t: return "CreateAccountInformSteam3Response_t";
                case DownloadFromDFSResponse_t: return "DownloadFromDFSResponse_t";
                case ClientMarketingMessageUpdate_t: return "ClientMarketingMessageUpdate_t";
                case ValidateEmailResponse_t: return "ValidateEmailResponse_t";
                case RequestChangeEmailResponse_t: return "RequestChangeEmailResponse_t";
                case VerifyPasswordResponse_t: return "VerifyPasswordResponse_t";
                case Steam2LoginResponse_t: return "Steam2LoginResponse_t";
                case WebAuthRequestCallback_t: return "WebAuthRequestCallback_t";
                case MicroTxnAuthRequestCallback_t: return "MicroTxnAuthRequestCallback_t";
                case MicroTxnAuthResponse_t: return "MicroTxnAuthResponse_t";
                case AppMinutesPlayedDataNotice_t: return "AppMinutesPlayedDataNotice_t";
                case MicroTxnInfoUpdated_t: return "MicroTxnInfoUpdated_t";
                case WalletBalanceUpdated_t: return "WalletBalanceUpdated_t";
                case EnableMachineLockingResponse_t: return "EnableMachineLockingResponse_t";
                case MachineLockProgressResponse_t: return "MachineLockProgressResponse_t";
                case Steam3ExtraLoginProgress_t: return "Steam3ExtraLoginProgress_t";
                case RequestAccountDataResult_t: return "RequestAccountDataResult_t";
                case IsAccountNameInUseResult_t: return "IsAccountNameInUseResult_t";
                case LoginInformationChanged_t: return "LoginInformationChanged_t";
                case RequestSpecialSurveyResult_t: return "RequestSpecialSurveyResult_t";
                case SendSpecialSurveyResponseResult_t: return "SendSpecialSurveyResponseResult_t";
                case UpdateItemAnnouncement_t: return "UpdateItemAnnouncement_t";
                case ChangeSteamGuardOptionsResponse_t: return "ChangeSteamGuardOptionsResponse_t";
                case UpdateCommentNotification_t: return "UpdateCommentNotification_t";
                case FriendUserStatusPublished_t: return "FriendUserStatusPublished_t";
                case UpdateOfflineMessageNotification_t: return "UpdateOfflineMessageNotification_t";
                case FriendMessageHistoryChatLog_t: return "FriendMessageHistoryChatLog_t";
                case TestAvailablePasswordResponse_t: return "TestAvailablePasswordResponse_t";
                case GetSteamGuardDetailsResponse_t: return "GetSteamGuardDetailsResponse_t";
                case k_iSteamAppsCallbacks: return "k_iSteamAppsCallbacks";
                case AppDataChanged_t: return "AppDataChanged_t";
                case RequestAppCallbacksComplete_t: return "RequestAppCallbacksComplete_t";
                case AppInfoUpdateComplete_t: return "AppInfoUpdateComplete_t";
                case AppEventTriggered_t: return "AppEventTriggered_t";
                case DlcInstalled_t: return "DlcInstalled_t";
                case AppEventStateChange_t: return "AppEventStateChange_t";
                case AppValidationComplete_t: return "AppValidationComplete_t";
                case RegisterActivationCodeResponse_t: return "RegisterActivationCodeResponse_t";
                case DownloadScheduleChanged_t: return "DownloadScheduleChanged_t";
                case DlcInstallRequest_t: return "DlcInstallRequest_t";
                case AppLaunchTenFootOverlay_t: return "AppLaunchTenFootOverlay_t";
                case AppBackupStatus_t: return "AppBackupStatus_t";
                case RequestAppProofOfPurchaseKeyResponse_t: return "RequestAppProofOfPurchaseKeyResponse_t";
                case NewUrlLaunchParameters_t: return "NewUrlLaunchParameters_t";
                case AppProofOfPurchaseKeyResponse_t: return "AppProofOfPurchaseKeyResponse_t";
                case FileDetailsResult_t: return "FileDetailsResult_t";
                case TimedTrialStatus_t: return "TimedTrialStatus_t";
                case k_iSteamUserStatsCallbacks: return "k_iSteamUserStatsCallbacks";
                case UserStatsReceived_t: return "UserStatsReceived_t";
                case UserStatsStored_t: return "UserStatsStored_t";
                case UserAchievementStored_t: return "UserAchievementStored_t";
                case LeaderboardFindResult_t: return "LeaderboardFindResult_t";
                case LeaderboardScoresDownloaded_t: return "LeaderboardScoresDownloaded_t";
                case LeaderboardScoreUploaded_t: return "LeaderboardScoreUploaded_t";
                case NumberOfCurrentPlayers_t: return "NumberOfCurrentPlayers_t";
                case UserStatsUnloaded_t: return "UserStatsUnloaded_t";
                case UserAchievementIconFetched_t: return "UserAchievementIconFetched_t";
                case GlobalAchievementPercentagesReady_t: return "GlobalAchievementPercentagesReady_t";
                case LeaderboardUGCSet_t: return "LeaderboardUGCSet_t";
                case GlobalStatsReceived_t: return "GlobalStatsReceived_t";
                case k_iSteamNetworkingCallbacks: return "k_iSteamNetworkingCallbacks";
                case SocketStatusCallback_t: return "SocketStatusCallback_t";
                case P2PSessionRequest_t: return "P2PSessionRequest_t";
                case P2PSessionConnectFail_t: return "P2PSessionConnectFail_t";
                case SteamNetworkingSocketsConfigUpdated_t: return "SteamNetworkingSocketsConfigUpdated_t";
                case SteamNetworkingSocketsCert_t: return "SteamNetworkingSocketsCert_t";
                case SteamNetworkingSocketsRecvP2PFailure_t: return "SteamNetworkingSocketsRecvP2PFailure_t";
                case SteamNetworkingSocketsRecvP2PRendezvous_t: return "SteamNetworkingSocketsRecvP2PRendezvous_t";
                case k_iSteamNetworkingSocketsCallbacks: return "k_iSteamNetworkingSocketsCallbacks";
                case SteamNetConnectionStatusChangedCallback_t: return "SteamNetConnectionStatusChangedCallback_t";
                case SteamNetAuthenticationStatus_t: return "SteamNetAuthenticationStatus_t";
                case k_iSteamNetworkingMessagesCallbacks: return "k_iSteamNetworkingMessagesCallbacks";
                case SteamNetworkingMessagesSessionRequest_t: return "SteamNetworkingMessagesSessionRequest_t";
                case SteamNetworkingMessagesSessionFailed_t: return "SteamNetworkingMessagesSessionFailed_t";
                case k_iSteamNetworkingUtilsCallbacks: return "k_iSteamNetworkingUtilsCallbacks";
                case SteamRelayNetworkStatus_t: return "SteamRelayNetworkStatus_t";
                case k_iClientRemoteStorageCallbacks: return "k_iClientRemoteStorageCallbacks";
                case RemoteStorageAppSyncedClient_t: return "RemoteStorageAppSyncedClient_t";
                case RemoteStorageAppSyncedServer_t: return "RemoteStorageAppSyncedServer_t";
                case RemoteStorageAppSyncProgress_t: return "RemoteStorageAppSyncProgress_t";
                case RemoteStorageAppInfoLoaded_t: return "RemoteStorageAppInfoLoaded_t";
                case RemoteStorageAppSyncStatusCheck_t: return "RemoteStorageAppSyncStatusCheck_t";
                case RemoteStorageConflictResolution_t: return "RemoteStorageConflictResolution_t";
                case RemoteStorageFileShareResult_t: return "RemoteStorageFileShareResult_t";
                case RemoteStorageDownloadUGCResultold_t: return "RemoteStorageDownloadUGCResultold_t";
                case RemoteStoragePublishFileResult_t: return "RemoteStoragePublishFileResult_t";
                case RemoteStorageGetPublishedFileDetailsResultold_t: return "RemoteStorageGetPublishedFileDetailsResultold_t";
                case RemoteStorageDeletePublishedFileResult_t: return "RemoteStorageDeletePublishedFileResult_t";
                case RemoteStorageEnumerateUserPublishedFilesResult_t: return "RemoteStorageEnumerateUserPublishedFilesResult_t";
                case RemoteStorageSubscribePublishedFileResult_t: return "RemoteStorageSubscribePublishedFileResult_t";
                case RemoteStorageEnumerateUserSubscribedFilesResult_t: return "RemoteStorageEnumerateUserSubscribedFilesResult_t";
                case RemoteStorageUnsubscribePublishedFileResult_t: return "RemoteStorageUnsubscribePublishedFileResult_t";
                case RemoteStorageUpdatePublishedFileResult_t: return "RemoteStorageUpdatePublishedFileResult_t";
                case RemoteStorageDownloadUGCResult_t: return "RemoteStorageDownloadUGCResult_t";
                case RemoteStorageGetPublishedFileDetailsResult_t: return "RemoteStorageGetPublishedFileDetailsResult_t";
                case RemoteStorageEnumerateWorkshopFilesResult_t: return "RemoteStorageEnumerateWorkshopFilesResult_t";
                case RemoteStorageGetPublishedItemVoteDetailsResult_t: return "RemoteStorageGetPublishedItemVoteDetailsResult_t";
                case RemoteStoragePublishedFileSubscribed_t: return "RemoteStoragePublishedFileSubscribed_t";
                case RemoteStoragePublishedFileUnsubscribed_t: return "RemoteStoragePublishedFileUnsubscribed_t";
                case RemoteStoragePublishedFileDeleted_t: return "RemoteStoragePublishedFileDeleted_t";
                case RemoteStorageUpdateUserPublishedItemVoteResult_t: return "RemoteStorageUpdateUserPublishedItemVoteResult_t";
                case RemoteStorageUserVoteDetails_t: return "RemoteStorageUserVoteDetails_t";
                case RemoteStorageEnumerateUserSharedWorkshopFilesResult_t: return "RemoteStorageEnumerateUserSharedWorkshopFilesResult_t";
                case RemoteStorageSetUserPublishedFileActionResult_t: return "RemoteStorageSetUserPublishedFileActionResult_t";
                case RemoteStorageEnumeratePublishedFilesByUserActionResult_t: return "RemoteStorageEnumeratePublishedFilesByUserActionResult_t";
                case RemoteStoragePublishFileProgress_t: return "RemoteStoragePublishFileProgress_t";
                case RemoteStoragePublishedFileUpdated_t: return "RemoteStoragePublishedFileUpdated_t";
                case RemoteStorageFileWriteAsyncComplete_t: return "RemoteStorageFileWriteAsyncComplete_t";
                case RemoteStorageFileReadAsyncComplete_t: return "RemoteStorageFileReadAsyncComplete_t";
                case k_iClientDepotBuilderCallbacks: return "k_iClientDepotBuilderCallbacks";
                    //case k_iSteamUserItemsCallbacks: return "k_iSteamUserItemsCallbacks";
                case k_iSteamGameServerItemsCallbacks: return "k_iSteamGameServerItemsCallbacks";
                case k_iClientUtilsCallbacks: return "k_iClientUtilsCallbacks";
                case CellIDChanged_t: return "CellIDChanged_t";
                case k_iSteamGameCoordinatorCallbacks: return "k_iSteamGameCoordinatorCallbacks";
                case GCMessageAvailable_t: return "GCMessageAvailable_t";
                case GCMessageFailed_t: return "GCMessageFailed_t";
                    //case k_iSteamGameServerStatsCallbacks: return "k_iSteamGameServerStatsCallbacks";
                case GSStatsReceived_t: return "GSStatsReceived_t";
                case GSStatsStored_t: return "GSStatsStored_t";
                case k_iSteam2AsyncCallbacks: return "k_iSteam2AsyncCallbacks";
                case k_iSteamGameStatsCallbacks: return "k_iSteamGameStatsCallbacks";
                case GameStatsSessionIssued_t: return "GameStatsSessionIssued_t";
                case GameStatsSessionClosed_t: return "GameStatsSessionClosed_t";
                case k_iClientHTTPCallbacks: return "k_iClientHTTPCallbacks";
                case HTTPRequestCompleted_t: return "HTTPRequestCompleted_t";
                case HTTPRequestHeadersReceived_t: return "HTTPRequestHeadersReceived_t";
                case HTTPRequestDataReceived_t: return "HTTPRequestDataReceived_t";
                case k_iClientScreenshotsCallbacks: return "k_iClientScreenshotsCallbacks";
                case ScreenshotUploadProgress_t: return "ScreenshotUploadProgress_t";
                case ScreenshotWritten_t: return "ScreenshotWritten_t";
                case ScreenshotUploaded_t: return "ScreenshotUploaded_t";
                case ScreenshotBatchComplete_t: return "ScreenshotBatchComplete_t";
                case ScreenshotDeleted_t: return "ScreenshotDeleted_t";
                case ScreenshotTriggered_t: return "ScreenshotTriggered_t";
                case k_iSteamScreenshotsCallbacks: return "k_iSteamScreenshotsCallbacks";
                case ScreenshotReady_t: return "ScreenshotReady_t";
                case ScreenshotRequested_t: return "ScreenshotRequested_t";
                case k_iClientAudioCallbacks: return "k_iClientAudioCallbacks";
                case k_iClientUnifiedMessagesCallbacks: return "k_iClientUnifiedMessagesCallbacks";
                case SteamUnifiedMessagesSendMethodResult_t: return "SteamUnifiedMessagesSendMethodResult_t";
                case k_iSteamStreamLauncherCallbacks: return "k_iSteamStreamLauncherCallbacks";
                case k_iClientControllerCallbacks: return "k_iClientControllerCallbacks";
                case k_iSteamControllerCallbacks: return "k_iSteamControllerCallbacks";
                case k_iClientParentalSettingsCallbacks: return "k_iClientParentalSettingsCallbacks";
                case k_iClientDeviceAuthCallbacks: return "k_iClientDeviceAuthCallbacks";
                case k_iClientNetworkDeviceManagerCallbacks: return "k_iClientNetworkDeviceManagerCallbacks";
                case k_iClientMusicCallbacks: return "k_iClientMusicCallbacks";
                case k_iClientRemoteClientManagerCallbacks: return "k_iClientRemoteClientManagerCallbacks";
                case k_iClientUGCCallbacks: return "k_iClientUGCCallbacks";
                case SteamUGCQueryCompleted_t: return "SteamUGCQueryCompleted_t";
                case SteamUGCRequestUGCDetailsResult_t: return "SteamUGCRequestUGCDetailsResult_t";
                case CreateItemResult_t: return "CreateItemResult_t";
                case SubmitItemUpdateResult_t: return "SubmitItemUpdateResult_t";
                case ItemInstalled_t: return "ItemInstalled_t";
                case DownloadItemResult_t: return "DownloadItemResult_t";
                case UserFavoriteItemsListChanged_t: return "UserFavoriteItemsListChanged_t";
                case SetUserItemVoteResult_t: return "SetUserItemVoteResult_t";
                case GetUserItemVoteResult_t: return "GetUserItemVoteResult_t";
                case StartPlaytimeTrackingResult_t: return "StartPlaytimeTrackingResult_t";
                case StopPlaytimeTrackingResult_t: return "StopPlaytimeTrackingResult_t";
                case AddUGCDependencyResult_t: return "AddUGCDependencyResult_t";
                case RemoveUGCDependencyResult_t: return "RemoveUGCDependencyResult_t";
                case AddAppDependencyResult_t: return "AddAppDependencyResult_t";
                case RemoveAppDependencyResult_t: return "RemoveAppDependencyResult_t";
                case GetAppDependenciesResult_t: return "GetAppDependenciesResult_t";
                case DeleteItemResult_t: return "DeleteItemResult_t";
                case k_iSteamStreamClientCallbacks: return "k_iSteamStreamClientCallbacks";
                case k_IClientProductBuilderCallbacks: return "k_IClientProductBuilderCallbacks";
                case k_iClientShortcutsCallbacks: return "k_iClientShortcutsCallbacks";
                case k_iClientRemoteControlManagerCallbacks: return "k_iClientRemoteControlManagerCallbacks";
                case k_iSteamAppListCallbacks: return "k_iSteamAppListCallbacks";
                case SteamAppInstalled_t: return "SteamAppInstalled_t";
                case SteamAppUninstalled_t: return "SteamAppUninstalled_t";
                case k_iSteamMusicCallbacks: return "k_iSteamMusicCallbacks";
                case PlaybackStatusHasChanged_t: return "PlaybackStatusHasChanged_t";
                case VolumeHasChanged_t: return "VolumeHasChanged_t";
                case MusicPlayerWantsVolume_t: return "MusicPlayerWantsVolume_t";
                case MusicPlayerSelectsQueueEntry_t: return "MusicPlayerSelectsQueueEntry_t";
                case MusicPlayerSelectsPlaylistEntry_t: return "MusicPlayerSelectsPlaylistEntry_t";
                case k_iSteamMusicRemoteCallbacks: return "k_iSteamMusicRemoteCallbacks";
                case MusicPlayerRemoteWillActivate_t: return "MusicPlayerRemoteWillActivate_t";
                case MusicPlayerRemoteWillDeactivate_t: return "MusicPlayerRemoteWillDeactivate_t";
                case MusicPlayerRemoteToFront_t: return "MusicPlayerRemoteToFront_t";
                case MusicPlayerWillQuit_t: return "MusicPlayerWillQuit_t";
                case MusicPlayerWantsPlay_t: return "MusicPlayerWantsPlay_t";
                case MusicPlayerWantsPause_t: return "MusicPlayerWantsPause_t";
                case MusicPlayerWantsPlayPrevious_t: return "MusicPlayerWantsPlayPrevious_t";
                case MusicPlayerWantsPlayNext_t: return "MusicPlayerWantsPlayNext_t";
                case MusicPlayerWantsShuffled_t: return "MusicPlayerWantsShuffled_t";
                case MusicPlayerWantsLooped_t: return "MusicPlayerWantsLooped_t";
                case MusicPlayerWantsPlayingRepeatStatus_t: return "MusicPlayerWantsPlayingRepeatStatus_t";
                case k_iClientVRCallbacks: return "k_iClientVRCallbacks";
                    //case k_iClientGameNotificationCallbacks: return "k_iClientGameNotificationCallbacks";
                case k_iClientReservedCallbacks: return "k_iClientReservedCallbacks";
                    //case k_iSteamGameNotificationCallbacks: return "k_iSteamGameNotificationCallbacks";
                case k_iSteamReservedCallbacks: return "k_iSteamReservedCallbacks";
                case k_iSteamHTMLSurfaceCallbacks: return "k_iSteamHTMLSurfaceCallbacks";
                case HTML_BrowserReady_t: return "HTML_BrowserReady_t";
                case HTML_NeedsPaint_t: return "HTML_NeedsPaint_t";
                case HTML_StartRequest_t: return "HTML_StartRequest_t";
                case HTML_CloseBrowser_t: return "HTML_CloseBrowser_t";
                case HTML_URLChanged_t: return "HTML_URLChanged_t";
                case HTML_FinishedRequest_t: return "HTML_FinishedRequest_t";
                case HTML_OpenLinkInNewTab_t: return "HTML_OpenLinkInNewTab_t";
                case HTML_ChangedTitle_t: return "HTML_ChangedTitle_t";
                case HTML_SearchResults_t: return "HTML_SearchResults_t";
                case HTML_CanGoBackAndForward_t: return "HTML_CanGoBackAndForward_t";
                case HTML_HorizontalScroll_t: return "HTML_HorizontalScroll_t";
                case HTML_VerticalScroll_t: return "HTML_VerticalScroll_t";
                case HTML_LinkAtPosition_t: return "HTML_LinkAtPosition_t";
                case HTML_JSAlert_t: return "HTML_JSAlert_t";
                case HTML_JSConfirm_t: return "HTML_JSConfirm_t";
                case HTML_FileOpenDialog_t: return "HTML_FileOpenDialog_t";
                case HTML_NewWindow_t: return "HTML_NewWindow_t";
                case HTML_SetCursor_t: return "HTML_SetCursor_t";
                case HTML_StatusText_t: return "HTML_StatusText_t";
                case HTML_ShowToolTip_t: return "HTML_ShowToolTip_t";
                case HTML_UpdateToolTip_t: return "HTML_UpdateToolTip_t";
                case HTML_HideToolTip_t: return "HTML_HideToolTip_t";
                case HTML_BrowserRestarted_t: return "HTML_BrowserRestarted_t";
                case k_iClientVideoCallbacks: return "k_iClientVideoCallbacks";
                case BroadcastUploadStart_t: return "BroadcastUploadStart_t";
                case BroadcastUploadStop_t: return "BroadcastUploadStop_t";
                case GetVideoURLResult_t: return "GetVideoURLResult_t";
                case GetOPFSettingsResult_t: return "GetOPFSettingsResult_t";
                    //case k_iClientInventoryCallbacks: return "k_iClientInventoryCallbacks";
                case SteamInventoryResultReady_t: return "SteamInventoryResultReady_t";
                case SteamInventoryFullUpdate_t: return "SteamInventoryFullUpdate_t";
                case SteamInventoryDefinitionUpdate_t: return "SteamInventoryDefinitionUpdate_t";
                case SteamInventoryEligiblePromoItemDefIDs_t: return "SteamInventoryEligiblePromoItemDefIDs_t";
                case SteamInventoryStartPurchaseResult_t: return "SteamInventoryStartPurchaseResult_t";
                case SteamInventoryRequestPricesResult_t: return "SteamInventoryRequestPricesResult_t";
                case k_iClientBluetoothManagerCallbacks: return "k_iClientBluetoothManagerCallbacks";
                case k_iClientSharedConnectionCallbacks: return "k_iClientSharedConnectionCallbacks";
                case k_ISteamParentalSettingsCallbacks: return "k_ISteamParentalSettingsCallbacks";
                case SteamParentalSettingsChanged_t: return "SteamParentalSettingsChanged_t";
                case k_iClientShaderCallbacks: return "k_iClientShaderCallbacks";
                case k_iSteamGameSearchCallbacks: return "k_iSteamGameSearchCallbacks";
                case SearchForGameProgressCallback_t: return "SearchForGameProgressCallback_t";
                case SearchForGameResultCallback_t: return "SearchForGameResultCallback_t";
                case RequestPlayersForGameProgressCallback_t: return "RequestPlayersForGameProgressCallback_t";
                case RequestPlayersForGameResultCallback_t: return "RequestPlayersForGameResultCallback_t";
                case RequestPlayersForGameFinalResultCallback_t: return "RequestPlayersForGameFinalResultCallback_t";
                case SubmitPlayerResultResultCallback_t: return "SubmitPlayerResultResultCallback_t";
                case EndGameResultCallback_t: return "EndGameResultCallback_t";
                case k_iSteamPartiesCallbacks: return "k_iSteamPartiesCallbacks";
                case JoinPartyCallback_t: return "JoinPartyCallback_t";
                case CreateBeaconCallback_t: return "CreateBeaconCallback_t";
                case ReservationNotificationCallback_t: return "ReservationNotificationCallback_t";
                case ChangeNumOpenSlotsCallback_t: return "ChangeNumOpenSlotsCallback_t";
                case AvailableBeaconLocationsUpdated_t: return "AvailableBeaconLocationsUpdated_t";
                case ActiveBeaconsUpdated_t: return "ActiveBeaconsUpdated_t";
                case k_iClientPartiesCallbacks: return "k_iClientPartiesCallbacks";
                case k_iSteamSTARCallbacks: return "k_iSteamSTARCallbacks";
                case k_iClientSTARCallbacks: return "k_iClientSTARCallbacks";
                case k_iSteamRemotePlayCallbacks: return "k_iSteamRemotePlayCallbacks";
                case SteamRemotePlaySessionConnected_t: return "SteamRemotePlaySessionConnected_t";
                case SteamRemotePlaySessionDisconnected_t: return "SteamRemotePlaySessionDisconnected_t";
                case k_iClientCompatCallbacks: return "k_iClientCompatCallbacks";
                case k_iSteamChatCallbacks: return "k_iSteamChatCallbacks";
                default: return "unknown";
            }
        }
    }
    #pragma endregion
}
