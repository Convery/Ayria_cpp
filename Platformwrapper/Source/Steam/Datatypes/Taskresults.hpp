/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#pragma once
#include <Steam.hpp>
#include "Resultcodes.hpp"

// Asynchronous task-results.
namespace Steam::Tasks
{
    #pragma region Resultstructs
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
    struct RemoteStorageGetPublishedFileDetailsResult_t { EResult m_eResult; uint64_t m_nPublishedFileId; uint32_t m_nCreatorAppID; uint32_t m_nConsumerAppID; char m_rgchTitle[129]; char m_rgchDescription[8000]; uint64_t m_hFile; uint64_t m_hPreviewFile; uint64_t m_ulSteamIDOwner; uint32_t m_rtimeCreated; uint32_t m_rtimeUpdated; uint32_t m_eVisibility; bool m_bBanned; char m_rgchTags[1025]; bool m_bTagsTruncated; char m_pchFileName[260]; int32_t m_nFileSize; int32_t m_nPreviewFileSize; char m_rgchURL[256]; uint32_t m_eFileType; bool m_bAcceptedForUse; };
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

    #pragma region Resulttypes
    namespace Types
    {
        enum class ECallbackType : int32_t
        {
            // Base callbacks.
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
            k_iSteamNetworkingSocketsCallbacks = 1220,
            k_iSteamNetworkingMessagesCallbacks = 1250,
            k_iSteamNetworkingUtilsCallbacks = 1280,
            k_iClientRemoteStorageCallbacks = 1300,
            k_iClientDepotBuilderCallbacks = 1400,
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
            k_iClientGameNotificationCallbacks = 4300,
            k_iSteamGameNotificationCallbacks = 4400,
            k_iSteamHTMLSurfaceCallbacks = 4500,
            k_iClientVideoCallbacks = 4600,
            k_iClientInventoryCallbacks = 4700,
            k_iClientBluetoothManagerCallbacks = 4800,
            k_iClientSharedConnectionCallbacks = 4900,
            k_ISteamParentalSettingsCallbacks = 5000,
            k_iClientShaderCallbacks = 5100,
            k_iSteamGameSearchCallbacks = 5200,
            k_iSteamPartiesCallbacks = 5300,
            k_iClientPartiesCallbacks = 5400,
            k_iSteamSTARCallbacks = 5500,
            k_iClientSTARCallbacks = 5600,
            k_iSteamRemotePlayCallbacks = 5700,
            k_iClientCompatCallbacks = 5800,
            k_iSteamChatCallbacks = 5900,


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

            FinalPriceMsg_t = k_iSteamBillingCallbacks + 1,
            PurchaseMsg_t = k_iSteamBillingCallbacks + 2,

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

            CSClientApprove_t = k_iSteamContentServerCallbacks + 1,
            CSClientDeny_t = k_iSteamContentServerCallbacks + 2,

            IPCountry_t = k_iSteamUtilsCallbacks + 1,
            LowBatteryPower_t = k_iSteamUtilsCallbacks + 2,
            SteamAPICallCompleted_t = k_iSteamUtilsCallbacks + 3,
            SteamShutdown_t = k_iSteamUtilsCallbacks + 4,
            CheckFileSignature_t = k_iSteamUtilsCallbacks + 5,
            SteamConfigStoreChanged_t = k_iSteamUtilsCallbacks + 11,
            GamepadTextInputDismissed_t = k_iSteamUtilsCallbacks + 14,

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

            SocketStatusCallback_t = k_iSteamNetworkingCallbacks + 1,
            P2PSessionRequest_t = k_iSteamNetworkingCallbacks + 2,
            P2PSessionConnectFail_t = k_iSteamNetworkingCallbacks + 3,
            SteamNetworkingSocketsConfigUpdated_t = k_iSteamNetworkingCallbacks + 95,
            SteamNetworkingSocketsCert_t = k_iSteamNetworkingCallbacks + 96,
            SteamNetworkingSocketsRecvP2PFailure_t = k_iSteamNetworkingCallbacks + 97,
            SteamNetworkingSocketsRecvP2PRendezvous_t = k_iSteamNetworkingCallbacks + 98,

            SteamNetConnectionStatusChangedCallback_t = k_iSteamNetworkingSocketsCallbacks + 1,
            SteamNetAuthenticationStatus_t = k_iSteamNetworkingSocketsCallbacks + 2,

            SteamNetworkingMessagesSessionRequest_t = k_iSteamNetworkingMessagesCallbacks + 1,
            SteamNetworkingMessagesSessionFailed_t = k_iSteamNetworkingMessagesCallbacks + 2,

            SteamRelayNetworkStatus_t = k_iSteamNetworkingUtilsCallbacks + 1,

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

            CellIDChanged_t = k_iClientUtilsCallbacks + 3,

            GCMessageAvailable_t = k_iSteamGameCoordinatorCallbacks + 1,
            GCMessageFailed_t = k_iSteamGameCoordinatorCallbacks + 2,

