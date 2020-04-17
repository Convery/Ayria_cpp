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

    struct SteamUserstats
    {
        uint32_t GetNumStats(CGameID nGameID)
        {
            Traceprint();
            return 0;
        }
        const char *GetStatName(CGameID nGameID, uint32_t iStat)
        {
            Traceprint();
            return "";
        }
        uint32_t GetStatType(CGameID nGameID, const char *pchName)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetNumAchievements(CGameID nGameID)
        {
            Traceprint();
            return 0;
        }
        const char *GetAchievementName0(CGameID nGameID, uint32_t iAchievement)
        {
            Traceprint();
            return "";
        }
        uint32_t GetNumGroupAchievements(CGameID)
        {
            Traceprint();
            return 0;
        }
        const char *GetGroupAchievementName(CGameID, uint32_t iAchievement)
        {
            Traceprint();
            return "";
        }
        bool RequestCurrentStats0(CGameID nGameID)
        {
            Traceprint();
            return true;
        }
        bool GetStat1(CGameID nGameID, const char *pchName, int32_t *pData) const
        {
            Infoprint(va("Get stat \"%s\"..", pchName));
            return false;
        }
        bool GetStat2(CGameID nGameID, const char *pchName, float *pData) const
        {
            Infoprint(va("Get stat \"%s\"..", pchName));
            return false;
        }
        bool SetStat1(CGameID nGameID, const char *pchName, int32_t nData) const
        {
            Infoprint(va("Set stat \"%s\" = %d", pchName, nData));
            return false;
        }
        bool SetStat2(CGameID nGameID, const char *pchName, float fData) const
        {
            Infoprint(va("Set stat \"%s\" = %f", pchName, fData));
            return false;
        }
        bool UpdateAvgRateStat0(CGameID nGameID, const char *pchName, float, double dSessionLength)
        {
            Traceprint();
            return false;
        }
        bool GetAchievement0(CGameID nGameID, const char *pchName, bool *pbAchieved) const
        {
            Infoprint(va("Get achievement \"%s\"..", pchName));
            return false;
        }
        bool GetGroupAchievement(CGameID nGameID, const char *pchName, bool *pbAchieved)
        {
            Traceprint();
            return false;
        }
        bool SetAchievement0(CGameID nGameID, const char *pchName) const
        {
            Infoprint(va("Achievement \"%s\" progress: 100%%", pchName));
            return false;
        }
        bool SetGroupAchievement(CGameID nGameID, const char *pchName)
        {
            Traceprint();
            return false;
        }
        bool StoreStats0(CGameID nGameID)
        {
            Traceprint();
            return true;
        }
        bool ClearAchievement0(CGameID nGameID, const char *pchName) const
        {
            Infoprint(va("Achievement \"%s\" progress: 0%%", pchName));
            return false;
        }
        bool ClearGroupAchievement(CGameID nGameID, const char *pchName)
        {
            Traceprint();
            return false;
        }
        int GetAchievementIcon0(CGameID nGameID, const char *pchName)
        {
            Traceprint();
            return 0;
        }
        const char *GetAchievementDisplayAttribute0(CGameID nGameID, const char *pchName, const char *pchKey)
        {
            Traceprint();
            return "";
        }
        bool UpdateAvgRateStat1(CGameID nGameID, const char *pchName, uint32_t nCountThisSession, double dSessionLength)
        {
            Traceprint();
            return false;
        }
        bool IndicateAchievementProgress0(CGameID nGameID, const char *pchName, uint32_t nCurProgress, uint32_t nMaxProgress) const
        {
            Infoprint(va("Achievement \"%s\" progress: %f%%", pchName, float(nCurProgress) / float(nMaxProgress)));

            /*
                TODO(Convery):
                Trigger a toaster-popup.
            */

            return false;
        }
        bool RequestCurrentStats1()
        {
            Traceprint();
            return true;
        }
        bool GetStat3(const char *pchName, int32_t *pData) const
        {
            return GetStat1({}, pchName, pData);
        }
        bool GetStat4(const char *pchName, float *pData) const
        {
            return GetStat2({}, pchName, pData);
        }
        bool SetStat3(const char *pchName, int32_t nData) const
        {
            return SetStat1({}, pchName, nData);
        }
        bool SetStat4(const char *pchName, float fData) const
        {
            return SetStat2({}, pchName, fData);
        }
        bool UpdateAvgRateStat2(const char *pchName, float, double dSessionLength)
        {
            Traceprint();
            return false;
        }
        bool GetAchievement1(const char *pchName, bool *pbAchieved) const
        {
            return GetAchievement0({}, pchName, pbAchieved);
        }
        bool SetAchievement1(const char *pchName) const
        {
            return SetAchievement0({}, pchName);
        }
        bool ClearAchievement1(const char *pchName) const
        {
            return ClearAchievement0({}, pchName);
        }
        bool StoreStats1()
        {
            Traceprint();
            return true;
        }
        int GetAchievementIcon1(const char *pchName)
        {
            return GetAchievementIcon0({}, pchName);
        }
        const char *GetAchievementDisplayAttribute1(const char *pchName, const char *pchKey)
        {
            return GetAchievementDisplayAttribute0({}, pchName, pchKey);
        }
        bool IndicateAchievementProgress1(const char *pchName, uint32_t nCurProgress, uint32_t nMaxProgress) const
        {
            return IndicateAchievementProgress0({}, pchName, nCurProgress, nMaxProgress);
        }
        uint64_t RequestUserStats(CSteamID steamIDUser)
        {
            Traceprint();
            return 0;
        }
        bool GetUserStat1(CSteamID steamIDUser, const char *pchName, int32_t *pData) const
        {
            if (steamIDUser.ConvertToUint64() == Ayria::Global.UserID)
                return false;
            return GetStat3(pchName, pData);
        }
        bool GetUserStat2(CSteamID steamIDUser, const char *pchName, float *pData) const
        {
            if (steamIDUser.ConvertToUint64() == Ayria::Global.UserID)
                return false;
            return GetStat4(pchName, pData);
        }
        bool GetUserAchievement(CSteamID steamIDUser, const char *pchName, bool *pbAchieved) const
        {
            if (steamIDUser.ConvertToUint64() == Ayria::Global.UserID)
                return false;
            return GetAchievement1(pchName, pbAchieved);
        }
        bool ResetAllStats(bool bAchievementsToo)
        {
            Traceprint();
            return true;
        }
        uint64_t FindOrCreateLeaderboard(const char *pchLeaderboardName, uint32_t eLeaderboardSortMethod, uint32_t eLeaderboardDisplayType)
        {
            return FindLeaderboard(pchLeaderboardName);
        }
        uint64_t FindLeaderboard(const char *pchLeaderboardName)
        {
            return 0;
        }
        const char *GetLeaderboardName(uint64_t hSteamLeaderboard)
        {
            Traceprint();
            return "";
        }
        int GetLeaderboardEntryCount(uint64_t hSteamLeaderboard)
        {
            Traceprint();
            return (int)0;
        }
        uint32_t GetLeaderboardSortMethod(uint64_t hSteamLeaderboard)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetLeaderboardDisplayType(uint64_t hSteamLeaderboard)
        {
            Traceprint();
            return 0;
        }
        uint64_t DownloadLeaderboardEntries(uint64_t hSteamLeaderboard, uint32_t eLeaderboardDataRequest, int nRangeStart, int nRangeEnd)
        {
            return 0;
        }
        uint64_t UploadLeaderboardScore0(uint64_t hSteamLeaderboard, int32_t nScore, int32_t *pScoreDetails, int cScoreDetailsCount)
        {
            Traceprint();
            return 0;
        }
        uint64_t UploadLeaderboardScore1(uint64_t hSteamLeaderboard, uint32_t eLeaderboardUploadScoreMethod, int32_t nScore, int32_t *pScoreDetails, int cScoreDetailsCount)
        {
            Traceprint();
            return 0;
        }
        uint64_t GetNumberOfCurrentPlayers()
        {
            Traceprint();
            return 0;
        }
        bool GetAchievementAndUnlockTime(const char *pchName, bool *pbAchieved, uint32_t *prtTime)
        {
            Traceprint();
            return false;
        }
        bool GetUserAchievementAndUnlockTime(CSteamID steamIDUser, const char *pchName, bool *pbAchieved, uint32_t *prtTime)
        {
            Traceprint();
            return false;
        }
        bool GetDownloadedLeaderboardEntry(uint64_t hSteamLeaderboardEntries, int index, struct LeaderboardEntry_t *pLeaderboardEntry, int32_t *pDetails, int cDetailsMax)
        {
            Traceprint();
            return false;
        }
        uint64_t AttachLeaderboardUGC(uint64_t hSteamLeaderboard, uint32_t hUGC)
        {
            Traceprint();
            return 0;
        }
        uint64_t DownloadLeaderboardEntriesForUsers(uint64_t hSteamLeaderboard, CSteamID *prgUsers, int cUsers)
        {
            Traceprint();
            return 0;
        }
        uint64_t RequestGlobalAchievementPercentages()
        {
            Traceprint();
            return 0;
        }
        int GetMostAchievedAchievementInfo(char *pchName, uint32_t unNameBufLen, float *pflPercent, bool *pbAchieved)
        {
            Traceprint();
            return 0;
        }
        int GetNextMostAchievedAchievementInfo(int iIteratorPrevious, char *pchName, uint32_t unNameBufLen, float *pflPercent, bool *pbAchieved)
        {
            Traceprint();
            return 0;
        }
        bool GetAchievementAchievedPercent(const char *pchName, float *pflPercent)
        {
            Traceprint();
            return false;
        }
        uint64_t RequestGlobalStats(int nHistoryDays)
        {
            Traceprint();
            return 0;
        }
        bool GetGlobalStat1(const char *pchStatName, int64_t *pData)
        {
            Traceprint();
            return false;
        }
        bool GetGlobalStat2(const char *pchStatName, double *pData)
        {
            Traceprint();
            return false;
        }
        int32_t GetGlobalStatHistory1(const char *pchStatName, int64_t *pData, uint32_t cubData)
        {
            Traceprint();
            return 0;
        }
        int32_t GetGlobalStatHistory2(const char *pchStatName, double *pData, uint32_t cubData)
        {
            Traceprint();
            return 0;
        }
        const char *GetAchievementName1(uint32_t iAchievement)
        {
            Traceprint();
            return "";
        }
    };

    struct SteamUserstats001 : Interface_t
    {
        SteamUserstats001()
        {
            Createmethod(0, SteamUserstats, GetNumStats);
            Createmethod(1, SteamUserstats, GetStatName);
            Createmethod(2, SteamUserstats, GetStatType);
            Createmethod(3, SteamUserstats, GetNumAchievements);
            Createmethod(4, SteamUserstats, GetAchievementName0);
            Createmethod(5, SteamUserstats, GetNumGroupAchievements);
            Createmethod(6, SteamUserstats, GetGroupAchievementName);
            Createmethod(7, SteamUserstats, RequestCurrentStats0);
            Createmethod(8, SteamUserstats, GetStat1);
            Createmethod(9, SteamUserstats, GetStat2);
            Createmethod(10, SteamUserstats, SetStat1);
            Createmethod(11, SteamUserstats, SetStat2);
            Createmethod(12, SteamUserstats, UpdateAvgRateStat0);
            Createmethod(13, SteamUserstats, GetAchievement0);
            Createmethod(14, SteamUserstats, GetGroupAchievement);
            Createmethod(15, SteamUserstats, SetAchievement0);
            Createmethod(16, SteamUserstats, SetGroupAchievement);
            Createmethod(17, SteamUserstats, StoreStats0);
            Createmethod(18, SteamUserstats, ClearAchievement0);
            Createmethod(19, SteamUserstats, ClearGroupAchievement);
            Createmethod(20, SteamUserstats, GetAchievementIcon0);
            Createmethod(21, SteamUserstats, GetAchievementDisplayAttribute0);
        };
    };
    struct SteamUserstats002 : Interface_t
    {
        SteamUserstats002()
        {
            Createmethod(0, SteamUserstats, GetNumStats);
            Createmethod(1, SteamUserstats, GetStatName);
            Createmethod(2, SteamUserstats, GetStatType);
            Createmethod(3, SteamUserstats, GetNumAchievements);
            Createmethod(4, SteamUserstats, GetAchievementName0);
            Createmethod(5, SteamUserstats, RequestCurrentStats0);
            Createmethod(6, SteamUserstats, GetStat1);
            Createmethod(7, SteamUserstats, GetStat2);
            Createmethod(8, SteamUserstats, SetStat1);
            Createmethod(9, SteamUserstats, SetStat2);
            Createmethod(10, SteamUserstats, UpdateAvgRateStat1);
            Createmethod(11, SteamUserstats, GetAchievement0);
            Createmethod(12, SteamUserstats, SetAchievement0);
            Createmethod(13, SteamUserstats, ClearAchievement0);
            Createmethod(14, SteamUserstats, StoreStats0);
            Createmethod(15, SteamUserstats, GetAchievementIcon0);
            Createmethod(16, SteamUserstats, GetAchievementDisplayAttribute0);
            Createmethod(17, SteamUserstats, IndicateAchievementProgress0);
        };
    };
    struct SteamUserstats003 : Interface_t
    {
        SteamUserstats003()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement0);
            Createmethod(7, SteamUserstats, SetAchievement0);
            Createmethod(8, SteamUserstats, ClearAchievement0);
            Createmethod(9, SteamUserstats, StoreStats0);
            Createmethod(10, SteamUserstats, GetAchievementIcon0);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute0);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress0);
        };
    };
    struct SteamUserstats004 : Interface_t
    {
        SteamUserstats004()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStat1);
            Createmethod(15, SteamUserstats, GetUserStat2);
            Createmethod(16, SteamUserstats, GetUserAchievement);
            Createmethod(17, SteamUserstats, ResetAllStats);
        };
    };
    struct SteamUserstats005 : Interface_t
    {
        SteamUserstats005()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStat1);
            Createmethod(15, SteamUserstats, GetUserStat2);
            Createmethod(16, SteamUserstats, GetUserAchievement);
            Createmethod(17, SteamUserstats, ResetAllStats);
            Createmethod(18, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(19, SteamUserstats, FindLeaderboard);
            Createmethod(20, SteamUserstats, GetLeaderboardName);
            Createmethod(21, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(22, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(23, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(24, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(25, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(26, SteamUserstats, UploadLeaderboardScore0);
        };
    };
    struct SteamUserstats006 : Interface_t
    {
        SteamUserstats006()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStat1);
            Createmethod(15, SteamUserstats, GetUserStat2);
            Createmethod(16, SteamUserstats, GetUserAchievement);
            Createmethod(17, SteamUserstats, ResetAllStats);
            Createmethod(18, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(19, SteamUserstats, FindLeaderboard);
            Createmethod(20, SteamUserstats, GetLeaderboardName);
            Createmethod(21, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(22, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(23, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(24, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(25, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(26, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(27, SteamUserstats, GetNumberOfCurrentPlayers);
        };
    };
    struct SteamUserstats007 : Interface_t
    {
        SteamUserstats007()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStat1);
            Createmethod(16, SteamUserstats, GetUserStat2);
            Createmethod(17, SteamUserstats, GetUserAchievement);
            Createmethod(18, SteamUserstats, GetUserAchievementAndUnlockTime);
            Createmethod(19, SteamUserstats, ResetAllStats);
            Createmethod(20, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(21, SteamUserstats, FindLeaderboard);
            Createmethod(22, SteamUserstats, GetLeaderboardName);
            Createmethod(23, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(24, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(25, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(26, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(27, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(28, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(29, SteamUserstats, GetNumberOfCurrentPlayers);
        };
    };
    struct SteamUserstats008 : Interface_t
    {
        SteamUserstats008()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStat1);
            Createmethod(16, SteamUserstats, GetUserStat2);
            Createmethod(17, SteamUserstats, GetUserAchievement);
            Createmethod(18, SteamUserstats, GetUserAchievementAndUnlockTime);
            Createmethod(19, SteamUserstats, ResetAllStats);
            Createmethod(20, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(21, SteamUserstats, FindLeaderboard);
            Createmethod(22, SteamUserstats, GetLeaderboardName);
            Createmethod(23, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(24, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(25, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(26, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(27, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(28, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(29, SteamUserstats, AttachLeaderboardUGC);
            Createmethod(30, SteamUserstats, GetNumberOfCurrentPlayers);

        };
    };
    struct SteamUserstats009 : Interface_t
    {
        SteamUserstats009()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStat1);
            Createmethod(16, SteamUserstats, GetUserStat2);
            Createmethod(17, SteamUserstats, GetUserAchievement);
            Createmethod(18, SteamUserstats, GetUserAchievementAndUnlockTime);
            Createmethod(19, SteamUserstats, ResetAllStats);
            Createmethod(20, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(21, SteamUserstats, FindLeaderboard);
            Createmethod(22, SteamUserstats, GetLeaderboardName);
            Createmethod(23, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(24, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(25, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(26, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(27, SteamUserstats, DownloadLeaderboardEntriesForUsers);
            Createmethod(28, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(29, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(30, SteamUserstats, AttachLeaderboardUGC);
            Createmethod(31, SteamUserstats, GetNumberOfCurrentPlayers);
        };
    };
    struct SteamUserstats010 : Interface_t
    {
        SteamUserstats010()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStat1);
            Createmethod(16, SteamUserstats, GetUserStat2);
            Createmethod(17, SteamUserstats, GetUserAchievement);
            Createmethod(18, SteamUserstats, GetUserAchievementAndUnlockTime);
            Createmethod(19, SteamUserstats, ResetAllStats);
            Createmethod(20, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(21, SteamUserstats, FindLeaderboard);
            Createmethod(22, SteamUserstats, GetLeaderboardName);
            Createmethod(23, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(24, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(25, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(26, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(27, SteamUserstats, DownloadLeaderboardEntriesForUsers);
            Createmethod(28, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(29, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(30, SteamUserstats, AttachLeaderboardUGC);
            Createmethod(31, SteamUserstats, GetNumberOfCurrentPlayers);
            Createmethod(32, SteamUserstats, RequestGlobalAchievementPercentages);
            Createmethod(33, SteamUserstats, GetMostAchievedAchievementInfo);
            Createmethod(34, SteamUserstats, GetNextMostAchievedAchievementInfo);
            Createmethod(35, SteamUserstats, GetAchievementAchievedPercent);
            Createmethod(36, SteamUserstats, RequestGlobalStats);
            Createmethod(37, SteamUserstats, GetGlobalStat1);
            Createmethod(38, SteamUserstats, GetGlobalStat2);
            Createmethod(39, SteamUserstats, GetGlobalStatHistory1);
            Createmethod(40, SteamUserstats, GetGlobalStatHistory2);
        };
    };
    struct SteamUserstats011 : Interface_t
    {
        SteamUserstats011()
        {
            Createmethod(0, SteamUserstats, RequestCurrentStats1);
            Createmethod(1, SteamUserstats, GetStat3);
            Createmethod(2, SteamUserstats, GetStat4);
            Createmethod(3, SteamUserstats, SetStat3);
            Createmethod(4, SteamUserstats, SetStat4);
            Createmethod(5, SteamUserstats, UpdateAvgRateStat2);
            Createmethod(6, SteamUserstats, GetAchievement1);
            Createmethod(7, SteamUserstats, SetAchievement1);
            Createmethod(8, SteamUserstats, ClearAchievement1);
            Createmethod(9, SteamUserstats, GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, GetNumAchievements);
            Createmethod(15, SteamUserstats, GetAchievementName1);
            Createmethod(16, SteamUserstats, RequestUserStats);
            Createmethod(17, SteamUserstats, GetUserStat1);
            Createmethod(18, SteamUserstats, GetUserStat2);
            Createmethod(19, SteamUserstats, GetUserAchievement);
            Createmethod(20, SteamUserstats, GetUserAchievementAndUnlockTime);
            Createmethod(21, SteamUserstats, ResetAllStats);
            Createmethod(22, SteamUserstats, FindOrCreateLeaderboard);
            Createmethod(23, SteamUserstats, FindLeaderboard);
            Createmethod(24, SteamUserstats, GetLeaderboardName);
            Createmethod(25, SteamUserstats, GetLeaderboardEntryCount);
            Createmethod(26, SteamUserstats, GetLeaderboardSortMethod);
            Createmethod(27, SteamUserstats, GetLeaderboardDisplayType);
            Createmethod(28, SteamUserstats, DownloadLeaderboardEntries);
            Createmethod(29, SteamUserstats, DownloadLeaderboardEntriesForUsers);
            Createmethod(30, SteamUserstats, GetDownloadedLeaderboardEntry);
            Createmethod(31, SteamUserstats, UploadLeaderboardScore1);
            Createmethod(32, SteamUserstats, AttachLeaderboardUGC);
            Createmethod(33, SteamUserstats, GetNumberOfCurrentPlayers);
            Createmethod(34, SteamUserstats, RequestGlobalAchievementPercentages);
            Createmethod(35, SteamUserstats, GetMostAchievedAchievementInfo);
            Createmethod(36, SteamUserstats, GetNextMostAchievedAchievementInfo);
            Createmethod(37, SteamUserstats, GetAchievementAchievedPercent);
            Createmethod(38, SteamUserstats, RequestGlobalStats);
            Createmethod(39, SteamUserstats, GetGlobalStat1);
            Createmethod(40, SteamUserstats, GetGlobalStat2);
            Createmethod(41, SteamUserstats, GetGlobalStatHistory1);
            Createmethod(42, SteamUserstats, GetGlobalStatHistory2);
        };
    };

    struct Steamuserstatsloader
    {
        Steamuserstatsloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats001", SteamUserstats001);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats002", SteamUserstats002);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats003", SteamUserstats003);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats004", SteamUserstats004);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats005", SteamUserstats005);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats006", SteamUserstats006);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats007", SteamUserstats007);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats008", SteamUserstats008);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats009", SteamUserstats009);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats010", SteamUserstats010);
            Register(Interfacetype_t::USERSTATS, "SteamUserstats011", SteamUserstats011);
        }
    };
    static Steamuserstatsloader Interfaceloader{};
}
