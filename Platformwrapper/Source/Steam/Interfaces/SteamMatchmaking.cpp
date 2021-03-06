/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamMatchmaking
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
            const auto Serverlist = Matchmaking::getNetworkservers();
            if (Serverlist->size() < iLobby) return k_steamIDNil;

            return CSteamID(Serverlist->at(iLobby).HostID, 1, k_EAccountTypeGameServer);
        }
        void CreateLobby0(uint64_t ulGameID, bool bPrivate)
        {
            auto Session = Matchmaking::getLocalsession();
            Session->Hostinfo["isPrivate"] = bPrivate;
            Matchmaking::Update();
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
            for (const auto &Session : *Matchmaking::getNetworkservers())
                if (HostID == Session.HostID)
                    return int(Session.Playerdata.size());

            return 0;
        }
        CSteamID GetLobbyMemberByIndex(CSteamID steamIDLobby, int iMember)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            for (const auto &Session : *Matchmaking::getNetworkservers())
            {
                if (HostID == Session.HostID)
                {
                    if (iMember > Session.Playerdata.size()) return k_steamIDNil;
                    return CSteamID(Session.Playerdata.at(iMember).value("PlayerID", uint32_t()), 1, k_EAccountTypeIndividual);
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
                Result = Matchmaking::getLocalsession()->Sessiondata.value(pchKey, "").c_str();
                return Result.c_str();
            }
            else
            {
                for (const auto &Session : *Matchmaking::getNetworkservers())
                {
                    if (HostID == Session.HostID)
                    {
                        static std::string Result;
                        Result = Session.Sessiondata.value(pchKey, "");
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
                Matchmaking::getLocalsession()->Sessiondata[pchKey] = pchValue;
                Matchmaking::Update();
            }
        }
        const char *GetLobbyMemberData(CSteamID steamIDLobby, CSteamID steamIDUser, const char *pchKey)
        {
            const auto HostID = steamIDLobby.GetAccountID();
            const auto UserID = steamIDUser.GetAccountID();

            if (HostID == Steam.XUID.GetAccountID())
            {
                for (const auto &Item : Matchmaking::getLocalsession()->Playerdata)
                {
                    if (Item.value("PlayerID", uint32_t()) == UserID)
                    {
                        static std::string Result;
                        Result = Item.value("Steamdata", nlohmann::json::object()).value(pchKey, "").c_str();
                        return Result.c_str();
                    }
                }
                return "";
            }
            else
            {
                for (const auto &Session : *Matchmaking::getNetworkservers())
                {
                    if (HostID == Session.HostID)
                    {
                        for (const auto &Item : Session.Playerdata)
                        {
                            if (Item.value("PlayerID", uint32_t()) == UserID)
                            {
                                static std::string Result;
                                Result = Item.value("Steamdata", nlohmann::json::object()).value(pchKey, "").c_str();
                                return Result.c_str();
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
                for (auto &Player : Matchmaking::getLocalsession()->Playerdata)
                {
                    if (Player.value("PlayerID", uint32_t()) == UserID)
                    {
                        if (!Player.contains("Steamdata")) Player["Steamdata"] = nlohmann::json::object();
                        Player["Steamdata"][pchKey] = pchValue;
                        break;
                    }
                }

                Matchmaking::Update();
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
                for (const auto &Item : Matchmaking::getLocalsession()->Playerdata)
                {
                    const auto ID = Item.value("PlayerID", uint32_t());
                    if (ID) Users.push_back(ID);
                }
            }
            else
            {
                for (const auto &Session : *Matchmaking::getNetworkservers())
                {
                    if (HostID == Session.HostID)
                    {
                        for (const auto &Item : Session.Playerdata)
                        {
                            const auto ID = Item.value("PlayerID", uint32_t());
                            if (ID) Users.push_back(ID);
                        }

                        break;
                    }
                }
            }

            const std::string Safestring((const char *)pvMsgBody, cubMsgBody);
            if (const auto Callback = Ayria.API_Social)
            {
                auto Object = nlohmann::json::object();
                Object["Type"] = Hash::FNV1_32("Steamlobbychat");
                Object["Message"] = Safestring;
                Object["Clients"] = Users;

                Callback(Ayria.toFunctionID("SendIM"), Object.dump().c_str());
            }

            return true;
        }
        int GetLobbyChatEntry(CSteamID steamIDLobby, int iChatID, CSteamID *pSteamIDUser, void *pvData, int cubData, uint32_t  *peChatEntryType)
        {
            if (const auto Callback = Ayria.API_Social)
            {
                auto Object = nlohmann::json::object();
                Object["Type"] = Hash::FNV1_32("Steamlobbychat");
                Object["Offset"] = iChatID;
                Object["Count"] = 1;

                const auto Result = ParseJSON(Callback(Ayria.toFunctionID("ReadIM"), Object.dump().c_str()));
                const auto Value = Result.empty() ? nlohmann::json::object() : Result.at(0);
                if (!Value.contains("Message")) return 0;

                *peChatEntryType = Value.value("Type", 0);
                pSteamIDUser->Set(Value.value("Sender", 0), 1, k_EAccountTypeIndividual);
                std::strncpy((char *)pvData, Value["Message"].get<std::string>().c_str(), cubData);
                return (int)std::strlen((char *)pvData);
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
                Matchmaking::getLocalsession()->Gameinfo["Maxplayers"] = cMaxMembers;
                Matchmaking::Update();
                return true;
            }

            return false;
        }
        int GetLobbyMemberLimit(CSteamID steamIDLobby)
        {
            const auto HostID = steamIDLobby.GetAccountID();

            for (const auto &Session : *Matchmaking::getNetworkservers())
            {
                if (Session.HostID == HostID)
                    return Session.Gameinfo.value("Maxplayers", 6);
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
            auto Response = new Callbacks::LobbyCreated_t();
            const auto RequestID = Callbacks::Createrequest();

            Response->m_eResult = EResult::k_EResultOK;
            Response->m_ulSteamIDLobby = LobbyID.ConvertToUint64();
            Infoprint(va("Creating a lobby of type %d.", eLobbyType));
            Callbacks::Completerequest(RequestID, Callbacks::k_iSteamMatchmakingCallbacks + 13, Response);

            Matchmaking::Update();
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
            auto Response = new Callbacks::LobbyCreated_t();
            const auto RequestID = Callbacks::Createrequest();

            Response->m_eResult = EResult::k_EResultOK;
            Response->m_ulSteamIDLobby = LobbyID.ConvertToUint64();
            Infoprint(va("Creating a lobby of type %d for %d players.", eLobbyType, cMaxMembers));
            Callbacks::Completerequest(RequestID, Callbacks::k_iSteamMatchmakingCallbacks + 13, Response);

            Matchmaking::getLocalsession()->Gameinfo["Maxplayers"] = cMaxMembers;
            Matchmaking::Update();
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
    };

    struct SteamMatchmaking001 : Interface_t
    {
        SteamMatchmaking001()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame0);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame0);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame0);
            Createmethod(4, SteamMatchmaking, GetFavoriteGame2);
            Createmethod(5, SteamMatchmaking, AddFavoriteGame2);
            Createmethod(6, SteamMatchmaking, RemoveFavoriteGame2);
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
    struct SteamMatchmaking002 : Interface_t
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
    struct SteamMatchmaking003 : Interface_t
    {
        SteamMatchmaking003()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter0);
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
    struct SteamMatchmaking004 : Interface_t
    {
        SteamMatchmaking004()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter0);
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
    struct SteamMatchmaking005 : Interface_t
    {
        SteamMatchmaking005()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter0);
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
    struct SteamMatchmaking006 : Interface_t
    {
        SteamMatchmaking006()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter0);
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
    struct SteamMatchmaking007 : Interface_t
    {
        SteamMatchmaking007()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter1);
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
    struct SteamMatchmaking008 : Interface_t
    {
        SteamMatchmaking008()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter1);
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
    struct SteamMatchmaking009 : Interface_t
    {
        SteamMatchmaking009()
        {
            Createmethod(0, SteamMatchmaking, GetFavoriteGameCount);
            Createmethod(1, SteamMatchmaking, GetFavoriteGame1);
            Createmethod(2, SteamMatchmaking, AddFavoriteGame1);
            Createmethod(3, SteamMatchmaking, RemoveFavoriteGame1);
            Createmethod(4, SteamMatchmaking, RequestLobbyList2);
            Createmethod(5, SteamMatchmaking, AddRequestLobbyListStringFilter);
            Createmethod(6, SteamMatchmaking, AddRequestLobbyListNumericalFilter1);
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
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
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
