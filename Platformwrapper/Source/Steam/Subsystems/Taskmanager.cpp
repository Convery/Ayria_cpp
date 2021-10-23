/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#include <Steam.hpp>

namespace Steam::Tasks
{
    using CallID_t = uint64_t;
    using Result_t = struct { CallID_t RequestID; void *Databuffer; };
    struct Callback_t
    {
        virtual void Execute(void *Databuffer) = 0;
        virtual void Execute(void *Databuffer, bool Error, CallID_t CallID) = 0;
        virtual int32_t Callbacksize() = 0;

        // Padding intended.
        uint8_t Flags;
        int32_t Type;
    };

    static std::queue<std::pair<int32_t, Result_t>> Results;
    static std::atomic<CallID_t> Callbackcount{ 42 };
    static Hashmap<int32_t, Callback_t *> Callbacks;

    // Forward declaration, will be optimized out in release.
    std::string Taskname(int32_t Callbacktype);

    // Async requests to the backend.
    void Completerequest(CallID_t RequestID, int32_t Callbacktype, void *Databuffer)
    {
        Results.push({ Callbacktype, { RequestID, Databuffer } });
    }
    void Registercallback(void *Callback, int32_t Callbacktype)
    {
        // Special case, callback provides the type.
        if (Callbacktype == -1) Callbacktype = ((Callback_t *)Callback)->Type;

        // Register the callback handler for later use.
        auto &Entry = Callbacks[Callbacktype];
        Entry = (Callback_t *)Callback;
        Entry->Type = Callbacktype;

        Debugprint(va("Registering callback \"%s\"", Taskname(Callbacktype).c_str()));
    }
    CallID_t Createrequest()
    {
        return Callbackcount++;
    }
    void Runcallbacks()
    {
        while (!Results.empty())
        {
            const auto Entry = Results.front(); Results.pop();
            auto Callback = Callbacks.find(Entry.first);

            // Prefer the longer method as most implementations just discard the extra data.
            if(Callback != Callbacks.end()) Callback->second->Execute(Entry.second.Databuffer, false, Entry.second.RequestID);

            // Let's not leak (although technically UB).
            delete Entry.second.Databuffer;
        }
    }

