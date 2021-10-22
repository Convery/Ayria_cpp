/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    enum ELobbyDistanceFilter
    {
        k_ELobbyDistanceFilterClose,		// only lobbies in the same immediate region will be returned
        k_ELobbyDistanceFilterDefault,		// only lobbies in the same region or near by regions
        k_ELobbyDistanceFilterFar,			// for games that don't have many latency requirements, will return lobbies about half-way around the globe
        k_ELobbyDistanceFilterWorldwide,	// no filtering, will match lobbies as far as India to NY (not recommended, expect multiple seconds of latency between the clients)
    };
    enum ELobbyComparison
    {
        k_ELobbyComparisonEqualToOrLessThan = -2,
        k_ELobbyComparisonLessThan = -1,
        k_ELobbyComparisonEqual = 0,
        k_ELobbyComparisonGreaterThan = 1,
        k_ELobbyComparisonEqualToOrGreaterThan = 2,
        k_ELobbyComparisonNotEqual = 3,
    };
    enum EChatEntryType
    {
        k_EChatEntryTypeInvalid = 0,
        k_EChatEntryTypeChatMsg = 1,		    // Normal text message from another user
        k_EChatEntryTypeTyping = 2,			    // Another user is typing (not used in multi-user chat)
        k_EChatEntryTypeInviteGame = 3,		    // Invite from other user into that users current game
        k_EChatEntryTypeEmote = 4,			    // text emote message (deprecated, should be treated as ChatMsg)
        //k_EChatEntryTypeLobbyGameStart = 5,	// lobby game is starting (dead - listen for LobbyGameCreated_t callback instead)
        k_EChatEntryTypeLeftConversation = 6,   // user has left the conversation ( closed chat window )
        // Above are previous FriendMsgType entries, now merged into more generic chat entry types
        k_EChatEntryTypeEntered = 7,		    // user has entered the conversation (used in multi-user chat and group chat)
        k_EChatEntryTypeWasKicked = 8,		    // user was kicked (data: 64-bit steamid of actor performing the kick)
        k_EChatEntryTypeWasBanned = 9,		    // user was banned (data: 64-bit steamid of actor performing the ban)
        k_EChatEntryTypeDisconnected = 10,	    // user disconnected
        k_EChatEntryTypeHistoricalChat = 11,	// a chat message from user's chat history or offilne message
        //k_EChatEntryTypeReserved1 = 12,       // No longer used
        //k_EChatEntryTypeReserved2 = 13,       // No longer used
        k_EChatEntryTypeLinkBlocked = 14,       // a link was removed by the chat filter.
    };
    enum ELobbyType
    {
        k_ELobbyTypePrivate = 0,		// only way to join the lobby is to invite to someone else
        k_ELobbyTypeFriendsOnly = 1,	// shows for friends or invitees, but not in lobby list
        k_ELobbyTypePublic = 2,			// visible for friends and in lobby list
        k_ELobbyTypeInvisible = 3,		// returned by search, but not visible to other friends
                                        //    useful if you want a user in two lobbies, for example matching groups together
                                        //	  a user can be in only one regular lobby, and up to two invisible lobbies
        k_ELobbyTypePrivateUnique = 4,	// private, unique and does not delete when empty - only one of these may exist per unique keypair set
                                        // can only create from webapi
    };

    struct SteamMatchmaking
    {
        SteamAPICall_t CreateLobby2(ELobbyType eLobbyType);
        SteamAPICall_t CreateLobby3(ELobbyType eLobbyType, int cMaxMembers);
        SteamAPICall_t JoinLobby1(SteamID_t steamIDLobby);

        SteamID_t GetLobbyByIndex(int iLobby);
        SteamID_t GetLobbyMemberByIndex(SteamID_t steamIDLobby, int iMember);
        SteamID_t GetLobbyOwner(SteamID_t steamIDLobby);

        bool DeleteLobbyData(SteamID_t steamIDLobby, const char *pchKey);
        bool GetFavoriteGame0(int iGame, AppID_t *pnAppID, uint32_t *pnIP, uint16_t *pnConnPort, uint32_t *punFlags, uint32_t *pRTime32LastPlayedOnServer)
        { return false;}
        bool GetFavoriteGame1(int iGame, AppID_t *pnAppID, uint32_t *pnIP, uint16_t *pnConnPort, uint16_t *pnQueryPort, uint32_t *punFlags, uint32_t *pRTime32LastPlayedOnServer)
        { return false; }
        bool GetLobbyDataByIndex(SteamID_t steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize);
        bool GetLobbyGameServer(SteamID_t steamIDLobby, uint32_t *punGameServerIP, uint16_t *punGameServerPort, SteamID_t *psteamIDGameServer);
        bool InviteUserToLobby(SteamID_t steamIDLobby, SteamID_t steamIDInvitee);
        bool RemoveFavoriteGame0(AppID_t nAppID, uint32_t nIP, uint16_t nConnPort, uint32_t unFlags)
        { return true; }
        bool RemoveFavoriteGame1(AppID_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags)
        { return true; }
        bool RequestFriendsLobbies()
        {
            return true;
        }
        bool RequestLobbyData(SteamID_t steamIDLobby)
        {
            return true;
        }
        bool SendLobbyChatMsg(SteamID_t steamIDLobby, const void *pvMsgBody, int cubMsgBody);
        bool SetLinkedLobby(SteamID_t steamIDLobby, SteamID_t steamIDLobbyDependent);
        bool SetLobbyData1(SteamID_t steamIDLobby, const char *pchKey, const char *pchValue);
        bool SetLobbyJoinable(SteamID_t steamIDLobby, bool bLobbyJoinable);
        bool SetLobbyMemberLimit(SteamID_t steamIDLobby, int cMaxMembers);
        bool SetLobbyOwner(SteamID_t steamIDLobby, SteamID_t steamIDNewOwner);
        bool SetLobbyType(SteamID_t steamIDLobby, ELobbyType eLobbyType);

        const char *GetLobbyData(SteamID_t steamIDLobby, const char *pchKey);
        const char *GetLobbyMemberData(SteamID_t steamIDLobby, SteamID_t steamIDUser, const char *pchKey);

        int AddFavoriteGame0(AppID_t nAppID, uint32_t nIP, uint16_t nConnPort, uint32_t unFlags, uint32_t rTime32LastPlayedOnServer);
        int AddFavoriteGame1(AppID_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags, uint32_t rTime32LastPlayedOnServer);
        int GetFavoriteGameCount();
        int GetLobbyChatEntry(SteamID_t steamIDLobby, int iChatID, SteamID_t *pSteamIDUser, void *pvData, int cubData, EChatEntryType *peChatEntryType);
        int GetLobbyDataCount(SteamID_t steamIDLobby);
        int GetLobbyMemberLimit(SteamID_t steamIDLobby);
        int GetNumLobbyMembers(SteamID_t steamIDLobby);

        double GetLobbyDistance(SteamID_t steamIDLobby)
        {
            // DEPRECATED
            return 0.0;
        }

        void AddRequestLobbyListCompatibleMembersFilter(SteamID_t steamIDLobby);
        void AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter eLobbyDistanceFilter);
        void AddRequestLobbyListFilter(const char *pchKeyToMatch, const char *pchValueToMatch);
        void AddRequestLobbyListFilterSlotsAvailable(int nSlotsAvailable);
        void AddRequestLobbyListNearValueFilter(const char *pchKeyToMatch, int nValueToBeCloseTo);
        void AddRequestLobbyListNumericalFilter(const char *pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType);
        void AddRequestLobbyListResultCountFilter(int cMaxResults);
        void AddRequestLobbyListSlotsAvailableFilter();
        void AddRequestLobbyListStringFilter(const char *pchKeyToMatch, const char *pchValueToMatch, ELobbyComparison eComparisonType);
        void ChangeLobbyAdmin(SteamID_t steamIDLobby, SteamID_t steamIDNewAdmin);
        void CreateLobby0(uint64_t ulGameID, bool bPrivate);
        void CreateLobby1(bool bPrivate);
        void JoinLobby0(SteamID_t steamIDLobby)
        {
            JoinLobby1(steamIDLobby);
        }
        void LeaveLobby(SteamID_t steamIDLobby);
        void RequestLobbyList0(uint64_t ulGameID, MatchMakingKeyValuePair_t *pFilters, uint32_t nFilters)
        {
        }
        void RequestLobbyList1()
        {
        }
        void SetLobbyData0(SteamID_t steamIDLobby, const char *pchKey, const char *pchValue)
        {
            SetLobbyData1(steamIDLobby, pchKey, pchValue);
        }
        void SetLobbyGameServer(SteamID_t steamIDLobby, uint32_t unGameServerIP, uint16_t unGameServerPort, SteamID_t steamIDGameServer);
        void SetLobbyMemberData(SteamID_t steamIDLobby, const char *pchKey, const char *pchValue);
        void SetLobbyVoiceEnabled(SteamID_t steamIDLobby, bool bEnabled);
    };


    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    /* struct SteamMatchmaking2
    {
        int GetFavoriteGameCount()
        {
            Traceprint();
            return 0;
        }
        bool GetFavoriteGame0(int iGame, uint32_t *pnAppID, uint32_t *pnIP, uint16_t *pnConnPort, uint32_t *punFlags, uint32_t *pRTime32LastPlayedOnServer)
        {
            Traceprint();
            return false;
        }
        int AddFavoriteGame0(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint32_t unFlags, uint32_t rTime32LastPlayedOnServer)
        {
            Traceprint();
            return 0;
        }
        bool RemoveFavoriteGame0(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint32_t unFlags)
        {
            Traceprint();
            return false;
        }
        bool GetFavoriteGame2(int iGame, uint32_t *pnAppID, uint32_t *pnIP, uint16_t *pnConnPort, uint16_t *pnQueryPort, uint32_t *punFlags, uint32_t *pRTime32LastPlayedOnServer)
        {
            Traceprint();
            return false;
        }
        int AddFavoriteGame2(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags, uint32_t rTime32LastPlayedOnServer)
        {
            Traceprint();
            return false;
        }
        bool RemoveFavoriteGame2(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags)
        {
            Traceprint();
            return false;
        }
        void RequestLobbyList0(uint64_t ulGameID, struct MatchMakingKeyValuePair_t *pFilters, uint32_t nFilters)
        {
            Traceprint();
        }
        CSteamID GetLobbyByIndex(int iLobby)
        {
            const auto Serverlist = Matchmaking::getLANSessions();
            if ((int)Serverlist.size() < iLobby) return k_steamIDNil;

            return CSteamID(Serverlist.at(iLobby)->HostID, 1, k_EAccountTypeGameServer);
        }
        void CreateLobby0(uint64_t ulGameID, bool bPrivate)
        {
            auto Session = Matchmaking::getLocalsession();
            Session->Steam.isPrivate = bPrivate;
            Matchmaking::Invalidatesession();
        }
        void JoinLobby0(CSteamID steamIDLobby)
        {
            Traceprint();
        }
        void LeaveLobby(CSteamID steamIDLobby)
        {
            Traceprint();
        }
        bool InviteUserToLobby(CSteamID steamIDLobby, CSteamID steamIDInvitee)
        {
            Traceprint();
            return false;
        }
        int GetNumLobbyMembers(CSteamID steamIDLobby)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            for (const auto &Session : Matchmaking::getLANSessions())
                if (HostID == Session->HostID)
                    return int(Session->Players.size());

            return 0;
        }
        CSteamID GetLobbyMemberByIndex(CSteamID steamIDLobby, int iMember)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            for (const auto &Session : Matchmaking::getLANSessions())
            {
                if (HostID == Session->HostID)
                {
                    if (iMember > (int)Session->Players.size()) return k_steamIDNil;
                    return CSteamID(Session->Players.at(iMember).PlayerID, 1, k_EAccountTypeIndividual);
                }
            }

            return k_steamIDNil;
        }
        const char *GetLobbyData(CSteamID SteamIDLobby, const char *pchKey)
        {
            const auto HostID = SteamIDLobby.GetAccountID();

            if (HostID == Steam.XUID.GetAccountID())
            {
                static std::string Result;
                Result = Matchmaking::getLocalsession()->Steam.Keyvalues.value(pchKey, "").c_str();
                return Result.c_str();
            }
            else
            {
                for (const auto &Session : Matchmaking::getLANSessions())
                {
                    if (HostID == Session->HostID)
                    {
                        static std::string Result;
                        Result = Session->Steam.Keyvalues.value(pchKey, "");
                        return Result.c_str();
                    }
                }
            }

            return "";
        }
        void SetLobbyData0(CSteamID steamIDLobby, const char *pchKey, const char *pchValue) const
        {
            const auto HostID = steamIDLobby.GetAccountID();
            if (HostID == Steam.XUID.GetAccountID())
            {
                Infoprint(va("%s - Key: \"%s\" - Value: \"%s\"", __FUNCTION__, pchKey, pchValue));
                Matchmaking::getLocalsession()->Steam.Keyvalues[pchKey] = pchValue;
                Matchmaking::Invalidatesession();
            }
        }
        const char *GetLobbyMemberData(CSteamID steamIDLobby, CSteamID steamIDUser, const char *pchKey)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            const auto UserID = steamIDUser.GetAccountID();

            if (HostID == Steam.XUID.GetAccountID())
            {
                for (auto &Item : Matchmaking::getLocalsession()->Players)
                {
                    if(Item.PlayerID == UserID)
                    {
                        return Item.Gamedata.value(pchKey, "").c_str();
                    }
                }
                return "";
            }
            else
            {
                for (const auto &Session : Matchmaking::getLANSessions())
                {
                    if (HostID == Session->HostID)
                    {
                        for (const auto &Item : Session->Players)
                        {
                            if (Item.PlayerID == UserID)
                            {
                                return Item.Gamedata.value(pchKey, "").c_str();
                            }
                        }

                        break;
                    }
                }
            }
            return "";
        }
        void SetLobbyMemberData(CSteamID steamIDLobby, const char *pchKey, const char *pchValue) const
        {
            // TODO(tcn): This should probably be implemented via Ayria::Clientinfo::Publicdata or something.

            const auto HostID = steamIDLobby.GetAccountID();
            const auto UserID = Steam.XUID.GetAccountID();

            if (HostID == Steam.XUID.GetAccountID())
            {
                Infoprint(va("%s - Key: \"%s\" - Value: \"%s\"", __FUNCTION__, pchKey, pchValue));
                for (auto &Player : Matchmaking::getLocalsession()->Players)
                {
                    if (Player.PlayerID == UserID)
                    {
                        Player.Gamedata[pchKey] = pchValue;
                        break;
                    }
                }

                Matchmaking::Invalidatesession();
            }
        }
        void ChangeLobbyAdmin(CSteamID steamIDLobby, CSteamID steamIDNewAdmin)
        {
            Traceprint();
        }
        bool SendLobbyChatMsg(CSteamID steamIDLobby, const void *pvMsgBody, int cubMsgBody)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            std::vector<uint32_t> Users;

            if (HostID == Steam.XUID.GetAccountID())
            {
                for (const auto &Item : Matchmaking::getLocalsession()->Players)
                {
                    Users.push_back(Item.PlayerID);
                }
            }
            else
            {
                for (const auto &Session : Matchmaking::getLANSessions())
                {
                    if (HostID == Session->HostID)
                    {
                        for (const auto &Item : Session->Players)
                        {
                            Users.push_back(Item.PlayerID);
                        }

                        break;
                    }
                }
            }

            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Payload = nlohmann::json::object({ { "Type", Hash::FNV1_32("Steamlobbychat")},
                    { "Body", std::string((const char *)pvMsgBody, cubMsgBody) },
                    { "GroupID", steamIDLobby.GetAccountID()} });
                const auto Object = nlohmann::json::object({ { "Targets", Users}, { "Message", Payload.dump() } });
                const auto Result = Callback(Ayria.toFunctionID("Sendmessage"), DumpJSON(Object).c_str());
                return !!std::strstr(Result, "OK");
            }

            return true;
        }
        int GetLobbyChatEntry(CSteamID steamIDLobby, int iChatID, CSteamID *pSteamIDUser, void *pvData, int cubData, uint32_t *peChatEntryType)
        {
            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Result = Callback(Ayria.toFunctionID("Readmessages"), nullptr);
                const auto LobbyID = steamIDLobby.GetAccountID();
                const auto Array = ParseJSON(Result);

                for (const auto &Message : Array)
                {
                    const auto Payload = ParseJSON(Message.value("Message", std::string()));
                    const auto GroupID = Payload.value("GroupID", uint32_t());
                    const auto Type = Payload.value("Type", uint32_t());

                    // Skip to relevant group messages.
                    if (Hash::FNV1_32("Steamlobbychat") != Type) continue;
                    if (GroupID != LobbyID) continue;
                    if (iChatID--) continue;

                    const auto Body = Payload.value("Body", std::string());
                    const auto Size = std::min(Body.size(), size_t(cubData));
                    std::memcpy(pvData, Body.data(), Size);

                    const auto Sender = Message.value("Source", uint64_t());
                    *pSteamIDUser = CSteamID(uint32_t(Sender & 0xFFFFFFFF), 1, k_EAccountTypeIndividual);
                    return int(Size);
                }
            }

            return 0;
        }
        bool RequestLobbyData(CSteamID steamIDLobby)
        {
            Traceprint();
            return false;
        }
        bool GetFavoriteGame1(int iGame, uint32_t *pnAppID, uint32_t *pnIP, uint16_t *pnConnPort, uint16_t *pnQueryPort, uint32_t *punFlags, uint32_t *pRTime32LastPlayedOnServer)
        {
            Traceprint();
            return false;
        }
        int AddFavoriteGame1(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags, uint32_t rTime32LastPlayedOnServer)
        {
            Traceprint();
            return 1;
        }
        bool RemoveFavoriteGame1(uint32_t nAppID, uint32_t nIP, uint16_t nConnPort, uint16_t nQueryPort, uint32_t unFlags)
        {
            Traceprint();
            return false;
        }
        void RequestLobbyList1()
        {
            Traceprint();
        }
        void CreateLobby1(bool bPrivate)
        {
            CreateLobby0(0, bPrivate);
        }
        void SetLobbyGameServer(CSteamID steamIDLobby, uint32_t unGameServerIP, uint16_t unGameServerPort, CSteamID steamIDGameServer) const
        {
            Traceprint();
        }
        uint64_t RequestLobbyList2()
        {
            Traceprint();
            return 0;
        }
        void AddRequestLobbyListFilter(const char *pchKeyToMatch, const char *pchValueToMatch)
        {
            Traceprint();
        }
        void AddRequestLobbyListNumericalFilter0(const char *pchKeyToMatch, int nValueToMatch, int nComparisonType)
        {
            Traceprint();
        }
        void AddRequestLobbyListSlotsAvailableFilter()
        {
            Traceprint();
        }
        bool SetLobbyData1(CSteamID steamIDLobby, const char *pchKey, const char *pchValue) const
        {
            SetLobbyData0(steamIDLobby, pchKey, pchValue);
            return true;
        }
        bool GetLobbyGameServer(CSteamID steamIDLobby, uint32_t *punGameServerIP, uint16_t *punGameServerPort, CSteamID *psteamIDGameServer)
        {
            Traceprint();
            return false;
        }
        bool SetLobbyMemberLimit(CSteamID steamIDLobby, int cMaxMembers)
        {
            if (steamIDLobby.GetAccountID() == Steam.XUID.GetAccountID())
            {
                Matchmaking::getLocalsession()->Steam.Maxplayers = cMaxMembers;
                Matchmaking::Invalidatesession();
                return true;
            }

            return false;
        }
        int GetLobbyMemberLimit(CSteamID steamIDLobby)
        {
            const auto HostID = steamIDLobby.GetAccountID();

            for (const auto &Session : Matchmaking::getLANSessions())
            {
                if (Session->HostID == HostID)
                    return Session->Steam.Maxplayers;
            }

            return 6;
        }
        void SetLobbyVoiceEnabled(CSteamID steamIDLobby, bool bEnabled)
        {
            Traceprint();
        }
        bool RequestFriendsLobbies()
        {
            Traceprint();
            return false;
        }
        void AddRequestLobbyListNearValueFilter(const char *pchKeyToMatch, int nValueToBeCloseTo)
        {
            Traceprint();
        }
        uint64_t CreateLobby2(uint32_t eLobbyType)
        {
            const auto LobbyID = CSteamID(Steam.XUID.GetAccountID(), 0x40000, 1, k_EAccountTypeGameServer);
            auto Response = new LobbyCreated_t();
            const auto RequestID = Createrequest();

            Response->m_eResult = EResult::k_EResultOK;
            Response->m_ulSteamIDLobby = LobbyID.ConvertToUint64();
            Infoprint(va("Creating a lobby of type %d.", eLobbyType));
            Completerequest(RequestID, k_iSteamMatchmakingCallbacks + 13, Response);

            Matchmaking::Invalidatesession();
            return RequestID;
        }
        uint64_t JoinLobby1(CSteamID steamIDLobby) const
        {
            Infoprint(va("Want to join lobby 0x%llX.", steamIDLobby.ConvertToUint64()));
            return 0;
        }
        bool SetLobbyType(CSteamID steamIDLobby, uint32_t eLobbyType)
        {
            Traceprint();
            return true;
        }
        CSteamID GetLobbyOwner(CSteamID steamIDLobby)
        {
            return CSteamID(steamIDLobby.GetAccountID(), 1, k_EAccountTypeIndividual);
        }
        double GetLobbyDistance(CSteamID steamIDLobby)
        {
            Traceprint();
            return 0.0;
        }
        void AddRequestLobbyListStringFilter(const char *pchKeyToMatch, const char *pchValueToMatch, uint32_t eComparisonType)
        {
            Traceprint();
        }
        void AddRequestLobbyListNumericalFilter1(const char *pchKeyToMatch, int nValueToMatch, uint32_t eComparisonType)
        {
            Traceprint();
        }
        void AddRequestLobbyListFilterSlotsAvailable(int nSlotsAvailable)
        {
            Traceprint();
        }
        uint64_t CreateLobby3(uint32_t eLobbyType, int cMaxMembers) const
        {
            const auto LobbyID = CSteamID(Steam.XUID.GetAccountID(), 0x40000, 1, k_EAccountTypeGameServer);
            auto Response = new LobbyCreated_t();
            const auto RequestID = Createrequest();

            Response->m_eResult = EResult::k_EResultOK;
            Response->m_ulSteamIDLobby = LobbyID.ConvertToUint64();
            Infoprint(va("Creating a lobby of type %d for %d players.", eLobbyType, cMaxMembers));
            Completerequest(RequestID, k_iSteamMatchmakingCallbacks + 13, Response);

            Matchmaking::getLocalsession()->Steam.Maxplayers = cMaxMembers;
            Matchmaking::Invalidatesession();
            return RequestID;
        }
        int GetLobbyDataCount(CSteamID steamIDLobby)
        {
            Traceprint();
            return false;
        }
        bool GetLobbyDataByIndex(CSteamID steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize)
        {
            Traceprint();
            return false;
        }
        bool DeleteLobbyData(CSteamID steamIDLobby, const char *pchKey)
        {
            Traceprint();
            return true;
        }
        bool SetLobbyJoinable(CSteamID steamIDLobby, bool bLobbyJoinable)
        {
            Traceprint();
            return true;
        }
        bool SetLobbyOwner(CSteamID steamIDLobby, CSteamID steamIDNewOwner)
        {
            Traceprint();
            return true;
        }
        void AddRequestLobbyListDistanceFilter(uint32_t eLobbyDistanceFilter)
        {
            Traceprint();
        }
        void AddRequestLobbyListResultCountFilter(int cMaxResults)
        {
            Traceprint();
        }
        void AddRequestLobbyListCompatibleMembersFilter(CSteamID steamID)
        {
            Traceprint();
        }
        bool SetLinkedLobby(CSteamID steamIDLobby, CSteamID steamIDLobby2)
        {
            Traceprint();
            return true;
        }
    }; */

    struct SteamMatchmaking001 : Interface_t<23>
    {
        SteamMatchmaking001()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame0);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame0);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame0);
            Createmethod(4, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(5, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(6, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(7, SteamMatchmaking, RequestLobbyList0);
            Createmethod(8, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(9, SteamMatchmaking, CreateLobby0);
            Createmethod(10, SteamMatchmaking, JoinLobby0);
            Createmethod(11, SteamMatchmaking, LeaveLobby);
            Createmethod(12, SteamMatchmaking, InviteUserToLobby);
            Createmethod(13, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(14, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(15, SteamMatchmaking, GetLobbyData);
            Createmethod(16, SteamMatchmaking, SetLobbyData0);
            Createmethod(17, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(18, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(19, SteamMatchmaking, ChangeLobbyAdmin);
            Createmethod(20, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(21, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(22, SteamMatchmaking, RequestLobbyData);
        };
    };
    struct SteamMatchmaking002 : Interface_t<20>
    {
        SteamMatchmaking002()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(7, SteamMatchmaking, JoinLobby0);
            Createmethod(8, SteamMatchmaking, LeaveLobby);
            Createmethod(9, SteamMatchmaking, InviteUserToLobby);
            Createmethod(10, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(11, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(12, SteamMatchmaking, GetLobbyData);
            Createmethod(13, SteamMatchmaking, SetLobbyData0);
            Createmethod(14, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(15, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(16, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(17, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(18, SteamMatchmaking, RequestLobbyData);
            Createmethod(19, SteamMatchmaking, SetLobbyGameServer);
        };
    };
    struct SteamMatchmaking003 : Interface_t<28>
    {
        SteamMatchmaking003()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListSlotsAvailableFilter);
            Createmethod(8, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(9, SteamMatchmaking, CreateLobby1);
            Createmethod(10, SteamMatchmaking, JoinLobby0);
            Createmethod(11, SteamMatchmaking, LeaveLobby);
            Createmethod(12, SteamMatchmaking, InviteUserToLobby);
            Createmethod(13, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(14, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(15, SteamMatchmaking, GetLobbyData);
            Createmethod(16, SteamMatchmaking, SetLobbyData1);
            Createmethod(17, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(18, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(19, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(20, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(21, SteamMatchmaking, RequestLobbyData);
            Createmethod(22, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(23, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(24, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(25, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(26, SteamMatchmaking, SetLobbyVoiceEnabled);
            Createmethod(27, SteamMatchmaking, RequestFriendsLobbies);
        };
    };
    struct SteamMatchmaking004 : Interface_t<27>
    {
        SteamMatchmaking004()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListSlotsAvailableFilter);
            Createmethod(8, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(9, SteamMatchmaking, CreateLobby1);
            Createmethod(10, SteamMatchmaking, JoinLobby0);
            Createmethod(11, SteamMatchmaking, LeaveLobby);
            Createmethod(12, SteamMatchmaking, InviteUserToLobby);
            Createmethod(13, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(14, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(15, SteamMatchmaking, GetLobbyData);
            Createmethod(16, SteamMatchmaking, SetLobbyData1);
            Createmethod(17, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(18, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(19, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(20, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(21, SteamMatchmaking, RequestLobbyData);
            Createmethod(22, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(23, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(24, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(25, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(26, SteamMatchmaking, RequestFriendsLobbies);
        };
    };
    struct SteamMatchmaking005 : Interface_t<31>
    {
        SteamMatchmaking005()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListSlotsAvailableFilter);
            Createmethod(8, SteamMatchmaking, AddRequestLobbyListNearValueFilter);
            Createmethod(9, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(10, SteamMatchmaking, CreateLobby2);
            Createmethod(11, SteamMatchmaking, JoinLobby1);
            Createmethod(12, SteamMatchmaking, LeaveLobby);
            Createmethod(13, SteamMatchmaking, InviteUserToLobby);
            Createmethod(14, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(15, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(16, SteamMatchmaking, GetLobbyData);
            Createmethod(17, SteamMatchmaking, SetLobbyData1);
            Createmethod(18, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(19, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(20, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(21, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(22, SteamMatchmaking, RequestLobbyData);
            Createmethod(23, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(24, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(25, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(26, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(27, SteamMatchmaking, RequestFriendsLobbies);
            Createmethod(28, SteamMatchmaking, SetLobbyType);
            Createmethod(29, SteamMatchmaking, GetLobbyOwner);
            Createmethod(30, SteamMatchmaking, GetLobbyDistance);
        };
    };
    struct SteamMatchmaking006 : Interface_t<28>
    {
        SteamMatchmaking006()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListNearValueFilter);
            Createmethod(8, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(9, SteamMatchmaking, CreateLobby2);
            Createmethod(10, SteamMatchmaking, JoinLobby1);
            Createmethod(11, SteamMatchmaking, LeaveLobby);
            Createmethod(12, SteamMatchmaking, InviteUserToLobby);
            Createmethod(13, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(14, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(15, SteamMatchmaking, GetLobbyData);
            Createmethod(16, SteamMatchmaking, SetLobbyData1);
            Createmethod(17, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(18, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(19, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(20, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(21, SteamMatchmaking, RequestLobbyData);
            Createmethod(22, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(23, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(24, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(25, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(26, SteamMatchmaking, SetLobbyType);
            Createmethod(27, SteamMatchmaking, GetLobbyOwner);
        };
    };
    struct SteamMatchmaking007 : Interface_t<34>
    {
        SteamMatchmaking007()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListNearValueFilter);
            Createmethod(8, SteamMatchmaking, AddRequestLobbyListFilterSlotsAvailable);
            Createmethod(9, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(10, SteamMatchmaking, CreateLobby3);
            Createmethod(11, SteamMatchmaking, JoinLobby1);
            Createmethod(12, SteamMatchmaking, LeaveLobby);
            Createmethod(13, SteamMatchmaking, InviteUserToLobby);
            Createmethod(14, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(15, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(16, SteamMatchmaking, GetLobbyData);
            Createmethod(17, SteamMatchmaking, SetLobbyData1);
            Createmethod(18, SteamMatchmaking, GetLobbyDataCount);
            Createmethod(19, SteamMatchmaking, GetLobbyDataByIndex);
            Createmethod(20, SteamMatchmaking, DeleteLobbyData);
            Createmethod(21, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(22, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(23, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(24, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(25, SteamMatchmaking, RequestLobbyData);
            Createmethod(26, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(27, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(28, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(29, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(30, SteamMatchmaking, SetLobbyType);
            Createmethod(31, SteamMatchmaking, SetLobbyJoinable);
            Createmethod(32, SteamMatchmaking, GetLobbyOwner);
            Createmethod(33, SteamMatchmaking, SetLobbyOwner);
        };
    };
    struct SteamMatchmaking008 : Interface_t<36>
    {
        SteamMatchmaking008()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListNearValueFilter);
            Createmethod(8, SteamMatchmaking, AddRequestLobbyListFilterSlotsAvailable);
            Createmethod(9, SteamMatchmaking, AddRequestLobbyListDistanceFilter);
            Createmethod(10, SteamMatchmaking, AddRequestLobbyListResultCountFilter);
            Createmethod(11, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(12, SteamMatchmaking, CreateLobby3);
            Createmethod(13, SteamMatchmaking, JoinLobby1);
            Createmethod(14, SteamMatchmaking, LeaveLobby);
            Createmethod(15, SteamMatchmaking, InviteUserToLobby);
            Createmethod(16, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(17, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(18, SteamMatchmaking, GetLobbyData);
            Createmethod(19, SteamMatchmaking, SetLobbyData1);
            Createmethod(20, SteamMatchmaking, GetLobbyDataCount);
            Createmethod(21, SteamMatchmaking, GetLobbyDataByIndex);
            Createmethod(22, SteamMatchmaking, DeleteLobbyData);
            Createmethod(23, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(24, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(25, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(26, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(27, SteamMatchmaking, RequestLobbyData);
            Createmethod(28, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(29, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(30, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(31, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(32, SteamMatchmaking, SetLobbyType);
            Createmethod(33, SteamMatchmaking, SetLobbyJoinable);
            Createmethod(34, SteamMatchmaking, GetLobbyOwner);
            Createmethod(35, SteamMatchmaking, SetLobbyOwner);
        };
    };
    struct SteamMatchmaking009 : Interface_t<38>
    {
        SteamMatchmaking009()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList1);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter);
            Createmethod(7, SteamMatchmaking, AddRequestLobbyListNearValueFilter);
            Createmethod(8, SteamMatchmaking, AddRequestLobbyListFilterSlotsAvailable);
            Createmethod(9, SteamMatchmaking, AddRequestLobbyListDistanceFilter);
            Createmethod(10, SteamMatchmaking, AddRequestLobbyListResultCountFilter);
            Createmethod(11, SteamMatchmaking, AddRequestLobbyListCompatibleMembersFilter);
            Createmethod(12, SteamMatchmaking, GetLobbyByIndex);
            Createmethod(13, SteamMatchmaking, CreateLobby3);
            Createmethod(14, SteamMatchmaking, JoinLobby1);
            Createmethod(15, SteamMatchmaking, LeaveLobby);
            Createmethod(16, SteamMatchmaking, InviteUserToLobby);
            Createmethod(17, SteamMatchmaking, GetNumLobbyMembers);
            Createmethod(18, SteamMatchmaking, GetLobbyMemberByIndex);
            Createmethod(19, SteamMatchmaking, GetLobbyData);
            Createmethod(20, SteamMatchmaking, SetLobbyData1);
            Createmethod(21, SteamMatchmaking, GetLobbyDataCount);
            Createmethod(22, SteamMatchmaking, GetLobbyDataByIndex);
            Createmethod(23, SteamMatchmaking, DeleteLobbyData);
            Createmethod(24, SteamMatchmaking, GetLobbyMemberData);
            Createmethod(25, SteamMatchmaking, SetLobbyMemberData);
            Createmethod(26, SteamMatchmaking, SendLobbyChatMsg);
            Createmethod(27, SteamMatchmaking, GetLobbyChatEntry);
            Createmethod(28, SteamMatchmaking, RequestLobbyData);
            Createmethod(29, SteamMatchmaking, SetLobbyGameServer);
            Createmethod(30, SteamMatchmaking, GetLobbyGameServer);
            Createmethod(31, SteamMatchmaking, SetLobbyMemberLimit);
            Createmethod(32, SteamMatchmaking, GetLobbyMemberLimit);
            Createmethod(33, SteamMatchmaking, SetLobbyType);
            Createmethod(34, SteamMatchmaking, SetLobbyJoinable);
            Createmethod(35, SteamMatchmaking, GetLobbyOwner);
            Createmethod(36, SteamMatchmaking, SetLobbyOwner);
            Createmethod(37, SteamMatchmaking, SetLinkedLobby);
        };
    };

    struct Steammatchmakingloader
    {
        Steammatchmakingloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking001", SteamMatchmaking001);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking002", SteamMatchmaking002);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking003", SteamMatchmaking003);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking004", SteamMatchmaking004);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking005", SteamMatchmaking005);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking006", SteamMatchmaking006);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking007", SteamMatchmaking007);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking008", SteamMatchmaking008);
            Register(Interfacetype_t::MATCHMAKING, "SteamMatchmaking009", SteamMatchmaking009);
        }
    };
    static Steammatchmakingloader Interfaceloader{};
}
