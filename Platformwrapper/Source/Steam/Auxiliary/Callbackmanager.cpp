/*
    Initial author: Convery (tcn@ayria.se)
    Started: 25-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    namespace Callbacks
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

        std::unordered_map<int32_t, Callback_t *> Callbacks;
        std::queue<std::pair<int32_t, Result_t>> Results;
        std::atomic<CallID_t> Callbackcount{ 42 };

        // Forward declaration, will be optimized out in release.
        std::string Callbackname(int32_t Callbacktype);

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

            Debugprint(va("Registering callback \"%s\"", Callbackname(Callbacktype).c_str()));
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

                // Let's not leak.
                delete Entry.second.Databuffer;
            }
        }

        // Will be removed by the linker in release mode.
        std::string Callbackname(int32_t Callbacktype)
        {
            // Named lookup, added as we go.
            #define Case(x, y) case x: return y;
            switch (Callbacktype)
            {
                Case(100, "Base::Steamuser");
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

                Case(200, "Base::Steamgameserver");
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

                Case(300, "Base::Steamfriends");
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

                Case(400, "Base::Steambilling");

                Case(500, "Base::Steammatchmaking");
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

                Case(600, "Base::Steamcontent");

                Case(700, "Base::Steamutilities");
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

                Case(800, "Base::Clientfriends");
                Case(805, "FriendChatMsg");
                Case(810, "ChatRoomMsg");
                Case(811, "ChatRoomDlgClose");
                Case(812, "ChatRoomClosing");
                Case(819, "ClanInfoChanged");
                Case(836, "FriendsMenuChange");

                Case(900, "Base::Clientuser");
                Case(903, "PrimaryChatDestinationSet");
                Case(963, "FriendMessageHistoryChatLog");

                Case(1000, "Base::Steamapps");
                Case(1001, "AppDataChanged");
                Case(1002, "RequestAppCallbacksComplete");
                Case(1003, "AppInfoUpdateComplete");
                Case(1004, "AppEventTriggered");
                Case(1005, "DlcInstalled");
                Case(1006, "AppEventStateChange");

                Case(1100, "Base::Steamuserstats");
                Case(1101, "UserStatsReceived");
                Case(1102, "UserStatsStored");
                Case(1103, "UserAchievementStored");

                Case(1200, "Base::Steamnetworking");
                Case(1201, "SocketStatusChanged");
                Case(1202, "P2PSessionRequest");
                Case(1203, "P2PSessionConnectFail");

                Case(1300, "Base::Clientremotestorage");
                Case(1317, "RemoteStorageDownloadUGCResult");
            }
            #undef Case

            // Offset lookup, should never fail.
            #define Case(x, y) case y: return va("%s + %u", #x, Callbacktype % 100);
            switch ((Callbacktype) / 100 * 100)
            {
                Case(k_iSteamUserCallbacks, 100);
                Case(k_iSteamGameServerCallbacks, 200);
                Case(k_iSteamFriendsCallbacks, 300);
                Case(k_iSteamBillingCallbacks, 400);
                Case(k_iSteamMatchmakingCallbacks, 500);
                Case(k_iSteamContentServerCallbacks, 600);
                Case(k_iSteamUtilsCallbacks, 700);
                Case(k_iClientFriendsCallbacks, 800);
                Case(k_iClientUserCallbacks, 900);
                Case(k_iSteamAppsCallbacks, 1000);
                Case(k_iSteamUserStatsCallbacks, 1100);
                Case(k_iSteamNetworkingCallbacks, 1200);
                Case(k_iClientRemoteStorageCallbacks, 1300);
                Case(k_iSteamUserItemsCallbacks, 1400);
                Case(k_iSteamGameServerItemsCallbacks, 1500);
                Case(k_iClientUtilsCallbacks, 1600);
                Case(k_iSteamGameCoordinatorCallbacks, 1700);
                Case(k_iSteamGameServerStatsCallbacks, 1800);
                Case(k_iSteam2AsyncCallbacks, 1900);
                Case(k_iSteamGameStatsCallbacks, 2000);
                Case(k_iClientHTTPCallbacks, 2100);
                Case(k_iClientScreenshotsCallbacks, 2200);
                Case(k_iSteamScreenshotsCallbacks, 2300);
                Case(k_iClientAudioCallbacks, 2400);
                Case(k_iSteamUnifiedMessagesCallbacks, 2500);
                Case(k_iClientUnifiedMessagesCallbacks, 2600);
                Case(k_iClientControllerCallbacks, 2700);
                Case(k_iSteamControllerCallbacks, 2800);
                Case(k_iClientParentalSettingsCallbacks, 2900);
                Case(k_iClientDeviceAuthCallbacks, 3000);
                Case(k_iClientNetworkDeviceManagerCallbacks, 3100);
                Case(k_iClientMusicCallbacks, 3200);
                Case(k_iClientRemoteClientManagerCallbacks, 3300);
                Case(k_iClientUGCCallbacks, 3400);
                Case(k_iSteamStreamClientCallbacks, 3500);
                Case(k_IClientProductBuilderCallbacks, 3600);
                Case(k_iClientShortcutsCallbacks, 3700);
                Case(k_iClientRemoteControlManagerCallbacks, 3800);
                Case(k_iSteamAppListCallbacks, 3900);
                Case(k_iSteamMusicCallbacks, 4000);
                Case(k_iSteamMusicRemoteCallbacks, 4100);
                Case(k_iClientVRCallbacks, 4200);
                Case(k_iClientReservedCallbacks, 4300);
                Case(k_iSteamReservedCallbacks, 4400);
                Case(k_iSteamHTMLSurfaceCallbacks, 4500);
                Case(k_iClientVideoCallbacks, 4600);
                Case(k_iClientInventoryCallbacks, 4700);
            }
            #undef Case

            // Just in case.
            return va("%d", Callbacktype);
        };
    }
}
