/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    struct SteamFriends
    {
        const char *GetPersonaName()
        {
            return Global.Username.c_str();
        }
        void SetPersonaName0(const char *pchPersonaName)
        {
            Traceprint();
            Global.Username = pchPersonaName;
        }
        uint32_t GetPersonaState()
        {
            Traceprint();
            return 0;
        }
        void SetPersonaState(uint32_t ePersonaState)
        {
            Traceprint();
        }
        bool AddFriend(CSteamID steamIDFriend)
        {
            Traceprint();
            return true;
        }
        bool RemoveFriend(CSteamID steamIDFriend)
        {
            Traceprint();
            return true;
        }
        bool HasFriend0(CSteamID steamIDFriend)
        {
            Traceprint();
            return false;
        }
        uint32_t GetFriendRelationship(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetFriendPersonaState(CSteamID steamIDFriend)
        {
            Traceprint();

            // EPersonaState::Offline
            return 0;
        }
        bool Deprecated_GetFriendGamePlayed(CSteamID steamIDFriend, int32_t *pnGameID, uint32_t *punGameIP, uint16_t *pusGamePort)
        {
            Traceprint();
            return false;
        }
        const char *GetFriendPersonaName(CSteamID steamIDFriend)
        {
            Traceprint();
            return "Unknown player";
        }
        uint32_t AddFriendByName(const char *pchEmailOrAccountName)
        {
            Traceprint();
            return 1;
        }
        int GetFriendCount0()
        {
            return 0;
        }
        CSteamID GetFriendByIndex0(int iFriend)
        {
            Traceprint();
            return CSteamID();
        }
        void SendMsgToFriend0(CSteamID steamIDFriend, uint32_t eFriendMsgType, const char *pchMsgBody)
        {
            Traceprint();
        }
        void SetFriendRegValue(CSteamID steamIDFriend, const char *pchKey, const char *pchValue)
        {
            Traceprint();
        }
        const char *GetFriendRegValue(CSteamID steamIDFriend, const char *pchKey)
        {
            Traceprint();
            return "";
        }
        const char *GetFriendPersonaNameHistory(CSteamID steamIDFriend, int iPersonaName)
        {
            Traceprint();
            return "";
        }
        int GetChatMessage(CSteamID steamIDFriend, int iChatID, void *pvData, int cubData, uint32_t *peFriendMsgType)
        {
            Traceprint();
            return 0;
        }
        bool SendMsgToFriend1(CSteamID steamIDFriend, uint32_t eFriendMsgType, const void *pvMsgBody, int cubMsgBody)
        {
            Traceprint();
            return false;
        }
        int GetChatIDOfChatHistoryStart(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        void SetChatHistoryStart(CSteamID steamIDFriend, int iChatID)
        {
            Traceprint();
        }
        void ClearChatHistory(CSteamID steamIDFriend)
        {
            Traceprint();
        }
        uint32_t InviteFriendByEmail0(const char *pchEmailOrAccountName)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetBlockedFriendCount()
        {
            Traceprint();
            return 0;
        }
        bool GetFriendGamePlayed0(CSteamID steamIDFriend, uint64_t *pulGameID, uint32_t *punGameIP, uint16_t *pusGamePort)
        {
            Traceprint();
            return false;
        }
        bool GetFriendGamePlayed2(CSteamID steamDIFriend, uint64_t *pulGameID, uint32_t *punGameIP, uint16_t *pusGamePort, uint16_t *pusQueryPort)
        {
            Traceprint();
            return false;
        }
        int GetFriendCount1(uint32_t iFriendFlags)
        {
            return 0;
        }
        CSteamID GetFriendByIndex1(int iFriend, uint32_t iFriendFlags)
        {
            Traceprint();
            return CSteamID();
        }
        bool GetFriendGamePlayed1(CSteamID steamIDFriend, uint64_t *pulGameID, uint32_t *punGameIP, uint16_t *pusGamePort, uint16_t *pusQueryPort)
        {
            Traceprint();
            return false;
        }
        bool HasFriend1(CSteamID steamIDFriend, uint32_t iFriendFlags)
        {
            Traceprint();

            return false;
        }
        bool InviteFriendByEmail1(const char *emailAddr)
        {
            Traceprint();
            return false;
        }
        int GetClanCount()
        {
            Traceprint();
            return 0;
        }
        CSteamID GetClanByIndex(int iClan)
        {
            Traceprint();
            return CSteamID(Global.UserID);
        }
        const char *GetClanName(CSteamID steamIDClan)
        {
            Traceprint();
            return "AYA";
        }
        bool InviteFriendToClan(CSteamID steamIDfriend, CSteamID steamIDclan)
        {
            Traceprint();
            return false;
        }
        bool AcknowledgeInviteToClan(CSteamID steamID, bool)
        {
            Traceprint();
            return false;
        }
        int GetFriendCountFromSource(CSteamID steamIDSource)
        {
            return 0;
        }
        CSteamID GetFriendFromSourceByIndex(CSteamID steamIDSource, int iFriend)
        {
            Traceprint();
            return CSteamID();
        }
        int GetFriendAvatar0(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        bool IsUserInSource(CSteamID steamIDUser, CSteamID steamIDSource)
        {
            Traceprint();
            return false;
        }
        void SetInGameVoiceSpeaking(CSteamID steamIDUser, bool bSpeaking)
        {
            Traceprint();
        }
        void ActivateGameOverlay(const char *pchDialog)
        {
            Traceprint();
        }
        int GetFriendAvatar1(CSteamID steamIDFriend, int eAvatarSize)
        {
            Traceprint();
            return 0;
        }
        CSteamID GetFriendByIndex2(int iFriend, int iFriendFlags)
        {
            Traceprint();
            return CSteamID();
        }
        bool GetFriendGamePlayed3(CSteamID steamIDFriend, struct FriendGameInfo_t *pFriendGameInfo)
        {
            Traceprint();
            return false;
        }
        void ActivateGameOverlayToUser(const char *pchDialog, CSteamID steamID)
        {
            Traceprint();
        }
        void ActivateGameOverlayToWebPage(const char *pchURL)
        {
            Traceprint();
        }
        void ActivateGameOverlayToStore0(uint32_t nAppID)
        {
            Traceprint();
        }
        void SetPlayedWith(CSteamID steamIDUserPlayedWith)
        {
            Traceprint();
        }
        const char *GetClanTag(CSteamID steamIDClan)
        {
            Traceprint();
            return "AYA";
        }
        void ActivateGameOverlayInviteDialog(CSteamID steamIDLobby)
        {
            Traceprint();
        }
        int GetSmallFriendAvatar(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        int GetMediumFriendAvatar(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        int GetLargeFriendAvatar(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        bool RequestUserInformation(CSteamID steamIDUser, bool bRequireNameOnly)
        {
            Traceprint();
            return false;
        }
        uint64_t RequestClanOfficerList(CSteamID steamIDClan)
        {
            Traceprint();
            return 0;
        }
        CSteamID GetClanOwner(CSteamID steamIDClan)
        {
            Traceprint();
            return CSteamID(Global.UserID);
        }
        int GetClanOfficerCount(CSteamID steamIDClan)
        {
            Traceprint();
            return 0;
        }
        CSteamID GetClanOfficerByIndex(CSteamID steamIDClan, int iOfficer)
        {
            Traceprint();
            return CSteamID(Global.UserID);
        }
        uint32_t GetUserRestrictions()
        {
            Traceprint();
            return 0;
        }
        int GetFriendCount2(int iFriendFlags)
        {
            Traceprint();
            return 0;
        }
        bool HasFriend2(CSteamID steamIDFriend, int iFriendFlags)
        {
            Traceprint();
            return false;
        }
        bool SetRichPresence(const char *pchKey, const char *pchValue)
        {
            return true;
        }
        void ClearRichPresence()
        {
            Traceprint();
        }
        const char *GetFriendRichPresence(CSteamID steamIDFriend, const char *pchKey)
        {
            Traceprint();
            return "";
        }
        int GetFriendRichPresenceKeyCount(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        const char *GetFriendRichPresenceKeyByIndex(CSteamID steamIDFriend, int iKey)
        {
            Traceprint();
            return "";
        }
        bool InviteUserToGame(CSteamID steamIDFriend, const char *pchConnectString)
        {
            Traceprint();
            return false;
        }
        int GetCoplayFriendCount()
        {
            Traceprint();
            return 0;
        }
        CSteamID GetCoplayFriend(int iCoplayFriend)
        {
            Traceprint();
            return CSteamID();
        }
        int GetFriendCoplayTime(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetFriendCoplayGame(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        bool GetClanActivityCounts(CSteamID steamID, int *pnOnline, int *pnInGame, int *pnChatting)
        {
            Traceprint();
            return false;
        }
        uint64_t DownloadClanActivityCounts(CSteamID groupIDs[], int nIds)
        {
            Traceprint();
            return 0;
        }
        uint64_t JoinClanChatRoom(CSteamID groupID)
        {
            Traceprint();
            return 0;
        }
        bool LeaveClanChatRoom(CSteamID groupID)
        {
            Traceprint();
            return false;
        }
        int GetClanChatMemberCount(CSteamID groupID)
        {
            Traceprint();
            return 0;
        }
        CSteamID GetChatMemberByIndex(CSteamID groupID, int iIndex)
        {
            Traceprint();
            return CSteamID(uint64_t(0));
        }
        bool SendClanChatMessage(CSteamID groupID, const char *cszMessage)
        {
            Traceprint();
            return false;
        }
        int GetClanChatMessage(CSteamID groupID, int iChatID, void *pvData, int cubData, uint32_t *peChatEntryType, CSteamID *pSteamIDChatter)
        {
            Traceprint();
            return 0;
        }
        bool IsClanChatAdmin(CSteamID groupID, CSteamID userID)
        {
            Traceprint();
            return false;
        }
        bool IsClanChatWindowOpenInSteam(CSteamID groupID)
        {
            Traceprint();
            return false;
        }
        bool OpenClanChatWindowInSteam(CSteamID groupID)
        {
            Traceprint();
            return false;
        }
        bool CloseClanChatWindowInSteam(CSteamID groupID)
        {
            Traceprint();
            return false;
        }
        bool SetListenForFriendsMessages(bool bListen)
        {
            Traceprint();
            return false;
        }
        bool ReplyToFriendMessage(CSteamID friendID, const char *cszMessage)
        {
            Traceprint();
            return false;
        }
        int GetFriendMessage(CSteamID friendID, int iChatID, void *pvData, int cubData, uint32_t *peChatEntryType)
        {
            Traceprint();
            return 0;
        }
        void RequestFriendRichPresence(CSteamID steamIDFriend)
        {
            Traceprint();
            return;
        }
        uint64_t GetFollowerCount(CSteamID steamID)
        {
            Traceprint();
            return 0;
        }
        uint64_t IsFollowing(CSteamID steamID)
        {
            Traceprint();
            return 0;
        }
        uint64_t EnumerateFollowingList(uint32_t unStartIndex)
        {
            Traceprint();
            return 0;
        }
        uint64_t SetPersonaName1(const char *pchPersonaName)
        {
            Traceprint();
            return 0;
        }
        void ActivateGameOverlayToStore1(uint32_t nAppID, uint32_t eFlag)
        {
            Traceprint();
            return;
        }
        const char *GetPlayerNickname(CSteamID steamIDPlayer)
        {
            Traceprint();
            return "AYA";
        }
        uint64_t SetPersonaName2(const char *pchPersonaName)
        {
            Traceprint();
            Global.Username = pchPersonaName;
            return 0;
        }
        int GetFriendSteamLevel(CSteamID steamIDFriend)
        {
            Traceprint();
            return 0;
        }
        int16_t GetFriendsGroupCount()
        {
            Traceprint();
            return 0;
        }
        int16_t GetFriendsGroupIDByIndex(int32_t)
        {
            Traceprint();
            return 0;
        }
        const char *GetFriendsGroupName(int16_t)
        {
            Traceprint();
            return "";
        }
        int GetFriendsGroupMembersCount(int16_t)
        {
            Traceprint();
            return 0;
        }
        int GetFriendsGroupMembersList(int16_t, CSteamID *, int32_t)
        {
            Traceprint();
            return 0;
        }
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
            Createmethod(9, SteamFriends, Deprecated_GetFriendGamePlayed);
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
            Createmethod(25, SteamFriends, GetFriendGamePlayed0);
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
            Createmethod(11, SteamFriends, GetFriendGamePlayed1);
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
            Createmethod(9, SteamFriends, GetFriendGamePlayed1);
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
            Createmethod(9, SteamFriends, GetFriendGamePlayed1);
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
            Createmethod(20, SteamFriends, ActivateGameOverlayToUser);
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(9, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(22, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(21, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(58, SteamFriends, GetFriendMessage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(59, SteamFriends, GetFriendMessage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(59, SteamFriends, GetFriendMessage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(23, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(59, SteamFriends, GetFriendMessage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(24, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(60, SteamFriends, GetFriendMessage);
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
            Createmethod(8, SteamFriends, GetFriendGamePlayed3);
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
            Createmethod(30, SteamFriends, ActivateGameOverlayToWebPage);
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
            Createmethod(66, SteamFriends, GetFriendMessage);
            Createmethod(67, SteamFriends, GetFollowerCount);
            Createmethod(68, SteamFriends, IsFollowing);
            Createmethod(69, SteamFriends, EnumerateFollowingList);
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
        }
    };
    static Steamfriendsloader Interfaceloader{};
}