            GSStatsReceived_t = k_iSteamGameServerStatsCallbacks + 0,
            GSStatsStored_t = k_iSteamGameServerStatsCallbacks + 1,

            GameStatsSessionIssued_t = k_iSteamGameStatsCallbacks + 1,
            GameStatsSessionClosed_t = k_iSteamGameStatsCallbacks + 2,

            HTTPRequestCompleted_t = k_iClientHTTPCallbacks + 1,
            HTTPRequestHeadersReceived_t = k_iClientHTTPCallbacks + 2,
            HTTPRequestDataReceived_t = k_iClientHTTPCallbacks + 3,

            ScreenshotUploadProgress_t = k_iClientScreenshotsCallbacks + 1,
            ScreenshotWritten_t = k_iClientScreenshotsCallbacks + 2,
            ScreenshotUploaded_t = k_iClientScreenshotsCallbacks + 3,
            ScreenshotBatchComplete_t = k_iClientScreenshotsCallbacks + 4,
            ScreenshotDeleted_t = k_iClientScreenshotsCallbacks + 5,
            ScreenshotTriggered_t = k_iClientScreenshotsCallbacks + 6,

            ScreenshotReady_t = k_iSteamScreenshotsCallbacks + 1,
            ScreenshotRequested_t = k_iSteamScreenshotsCallbacks + 2,

            SteamUnifiedMessagesSendMethodResult_t = k_iClientUnifiedMessagesCallbacks + 1,

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

            SteamAppInstalled_t = k_iSteamAppListCallbacks + 1,
            SteamAppUninstalled_t = k_iSteamAppListCallbacks + 2,

            PlaybackStatusHasChanged_t = k_iSteamMusicCallbacks + 1,
            VolumeHasChanged_t = k_iSteamMusicCallbacks + 2,
            MusicPlayerWantsVolume_t = k_iSteamMusicCallbacks + 11,
            MusicPlayerSelectsQueueEntry_t = k_iSteamMusicCallbacks + 12,
            MusicPlayerSelectsPlaylistEntry_t = k_iSteamMusicCallbacks + 13,

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

            BroadcastUploadStart_t = k_iClientVideoCallbacks + 4,
            BroadcastUploadStop_t = k_iClientVideoCallbacks + 5,
            GetVideoURLResult_t = k_iClientVideoCallbacks + 11,
            GetOPFSettingsResult_t = k_iClientVideoCallbacks + 24,

            SteamInventoryResultReady_t = k_iClientInventoryCallbacks + 0,
            SteamInventoryFullUpdate_t = k_iClientInventoryCallbacks + 1,
            SteamInventoryDefinitionUpdate_t = k_iClientInventoryCallbacks + 2,
            SteamInventoryEligiblePromoItemDefIDs_t = k_iClientInventoryCallbacks + 3,
            SteamInventoryStartPurchaseResult_t = k_iClientInventoryCallbacks + 4,
            SteamInventoryRequestPricesResult_t = k_iClientInventoryCallbacks + 5,

            SteamParentalSettingsChanged_t = k_ISteamParentalSettingsCallbacks + 1,

            SearchForGameProgressCallback_t = k_iSteamGameSearchCallbacks + 1,
            SearchForGameResultCallback_t = k_iSteamGameSearchCallbacks + 2,
            RequestPlayersForGameProgressCallback_t = k_iSteamGameSearchCallbacks + 11,
            RequestPlayersForGameResultCallback_t = k_iSteamGameSearchCallbacks + 12,
            RequestPlayersForGameFinalResultCallback_t = k_iSteamGameSearchCallbacks + 13,
            SubmitPlayerResultResultCallback_t = k_iSteamGameSearchCallbacks + 14,
            EndGameResultCallback_t = k_iSteamGameSearchCallbacks + 15,

            JoinPartyCallback_t = k_iSteamPartiesCallbacks + 1,
            CreateBeaconCallback_t = k_iSteamPartiesCallbacks + 2,
            ReservationNotificationCallback_t = k_iSteamPartiesCallbacks + 3,
            ChangeNumOpenSlotsCallback_t = k_iSteamPartiesCallbacks + 4,
            AvailableBeaconLocationsUpdated_t = k_iSteamPartiesCallbacks + 5,
            ActiveBeaconsUpdated_t = k_iSteamPartiesCallbacks + 6,

            SteamRemotePlaySessionConnected_t = k_iSteamRemotePlayCallbacks + 1,
            SteamRemotePlaySessionDisconnected_t = k_iSteamRemotePlayCallbacks + 2,
        };

        // Readble representation for debugging.
        namespace { static std::string TMP; }
        constexpr const char *toString(ECallbackType Code)
        {
            if constexpr (Build::isDebug)
            {
                switch (Code)
                {
                    case ECallbackType::k_iSteamUserCallbacks: return "k_iSteamUserCallbacks";
                    case ECallbackType::SteamServersConnected_t: return "SteamServersConnected_t";
                }
            }

            TMP = std::format("{}", (int32_t)Code);
            return TMP.c_str();
        }
    }
    #pragma endregion
}
