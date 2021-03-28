/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct GameInfo_t
    {
        GameID_t m_gameID;
        uint32_t m_unGameIP;
        uint16_t m_usGamePort;
        uint16_t m_usQueryPort;
        SteamID_t m_steamIDLobby;
    };
    static Hashmap<uint64_t, GameInfo_t> Friendsgames;
    uint32_t Personastate;

    static sqlite::database Database()
    {
        try
        {
            sqlite::sqlite_config Config{};
            Config.flags = sqlite::OpenFlags::CREATE | sqlite::OpenFlags::SQLITE_OPEN_READWRITE | sqlite::OpenFlags::FULLMUTEX;
            return sqlite::database("./Ayria/Client.db", Config);
        }
        catch (std::exception &e)
        {
            Errorprint(va("Could not connect to the database: %s", e.what()));
            return sqlite::database(":memory:");
        }
    }

    struct SteamFriends
    {
        enum EActivateGameOverlayToWebPageMode
        {
            k_EActivateGameOverlayToWebPageMode_Default = 0,
            k_EActivateGameOverlayToWebPageMode_Modal = 1
        };
        enum EChatRoomEnterResponse
        {
            k_EChatRoomEnterResponseSuccess = 1,				// Success
            k_EChatRoomEnterResponseDoesntExist = 2,			// Chat doesn't exist (probably closed)
            k_EChatRoomEnterResponseNotAllowed = 3,				// General Denied - You don't have the permissions needed to join the chat
            k_EChatRoomEnterResponseFull = 4,					// Chat room has reached its maximum size
            k_EChatRoomEnterResponseError = 5,					// Unexpected Error
            k_EChatRoomEnterResponseBanned = 6,					// You are banned from this chat room and may not join
            k_EChatRoomEnterResponseLimited = 7,				// Joining this chat is not allowed because you are a limited user (no value on account)
            k_EChatRoomEnterResponseClanDisabled = 8,			// Attempt to join a clan chat when the clan is locked or disabled
            k_EChatRoomEnterResponseCommunityBan = 9,			// Attempt to join a chat when the user has a community lock on their account
            k_EChatRoomEnterResponseMemberBlockedYou = 10,		// Join failed - some member in the chat has blocked you from joining
            k_EChatRoomEnterResponseYouBlockedMember = 11,		// Join failed - you have blocked some member already in the chat
            k_EChatRoomEnterResponseNoRankingDataLobby = 12,	// There is no ranking data available for the lobby
            k_EChatRoomEnterResponseNoRankingDataUser = 13,		// There is no ranking data available for the user
            k_EChatRoomEnterResponseRankOutOfRange = 14,		// The user is out of the allowable ranking range
        };
        enum EOverlayToStoreFlag
        {
            k_EOverlayToStoreFlag_None = 0,
            k_EOverlayToStoreFlag_AddToCart = 1,
            k_EOverlayToStoreFlag_AddToCartAndShow = 2,
        };
        enum EFriendRelationship
        {
            k_EFriendRelationshipNone = 0,
            k_EFriendRelationshipBlocked = 1,			// this doesn't get stored; the user has just done an Ignore on an friendship invite
            k_EFriendRelationshipRequestRecipient = 2,
            k_EFriendRelationshipFriend = 3,
            k_EFriendRelationshipRequestInitiator = 4,
            k_EFriendRelationshipIgnored = 5,			// this is stored; the user has explicit blocked this other user from comments/chat/etc
            k_EFriendRelationshipIgnoredFriend = 6,
            k_EFriendRelationshipSuggested_DEPRECATED = 7,		// was used by the original implementation of the facebook linking feature, but now unused.

            // keep this updated
            k_EFriendRelationshipMax = 8,
        };
        enum EUserRestriction
        {
            k_nUserRestrictionNone = 0,	// no known chat/content restriction
            k_nUserRestrictionUnknown = 1,	// we don't know yet (user offline)
            k_nUserRestrictionAnyChat = 2,	// user is not allowed to (or can't) send/recv any chat
            k_nUserRestrictionVoiceChat = 4,	// user is not allowed to (or can't) send/recv voice chat
            k_nUserRestrictionGroupChat = 8,	// user is not allowed to (or can't) send/recv group chat
            k_nUserRestrictionRating = 16,	// user is too young according to rating in current region
            k_nUserRestrictionGameInvites = 32,	// user cannot send or recv game invites (e.g. mobile)
            k_nUserRestrictionTrading = 64,	// user cannot participate in trading (console, mobile)
        };
        enum EChatEntryType
        {
            k_EChatEntryTypeInvalid = 0,
            k_EChatEntryTypeChatMsg = 1,		// Normal text message from another user
            k_EChatEntryTypeTyping = 2,			// Another user is typing (not used in multi-user chat)
            k_EChatEntryTypeInviteGame = 3,		// Invite from other user into that users current game
            k_EChatEntryTypeEmote = 4,			// text emote message (deprecated, should be treated as ChatMsg)
            //k_EChatEntryTypeLobbyGameStart = 5,	// lobby game is starting (dead - listen for LobbyGameCreated_t callback instead)
            k_EChatEntryTypeLeftConversation = 6, // user has left the conversation ( closed chat window )
            // Above are previous FriendMsgType entries, now merged into more generic chat entry types
            k_EChatEntryTypeEntered = 7,		// user has entered the conversation (used in multi-user chat and group chat)
            k_EChatEntryTypeWasKicked = 8,		// user was kicked (data: 64-bit steamid of actor performing the kick)
            k_EChatEntryTypeWasBanned = 9,		// user was banned (data: 64-bit steamid of actor performing the ban)
            k_EChatEntryTypeDisconnected = 10,	// user disconnected
            k_EChatEntryTypeHistoricalChat = 11,	// a chat message from user's chat history or offilne message
            //k_EChatEntryTypeReserved1 = 12, // No longer used
            //k_EChatEntryTypeReserved2 = 13, // No longer used
            k_EChatEntryTypeLinkBlocked = 14, // a link was removed by the chat filter.
        };
        enum EPersonaState
        {
            k_EPersonaStateOffline = 0,			// friend is not currently logged on
            k_EPersonaStateOnline = 1,			// friend is logged on
            k_EPersonaStateBusy = 2,			// user is on, but busy
            k_EPersonaStateAway = 3,			// auto-away feature
            k_EPersonaStateSnooze = 4,			// auto-away for a long time
            k_EPersonaStateLookingToTrade = 5,	// Online, trading
            k_EPersonaStateLookingToPlay = 6,	// Online, wanting to play
            k_EPersonaStateInvisible = 7,		// Online, but appears offline to friends.  This status is never published to clients.
            k_EPersonaStateMax,
        };
        enum EFriendFlags
        {
            k_EFriendFlagNone = 0x00,
            k_EFriendFlagBlocked = 0x01,
            k_EFriendFlagFriendshipRequested = 0x02,
            k_EFriendFlagImmediate = 0x04,			// "regular" friend
            k_EFriendFlagClanMember = 0x08,
            k_EFriendFlagOnGameServer = 0x10,
            k_EFriendFlagHasPlayedWith	= 0x20,	// not currently used
            k_EFriendFlagFriendOfFriend	= 0x40, // not currently used
            k_EFriendFlagRequestingFriendship = 0x80,
            k_EFriendFlagRequestingInfo = 0x100,
            k_EFriendFlagIgnored = 0x200,
            k_EFriendFlagIgnoredFriend = 0x400,
            k_EFriendFlagSuggested = 0x800,	// not used
            k_EFriendFlagChatMember = 0x1000,
            k_EFriendFlagAll = 0xFFFF,
        };

        using Relationflags_t = union { uint8_t Raw; struct { uint8_t isFriend : 1, isBlocked : 1; }; };
        union Grouptype_t
        {
            uint8_t Raw;
            struct
            {
                uint8_t
                    isClan : 1,
                    isPublic : 1,
                    isGamelobby : 1,
                    isLocalhost : 1,
                    isChatgroup : 1,
                    isFriendgroup : 1;
            };
        };
        union GroupID_t
        {
            uint64_t Raw;
            struct
            {
                uint32_t AdminID;   // Creator.
                uint16_t RoomID;    // Random.
                uint8_t Limit;      // Users.
                uint8_t Type;       // Server, localhost, or clan.
            };
        };

        // Simple helper to check permutations.
        static EFriendRelationship Steamrelation(Relationflags_t Client, Relationflags_t Friend)
        {
            // Only 6 possible states for the flags.. currently.
            if (!Client.isFriend && Friend.isFriend) return k_EFriendRelationshipRequestRecipient;
            if (Client.isFriend && !Friend.isFriend) return k_EFriendRelationshipRequestInitiator;
            if (Client.isFriend && Friend.isFriend) return k_EFriendRelationshipFriend;
            if (Friend.isBlocked) return k_EFriendRelationshipIgnored;
            if (Client.isBlocked) return k_EFriendRelationshipBlocked;
            return k_EFriendRelationshipNone;
        }

        //
        AppID_t GetFriendCoplayGame(SteamID_t steamIDFriend)
        {
            if (Friendsgames.contains(steamIDFriend.FullID))
                return Friendsgames[steamIDFriend.FullID].m_gameID.AppID;
            return 0;
        }
        EFriendRelationship GetFriendRelationship(SteamID_t steamIDFriend)
        {
            Relationflags_t Clientflags{}, Friendflags{};

            // Check the database for any flags related to the clients.
            Database() << "SELECT Flags FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                       << Global.XUID.UserID << steamIDFriend.UserID >> Clientflags.Raw;
            Database() << "SELECT Flags FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                       << steamIDFriend.UserID << Global.XUID.UserID >> Friendflags.Raw;

            return Steamrelation(Clientflags, Friendflags);
        }
        EPersonaState GetFriendPersonaState(SteamID_t steamIDFriend)
        {
            uint32_t Lastupdate{};

            // If the client is in the database, they are online.
            Database() << "SELECT Lastupdate FROM Onlineclients WHERE ClientID = ? ORDER BY Authenticated DESC LIMIT 1;"
                       << steamIDFriend.UserID >> Lastupdate;

            // Status is inferred by activity.
            if (time(NULL) - Lastupdate < 15) return k_EPersonaStateOnline;
            if (Lastupdate == 0) return k_EPersonaStateOffline;
            return k_EPersonaStateAway;
        }
        EPersonaState GetPersonaState()
        {
            return EPersonaState(Personastate);
        }
        EUserRestriction GetUserRestrictions()
        {
            return k_nUserRestrictionNone;
        }
        FriendsGroupID_t GetFriendsGroupIDByIndex(int iFG)
        {
            return GroupID_t{ GetClanByIndex(iFG).FullID }.RoomID;
        }

        HSteamCall AddFriendByName(const char *pchEmailOrAccountName)
        {
            uint32_t AccountID{};

            Database() << "SELECT ClientID FROM Onlineclients WHERE Username = ? ORDER BY Authenticated LIMIT 1;"
                       << std::string(pchEmailOrAccountName) >> AccountID;

            if (AccountID)
            {
                SteamID_t ID;
                ID.UserID = AccountID;
                AddFriend(ID);
            }

            return 0;
        }
        HSteamCall InviteFriendByEmail0(const char *pchEmailOrAccountName)
        {
            return AddFriendByName(pchEmailOrAccountName);
        }

        SteamAPICall_t DownloadClanActivityCounts(SteamID_t *psteamIDClans, int cClansToRequest)
        {
            return 0;
        }
        SteamAPICall_t EnumerateFollowingList(uint32_t unStartIndex)
        {
            return 0;
        }
        SteamAPICall_t GetFollowerCount(SteamID_t steamID)
        {
            return 0;
        }
        SteamAPICall_t IsFollowing(SteamID_t steamID)
        {
            return 0;
        }
        SteamAPICall_t JoinClanChatRoom(SteamID_t groupID)
        {
            Ayriarequest("Usergroup::Requestjoin", JSON::Object_t({ { "GroupID", groupID.FullID }, { "ProviderID", Hash::WW32("Steam") } }));
            const auto RequestID = Callbacks::Createrequest();

            // Poll in the background.
            std::thread([=]()
            {
                for (size_t i = 0; i < 120; ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    std::vector<uint32_t> MemberIDs{};
                    Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                               << groupID.UserID >> MemberIDs;

                    if (MemberIDs.contains(Global.XUID.UserID))
                    {
                        const auto Response = new Callbacks::JoinClanChatRoomCompletionResult_t();
                        Response->m_eChatRoomEnterResponse = k_EChatRoomEnterResponseSuccess;
                        Response->m_steamIDClanChat = groupID;

                        Callbacks::Completerequest(RequestID, Callbacks::Types::JoinClanChatRoomCompletionResult_t, Response);
                        return;
                    }
                }

                const auto Response = new Callbacks::JoinClanChatRoomCompletionResult_t();
                Response->m_eChatRoomEnterResponse = k_EChatRoomEnterResponseFull;
                Response->m_steamIDClanChat = groupID;
                Callbacks::Completerequest(RequestID, Callbacks::Types::JoinClanChatRoomCompletionResult_t, Response);

            }).detach();

            return RequestID;
        }
        SteamAPICall_t RequestClanOfficerList(SteamID_t steamIDClan)
        {
            return 0;
        }
        SteamAPICall_t SetPersonaName1(const char *pchPersonaName)
        {
            SetPersonaName0(pchPersonaName);

            const auto Response = new Callbacks::SetPersonaNameResponse_t();
            const auto RequestID = Callbacks::Createrequest();
            Response->m_result = EResult::k_EResultOK;
            Response->m_bSuccess = true;

            Callbacks::Completerequest(RequestID, Callbacks::Types::SetPersonaNameResponse_t, Response);
            return RequestID;
        }

        SteamID_t GetChatMemberByIndex(SteamID_t groupID, int iIndex)
        {
            SteamID_t Result = Global.XUID;

            std::vector<uint32_t> MemberIDs{};
            Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                       << groupID.FullID >> MemberIDs;

            if (MemberIDs.size() < iIndex) return {};
            Result.UserID = MemberIDs[iIndex];
            return Result;
        }
        SteamID_t GetClanByIndex(int iClan)
        {
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID FROM Usergroups;" >> GroupIDs;

            for (const auto &Item : GroupIDs)
            {
                if (Grouptype_t{ GroupID_t{ Item }.Type }.isFriendgroup && iClan-- == 0)
                    return { Item };
            }

            return {};
        }
        SteamID_t GetClanOfficerByIndex(SteamID_t steamIDClan, int iOfficer)
        { return GetClanOwner(steamIDClan); }
        SteamID_t GetClanOwner(SteamID_t steamIDClan)
        {
            SteamID_t ID = Global.XUID;
            ID.UserID = GroupID_t{ steamIDClan.FullID }.AdminID;
            return ID;
        }
        SteamID_t GetCoplayFriend(int iCoplayFriend)
        {
            return {};
        }
        SteamID_t GetFriendByIndex0(int iFriend)
        {
            return GetFriendByIndex1(iFriend, k_EFriendFlagImmediate);
        }
        SteamID_t GetFriendByIndex1(int iFriend, EFriendFlags iFriendFlags)
        {
            Hashmap<uint32_t, Relationflags_t> Source, Target;
            SteamID_t Result{ 0x1100001DEADC0DEULL };
            Hashset<uint32_t> Results;

            // Could probably make a better SQL query for this.
            Database() << "SELECT (SourceID, TargetID, Flags) FROM Relationships WHERE SourceID = ? OR TargetID = ?;"
                       << Global.XUID.UserID << Global.XUID.UserID
                       >> [&](uint32_t &SourceID, uint32_t &TargetID, uint8_t Flags)
                       {
                           if (Global.XUID.UserID == SourceID) Source[TargetID] = { Flags };
                           if (Global.XUID.UserID == TargetID) Target[SourceID] = { Flags };
                       };

            // Match the requested flags.
            for (const auto &[FriendID, Flags] : Source)
            {
                const auto Relation = Steamrelation(Flags, Target[FriendID]);

                switch (Relation)
                {
                    case k_EFriendRelationshipRequestRecipient:
                        if (iFriendFlags & k_EFriendFlagRequestingFriendship)
                            Results.insert(FriendID);
                                break;

                    case k_EFriendRelationshipRequestInitiator:
                        if (iFriendFlags & k_EFriendFlagFriendshipRequested)
                            Results.insert(FriendID);
                                break;

                    case k_EFriendRelationshipFriend:
                        if (iFriendFlags & k_EFriendFlagImmediate)
                            Results.insert(FriendID);
                                break;

                    case k_EFriendRelationshipIgnored:
                        if (iFriendFlags & k_EFriendFlagIgnored)
                            Results.insert(FriendID);
                                break;

                    case k_EFriendRelationshipBlocked:
                        if (iFriendFlags & k_EFriendFlagBlocked)
                            Results.insert(FriendID);
                                break;

                    case k_EFriendRelationshipNone:
                        if (iFriendFlags & k_EFriendFlagNone)
                            Results.insert(FriendID);
                                break;
                }
            }

            // By offset.
            for (const auto &ID : Results)
            {
                if (0 == iFriend--)
                {
                    Result.UserID = ID;
                    return Result;
                }
            }

            return {};
        }
        SteamID_t GetFriendFromSourceByIndex(SteamID_t steamIDSource, int iFriend)
        {
            return GetFriendByIndex0(iFriend);
        }

        bool AcknowledgeInviteToClan(SteamID_t steamID, bool bAcceptOrDenyClanInvite)
        {
            uint64_t GroupID{};
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID FROM Grouprequests WHERE ClientID = ? AND ProviderID = ?;"
                       << steamID.UserID << Hash::WW32("Steam") >> GroupIDs;

            for (const auto &Item : GroupIDs)
            {
                if (GroupID_t{ Item }.AdminID == Global.XUID.UserID)
                {
                    GroupID = Item;
                    break;
                }
            }

            // No applications for a group we are admin of.
            if (GroupID == 0) return false;

            const auto Request = JSON::Object_t({
                { "isAccepted", bAcceptOrDenyClanInvite },
                { "ProviderID", Hash::WW32("Steam") },
                { "ClientID", steamID.UserID },
                { "GroupID", GroupID },
            });

            Ayriarequest("Usergroups::Answerrequest", JSON::Object_t({}));
            return true;
        }
        bool AddFriend(SteamID_t steamIDFriend)
        {
            const auto Request = JSON::Object_t({
                { "TargetID", steamIDFriend.UserID },
                { "isFriend", true}
            });
            Ayriarequest("Relationships::Update", Request);

            const auto Request = new Callbacks::FriendAdded_t();
            Request->m_m_ulSteamID = steamIDFriend.FullID;
            Request->m_result = EResult::k_EResultOK;

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::Types::FriendAdded_t, Request);
            return true;
        }
        bool CloseClanChatWindowInSteam(SteamID_t groupID)
        { return true; }
        bool GetClanActivityCounts(SteamID_t steamID, int *pnOnline, int *pnInGame, int *pnChatting)
        {
            return false;
        }
        bool GetFriendGamePlayed0(SteamID_t steamIDFriend, GameInfo_t *pFriendGameInfo)
        {
            if (!Friendsgames.contains(steamIDFriend.FullID)) return false;
            *pFriendGameInfo = Friendsgames[steamIDFriend.FullID];
            return true;
        }
        bool GetFriendGamePlayed1(SteamID_t steamIDFriend, int32_t *pnGameID, uint32_t *punGameIP, uint16_t *pusGamePort)
        {
            if (!Friendsgames.contains(steamIDFriend.FullID)) return false;
            const auto Entry = &Friendsgames[steamIDFriend.FullID];
            *pusGamePort = Entry->m_usGamePort;
            *pnGameID = Entry->m_gameID.AppID;
            *punGameIP = Entry->m_unGameIP;
            return true;
        }
        bool GetFriendGamePlayed2(SteamID_t steamIDFriend, uint64_t *pulGameID, uint32_t *punGameIP, uint16_t *pusGamePort)
        {
            if (!Friendsgames.contains(steamIDFriend.FullID)) return false;
            const auto Entry = &Friendsgames[steamIDFriend.FullID];
            *pulGameID = Entry->m_gameID.FullID;
            *pusGamePort = Entry->m_usGamePort;
            *punGameIP = Entry->m_unGameIP;
            return true;
        }
        bool GetFriendGamePlayed3(SteamID_t steamIDFriend, uint64_t *pulGameID, uint32_t *punGameIP, uint16_t *pusGamePort, uint16_t *pusQueryPort)
        {
            if (!Friendsgames.contains(steamIDFriend.FullID)) return false;
            const auto Entry = &Friendsgames[steamIDFriend.FullID];
            *pusQueryPort = Entry->m_usQueryPort;
            *pulGameID = Entry->m_gameID.FullID;
            *pusGamePort = Entry->m_usGamePort;
            *punGameIP = Entry->m_unGameIP;
            return true;
        }
        bool HasFriend0(SteamID_t steamIDFriend)
        {
            return HasFriend1(steamIDFriend, k_EFriendFlagImmediate);
        }
        bool HasFriend1(SteamID_t steamIDFriend, EFriendFlags iFriendFlags)
        {
            const auto Relation = GetFriendRelationship(steamIDFriend);

            switch (Relation)
            {
                case k_EFriendRelationshipRequestRecipient:
                    if (iFriendFlags & k_EFriendFlagRequestingFriendship)
                        return true;
                    break;

                case k_EFriendRelationshipRequestInitiator:
                    if (iFriendFlags & k_EFriendFlagFriendshipRequested)
                        return true;
                    break;

                case k_EFriendRelationshipFriend:
                    if (iFriendFlags & k_EFriendFlagImmediate)
                        return true;
                    break;

                case k_EFriendRelationshipIgnored:
                    if (iFriendFlags & k_EFriendFlagIgnored)
                        return true;
                    break;

                case k_EFriendRelationshipBlocked:
                    if (iFriendFlags & k_EFriendFlagBlocked)
                        return true;
                    break;

                case k_EFriendRelationshipNone:
                    if (iFriendFlags & k_EFriendFlagNone)
                        return true;
                    break;
            }

            return false;
        }
        bool InviteFriendByEmail1(const char *emailAddr)
        {
            InviteFriendByEmail0(emailAddr);
            return true;
        }
        bool InviteFriendToClan(SteamID_t steamIDfriend, SteamID_t steamIDclan)
        {
            const auto Message = JSON::Object_t({
                { "ClanID", steamIDclan.FullID },
                { "Type", "Steam_Claninvite" }
            });

            const auto Request = JSON::Object_t({
                { "B64Message", Base64::Encode(JSON::Dump(Message)) },
                { "ProviderID", Hash::WW32("Steam") },
                { "TargetID", steamIDfriend.UserID }
            });

            Ayriarequest("Messaging::Sendtouser", Request);
            return true;
        }
        bool InviteUserToGame(SteamID_t steamIDFriend, const char *pchConnectString)
        {
            const auto Message = JSON::Object_t({
                { "Connectstring", std::string(pchConnectString) },
                { "Type", "Steam_Gameinvite" }
            });
            const auto Request = JSON::Object_t({
                { "B64Message", Base64::Encode(JSON::Dump(Message)) },
                { "ProviderID", Hash::WW32("Steam") },
                { "TargetID", steamIDFriend.UserID }
            });

            Ayriarequest("Messaging::Sendtouser", Request);
            return true;
        }
        bool IsClanChatAdmin(SteamID_t groupID, SteamID_t userID)
        {
            return GroupID_t{ groupID.FullID }.AdminID == userID.UserID;
        }
        bool IsClanChatWindowOpenInSteam(SteamID_t groupID)
        { return false; }
        bool IsClanOfficialGameGroup(SteamID_t steamIDClan)
        { return false; }
        bool IsClanPublic(SteamID_t steamIDClan)
        {
            return Grouptype_t{ GroupID_t{ steamIDClan.FullID }.Type }.isPublic;
        }
        bool IsUserInSource(SteamID_t steamIDUser, SteamID_t steamIDSource)
        {
            std::vector<uint32_t> MemberIDs{};
            Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                       << steamIDSource.FullID >> MemberIDs;

            return MemberIDs.end() != std::find(MemberIDs.begin(), MemberIDs.end(), steamIDUser.UserID);
        }
        bool LeaveClanChatRoom(SteamID_t groupID)
        {
            Ayriarequest("Usergroups::Leavegroup", JSON::Object_t({ {"GroupID", groupID.UserID} }));
            return true;
        }
        bool OpenClanChatWindowInSteam(SteamID_t groupID)
        {
            return false;
        }
        bool RegisterProtocolInOverlayBrowser(const char *pchProtocol)
        {
            Infoprint(va("%s: %s", __FUNCTION__, pchProtocol));
            return true;
        }
        bool RemoveFriend(SteamID_t steamIDFriend)
        {
            Ayriarequest("Relationships::Clear", JSON::Object_t({ { "TargetID", steamIDFriend.UserID } }));
            return true;
        }
        bool ReplyToFriendMessage(SteamID_t steamIDFriend, const char *pchMsgToSend)
        {
            const auto Request = JSON::Object_t({
                { "B64Message", Base64::Encode(pchMsgToSend) },
                { "ProviderID", Hash::WW32("Steam") },
                { "TargetID", steamIDfriend.UserID },
                { "isPrivate", true },
                { "Save", true }
            });

            Ayriarequest("Messaging::Sendtouser", Request);
            return true;
        }
        bool RequestUserInformation(SteamID_t steamIDUser, bool bRequireNameOnly)
        {
            return true;
        }
        bool SendClanChatMessage(SteamID_t groupID, const char *cszMessage)
        {
            const auto Request = JSON::Object_t({
                { "B64Message", Base64::Encode(cszMessage) },
                { "ProviderID", Hash::WW32("Steam") },
                { "GroupID", groupID.FullID },
                { "Save", true }
            });

            Ayriarequest("Messaging::Sendtogroup", Request);
            return true;
        }
        bool SendMsgToFriend1(SteamID_t steamIDFriend, EChatEntryType eFriendMsgType, const void *pvMsgBody, int cubMsgBody)
        {
            if (eFriendMsgType != k_EChatEntryTypeChatMsg && eFriendMsgType == k_EChatEntryTypeEmote)
                return false;

            const auto Request = JSON::Object_t({
                { "B64Message", Base64::Encode(pvMsgBody, cubMsgBody) },
                { "ProviderID", Hash::WW32("Steam") },
                { "TargetID", steamIDFriend.UserID },
                { "isPrivate", true },
                { "Save", true }
            });

            Ayriarequest("Messaging::Sendtouser", Request);
            return true;
        }
        bool SetListenForFriendsMessages(bool bInterceptEnabled)
        { return true; }
        bool SetRichPresence(const char *pchKey, const char *pchValue)
        {
            Ayriarequest("Presence::Update", JSON::Object_t({ { std::string(pchKey), std::string(pchValue) } }));
            return true;
        }

        const char *GetClanName(SteamID_t steamIDClan)
        {
            static std::string Name{};
            Database() << "SELECT Groupname FROM Usergroups WHERE GroupID = ?;" << steamIDClan.FullID >> Name;
            return Name.c_str();
        }
        const char *GetClanTag(SteamID_t steamIDClan)
        {
            return "";
        }
        const char *GetFriendPersonaName(SteamID_t steamIDFriend)
        {
            static std::string Name{};
            Database() << "SELECT Username FROM Onlineclients WHERE ClientID = ? ORDER BY Authenticated DESC LIMIT 1;"
                       << steamIDFriend.UserID >> Name;
            return Name.c_str();
        }
        const char *GetFriendPersonaNameHistory(SteamID_t steamIDFriend, int iPersonaName)
        {
            if (iPersonaName == 0) return GetFriendPersonaName(steamIDFriend);
            else return "";
        }
        const char *GetFriendRegValue(SteamID_t steamIDFriend, const char *pchKey)
        {
            return GetFriendRichPresence(steamIDFriend, pchKey);
        }
        const char *GetFriendRichPresence(SteamID_t steamIDFriend, const char *pchKey)
        {
            static std::string Cache{};

            Database() << "SELECT Value FROM Presence WHERE ClientID = ? AND Key = ?;"
                       << steamIDFriend.UserID << std::string(pchKey) >> Cache;

            return Cache.c_str();
        }
        const char *GetFriendRichPresenceKeyByIndex(SteamID_t steamIDFriend, int iKey)
        {
            static std::string Cache{};

            Database() << "SELECT Key FROM Presence WHERE ClientID = ? LIMIT 1 OFFSET ?;"
                << steamIDFriend.UserID << iKey >> Cache;

            return Cache.c_str();
        }
        const char *GetFriendsGroupName(FriendsGroupID_t friendsGroupID)
        {
            static std::string Name{};
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID from Usergroups;" >> GroupIDs;

            for (const auto &ID : GroupIDs)
            {
                if (Grouptype_t{ GroupID_t{ ID }.Type }.isFriendgroup && GroupID_t{ ID }.RoomID == friendsGroupID)
                {
                    Database() << "SELECT Groupname FROM Usergroups WHERE GroupID = ?;" << ID >> Name;
                    return Name.c_str();
                }
            }

            return "";
        }
        const char *GetPersonaName()
        {
            // Persona names 'should' support raw UTF8..
            return (const char *)Global.Username;
        }
        const char *GetPlayerNickname(SteamID_t steamIDPlayer)
        {
            return GetFriendPersonaName(steamIDPlayer);
        }

        int GetChatIDOfChatHistoryStart(SteamID_t steamIDFriend)
        { return 0; }
        int GetChatMessage(SteamID_t steamIDFriend, int iChatID, void *pvData, int cubData, EChatEntryType *peFriendMsgType)
        {
            std::string B64Message{};
            uint32_t Timestamp{};

            Database() << "SELECT (B64Message, Timestamp) FROM Messages WHERE SourceID = ? AND ProviderID = ? AND Transient = false LIMIT 1 OFFSET ?;"
                       << steamIDFriend.UserID << Hash::WW32("Steam") << iChatID >> std::tie(B64Message, Timestamp);

            const auto Decoded = Base64::Decode(B64Message);
            int Min = std::min(cubData, int(Decoded.size()));

            *peFriendMsgType = (uint32_t(time(NULL)) - Timestamp) > 30 ? k_EChatEntryTypeHistoricalChat : k_EChatEntryTypeChatMsg;
            std::memcpy(pvData, Decoded.data(), Min);
            return Min;
        }
        int GetClanChatMemberCount(SteamID_t groupID)
        {
            std::vector<uint32_t> MemberIDs{};

            Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                       << groupID.FullID >> MemberIDs;

            return MemberIDs.size();
        }
        int GetClanChatMessage(SteamID_t groupID, int iMessage, void *prgchText, int cchTextMax, EChatEntryType *peChatEntryType, SteamID_t *pSteamIDChatter)
        {
            std::string B64Message{};
            uint32_t Timestamp{};
            uint32_t SourceID{};

            Database() << "SELECT (B64Message, Timestamp, SourceID) FROM Messages WHERE GroupID = ? LIMIT 1 OFFSET ?;"
                       << groupID.FullID << iMessage >> std::tie(B64Message, Timestamp, SourceID);

            const auto Decoded = Base64::Decode(B64Message);
            int Min = std::min(cchTextMax, int(Decoded.size()));

            *pSteamIDChatter = Global.XUID;
            pSteamIDChatter->UserID = SourceID;

            *peChatEntryType = (uint32_t(time(NULL)) - Timestamp) > 30 ? k_EChatEntryTypeHistoricalChat : k_EChatEntryTypeChatMsg;
            std::memcpy(prgchText, Decoded.data(), Min);
            return Min;
        }
        int GetClanCount()
        {
            int Count = 0;
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID FROM Usergroups;" >> GroupIDs;

            for (const auto &Item : GroupIDs)
            {
                if (Grouptype_t{ GroupID_t{ Item }.Type }.isClan)
                    Count++;
            }

            return Count;
        }
        int GetClanOfficerCount(SteamID_t steamIDClan)
        {
            return 0;
        }
        int GetCoplayFriendCount()
        {
            return 0;
        }
        int GetFriendAvatar0(SteamID_t steamIDFriend)
        {
            return GetFriendAvatar0(steamIDFriend, 128);
        }
        int GetFriendAvatar1(SteamID_t steamIDFriend, int eAvatarSize)
        {
            return 0;
        }
        int GetFriendCoplayTime(SteamID_t steamIDFriend)
        {
            return 0;
        }
        int GetFriendCount0()
        {
            return GetFriendCount1(k_EFriendFlagImmediate);
        }
        int GetFriendCount1(EFriendFlags iFriendFlags)
        {
            int Count{};

            while (GetFriendByIndex1(Count, iFriendFlags).FullID)
                Count++;

            return Count;
        }
        int GetFriendCountFromSource(SteamID_t steamIDSource)
        {
            return 0;
        }
        int GetFriendMessage(SteamID_t friendID, int iChatID, void *pvData, int cubData, EChatEntryType *peChatEntryType)
        {
            return GetChatMessage(friendID, iChatID, pvData, cubData, peChatEntryType);
        }
        int GetFriendRichPresenceKeyCount(SteamID_t steamIDFriend)
        {
            int Count{};

            Database() << "SELECT COUNT(*) FROM Presence WHERE ClientID = ?;" << steamIDFriend.UserID >> Count;

            return Count;
        }
        int GetFriendSteamLevel(SteamID_t steamIDFriend)
        {
            return 42;
        }
        int GetFriendsGroupCount()
        {
            return 0;
        }
        int GetFriendsGroupMembersCount(FriendsGroupID_t friendsGroupID)
        {
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID FROM Usergroups;" >> GroupIDs;

            for (const auto &ID : GroupIDs)
            {
                if (Grouptype_t{ GroupID_t{ ID }.Type }.isFriendgroup && GroupID_t{ ID }.RoomID == friendsGroupID)
                {
                    std::vector<uint32_t> MemberIDs{};
                    Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                               << ID >> MemberIDs;

                    return int(MemberIDs.size());
                }
            }

            return 0;
        }
        int GetLargeFriendAvatar(SteamID_t steamIDFriend)
        {
            return GetFriendAvatar0(steamIDFriend, 184);
        }
        int GetMediumFriendAvatar(SteamID_t steamIDFriend)
        {
            return GetFriendAvatar0(steamIDFriend, 64);
        }
        int GetNumChatsWithUnreadPriorityMessages()
        {
            return 0;
        }
        int GetSmallFriendAvatar(SteamID_t steamIDFriend)
        {
            return GetFriendAvatar0(steamIDFriend, 32);
        }

        uint32_t GetBlockedFriendCount()
        {
            uint32_t Count{};

            Database() << "SELECT Flags FROM Relationships WHERE SourceID = ?;"
                       << Global.XUID.UserID
                       >> [&](uint8_t Flags)
                       {
                           if (Relationflags_t{ Flags }.isBlocked)
                               Count++;
                       };

            return Count;
        }

        void ActivateGameOverlay(const char *pchDialog)
        {}
        void ActivateGameOverlayInviteDialog(SteamID_t steamIDLobby)
        {}
        void ActivateGameOverlayInviteDialogConnectString(const char *pchConnectString)
        {}
        void ActivateGameOverlayRemotePlayTogetherInviteDialog(SteamID_t steamIDLobby)
        {}
        void ActivateGameOverlayToStore0(AppID_t nAppID)
        {
            return ActivateGameOverlayToStore1(nAppID, k_EOverlayToStoreFlag_None);
        }
        void ActivateGameOverlayToStore1(AppID_t nAppID, EOverlayToStoreFlag eFlag)
        {}
        void ActivateGameOverlayToUser(const char *pchDialog, SteamID_t steamID)
        {}
        void ActivateGameOverlayToWebPage0(const char *pchURL)
        {
            return ActivateGameOverlayToWebPage1(pchURL, k_EActivateGameOverlayToWebPageMode_Default);
        }
        void ActivateGameOverlayToWebPage1(const char *pchURL, EActivateGameOverlayToWebPageMode eMode)
        {}
        void ClearChatHistory(SteamID_t steamIDFriend)
        {
            Database() << "DELETE FROM Messages WJERE SourceID = ? AND ProviderID = ? AND Transient = false;"
                       << steamIDFriend.UserID << Hash::WW32("Source");
        }
        void ClearRichPresence()
        {
            Database() << "DELETE FROM Presence WHERE ClientID = ?;" << Global.XUID.UserID;
        }
        void GetFriendsGroupMembersList(FriendsGroupID_t friendsGroupID, SteamID_t *pOutSteamIDMembers, int nMembersCount)
        {
            std::vector<uint64_t> GroupIDs{};
            Database() << "SELECT GroupID FROM Usergroups;" >> GroupIDs;

            for (const auto &ID : GroupIDs)
            {
                if (Grouptype_t{ GroupID_t{ ID }.Type }.isFriendgroup && GroupID_t{ ID }.RoomID == friendsGroupID)
                {
                    std::vector<uint32_t> MemberIDs{};
                    Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                               << ID >> MemberIDs;

                    for (int i = 0; i < std::min(int(MemberIDs.size()), nMembersCount); ++i)
                    {
                        pOutSteamIDMembers[i] = Global.XUID;
                        pOutSteamIDMembers[i].UserID = MemberIDs[i];
                    }

                    return;
                }
            }
        }
        void RequestFriendRichPresence(SteamID_t steamIDFriend)
        {}
        void SendMsgToFriend0(SteamID_t steamIDFriend, EChatEntryType eFriendMsgType, const char *pchMsgBody)
        {
            SendMsgToFriend1(steamIDFriend, eFriendMsgType, pchMsgBody, std::strlen(pchMsgBody));
        }
        void SetChatHistoryStart(SteamID_t steamIDFriend, int iChatID)
        {}
        void SetFriendRegValue(SteamID_t steamIDFriend, const char *pchKey, const char *pchValue)
        {
            Traceprint();
        }
        void SetInGameVoiceSpeaking(SteamID_t steamIDUser, bool bSpeaking)
        {}
        void SetPersonaName0(const char *pchPersonaName)
        {
            std::memcpy(Global.Username, pchPersonaName, std::min(size_t(20), std::strlen(pchPersonaName)));

            // Notify the game that the players state changed.
            const auto Notification = new Callbacks::PersonaStateChange_t();
            Notification->m_ulSteamID = Global.XUID.FullID;
            Notification->m_nChangeFlags = 1;

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::Types::PersonaStateChange_t, Notification);
        }
        void SetPersonaState(EPersonaState ePersonaState)
        {
            Personastate = ePersonaState;
        }
        void SetPlayedWith(SteamID_t steamIDUserPlayedWith)
        {}
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamFriends001 : Interface_t
    {
        SteamFriends001()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, SetPersonaState);
            Createmethod(4, SteamFriends, AddFriend);
            Createmethod(5, SteamFriends, RemoveFriend);
            Createmethod(6, SteamFriends, HasFriend0);
            Createmethod(7, SteamFriends, GetFriendRelationship);
            Createmethod(8, SteamFriends, GetFriendPersonaState);
            Createmethod(9, SteamFriends, GetFriendGamePlayed0);
            Createmethod(10, SteamFriends, GetFriendPersonaName);
            Createmethod(11, SteamFriends, AddFriendByName);
            Createmethod(12, SteamFriends, GetFriendCount0);
            Createmethod(13, SteamFriends, GetFriendByIndex0);
            Createmethod(14, SteamFriends, SendMsgToFriend0);
            Createmethod(15, SteamFriends, SetFriendRegValue);
            Createmethod(16, SteamFriends, GetFriendRegValue);
            Createmethod(17, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(18, SteamFriends, GetChatMessage);
            Createmethod(19, SteamFriends, SendMsgToFriend1);
            Createmethod(20, SteamFriends, GetChatIDOfChatHistoryStart);
            Createmethod(21, SteamFriends, SetChatHistoryStart);
            Createmethod(22, SteamFriends, ClearChatHistory);
            Createmethod(23, SteamFriends, InviteFriendByEmail0);
            Createmethod(24, SteamFriends, GetBlockedFriendCount);
            Createmethod(25, SteamFriends, GetFriendGamePlayed1);
            Createmethod(26, SteamFriends, GetFriendGamePlayed2);
        };
    };
    struct SteamFriends002 : Interface_t
    {
        SteamFriends002()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, SetPersonaState);
            Createmethod(4, SteamFriends, GetFriendCount1);
            Createmethod(5, SteamFriends, GetFriendByIndex1);
            Createmethod(6, SteamFriends, GetFriendRelationship);
            Createmethod(7, SteamFriends, GetFriendPersonaState);
            Createmethod(8, SteamFriends, GetFriendPersonaName);
            Createmethod(9, SteamFriends, SetFriendRegValue);
            Createmethod(10, SteamFriends, GetFriendRegValue);
            Createmethod(11, SteamFriends, GetFriendGamePlayed3);
            Createmethod(12, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(13, SteamFriends, AddFriend);
            Createmethod(14, SteamFriends, RemoveFriend);
            Createmethod(15, SteamFriends, HasFriend1);
            Createmethod(16, SteamFriends, AddFriendByName);
            Createmethod(17, SteamFriends, InviteFriendByEmail1);
            Createmethod(18, SteamFriends, GetChatMessage);
            Createmethod(19, SteamFriends, SendMsgToFriend1);
            Createmethod(20, SteamFriends, GetChatIDOfChatHistoryStart);
            Createmethod(21, SteamFriends, SetChatHistoryStart);
            Createmethod(22, SteamFriends, ClearChatHistory);
            Createmethod(23, SteamFriends, GetClanCount);
            Createmethod(24, SteamFriends, GetClanByIndex);
            Createmethod(25, SteamFriends, GetClanName);
            Createmethod(26, SteamFriends, InviteFriendToClan);
            Createmethod(27, SteamFriends, AcknowledgeInviteToClan);
            Createmethod(28, SteamFriends, GetFriendCountFromSource);
            Createmethod(29, SteamFriends, GetFriendFromSourceByIndex);
        };
    };
    struct SteamFriends003 : Interface_t
    {
        SteamFriends003()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendAvatar0);
            Createmethod(9, SteamFriends, GetFriendGamePlayed3);
            Createmethod(10, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(11, SteamFriends, HasFriend1);
            Createmethod(12, SteamFriends, GetClanCount);
            Createmethod(13, SteamFriends, GetClanByIndex);
            Createmethod(14, SteamFriends, GetClanName);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
        };
    };
    struct SteamFriends004 : Interface_t
    {
        SteamFriends004()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendAvatar1);
            Createmethod(9, SteamFriends, GetFriendGamePlayed3);
            Createmethod(10, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(11, SteamFriends, HasFriend1);
            Createmethod(12, SteamFriends, GetClanCount);
            Createmethod(13, SteamFriends, GetClanByIndex);
            Createmethod(14, SteamFriends, GetClanName);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
        };
    };
    struct SteamFriends005 : Interface_t
    {
        SteamFriends005()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendAvatar1);
            Createmethod(9, SteamFriends, GetFriendGamePlayed0);
            Createmethod(10, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(11, SteamFriends, HasFriend1);
            Createmethod(12, SteamFriends, GetClanCount);
            Createmethod(13, SteamFriends, GetClanByIndex);
            Createmethod(14, SteamFriends, GetClanName);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
            Createmethod(20, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(22, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(23, SteamFriends, SetPlayedWith);
        };
    };
    struct SteamFriends006 : Interface_t
    {
        SteamFriends006()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendAvatar1);
            Createmethod(9, SteamFriends, GetFriendGamePlayed0);
            Createmethod(10, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(11, SteamFriends, HasFriend1);
            Createmethod(12, SteamFriends, GetClanCount);
            Createmethod(13, SteamFriends, GetClanByIndex);
            Createmethod(14, SteamFriends, GetClanName);
            Createmethod(15, SteamFriends, GetClanTag);
            Createmethod(16, SteamFriends, GetFriendCountFromSource);
            Createmethod(17, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(18, SteamFriends, IsUserInSource);
            Createmethod(19, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(20, SteamFriends, ActivateGameOverlay);
            Createmethod(21, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(22, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(23, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(24, SteamFriends, SetPlayedWith);
            Createmethod(25, SteamFriends, ActivateGameOverlayInviteDialog);
        };
    };
    struct SteamFriends007 : Interface_t
    {
        SteamFriends007()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
            Createmethod(20, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(22, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(23, SteamFriends, SetPlayedWith);
            Createmethod(24, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(25, SteamFriends, GetSmallFriendAvatar);
            Createmethod(26, SteamFriends, GetMediumFriendAvatar);
            Createmethod(27, SteamFriends, GetLargeFriendAvatar);
        };
    };
    struct SteamFriends008 : Interface_t
    {
        SteamFriends008()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
            Createmethod(20, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(22, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(23, SteamFriends, SetPlayedWith);
            Createmethod(24, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(25, SteamFriends, GetSmallFriendAvatar);
            Createmethod(26, SteamFriends, GetMediumFriendAvatar);
            Createmethod(27, SteamFriends, GetLargeFriendAvatar);
            Createmethod(28, SteamFriends, RequestUserInformation);
            Createmethod(29, SteamFriends, RequestClanOfficerList);
            Createmethod(30, SteamFriends, GetClanOwner);
            Createmethod(31, SteamFriends, GetClanOfficerCount);
            Createmethod(32, SteamFriends, GetClanOfficerByIndex);
            Createmethod(33, SteamFriends, GetUserRestrictions);
        };
    };
    struct SteamFriends009 : Interface_t
    {
        SteamFriends009()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetFriendCountFromSource);
            Createmethod(16, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(17, SteamFriends, IsUserInSource);
            Createmethod(18, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(19, SteamFriends, ActivateGameOverlay);
            Createmethod(20, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(22, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(23, SteamFriends, SetPlayedWith);
            Createmethod(24, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(25, SteamFriends, GetSmallFriendAvatar);
            Createmethod(26, SteamFriends, GetMediumFriendAvatar);
            Createmethod(27, SteamFriends, GetLargeFriendAvatar);
            Createmethod(28, SteamFriends, RequestUserInformation);
            Createmethod(29, SteamFriends, RequestClanOfficerList);
            Createmethod(30, SteamFriends, GetClanOwner);
            Createmethod(31, SteamFriends, GetClanOfficerCount);
            Createmethod(32, SteamFriends, GetClanOfficerByIndex);
            Createmethod(33, SteamFriends, GetUserRestrictions);
            Createmethod(34, SteamFriends, SetRichPresence);
            Createmethod(35, SteamFriends, ClearRichPresence);
            Createmethod(36, SteamFriends, GetFriendRichPresence);
            Createmethod(37, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(38, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(39, SteamFriends, InviteUserToGame);
            Createmethod(40, SteamFriends, GetCoplayFriendCount);
            Createmethod(41, SteamFriends, GetCoplayFriend);
            Createmethod(42, SteamFriends, GetFriendCoplayTime);
            Createmethod(43, SteamFriends, GetFriendCoplayGame);
        };
    };
    struct SteamFriends010 : Interface_t
    {
        SteamFriends010()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetClanActivityCounts);
            Createmethod(16, SteamFriends, DownloadClanActivityCounts);
            Createmethod(17, SteamFriends, GetFriendCountFromSource);
            Createmethod(18, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(19, SteamFriends, IsUserInSource);
            Createmethod(20, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(21, SteamFriends, ActivateGameOverlay);
            Createmethod(22, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(24, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(25, SteamFriends, SetPlayedWith);
            Createmethod(26, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(27, SteamFriends, GetSmallFriendAvatar);
            Createmethod(28, SteamFriends, GetMediumFriendAvatar);
            Createmethod(29, SteamFriends, GetLargeFriendAvatar);
            Createmethod(30, SteamFriends, RequestUserInformation);
            Createmethod(31, SteamFriends, RequestClanOfficerList);
            Createmethod(32, SteamFriends, GetClanOwner);
            Createmethod(33, SteamFriends, GetClanOfficerCount);
            Createmethod(34, SteamFriends, GetClanOfficerByIndex);
            Createmethod(35, SteamFriends, GetUserRestrictions);
            Createmethod(36, SteamFriends, SetRichPresence);
            Createmethod(37, SteamFriends, ClearRichPresence);
            Createmethod(38, SteamFriends, GetFriendRichPresence);
            Createmethod(39, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(40, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(41, SteamFriends, InviteUserToGame);
            Createmethod(42, SteamFriends, GetCoplayFriendCount);
            Createmethod(43, SteamFriends, GetCoplayFriend);
            Createmethod(44, SteamFriends, GetFriendCoplayTime);
            Createmethod(45, SteamFriends, GetFriendCoplayGame);
            Createmethod(46, SteamFriends, JoinClanChatRoom);
            Createmethod(47, SteamFriends, LeaveClanChatRoom);
            Createmethod(48, SteamFriends, GetClanChatMemberCount);
            Createmethod(49, SteamFriends, GetChatMemberByIndex);
            Createmethod(50, SteamFriends, SendClanChatMessage);
            Createmethod(51, SteamFriends, GetClanChatMessage);
            Createmethod(52, SteamFriends, IsClanChatAdmin);
            Createmethod(53, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(54, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(55, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(56, SteamFriends, SetListenForFriendsMessages);
            Createmethod(57, SteamFriends, ReplyToFriendMessage);
            Createmethod(58, SteamFriends, GetChatMessage);
        };
    };
    struct SteamFriends011 : Interface_t
    {
        SteamFriends011()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName0);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetClanActivityCounts);
            Createmethod(16, SteamFriends, DownloadClanActivityCounts);
            Createmethod(17, SteamFriends, GetFriendCountFromSource);
            Createmethod(18, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(19, SteamFriends, IsUserInSource);
            Createmethod(20, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(21, SteamFriends, ActivateGameOverlay);
            Createmethod(22, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(24, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(25, SteamFriends, SetPlayedWith);
            Createmethod(26, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(27, SteamFriends, GetSmallFriendAvatar);
            Createmethod(28, SteamFriends, GetMediumFriendAvatar);
            Createmethod(29, SteamFriends, GetLargeFriendAvatar);
            Createmethod(30, SteamFriends, RequestUserInformation);
            Createmethod(31, SteamFriends, RequestClanOfficerList);
            Createmethod(32, SteamFriends, GetClanOwner);
            Createmethod(33, SteamFriends, GetClanOfficerCount);
            Createmethod(34, SteamFriends, GetClanOfficerByIndex);
            Createmethod(35, SteamFriends, GetUserRestrictions);
            Createmethod(36, SteamFriends, SetRichPresence);
            Createmethod(37, SteamFriends, ClearRichPresence);
            Createmethod(38, SteamFriends, GetFriendRichPresence);
            Createmethod(39, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(40, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(41, SteamFriends, RequestFriendRichPresence);
            Createmethod(42, SteamFriends, InviteUserToGame);
            Createmethod(43, SteamFriends, GetCoplayFriendCount);
            Createmethod(44, SteamFriends, GetCoplayFriend);
            Createmethod(45, SteamFriends, GetFriendCoplayTime);
            Createmethod(46, SteamFriends, GetFriendCoplayGame);
            Createmethod(47, SteamFriends, JoinClanChatRoom);
            Createmethod(48, SteamFriends, LeaveClanChatRoom);
            Createmethod(49, SteamFriends, GetClanChatMemberCount);
            Createmethod(50, SteamFriends, GetChatMemberByIndex);
            Createmethod(51, SteamFriends, SendClanChatMessage);
            Createmethod(52, SteamFriends, GetClanChatMessage);
            Createmethod(53, SteamFriends, IsClanChatAdmin);
            Createmethod(54, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(55, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(56, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(57, SteamFriends, SetListenForFriendsMessages);
            Createmethod(58, SteamFriends, ReplyToFriendMessage);
            Createmethod(59, SteamFriends, GetChatMessage);
            Createmethod(60, SteamFriends, GetFollowerCount);
            Createmethod(61, SteamFriends, IsFollowing);
            Createmethod(62, SteamFriends, EnumerateFollowingList);
        };
    };
    struct SteamFriends012 : Interface_t
    {
        SteamFriends012()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName1);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetClanActivityCounts);
            Createmethod(16, SteamFriends, DownloadClanActivityCounts);
            Createmethod(17, SteamFriends, GetFriendCountFromSource);
            Createmethod(18, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(19, SteamFriends, IsUserInSource);
            Createmethod(20, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(21, SteamFriends, ActivateGameOverlay);
            Createmethod(22, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(24, SteamFriends, ActivateGameOverlayToStore0);
            Createmethod(25, SteamFriends, SetPlayedWith);
            Createmethod(26, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(27, SteamFriends, GetSmallFriendAvatar);
            Createmethod(28, SteamFriends, GetMediumFriendAvatar);
            Createmethod(29, SteamFriends, GetLargeFriendAvatar);
            Createmethod(30, SteamFriends, RequestUserInformation);
            Createmethod(31, SteamFriends, RequestClanOfficerList);
            Createmethod(32, SteamFriends, GetClanOwner);
            Createmethod(33, SteamFriends, GetClanOfficerCount);
            Createmethod(34, SteamFriends, GetClanOfficerByIndex);
            Createmethod(35, SteamFriends, GetUserRestrictions);
            Createmethod(36, SteamFriends, SetRichPresence);
            Createmethod(37, SteamFriends, ClearRichPresence);
            Createmethod(38, SteamFriends, GetFriendRichPresence);
            Createmethod(39, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(40, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(41, SteamFriends, RequestFriendRichPresence);
            Createmethod(42, SteamFriends, InviteUserToGame);
            Createmethod(43, SteamFriends, GetCoplayFriendCount);
            Createmethod(44, SteamFriends, GetCoplayFriend);
            Createmethod(45, SteamFriends, GetFriendCoplayTime);
            Createmethod(46, SteamFriends, GetFriendCoplayGame);
            Createmethod(47, SteamFriends, JoinClanChatRoom);
            Createmethod(48, SteamFriends, LeaveClanChatRoom);
            Createmethod(49, SteamFriends, GetClanChatMemberCount);
            Createmethod(50, SteamFriends, GetChatMemberByIndex);
            Createmethod(51, SteamFriends, SendClanChatMessage);
            Createmethod(52, SteamFriends, GetClanChatMessage);
            Createmethod(53, SteamFriends, IsClanChatAdmin);
            Createmethod(54, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(55, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(56, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(57, SteamFriends, SetListenForFriendsMessages);
            Createmethod(58, SteamFriends, ReplyToFriendMessage);
            Createmethod(59, SteamFriends, GetChatMessage);
            Createmethod(60, SteamFriends, GetFollowerCount);
            Createmethod(61, SteamFriends, IsFollowing);
            Createmethod(62, SteamFriends, EnumerateFollowingList);
        };
    };
    struct SteamFriends013 : Interface_t
    {
        SteamFriends013()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName1);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, HasFriend1);
            Createmethod(11, SteamFriends, GetClanCount);
            Createmethod(12, SteamFriends, GetClanByIndex);
            Createmethod(13, SteamFriends, GetClanName);
            Createmethod(14, SteamFriends, GetClanTag);
            Createmethod(15, SteamFriends, GetClanActivityCounts);
            Createmethod(16, SteamFriends, DownloadClanActivityCounts);
            Createmethod(17, SteamFriends, GetFriendCountFromSource);
            Createmethod(18, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(19, SteamFriends, IsUserInSource);
            Createmethod(20, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(21, SteamFriends, ActivateGameOverlay);
            Createmethod(22, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(24, SteamFriends, ActivateGameOverlayToStore1);
            Createmethod(25, SteamFriends, SetPlayedWith);
            Createmethod(26, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(27, SteamFriends, GetSmallFriendAvatar);
            Createmethod(28, SteamFriends, GetMediumFriendAvatar);
            Createmethod(29, SteamFriends, GetLargeFriendAvatar);
            Createmethod(30, SteamFriends, RequestUserInformation);
            Createmethod(31, SteamFriends, RequestClanOfficerList);
            Createmethod(32, SteamFriends, GetClanOwner);
            Createmethod(33, SteamFriends, GetClanOfficerCount);
            Createmethod(34, SteamFriends, GetClanOfficerByIndex);
            Createmethod(35, SteamFriends, GetUserRestrictions);
            Createmethod(36, SteamFriends, SetRichPresence);
            Createmethod(37, SteamFriends, ClearRichPresence);
            Createmethod(38, SteamFriends, GetFriendRichPresence);
            Createmethod(39, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(40, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(41, SteamFriends, RequestFriendRichPresence);
            Createmethod(42, SteamFriends, InviteUserToGame);
            Createmethod(43, SteamFriends, GetCoplayFriendCount);
            Createmethod(44, SteamFriends, GetCoplayFriend);
            Createmethod(45, SteamFriends, GetFriendCoplayTime);
            Createmethod(46, SteamFriends, GetFriendCoplayGame);
            Createmethod(47, SteamFriends, JoinClanChatRoom);
            Createmethod(48, SteamFriends, LeaveClanChatRoom);
            Createmethod(49, SteamFriends, GetClanChatMemberCount);
            Createmethod(50, SteamFriends, GetChatMemberByIndex);
            Createmethod(51, SteamFriends, SendClanChatMessage);
            Createmethod(52, SteamFriends, GetClanChatMessage);
            Createmethod(53, SteamFriends, IsClanChatAdmin);
            Createmethod(54, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(55, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(56, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(57, SteamFriends, SetListenForFriendsMessages);
            Createmethod(58, SteamFriends, ReplyToFriendMessage);
            Createmethod(59, SteamFriends, GetChatMessage);
            Createmethod(60, SteamFriends, GetFollowerCount);
            Createmethod(61, SteamFriends, IsFollowing);
            Createmethod(62, SteamFriends, EnumerateFollowingList);
        };
    };
    struct SteamFriends014 : Interface_t
    {
        SteamFriends014()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName1);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, GetPlayerNickname);
            Createmethod(11, SteamFriends, HasFriend1);
            Createmethod(12, SteamFriends, GetClanCount);
            Createmethod(13, SteamFriends, GetClanByIndex);
            Createmethod(14, SteamFriends, GetClanName);
            Createmethod(15, SteamFriends, GetClanTag);
            Createmethod(16, SteamFriends, GetClanActivityCounts);
            Createmethod(17, SteamFriends, DownloadClanActivityCounts);
            Createmethod(18, SteamFriends, GetFriendCountFromSource);
            Createmethod(19, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(20, SteamFriends, IsUserInSource);
            Createmethod(21, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(22, SteamFriends, ActivateGameOverlay);
            Createmethod(23, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(24, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(25, SteamFriends, ActivateGameOverlayToStore1);
            Createmethod(26, SteamFriends, SetPlayedWith);
            Createmethod(27, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(28, SteamFriends, GetSmallFriendAvatar);
            Createmethod(29, SteamFriends, GetMediumFriendAvatar);
            Createmethod(30, SteamFriends, GetLargeFriendAvatar);
            Createmethod(31, SteamFriends, RequestUserInformation);
            Createmethod(32, SteamFriends, RequestClanOfficerList);
            Createmethod(33, SteamFriends, GetClanOwner);
            Createmethod(34, SteamFriends, GetClanOfficerCount);
            Createmethod(35, SteamFriends, GetClanOfficerByIndex);
            Createmethod(36, SteamFriends, GetUserRestrictions);
            Createmethod(37, SteamFriends, SetRichPresence);
            Createmethod(38, SteamFriends, ClearRichPresence);
            Createmethod(39, SteamFriends, GetFriendRichPresence);
            Createmethod(40, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(41, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(42, SteamFriends, RequestFriendRichPresence);
            Createmethod(43, SteamFriends, InviteUserToGame);
            Createmethod(44, SteamFriends, GetCoplayFriendCount);
            Createmethod(45, SteamFriends, GetCoplayFriend);
            Createmethod(46, SteamFriends, GetFriendCoplayTime);
            Createmethod(47, SteamFriends, GetFriendCoplayGame);
            Createmethod(48, SteamFriends, JoinClanChatRoom);
            Createmethod(49, SteamFriends, LeaveClanChatRoom);
            Createmethod(50, SteamFriends, GetClanChatMemberCount);
            Createmethod(51, SteamFriends, GetChatMemberByIndex);
            Createmethod(52, SteamFriends, SendClanChatMessage);
            Createmethod(53, SteamFriends, GetClanChatMessage);
            Createmethod(54, SteamFriends, IsClanChatAdmin);
            Createmethod(55, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(56, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(57, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(58, SteamFriends, SetListenForFriendsMessages);
            Createmethod(59, SteamFriends, ReplyToFriendMessage);
            Createmethod(60, SteamFriends, GetChatMessage);
            Createmethod(61, SteamFriends, GetFollowerCount);
            Createmethod(62, SteamFriends, IsFollowing);
            Createmethod(63, SteamFriends, EnumerateFollowingList);
        };
    };
    struct SteamFriends015 : Interface_t
    {
        SteamFriends015()
        {
            Createmethod(0, SteamFriends, GetPersonaName);
            Createmethod(1, SteamFriends, SetPersonaName1);
            Createmethod(2, SteamFriends, GetPersonaState);
            Createmethod(3, SteamFriends, GetFriendCount1);
            Createmethod(4, SteamFriends, GetFriendByIndex1);
            Createmethod(5, SteamFriends, GetFriendRelationship);
            Createmethod(6, SteamFriends, GetFriendPersonaState);
            Createmethod(7, SteamFriends, GetFriendPersonaName);
            Createmethod(8, SteamFriends, GetFriendGamePlayed0);
            Createmethod(9, SteamFriends, GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends, GetFriendSteamLevel);
            Createmethod(11, SteamFriends, GetPlayerNickname);
            Createmethod(12, SteamFriends, GetFriendsGroupCount);
            Createmethod(13, SteamFriends, GetFriendsGroupIDByIndex);
            Createmethod(14, SteamFriends, GetFriendsGroupName);
            Createmethod(15, SteamFriends, GetFriendsGroupMembersCount);
            Createmethod(16, SteamFriends, GetFriendsGroupMembersList);
            Createmethod(17, SteamFriends, HasFriend1);
            Createmethod(18, SteamFriends, GetClanCount);
            Createmethod(19, SteamFriends, GetClanByIndex);
            Createmethod(20, SteamFriends, GetClanName);
            Createmethod(21, SteamFriends, GetClanTag);
            Createmethod(22, SteamFriends, GetClanActivityCounts);
            Createmethod(23, SteamFriends, DownloadClanActivityCounts);
            Createmethod(24, SteamFriends, GetFriendCountFromSource);
            Createmethod(25, SteamFriends, GetFriendFromSourceByIndex);
            Createmethod(26, SteamFriends, IsUserInSource);
            Createmethod(27, SteamFriends, SetInGameVoiceSpeaking);
            Createmethod(28, SteamFriends, ActivateGameOverlay);
            Createmethod(29, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(30, SteamFriends, ActivateGameOverlayToWebPage0);
            Createmethod(31, SteamFriends, ActivateGameOverlayToStore1);
            Createmethod(32, SteamFriends, SetPlayedWith);
            Createmethod(33, SteamFriends, ActivateGameOverlayInviteDialog);
            Createmethod(34, SteamFriends, GetSmallFriendAvatar);
            Createmethod(35, SteamFriends, GetMediumFriendAvatar);
            Createmethod(36, SteamFriends, GetLargeFriendAvatar);
            Createmethod(37, SteamFriends, RequestUserInformation);
            Createmethod(38, SteamFriends, RequestClanOfficerList);
            Createmethod(39, SteamFriends, GetClanOwner);
            Createmethod(40, SteamFriends, GetClanOfficerCount);
            Createmethod(41, SteamFriends, GetClanOfficerByIndex);
            Createmethod(42, SteamFriends, GetUserRestrictions);
            Createmethod(43, SteamFriends, SetRichPresence);
            Createmethod(44, SteamFriends, ClearRichPresence);
            Createmethod(45, SteamFriends, GetFriendRichPresence);
            Createmethod(46, SteamFriends, GetFriendRichPresenceKeyCount);
            Createmethod(47, SteamFriends, GetFriendRichPresenceKeyByIndex);
            Createmethod(48, SteamFriends, RequestFriendRichPresence);
            Createmethod(49, SteamFriends, InviteUserToGame);
            Createmethod(50, SteamFriends, GetCoplayFriendCount);
            Createmethod(51, SteamFriends, GetCoplayFriend);
            Createmethod(52, SteamFriends, GetFriendCoplayTime);
            Createmethod(53, SteamFriends, GetFriendCoplayGame);
            Createmethod(54, SteamFriends, JoinClanChatRoom);
            Createmethod(55, SteamFriends, LeaveClanChatRoom);
            Createmethod(56, SteamFriends, GetClanChatMemberCount);
            Createmethod(57, SteamFriends, GetChatMemberByIndex);
            Createmethod(58, SteamFriends, SendClanChatMessage);
            Createmethod(59, SteamFriends, GetClanChatMessage);
            Createmethod(60, SteamFriends, IsClanChatAdmin);
            Createmethod(61, SteamFriends, IsClanChatWindowOpenInSteam);
            Createmethod(62, SteamFriends, OpenClanChatWindowInSteam);
            Createmethod(63, SteamFriends, CloseClanChatWindowInSteam);
            Createmethod(64, SteamFriends, SetListenForFriendsMessages);
            Createmethod(65, SteamFriends, ReplyToFriendMessage);
            Createmethod(66, SteamFriends, GetChatMessage);
            Createmethod(67, SteamFriends, GetFollowerCount);
            Createmethod(68, SteamFriends, IsFollowing);
            Createmethod(69, SteamFriends, EnumerateFollowingList);
        };
    };
    struct SteamFriends016 : Interface_t
    {
        SteamFriends016()
        {
            Createmethod(0, SteamFriends,    GetPersonaName);
            Createmethod(1, SteamFriends,    SetPersonaName1);
            Createmethod(2, SteamFriends,    GetPersonaState);
            Createmethod(3, SteamFriends,    GetFriendCount1);
            Createmethod(4, SteamFriends,    GetFriendByIndex1);
            Createmethod(5, SteamFriends,    GetFriendRelationship);
            Createmethod(6, SteamFriends,    GetFriendPersonaState);
            Createmethod(7, SteamFriends,    GetFriendPersonaName);
            Createmethod(8, SteamFriends,    GetFriendGamePlayed0);
            Createmethod(9, SteamFriends,    GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends,   GetFriendSteamLevel);
            Createmethod(11, SteamFriends,   GetPlayerNickname);
            Createmethod(12, SteamFriends,   GetFriendsGroupCount);
            Createmethod(13, SteamFriends,   GetFriendsGroupIDByIndex);
            Createmethod(14, SteamFriends,   GetFriendsGroupName);
            Createmethod(15, SteamFriends,   GetFriendsGroupMembersCount);
            Createmethod(16, SteamFriends,   GetFriendsGroupMembersList);
            Createmethod(17, SteamFriends,   HasFriend1);
            Createmethod(18, SteamFriends,   GetClanCount);
            Createmethod(19, SteamFriends,   GetClanByIndex);
            Createmethod(20, SteamFriends,   GetClanName);
            Createmethod(21, SteamFriends,   GetClanTag);
            Createmethod(22, SteamFriends,   GetClanActivityCounts);
            Createmethod(23, SteamFriends,   DownloadClanActivityCounts);
            Createmethod(24, SteamFriends,   GetFriendCountFromSource);
            Createmethod(25, SteamFriends,   GetFriendFromSourceByIndex);
            Createmethod(26, SteamFriends,   IsUserInSource);
            Createmethod(27, SteamFriends,   SetInGameVoiceSpeaking);
            Createmethod(28, SteamFriends,   ActivateGameOverlay);
            Createmethod(29, SteamFriends,   ActivateGameOverlayToUser);
            Createmethod(30, SteamFriends,   ActivateGameOverlayToWebPage1);
            Createmethod(31, SteamFriends,   ActivateGameOverlayToStore1);
            Createmethod(32, SteamFriends,   SetPlayedWith);
            Createmethod(33, SteamFriends,   ActivateGameOverlayInviteDialog);
            Createmethod(34, SteamFriends,   GetSmallFriendAvatar);
            Createmethod(35, SteamFriends,   GetMediumFriendAvatar);
            Createmethod(36, SteamFriends,   GetLargeFriendAvatar);
            Createmethod(37, SteamFriends,   RequestUserInformation);
            Createmethod(38, SteamFriends,   RequestClanOfficerList);
            Createmethod(39, SteamFriends,   GetClanOwner);
            Createmethod(40, SteamFriends,   GetClanOfficerCount);
            Createmethod(41, SteamFriends,   GetClanOfficerByIndex);
            Createmethod(42, SteamFriends,   GetUserRestrictions);
            Createmethod(43, SteamFriends,   SetRichPresence);
            Createmethod(44, SteamFriends,   ClearRichPresence);
            Createmethod(45, SteamFriends,   GetFriendRichPresence);
            Createmethod(46, SteamFriends,   GetFriendRichPresenceKeyCount);
            Createmethod(47, SteamFriends,   GetFriendRichPresenceKeyByIndex);
            Createmethod(48, SteamFriends,   RequestFriendRichPresence);
            Createmethod(49, SteamFriends,   InviteUserToGame);
            Createmethod(50, SteamFriends,   GetCoplayFriendCount);
            Createmethod(51, SteamFriends,   GetCoplayFriend);
            Createmethod(52, SteamFriends,   GetFriendCoplayTime);
            Createmethod(53, SteamFriends,   GetFriendCoplayGame);
            Createmethod(54, SteamFriends,   JoinClanChatRoom);
            Createmethod(55, SteamFriends,   LeaveClanChatRoom);
            Createmethod(56, SteamFriends,   GetClanChatMemberCount);
            Createmethod(57, SteamFriends,   GetChatMemberByIndex);
            Createmethod(58, SteamFriends,   SendClanChatMessage);
            Createmethod(59, SteamFriends,   GetClanChatMessage);
            Createmethod(60, SteamFriends,   IsClanChatAdmin);
            Createmethod(61, SteamFriends,   IsClanChatWindowOpenInSteam);
            Createmethod(62, SteamFriends,   OpenClanChatWindowInSteam);
            Createmethod(63, SteamFriends,   CloseClanChatWindowInSteam);
            Createmethod(64, SteamFriends,   SetListenForFriendsMessages);
            Createmethod(65, SteamFriends,   ReplyToFriendMessage);
            Createmethod(66, SteamFriends,   GetFriendMessage);
            Createmethod(67, SteamFriends,   GetFollowerCount);
            Createmethod(68, SteamFriends,   IsFollowing);
            Createmethod(69, SteamFriends,   EnumerateFollowingList);

            Createmethod(70, SteamFriends,   IsClanPublic);
            Createmethod(71, SteamFriends,   IsClanOfficialGameGroup);
            Createmethod(72, SteamFriends,   GetNumChatsWithUnreadPriorityMessages);
        };
    };
    struct SteamFriends017 : Interface_t
    {
        SteamFriends017()
        {
            Createmethod(0, SteamFriends,    GetPersonaName);
            Createmethod(1, SteamFriends,    SetPersonaName1);
            Createmethod(2, SteamFriends,    GetPersonaState);
            Createmethod(3, SteamFriends,    GetFriendCount1);
            Createmethod(4, SteamFriends,    GetFriendByIndex1);
            Createmethod(5, SteamFriends,    GetFriendRelationship);
            Createmethod(6, SteamFriends,    GetFriendPersonaState);
            Createmethod(7, SteamFriends,    GetFriendPersonaName);
            Createmethod(8, SteamFriends,    GetFriendGamePlayed);
            Createmethod(9, SteamFriends,    GetFriendPersonaNameHistory);
            Createmethod(10, SteamFriends,   GetFriendSteamLevel);
            Createmethod(11, SteamFriends,   GetPlayerNickname);
            Createmethod(12, SteamFriends,   GetFriendsGroupCount);
            Createmethod(13, SteamFriends,   GetFriendsGroupIDByIndex);
            Createmethod(14, SteamFriends,   GetFriendsGroupName);
            Createmethod(15, SteamFriends,   GetFriendsGroupMembersCount);
            Createmethod(16, SteamFriends,   GetFriendsGroupMembersList);
            Createmethod(17, SteamFriends,   HasFriend1);
            Createmethod(18, SteamFriends,   GetClanCount);
            Createmethod(19, SteamFriends,   GetClanByIndex);
            Createmethod(20, SteamFriends,   GetClanName);
            Createmethod(21, SteamFriends,   GetClanTag);
            Createmethod(22, SteamFriends,   GetClanActivityCounts);
            Createmethod(23, SteamFriends,   DownloadClanActivityCounts);
            Createmethod(24, SteamFriends,   GetFriendCountFromSource);
            Createmethod(25, SteamFriends,   GetFriendFromSourceByIndex);
            Createmethod(26, SteamFriends,   IsUserInSource);
            Createmethod(27, SteamFriends,   SetInGameVoiceSpeaking);
            Createmethod(28, SteamFriends,   ActivateGameOverlay);
            Createmethod(29, SteamFriends,   ActivateGameOverlayToUser);
            Createmethod(30, SteamFriends,   ActivateGameOverlayToWebPage1);
            Createmethod(31, SteamFriends,   ActivateGameOverlayToStore1);
            Createmethod(32, SteamFriends,   SetPlayedWith);
            Createmethod(33, SteamFriends,   ActivateGameOverlayInviteDialog);
            Createmethod(34, SteamFriends,   GetSmallFriendAvatar);
            Createmethod(35, SteamFriends,   GetMediumFriendAvatar);
            Createmethod(36, SteamFriends,   GetLargeFriendAvatar);
            Createmethod(37, SteamFriends,   RequestUserInformation);
            Createmethod(38, SteamFriends,   RequestClanOfficerList);
            Createmethod(39, SteamFriends,   GetClanOwner);
            Createmethod(40, SteamFriends,   GetClanOfficerCount);
            Createmethod(41, SteamFriends,   GetClanOfficerByIndex);
            Createmethod(42, SteamFriends,   GetUserRestrictions);
            Createmethod(43, SteamFriends,   SetRichPresence);
            Createmethod(44, SteamFriends,   ClearRichPresence);
            Createmethod(45, SteamFriends,   GetFriendRichPresence);
            Createmethod(46, SteamFriends,   GetFriendRichPresenceKeyCount);
            Createmethod(47, SteamFriends,   GetFriendRichPresenceKeyByIndex);
            Createmethod(48, SteamFriends,   RequestFriendRichPresence);
            Createmethod(49, SteamFriends,   InviteUserToGame);
            Createmethod(50, SteamFriends,   GetCoplayFriendCount);
            Createmethod(51, SteamFriends,   GetCoplayFriend);
            Createmethod(52, SteamFriends,   GetFriendCoplayTime);
            Createmethod(53, SteamFriends,   GetFriendCoplayGame);
            Createmethod(54, SteamFriends,   JoinClanChatRoom);
            Createmethod(55, SteamFriends,   LeaveClanChatRoom);
            Createmethod(56, SteamFriends,   GetClanChatMemberCount);
            Createmethod(57, SteamFriends,   GetChatMemberByIndex);
            Createmethod(58, SteamFriends,   SendClanChatMessage);
            Createmethod(59, SteamFriends,   GetClanChatMessage);
            Createmethod(60, SteamFriends,   IsClanChatAdmin);
            Createmethod(61, SteamFriends,   IsClanChatWindowOpenInSteam);
            Createmethod(62, SteamFriends,   OpenClanChatWindowInSteam);
            Createmethod(63, SteamFriends,   CloseClanChatWindowInSteam);
            Createmethod(64, SteamFriends,   SetListenForFriendsMessages);
            Createmethod(65, SteamFriends,   ReplyToFriendMessage);
            Createmethod(66, SteamFriends,   GetFriendMessage);
            Createmethod(67, SteamFriends,   GetFollowerCount);
            Createmethod(68, SteamFriends,   IsFollowing);
            Createmethod(69, SteamFriends,   EnumerateFollowingList);

            Createmethod(70, SteamFriends,   IsClanPublic);
            Createmethod(71, SteamFriends,   IsClanOfficialGameGroup);
            Createmethod(72, SteamFriends,   GetNumChatsWithUnreadPriorityMessages);

            Createmethod(73, SteamFriends,   ActivateGameOverlayRemotePlayTogetherInviteDialog);
            Createmethod(74, SteamFriends,   RegisterProtocolInOverlayBrowser);
            Createmethod(75, SteamFriends,   ActivateGameOverlayInviteDialogConnectString);
        };
    };

    struct Steamfriendsloader
    {
        Steamfriendsloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::FRIENDS, "SteamFriends001", SteamFriends001);
            Register(Interfacetype_t::FRIENDS, "SteamFriends002", SteamFriends002);
            Register(Interfacetype_t::FRIENDS, "SteamFriends003", SteamFriends003);
            Register(Interfacetype_t::FRIENDS, "SteamFriends004", SteamFriends004);
            Register(Interfacetype_t::FRIENDS, "SteamFriends005", SteamFriends005);
            Register(Interfacetype_t::FRIENDS, "SteamFriends006", SteamFriends006);
            Register(Interfacetype_t::FRIENDS, "SteamFriends007", SteamFriends007);
            Register(Interfacetype_t::FRIENDS, "SteamFriends008", SteamFriends008);
            Register(Interfacetype_t::FRIENDS, "SteamFriends009", SteamFriends009);
            Register(Interfacetype_t::FRIENDS, "SteamFriends010", SteamFriends010);
            Register(Interfacetype_t::FRIENDS, "SteamFriends011", SteamFriends011);
            Register(Interfacetype_t::FRIENDS, "SteamFriends012", SteamFriends012);
            Register(Interfacetype_t::FRIENDS, "SteamFriends013", SteamFriends013);
            Register(Interfacetype_t::FRIENDS, "SteamFriends014", SteamFriends014);
            Register(Interfacetype_t::FRIENDS, "SteamFriends015", SteamFriends015);
            Register(Interfacetype_t::FRIENDS, "SteamFriends016", SteamFriends016);
            Register(Interfacetype_t::FRIENDS, "SteamFriends017", SteamFriends017);
        }
    };
    static Steamfriendsloader Interfaceloader{};
}