    // Will be removed by the linker in release mode.
    std::string Taskname(int32_t Callbacktype)
    {
        // Named lookup, added as we go.
        #define Case(x, y) case x: return y;
        switch (Callbacktype)
        {
            Case(101, "SteamServersConnected");
            Case(102, "SteamServerConnectFailure");
            Case(103, "SteamServersDisconnected");
            Case(104, "BeginLogonRetry");
            Case(113, "ClientGameServerDeny");
            Case(114, "PrimaryChatDestinationSetOld");
            Case(115, "GSPolicyResponse");
            Case(117, "IPCFailure");
            Case(125, "LicensesUpdated");
            Case(130, "AppLifetimeNotice");
            Case(141, "DRMSDKFileTransferResult");
            Case(143, "ValidateAuthTicketResponse");
            Case(152, "MicroTxnAuthorizationResponse");
            Case(154, "EncryptedAppTicketResponse");
            Case(163, "GetAuthSessionTicketResponse");
            Case(164, "GameWebCallback");
            Case(165, "StoreAuthURLResponse");
            Case(166, "MarketEligibilityResponse");
            Case(167, "DurationControl");

            Case(201, "GSClientApprove");
            Case(202, "GSClientDeny");
            Case(203, "GSClientKick");
            Case(204, "GSClientSteam2Deny");
            Case(205, "GSClientSteam2Accept");
            Case(206, "GSClientAchievementStatus");
            Case(207, "GSGameplayStats");
            Case(208, "GSClientGroupStatus");
            Case(209, "GSReputation");
            Case(210, "AssociateWithClanResult");
            Case(211, "ComputeNewPlayerCompatibilityResult");

            Case(304, "PersonaStateChange");
            Case(331, "GameOverlayActivated");
            Case(332, "GameServerChangeRequested");
            Case(333, "GameLobbyJoinRequested");
            Case(334, "AvatarImageLoaded");
            Case(335, "ClanOfficerListResponse");
            Case(336, "FriendRichPresenceUpdate");
            Case(337, "GameRichPresenceJoinRequested");
            Case(338, "GameConnectedClanChatMsg");
            Case(339, "GameConnectedChatJoin");
            Case(340, "GameConnectedChatLeave");
            Case(341, "DownloadClanActivityCountsResult");
            Case(342, "JoinClanChatRoomCompletionResult");
            Case(343, "GameConnectedFriendChatMsg");
            Case(344, "FriendsGetFollowerCount");
            Case(345, "FriendsIsFollowing");
            Case(346, "FriendsEnumerateFollowingList");
            Case(347, "SetPersonaNameResponse");
            Case(348, "UnreadChatMessagesChanged");

            Case(501, "FavoritesListChangedOld");
            Case(502, "FavoritesListChanged");
            Case(503, "LobbyInvite");
            Case(504, "LobbyEnter");
            Case(505, "LobbyDataUpdate");
            Case(506, "LobbyChatUpdate");
            Case(507, "LobbyChatMsg");
            Case(508, "LobbyAdminChange");
            Case(509, "LobbyGameCreated");
            Case(510, "LobbyMatchList");
            Case(511, "LobbyClosing");
            Case(512, "LobbyKicked");
            Case(513, "LobbyCreated");
            Case(514, "RequestFriendsLobbiesResponse");
            Case(515, "PSNGameBootInviteResult");
            Case(516, "FavoritesListAccountsUpdated");

            Case(701, "IPCountry");
            Case(702, "LowBatteryPower");
            Case(703, "SteamAPICallCompleted");
            Case(704, "SteamShutdown");
            Case(705, "CheckFileSignature");
            Case(706, "NetStartDialogFinished");
            Case(707, "NetStartDialogUnloaded");
            Case(708, "PS3SystemMenuClosed");
            Case(709, "PS3NPMessageSelected");
            Case(710, "PS3KeyboardDialogFinished");
            Case(711, "SteamConfigStoreChanged");
            Case(712, "PS3PSNStatusChange");
            Case(713, "SteamUtils_Reserved");
            Case(714, "GamepadTextInputDismissed");
            Case(715, "SteamUtils_Reserved");

            Case(805, "FriendChatMsg");
            Case(810, "ChatRoomMsg");
            Case(811, "ChatRoomDlgClose");
            Case(812, "ChatRoomClosing");
            Case(819, "ClanInfoChanged");
            Case(836, "FriendsMenuChange");

            Case(903, "PrimaryChatDestinationSet");
            Case(963, "FriendMessageHistoryChatLog");

            Case(1001, "AppDataChanged");
            Case(1002, "RequestAppCallbacksComplete");
            Case(1003, "AppInfoUpdateComplete");
            Case(1004, "AppEventTriggered");
            Case(1005, "DlcInstalled");
            Case(1006, "AppEventStateChange");
            Case(1008, "RegisterActivationCodeResponse");
            Case(1014, "NewUrlLaunchParameters");
            Case(1021, "AppProofOfPurchaseKeyResponse");
            Case(1023, "FileDetailsResult");

            Case(1101, "UserStatsReceived");
            Case(1102, "UserStatsStored");
            Case(1103, "UserAchievementStored");
            Case(1104, "LeaderboardFindResult");
            Case(1105, "LeaderboardScoresDownloaded");
            Case(1106, "LeaderboardScoreUploaded");
            Case(1107, "NumberOfCurrentPlayers");
            Case(1108, "UserStatsUnloaded");
            Case(1109, "UserAchievementIconFetched");
            Case(1110, "GlobalAchievementPercentagesReady");
            Case(1111, "LeaderboardUGCSet");
            Case(1112, "GlobalStatsReceived");

            Case(1201, "SocketStatusChanged");
            Case(1202, "P2PSessionRequest");
            Case(1203, "P2PSessionConnectFail");
            Case(1221, "SteamNetConnectionStatusChangedCallback");
            Case(1222, "SteamNetAuthenticationStatus");
            Case(1281, "SteamRelayNetworkStatus");

            Case(1301, "RemoteStorageAppSyncedClient");
            Case(1302, "RemoteStorageAppSyncedServer");
            Case(1303, "RemoteStorageAppSyncProgress");
            Case(1305, "RemoteStorageAppSyncStatusCheck");
            Case(1307, "RemoteStorageFileShareResult");
            Case(1309, "RemoteStoragePublishFileResult");
            Case(1311, "RemoteStorageDeletePublishedFileResult");
            Case(1312, "RemoteStorageEnumerateUserPublishedFilesResult");
            Case(1313, "RemoteStorageSubscribePublishedFileResult");
            Case(1314, "RemoteStorageEnumerateUserSubscribedFilesResult");
            Case(1315, "RemoteStorageUnsubscribePublishedFileResult");
            Case(1316, "RemoteStorageUpdatePublishedFileResult");
            Case(1317, "RemoteStorageDownloadUGCResult");
            Case(1318, "RemoteStorageGetPublishedFileDetailsResult");
            Case(1319, "RemoteStorageEnumerateWorkshopFilesResult");
            Case(1320, "RemoteStorageGetPublishedItemVoteDetailsResult");
            Case(1321, "RemoteStoragePublishedFileSubscribed");
            Case(1322, "RemoteStoragePublishedFileUnsubscribed");
            Case(1323, "RemoteStoragePublishedFileDeleted");
            Case(1324, "RemoteStorageUpdateUserPublishedItemVoteResult");
            Case(1325, "RemoteStorageUserVoteDetails");
            Case(1326, "RemoteStorageEnumerateUserSharedWorkshopFilesResult");
            Case(1327, "RemoteStorageSetUserPublishedFileActionResult");
            Case(1328, "RemoteStorageEnumeratePublishedFilesByUserActionResult");
            Case(1329, "RemoteStoragePublishFileProgress");
            Case(1330, "RemoteStoragePublishedFileUpdated");
            Case(1331, "RemoteStorageFileWriteAsyncComplete");
            Case(1332, "RemoteStorageFileReadAsyncComplete");

            Case(1800, "GSStatsReceived");
            Case(1801, "GSStatsStored");

            Case(2101, "HTTPRequestCompleted");
            Case(2102, "HTTPRequestHeadersReceived");
            Case(2103, "HTTPRequestDataReceived");
            Case(2301, "ScreenshotReady");
            Case(2302, "ScreenshotRequested");
            Case(3401, "SteamUGCQueryCompleted");
            Case(3402, "SteamUGCRequestUGCDetailsResult");
            Case(3403, "CreateItemResult");
            Case(3404, "SubmitItemUpdateResult");
            Case(3405, "ItemInstalled");
            Case(3406, "DownloadItemResult");
            Case(3407, "UserFavoriteItemsListChanged");
            Case(3408, "SetUserItemVoteResult");
            Case(3409, "GetUserItemVoteResult");
            Case(3410, "StartPlaytimeTrackingResult");
            Case(3411, "StopPlaytimeTrackingResult");
            Case(3412, "AddUGCDependencyResult");
            Case(3413, "RemoveUGCDependencyResult");
            Case(3414, "AddAppDependencyResult");
            Case(3415, "RemoveAppDependencyResult");
            Case(3416, "GetAppDependenciesResult");
            Case(3417, "DeleteItemResult");
            Case(3901, "SteamAppInstalled");
            Case(3902, "SteamAppUninstalled");
            Case(4001, "PlaybackStatusHasChanged");
            Case(4002, "VolumeHasChanged");
            Case(4011, "MusicPlayerWantsVolume");
            Case(4012, "MusicPlayerSelectsQueueEntry");
            Case(4013, "MusicPlayerSelectsPlaylistEntry");
            Case(4101, "MusicPlayerRemoteWillActivate");
            Case(4102, "MusicPlayerRemoteWillDeactivate");
            Case(4103, "MusicPlayerRemoteToFront");
            Case(4104, "MusicPlayerWillQuit");
            Case(4105, "MusicPlayerWantsPlay");
            Case(4106, "MusicPlayerWantsPause");
            Case(4107, "MusicPlayerWantsPlayPrevious");
            Case(4108, "MusicPlayerWantsPlayNext");
            Case(4109, "MusicPlayerWantsShuffled");
            Case(4110, "MusicPlayerWantsLooped");
            Case(4114, "MusicPlayerWantsPlayingRepeatStatus");
            Case(4501, "HTML_BrowserReady");
            Case(4502, "HTML_NeedsPaint");
            Case(4503, "HTML_StartRequest");
            Case(4504, "HTML_CloseBrowser");
            Case(4505, "HTML_URLChanged");
            Case(4506, "HTML_FinishedRequest");
            Case(4507, "HTML_OpenLinkInNewTab");
            Case(4508, "HTML_ChangedTitle");
            Case(4509, "HTML_SearchResults");
            Case(4510, "HTML_CanGoBackAndForward");
            Case(4511, "HTML_HorizontalScroll");
            Case(4512, "HTML_VerticalScroll");
            Case(4513, "HTML_LinkAtPosition");
            Case(4514, "HTML_JSAlert");
            Case(4515, "HTML_JSConfirm");
            Case(4516, "HTML_FileOpenDialog");
            Case(4521, "HTML_NewWindow");
            Case(4522, "HTML_SetCursor");
            Case(4523, "HTML_StatusText");
            Case(4524, "HTML_ShowToolTip");
            Case(4525, "HTML_UpdateToolTip");
            Case(4526, "HTML_HideToolTip");
            Case(4527, "HTML_BrowserRestarted");
            Case(4604, "BroadcastUploadStart");
            Case(4605, "BroadcastUploadStop");
            Case(4611, "GetVideoURLResult");
            Case(4624, "GetOPFSettingsResult");
            Case(4700, "SteamInventoryResultReady");
            Case(4701, "SteamInventoryFullUpdate");
            Case(4702, "SteamInventoryDefinitionUpdate");
            Case(4703, "SteamInventoryEligiblePromoItemDefIDs");
            Case(4704, "SteamInventoryStartPurchaseResult");
            Case(4705, "SteamInventoryRequestPricesResult");
            Case(5001, "SteamParentalSettingsChanged");
            Case(5201, "SearchForGameProgressCallback");
            Case(5202, "SearchForGameResultCallback");
            Case(5211, "RequestPlayersForGameProgressCallback");
            Case(5212, "RequestPlayersForGameResultCallback");
            Case(5213, "RequestPlayersForGameFinalResultCallback");
            Case(5214, "SubmitPlayerResultResultCallback");
            Case(5215, "EndGameResultCallback");
            Case(5301, "JoinPartyCallback");
            Case(5302, "CreateBeaconCallback");
            Case(5303, "ReservationNotificationCallback");
            Case(5304, "ChangeNumOpenSlotsCallback");
            Case(5305, "AvailableBeaconLocationsUpdated");
            Case(5306, "ActiveBeaconsUpdated");

            Case(5701, "SteamRemotePlaySessionConnected");
            Case(5702, "SteamRemotePlaySessionDisconnected");
        }
        #undef Case

        // Offset lookup, should never fail.
        #define Case(x, y) case x: return va("%s + %u", y, Callbacktype % 100);
        switch ((Callbacktype / 100) * 100)
        {
            Case(100, "k_iSteamUserCallbacks");
            Case(200, "k_iSteamGameServerCallbacks");
            Case(300, "k_iSteamFriendsCallbacks");
            Case(400, "k_iSteamBillingCallbacks");
            Case(500, "k_iSteamMatchmakingCallbacks");
            Case(600, "k_iSteamContentServerCallbacks");
            Case(700, "k_iSteamUtilsCallbacks");
            Case(800, "k_iClientFriendsCallbacks");
            Case(900, "k_iClientUserCallbacks");
            Case(1000, "k_iSteamAppsCallbacks");
            Case(1100, "k_iSteamUserStatsCallbacks");
            Case(1200, "k_iSteamNetworkingCallbacks");
            Case(1220, "k_iSteamNetworkingSocketsCallbacks");
            Case(1250, "k_iSteamNetworkingMessagesCallbacks");
            Case(1280, "k_iSteamNetworkingUtilsCallbacks");
            Case(1300, "k_iClientRemoteStorageCallbacks");
            Case(1400, "k_iClientDepotBuilderCallbacks");
            Case(1500, "k_iSteamGameServerItemsCallbacks");
            Case(1600, "k_iClientUtilsCallbacks");
            Case(1700, "k_iSteamGameCoordinatorCallbacks");
            Case(1800, "k_iSteamGameServerStatsCallbacks");
            Case(1900, "k_iSteam2AsyncCallbacks");
            Case(2000, "k_iSteamGameStatsCallbacks");
            Case(2100, "k_iClientHTTPCallbacks");
            Case(2200, "k_iClientScreenshotsCallbacks");
            Case(2300, "k_iSteamScreenshotsCallbacks");
            Case(2400, "k_iClientAudioCallbacks");
            Case(2500, "k_iClientUnifiedMessagesCallbacks");
            Case(2600, "k_iSteamStreamLauncherCallbacks");
            Case(2700, "k_iClientControllerCallbacks");
            Case(2800, "k_iSteamControllerCallbacks");
            Case(2900, "k_iClientParentalSettingsCallbacks");
            Case(3000, "k_iClientDeviceAuthCallbacks");
            Case(3100, "k_iClientNetworkDeviceManagerCallbacks");
            Case(3200, "k_iClientMusicCallbacks");
            Case(3300, "k_iClientRemoteClientManagerCallbacks");
            Case(3400, "k_iClientUGCCallbacks");
            Case(3500, "k_iSteamStreamClientCallbacks");
            Case(3600, "k_IClientProductBuilderCallbacks");
            Case(3700, "k_iClientShortcutsCallbacks");
            Case(3800, "k_iClientRemoteControlManagerCallbacks");
            Case(3900, "k_iSteamAppListCallbacks");
            Case(4000, "k_iSteamMusicCallbacks");
            Case(4100, "k_iSteamMusicRemoteCallbacks");
            Case(4200, "k_iClientVRCallbacks");
            Case(4300, "k_iClientGameNotificationCallbacks");
            Case(4400, "k_iSteamGameNotificationCallbacks");
            Case(4500, "k_iSteamHTMLSurfaceCallbacks");
            Case(4600, "k_iClientVideoCallbacks");
            Case(4700, "k_iClientInventoryCallbacks");
            Case(4800, "k_iClientBluetoothManagerCallbacks");
            Case(4900, "k_iClientSharedConnectionCallbacks");
            Case(5000, "k_ISteamParentalSettingsCallbacks");
            Case(5100, "k_iClientShaderCallbacks");
            Case(5200, "k_iSteamGameSearchCallbacks");
            Case(5300, "k_iSteamPartiesCallbacks");
            Case(5400, "k_iClientPartiesCallbacks");
            Case(5500, "k_iSteamSTARCallbacks");
            Case(5600, "k_iClientSTARCallbacks");
            Case(5700, "k_iSteamRemotePlayCallbacks");
            Case(5800, "k_iClientCompatCallbacks");
            Case(5900, "k_iSteamChatCallbacks");
            Case(6000, "k_iClientNetworkingUtilsCallbacks");
            Case(6100, "k_iClientSystemManagerCallbacks");
            Case(6200, "k_iClientStorageDeviceManagerCallbacks");
        }
        #undef Case

        // Just in case.
        return va("%d", Callbacktype);
    }
}
