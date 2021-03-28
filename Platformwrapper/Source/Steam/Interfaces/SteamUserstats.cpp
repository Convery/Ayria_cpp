/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct LeaderboardEntry_t
    {
        SteamID_t m_steamIDUser;    // user with the entry - use SteamFriends()->GetFriendPersonaName() & SteamFriends()->GetFriendAvatar() to get more info
        int32_t m_nGlobalRank;	    // [1..N], where N is the number of users with an entry in the leaderboard
        int32_t m_nScore;			// score as set in the leaderboard
        int32_t m_cDetails;		    // number of int32 details available for this entry
        UGCHandle_t m_hUGC;		    // handle for UGC attached to the entry
    };
    struct Entry_t : LeaderboardEntry_t
    {
        std::vector<int32_t> Details;

        operator LeaderboardEntry_t() const
        {
            m_cDetails = Details.size();
            return *this;
        }
    };
    static Hashmap<uint64_t, std::vector<Entry_t>> Downloadedentries;

    static bool Initialized = false;
    static sqlite::database Database()
    {
        try
        {
            sqlite::sqlite_config Config{};
            Config.flags = sqlite::OpenFlags::CREATE | sqlite::OpenFlags::SQLITE_OPEN_READWRITE| sqlite::OpenFlags::FULLMUTEX;
            auto DB = sqlite::database("./Ayria/Steam.db", Config);

            if (!Initialized)
            {
                DB << "CREATE TABLE IF NOT EXISTS Achievement ("
                      "API_Name text not null, "
                      "AppID integer not null, "
                      "Maxprogress integer, "
                      "Name text not null, "
                      "Description text, "
                      "Icon text, "
                      "PRIMARY_KEY(API_Name, AppID) );";

                DB << "CREATE TABLE IF NOT EXISTS Achievementprogress ("
                      "ClientID integer not null, "
                      "Currentprogress integer, "
                      "AppID integer not null, "
                      "Name text not null, "
                      "Unlocktime integer, "
                      "PRIMARY KEY (Name, ClientID, AppID) );";

                DB << "CREATE TABLE IF NOT EXISTS Groupachievement ("
                      "AppID integer not null, "
                      "Name text not null, "
                      "Unlocktime integer, "
                      "PRIMARY_KEY (AppID, Name) );";

                DB << "CREATE TABLE IF NOT EXISTS Userstat ("
                      "ClientID integer not null, "
                      "AppID integer not null, "
                      "Statname text not null, "
                      "Stattype integer, "
                      "Value, "
                      "PRIMARY KEY (ClientID, AppID, Statname) );";

                DB << "CREATE TABLE IF NOT EXISTS Globalstats ("
                      "Statname text primary key unique not null, "
                      "Stattype integer, "
                      "Value );";

                DB << "CREATE TABLE IF NOT EXISTS Leaderboards ("
                      "LeaderboardID integer not null, "
                      "ClientID integer not null, "
                      "AppID integer not null, "
                      "Leaderboardname text, "
                      "Displaytype integer, "
                      "Sortmethod integer, "
                      "Score integer, "
                      "Details blob, "
                      "PRIMARY KEY (LeaderboardID, ClientID, AppID) );";

                Initialized = true;
            }

            return DB;
        }
        catch (std::exception &e)
        {
            Errorprint(va("Could not connect to the database: %s", e.what()));
            return sqlite::database(":memory:");
        }
    }

    struct SteamUserstats
    {
        enum ELeaderboardUploadScoreMethod : uint32_t
        {
            k_ELeaderboardUploadScoreMethodNone = 0,
            k_ELeaderboardUploadScoreMethodKeepBest = 1,	// Leaderboard will keep user's best score
            k_ELeaderboardUploadScoreMethodForceUpdate = 2,	// Leaderboard will always replace score with specified
        };
        enum ELeaderboardDisplayType : uint32_t
        {
            k_ELeaderboardDisplayTypeNone = 0,
            k_ELeaderboardDisplayTypeNumeric = 1,			// simple numerical score
            k_ELeaderboardDisplayTypeTimeSeconds = 2,		// the score represents a time, in seconds
            k_ELeaderboardDisplayTypeTimeMilliSeconds = 3,	// the score represents a time, in milliseconds
        };
        enum ELeaderboardDataRequest : uint32_t
        {
            k_ELeaderboardDataRequestGlobal = 0,
            k_ELeaderboardDataRequestGlobalAroundUser = 1,
            k_ELeaderboardDataRequestFriends = 2,
            k_ELeaderboardDataRequestUsers = 3
        };
        enum ELeaderboardSortMethod : uint32_t
        {
            k_ELeaderboardSortMethodNone = 0,
            k_ELeaderboardSortMethodAscending = 1,	// top-score is lowest number
            k_ELeaderboardSortMethodDescending = 2,	// top-score is highest number
        };
        enum ESteamUserStatType : uint32_t
        {
            k_ESteamUserStatTypeINVALID = 0,
            k_ESteamUserStatTypeINT = 1,
            k_ESteamUserStatTypeFLOAT = 2,
            // Read as FLOAT, set with count / session length
            k_ESteamUserStatTypeAVGRATE = 3,
            k_ESteamUserStatTypeACHIEVEMENTS = 4,
            k_ESteamUserStatTypeGROUPACHIEVEMENTS = 5,

            // max, for sanity checks
            k_ESteamUserStatTypeMAX
        };

        ELeaderboardDisplayType GetLeaderboardDisplayType(SteamLeaderboard_t hSteamLeaderboard)
        {
            uint32_t Displaytype{};

            Database() << "SELECT Displaytype FROM Leaderboards WHERE LeaderboardID = ? AND AppID = ? LIMIT 1;"
                       << hSteamLeaderboard << Global.ApplicationID >> Displaytype;

            return ELeaderboardDisplayType{ Displaytype };
        }
        ELeaderboardSortMethod GetLeaderboardSortMethod(SteamLeaderboard_t hSteamLeaderboard)
        {
            uint32_t Sortmethod{};

            Database() << "SELECT Sortmethod FROM Leaderboards WHERE LeaderboardID = ? AND AppID = ? LIMIT 1;"
                       << hSteamLeaderboard << Global.ApplicationID >> Sortmethod;

            return ELeaderboardSortMethod{ Sortmethod };
        }
        ESteamUserStatType GetStatType(GameID_t nGameID, const char *pchName)
        {
            uint32_t Stattype{};

            Database() << "SELECT Stattype FROM Userstat WHERE AppID = ? AND Statname = ? LIMIT 1;"
                       << nGameID.AppID << std::string(pchName) >> Stattype;

            return ESteamUserStatType{ Stattype };
        }

        SteamAPICall_t AttachLeaderboardUGC(SteamLeaderboard_t hSteamLeaderboard, UGCHandle_t hUGC);
        SteamAPICall_t DownloadLeaderboardEntries(SteamLeaderboard_t hSteamLeaderboard, ELeaderboardDataRequest eLeaderboardDataRequest, int nRangeStart, int nRangeEnd);
        SteamAPICall_t DownloadLeaderboardEntriesForUsers(SteamLeaderboard_t hSteamLeaderboard, SteamID_t *prgUsers, int cUsers);
        SteamAPICall_t FindLeaderboard(const char *pchLeaderboardName);
        SteamAPICall_t FindOrCreateLeaderboard(const char *pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType);
        SteamAPICall_t GetNumberOfCurrentPlayers()
        {
            uint32_t Count{};

            try
            {
                sqlite::sqlite_config Config{};
                Config.flags = sqlite::OpenFlags::CREATE | sqlite::OpenFlags::SQLITE_OPEN_READWRITE| sqlite::OpenFlags::FULLMUTEX;
                sqlite::database("./Ayria/Client.db", Config) << "SELECT COUNT(*) FROM Onlineclients;" >> Count;
            }
            catch (std::exception &e) {}

            auto Result = new Callbacks::NumberOfCurrentPlayers_t();
            Result->m_cPlayers = Count;
            Result->m_bSuccess = 1;

            const auto ID = Callbacks::Createrequest();
            Callbacks::Completerequest(ID, Callbacks::Types::NumberOfCurrentPlayers_t, Result);
            return ID;
        }
        SteamAPICall_t RequestGlobalAchievementPercentages()
        {
            auto Result = new Callbacks::GlobalAchievementPercentagesReady_t();
            Result->m_nGameID = Global.ApplicationID;
            Result->m_eResult = EResult::k_EResultOK;

            const auto ID = Callbacks::Createrequest();
            Callbacks::Completerequest(ID, Callbacks::Types::GlobalAchievementPercentagesReady_t, Result);
            return ID;
        }
        SteamAPICall_t RequestGlobalStats(int nHistoryDays)
        {
            auto Result = new Callbacks::GlobalStatsReceived_t();
            Result->m_nGameID = Global.ApplicationID;
            Result->m_eResult = EResult::k_EResultOK;

            const auto ID = Callbacks::Createrequest();
            Callbacks::Completerequest(ID, Callbacks::Types::GlobalStatsReceived_t, Result);
            return ID;
        }
        SteamAPICall_t RequestUserStats(SteamID_t steamIDUser)
        {
            auto Result = new Callbacks::UserStatsReceived_t();
            Result->m_nGameID = Global.ApplicationID;
            Result->m_eResult = EResult::k_EResultOK;
            Result->m_steamIDUser = steamIDUser;

            const auto ID = Callbacks::Createrequest();
            Callbacks::Completerequest(ID, Callbacks::Types::UserStatsReceived_t, Result);
            return ID;
        }
        SteamAPICall_t UploadLeaderboardScore1(SteamLeaderboard_t hSteamLeaderboard, ELeaderboardUploadScoreMethod eLeaderboardUploadScoreMethod, int32_t nScore, const int32_t *pScoreDetails, int cScoreDetailsCount)
        {
            bool Changed{};
            std::vector<int32_t> Details;
            Details.reserve(cScoreDetailsCount);
            for (int i = 0; i < cScoreDetailsCount; ++i)
                Details.emplace_back(pScoreDetails[i]);

            if (eLeaderboardUploadScoreMethod == k_ELeaderboardUploadScoreMethodKeepBest)
            {
                int32_t Currentscore{};
                Database() << "SELECT Score FROM Leaderboards WHERE LeaderboardID = ? AND ClientID = ? AND AppID = ? LIMIT 1;"
                           << hSteamLeaderboard << Global.XUID.UserID << Global.ApplicationID >> Currentscore;

                Changed = nScore > Currentscore;
                nScore = std::max(Currentscore, nScore);
            }

            Database() << "REPLACE (LeaderboardID, ClientID, AppID, Score, Details) INTO Leaderboards VALUES (?,?,?,?,?);"
                       << hSteamLeaderboard << Global.XUID.UserID << Global.ApplicationID << nScore << Details;

            auto Result = new Callbacks::LeaderboardScoreUploaded_t();
            Result->m_hSteamLeaderboard = hSteamLeaderboard;
            Result->m_bScoreChanged = Changed;
            Result->m_nGlobalRankPrevious = 1;
            Result->m_nGlobalRankNew = 1;
            Result->m_nScore = nScore;
            Result->m_bSuccess = true;

            const auto ID = Callbacks::Createrequest();
            Callbacks::Completerequest(ID, Callbacks::Types::LeaderboardScoreUploaded_t, Result);
            return ID;
        }
        SteamAPICall_t UploadLeaderboardScore0(SteamLeaderboard_t hSteamLeaderboard, int32_t nScore, int32_t *pScoreDetails, int cScoreDetailsCount)
        {
            return UploadLeaderboardScore1(hSteamLeaderboard, k_ELeaderboardUploadScoreMethodForceUpdate, nScore, pScoreDetails, cScoreDetailsCount);
        }

        bool ClearAchievement0(GameID_t nGameID, const char *pchName)
        {
            Database() << "REPLACE (Currentprogress, Unlocktime) INTO Achievementprogress VALUES(0, 0) WHERE AppID = ? AND API_Name = ? AND ClientID = ?;"
                       << nGameID.AppID << Global.XUID.UserID << std::string(pchName);

            return true;
        }
        bool ClearAchievement1(const char *pchName)
        {
            return ClearAchievement0(GameID_t{ .AppID = Global.ApplicationID }, pchName);
        }
        bool ClearGroupAchievement(GameID_t nGameID, const char *pchName)
        {
            Database() << "REPLACE (Unlocktime) INTO Groupachievement VALUES(0) WHERE AppID = ? AND API_Name = ?;"
                       << nGameID.AppID << std::string(pchName);
            return true;
        }
        bool GetAchievement0(GameID_t nGameID, const char *pchName, bool *pbAchieved)
        {
            uint32_t Unlocktime{};

            Database() << "SELECT Unlocktime FROM Achievementprogress WHERE AppID = ? AND ClientID = ? AND API_Name = ?;"
                       << nGameID.AppID << Global.XUID.UserID << std::string(pchName) >> Unlocktime;

            *pbAchieved = !!Unlocktime;
            return true;
        }
        bool GetAchievement1(const char *pchName, bool *pbAchieved)
        {
            return GetAchievement0(GameID_t{ .AppID = Global.ApplicationID }, pchName, pbAchieved);
        }
        bool GetAchievementAchievedPercent(const char *pchName, float *pflPercent)
        {
            uint32_t Currentprogress{}, Maxprogress{};

            Database() << "SELECT Currentprogress FROM Achievementprogress WHERE AppID = ? AND ClientID = ? AND API_Name = ?;"
                       << Global.ApplicationID << Global.XUID.UserID << std::string(pchName) >> Currentprogress;

            Database() << "SELECT Maxprogress FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                       << Global.ApplicationID << std::string(pchName) >> Maxprogress;

            *pflPercent = float(Currentprogress) / float(Maxprogress) * 100;
            return true;
        }
        bool GetAchievementAndUnlockTime(const char *pchName, bool *pbAchieved, uint32_t *punUnlockTime)
        {
            return GetUserAchievementAndUnlockTime(Global.XUID, pchName, pbAchieved, punUnlockTime);
        }
        bool GetAchievementProgressLimitsFLOAT(const char *pchName, float *pfMinProgress, float *pfMaxProgress)
        {
            uint32_t Maxprogress{};

            Database() << "SELECT Maxprogress FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                       << Global.ApplicationID << std::string(pchName) >> Maxprogress;

            *pfMinProgress = 0;
            *pfMaxProgress = Maxprogress;

            return true;
        }
        bool GetAchievementProgressLimitsINT(const char *pchName, int32_t *pnMinProgress, int32_t *pnMaxProgress)
        {
            uint32_t Maxprogress{};

            Database() << "SELECT Maxprogress FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                << Global.ApplicationID << std::string(pchName) >> Maxprogress;

            *pnMinProgress = 0;
            *pnMaxProgress = Maxprogress;

            return true;
        }
        bool GetDownloadedLeaderboardEntry(SteamLeaderboardEntries_t hSteamLeaderboardEntries, int index, LeaderboardEntry_t *pLeaderboardEntry, int32_t *pDetails, int cDetailsMax)
        {
            if (!Downloadedentries.contains(hSteamLeaderboardEntries)) return false;

            const auto Entry = &Downloadedentries[hSteamLeaderboardEntries];
            if (index >= Entry->size()) return false;

            *pLeaderboardEntry = Entry->at(index);

            for (size_t i = 0; i < std::min(pLeaderboardEntry->m_cDetails, cDetailsMax); ++i)
            {
                pDetails[i] = Entry->at(index).Details[i];
            }

            return true;
        }
        bool GetGlobalStatFLOAT(const char *pchStatName, double *pData)
        {
            bool hasResult{};

            Database() << "SELECT Value FROM Globalstats WHERE Statname = ? LIMIT 1;" << std::string(pchStatName)
                       >> [&](std::variant<int64_t, double> Value)
                       {
                           if (Value.index() == 2)
                           {
                               *pData = std::get<double>(Value);
                               hasResult = true;
                           }
                       };

            return hasResult;
        }
        bool GetGlobalStatINT(const char *pchStatName, int64_t *pData)
        {
            bool hasResult{};

            Database() << "SELECT Value FROM Globalstats WHERE Statname = ? LIMIT 1;" << std::string(pchStatName)
                       >> [&](std::variant<int64_t, double> Value)
                       {
                           if (Value.index() == 1)
                           {
                               *pData = std::get<int64_t>(Value);
                               hasResult = true;
                           }
                       };

            return hasResult;
        }
        bool GetGroupAchievement(GameID_t nGameID, const char *pchName, bool *pbAchieved)
        {
            uint32_t Unlocktime{};

            Database() << "SELECT Unlocktime FROM Groupachievement WHERE AppID = ? AND API_Name = ?;"
                       << nGameID.AppID << std::string(pchName) >> Unlocktime;

            *pbAchieved = !!Unlocktime;
            return true;
        }
        bool GetStatFLOAT0(GameID_t nGameID, const char *pchName, float *pData)
        {
            bool hasResult{};

            Database() << "SELECT Value FROM Userstat WHERE Statname = ? AND ClientID = ? AND AppID = ? LIMIT 1;"
                       << std::string(pchName) << Global.XUID.UserID << nGameID.AppID
                       >> [&](std::variant<int32_t, float> Value)
                       {
                           if (Value.index() == 2)
                           {
                               *pData = std::get<float>(Value);
                               hasResult = true;
                           }
                       };

            return hasResult;
        }
        bool GetStatINT0(GameID_t nGameID, const char *pchName, int32_t *pData)
        {
            bool hasResult{};

            Database() << "SELECT Value FROM Userstat WHERE Statname = ? AND ClientID = ? AND AppID = ? LIMIT 1;"
                       << std::string(pchName) << Global.XUID.UserID << nGameID.AppID
                       >> [&](std::variant<int32_t, float> Value)
                       {
                           if (Value.index() == 1)
                           {
                               *pData = std::get<int32_t>(Value);
                               hasResult = true;
                           }
                       };

            return hasResult;
        }
        bool GetStatFLOAT1(const char *pchName, float *pData)
        {
            return GetStatFLOAT0(GameID_t{ .AppID = Global.ApplicationID }, pchName, pData);
        }
        bool GetStatINT1(const char *pchName, int32_t *pData)
        {
            return GetStatINT0(GameID_t{ .AppID = Global.ApplicationID }, pchName, pData);
        }
        bool GetUserAchievement(SteamID_t steamIDUser, const char *pchName, bool *pbAchieved)
        {
            return GetUserAchievementAndUnlockTime(steamIDUser, pchName, pbAchieved, nullptr);
        }
        bool GetUserAchievementAndUnlockTime(SteamID_t steamIDUser, const char *pchName, bool *pbAchieved, uint32_t *punUnlockTime)
        {
            uint32_t Unlocktime{};

            Database() << "SELECT Unlocktime FROM Achievementprogress WHERE AppID = ? AND ClientID = ? AND API_Name = ?;"
                       << Global.ApplicationID << steamIDUser.UserID << std::string(pchName) >> Unlocktime;

            *punUnlockTime = Unlocktime;
            *pbAchieved = !!Unlocktime;
            return true;
        }
        bool GetUserStatFLOAT(SteamID_t steamIDUser, const char *pchName, float *pData)
        { return false; }
        bool GetUserStatINT(SteamID_t steamIDUser, const char *pchName, int32_t *pData)
        { return false; }
        bool GetUserStatsData(void *pvData, uint32_t cubData, uint32_t *pcubWritten)
        {
            // Deprecated.
            Traceprint();
            return false;
        }
        bool IndicateAchievementProgress0(GameID_t nGameID, const char *pchName, uint32_t nCurProgress, uint32_t nMaxProgress)
        {
            // NOTE(tcn): Function does not set any stats, should only show a toaster-popup.
            return true;
        }
        bool IndicateAchievementProgress1(const char *pchName, uint32_t nCurProgress, uint32_t nMaxProgress)
        {
            return IndicateAchievementProgress0(GameID_t{ .AppID = Global.ApplicationID }, pchName, nCurProgress, nMaxProgress);
        }
        bool InstallPS3Trophies()
        { return true; }
        bool RequestCurrentStats1()
        { return true; }
        bool RequestCurrentStats0(GameID_t nGameID)
        { return true; }
        bool ResetAllStats(bool bAchievementsToo)
        {
            Database() << "DELETE FROM Userstat WHERE AppID = ? AND ClientID = ?;"
                       << Global.ApplicationID << Global.XUID.UserID;

            if (bAchievementsToo)
            {
                Database() << "DELETE FROM Achievementprogress WHERE AppID = ? AND ClientID = ?;"
                           << Global.ApplicationID << Global.XUID.UserID;
            }

            return true;
        }

        bool SetAchievement0(GameID_t nGameID, const char *pchName)
        {
            float Mi{}, Max{};
            GetAchievementProgressLimitsFLOAT(pchName, &Min, &Max);

            Database() << "REPLACE (AppID, ClientID, Currentprogress, Name, Unlocktime) INTO Achievementprogress VALUES (?,?,?,?,?);"
                       << nGameID.AppID << Global.XUID.UserID << Max << std::string(pchName) << uint32_t(time(NULL));

            return true;
        }
        bool SetAchievement1(const char *pchName)
        {
            return SetAchievement0(GameID_t{ .AppID = Global.ApplicationID }, pchName);
        }
        bool SetGroupAchievement(GameID_t nGameID, const char *pchName)
        {
            Database() << "REPLACE (Unlocktime) INTO Groupachievement VALUES(?) WHERE AppID = ? AND API_Name = ?;"
                       << uint32_t(time(NULL)) << nGameID.AppID << std::string(pchName);
            return true;
        }
        bool SetStatFLOAT0(GameID_t nGameID, const char *pchName, float fData)
        {
            Database() << "REPLACE (ClientID, AppID, Statname, Stattype, Value) INTO Userstat VALUES (?,?,?,?,?);"
                       << Global.XUID.UserID << nGameID.AppID << std::string(pchName) << uint32_t(k_ESteamUserStatTypeFLOAT) << fData;
            return true;
        }
        bool SetStatINT0(GameID_t nGameID, const char *pchName, int32_t nData)
        {
           Database() << "REPLACE (ClientID, AppID, Statname, Stattype, Value) INTO Userstat VALUES (?,?,?,?,?);"
                       << Global.XUID.UserID << nGameID.AppID << std::string(pchName) << uint32_t(k_ESteamUserStatTypeINT) << nData;
            return true;
        }
        bool SetStatFLOAT1(const char *pchName, float fData)
        {
            return SetStatFLOAT0(GameID_t{ .AppID = Global.ApplicationID }, pchName, fData);
        }
        bool SetStatINT1(const char *pchName, int32_t nData)
        {
            return SetStatINT0(GameID_t{ .AppID = Global.ApplicationID }, pchName, nData);
        }
        bool SetUserStatsData(const void *pvData, uint32_t cubData)
        {
            // Deprecated.
            Traceprint();
            return false;
        }
        bool StoreStats1()
        { return true; }
        bool StoreStats0(GameID_t nGameID)
        { return true; }
        bool UpdateAvgRateStat0(GameID_t nGameID, const char *pchName, float flCountThisSession, double dSessionLength)
        {
            return true;
        }
        bool UpdateAvgRateStat1(const char *pchName, float flCountThisSession, double dSessionLength)
        {
            return UpdateAvgRateStat0(GameID_t{ .AppID = Global.ApplicationID }, pchName, flCountThisSession, dSessionLength);
        }

        const char *GetAchievementDisplayAttribute0(GameID_t nGameID, const char *pchName, const char *pchKey)
        {
            static std::string Result{};

            if (Hash::WW32(pchKey) == Hash::WW32("name"))
            {
                Database() << "SELECT Name FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                           << nGameID.AppID << std::string(pchName) >> Result;
                return Result.c_str();
            }

            if (Hash::WW32(pchKey) == Hash::WW32("desc"))
            {
                Database() << "SELECT Description FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                           << nGameID.AppID << std::string(pchName) >> Result;
                return Result.c_str();
            }

            // If all else, the 'hidden' attribute.
            return "1";
        }
        const char *GetAchievementDisplayAttribute1(const char *pchName, const char *pchKey)
        {
            return GetAchievementDisplayAttribute0(GameID_t{ .AppID = Global.ApplicationID }, pchName, pchKey);
        }
        const char *GetAchievementName0(GameID_t nGameID, uint32_t iAchievement)
        {
            static std::string Result{};

            Database() << "SELECT Name FROM Achievement WHERE AppID = ? LIMIT 1 OFFSET ?;"
                       << nGameID.AppID << iAchievement >> Result;

            return Result.c_str();
        }
        const char *GetAchievementName1(uint32_t iAchievement)
        {
            return GetAchievementName0(GameID_t{ .AppID = Global.ApplicationID }, iAchievement);
        }
        const char *GetGroupAchievementName(GameID_t nGameID, uint32_t iAchievement)
        {
            static std::string Name{};

            Database() << "SELECT Name FROM Groupachievement WHERE AppID = ? LIMIT 1 OFFSET ?;"
                       << nGameID.AppID << iAchievement >> Name;

            return Name.c_str();
        }
        const char *GetLeaderboardName(SteamLeaderboard_t hSteamLeaderboard)
        {
            static std::string Result{};

            Database() << "SELECT Leaderboardname FROM Leaderboards WHERE AppID = ? AND LeaderboardID = ? LIMIT 1;"
                       << Global.ApplicationID << hSteamLeaderboard;

            return Result.c_str();
        }
        const char *GetStatName(GameID_t nGameID, uint32_t iStat)
        {
            static std::string Result{};

            Database() << "SELECT Statname FROM Userstat WHERE AppID = ? LIMIT 1 OFFSET ?;"
                       << nGameID.AppID << iStat >> Result;

            return Result.c_str();
        }

        int GetAchievementIcon0(GameID_t nGameID, const char *pchName);
        int GetAchievementIcon1(const char *pchName)
        {
            return GetAchievementIcon0(GameID_t{ .AppID = Global.ApplicationID }, pchName);
        }
        int GetLeaderboardEntryCount(SteamLeaderboard_t hSteamLeaderboard)
        {
            int Count{};

            Database() << "SELECT COUNT(*) FROM Leaderboards WHERE AppID = ? AND LeaderboardID = ?;"
                       << Global.ApplicationID << hSteamLeaderboard >> Count;

            return Count;
        }
        int GetMostAchievedAchievementInfo(char *pchName, uint32_t unNameBufLen, float *pflPercent, bool *pbAchieved);
        int GetNextMostAchievedAchievementInfo(int iIteratorPrevious, char *pchName, uint32_t unNameBufLen, float *pflPercent, bool *pbAchieved);

        int32_t GetGlobalStatHistoryFLOAT(const char *pchStatName, double *pData, uint32_t cubData)
        {
            if (!cubData) return 0;
            return GetGlobalStatFLOAT(pchStatName, pData);
        }
        int32_t GetGlobalStatHistoryINT(const char *pchStatName, int64_t *pData, uint32_t cubData)
        {
            if (!cubData) return 0;
            return GetGlobalStatINT(pchStatName, pData);
        }

        uint32_t GetNumAchievements1()
        {
            return GetNumAchievements0(GameID_t{ .AppID = Global.ApplicationID });
        }
        uint32_t GetNumAchievements0(GameID_t nGameID)
        {
            uint32_t Count{};

            Database() << "SELECT COUNT(*) FROM Achievement WHERE AppID = ?;"
                       << nGameID.AppID >> Count;

            return Count;
        }
        uint32_t GetNumGroupAchievements(GameID_t nGameID)
        {
            uint32_t Count{};

            Database() << "SELECT COUNT(*) FROM Groupachievement WHERE AppID = ?;"
                       << nGameID.AppID >> Count;

            return Count;
        }
        uint32_t GetNumStats(GameID_t nGameID)
        {
            uint32_t Count{};

            Database() << "SELECT COUNT(*) FROM Userstat WHERE AppID = ? AND ClientID = ?;"
                       << nGameID.AppID << Global.XUID.UserID >> Count;

            return Count;
        }

        uint64_t GetTrophySpaceRequiredBeforeInstall()
        {
            return 0;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamUserstats001 : Interface_t
    {
        SteamUserstats001()
        {
            Createmethod(0, SteamUserstats,  GetNumStats);
            Createmethod(1, SteamUserstats,  GetStatName);
            Createmethod(2, SteamUserstats,  GetStatType);
            Createmethod(3, SteamUserstats,  GetNumAchievements0);
            Createmethod(4, SteamUserstats,  GetAchievementName0);
            Createmethod(5, SteamUserstats,  GetNumGroupAchievements);
            Createmethod(6, SteamUserstats,  GetGroupAchievementName);
            Createmethod(7, SteamUserstats,  RequestCurrentStats0);
            Createmethod(8, SteamUserstats,  GetStatINT0);
            Createmethod(9, SteamUserstats,  GetStatFLOAT0);
            Createmethod(10, SteamUserstats, SetStatINT0);
            Createmethod(11, SteamUserstats, SetStatFLOAT0);
            Createmethod(12, SteamUserstats, UpdateAvgRateStat0);
            Createmethod(13, SteamUserstats, GetAchievement0);
            Createmethod(14, SteamUserstats, GetGroupAchievement);
            Createmethod(15, SteamUserstats, SetAchievement0);
            Createmethod(16, SteamUserstats, SetGroupAchievement);
            Createmethod(17, SteamUserstats, StoreStats0);
            Createmethod(18, SteamUserstats, ClearAchievement0);
            Createmethod(19, SteamUserstats, ClearGroupAchievement);
            Createmethod(20, SteamUserstats, GetAchClearGroupAchievementievementIcon0);
            Createmethod(21, SteamUserstats, GetAchievementDisplayAttribute0);
        };
    };
    struct SteamUserstats002 : Interface_t
    {
        SteamUserstats002()
        {
            Createmethod(0, SteamUserstats,  GetNumStats);
            Createmethod(1, SteamUserstats,  GetStatName);
            Createmethod(2, SteamUserstats,  GetStatType);
            Createmethod(3, SteamUserstats,  GetNumAchievements0);
            Createmethod(4, SteamUserstats,  GetAchievementName0);
            Createmethod(5, SteamUserstats,  RequestCurrentStats0);
            Createmethod(6, SteamUserstats,  GetStatINT0);
            Createmethod(7, SteamUserstats,  GetStatFLOAT0);
            Createmethod(8, SteamUserstats,  SetStatINT0);
            Createmethod(9, SteamUserstats,  SetStatFLOAT0);
            Createmethod(10, SteamUserstats, UpdateAvgRateStat0);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);

        };
    };
    struct SteamUserstats004 : Interface_t
    {
        SteamUserstats004()
        {
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStatINT);
            Createmethod(15, SteamUserstats, GetUserStatFLOAT);
            Createmethod(16, SteamUserstats, GetUserAchievement);
        };
    };
    struct SteamUserstats005 : Interface_t
    {
        SteamUserstats005()
        {
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStatINT);
            Createmethod(15, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  StoreStats1);
            Createmethod(10, SteamUserstats, GetAchievementIcon1);
            Createmethod(11, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(12, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(13, SteamUserstats, RequestUserStats);
            Createmethod(14, SteamUserstats, GetUserStatINT);
            Createmethod(15, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStatINT);
            Createmethod(16, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStatINT);
            Createmethod(16, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStatINT);
            Createmethod(16, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStatINT);
            Createmethod(16, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(37, SteamUserstats, GetGlobalStatINT);
            Createmethod(38, SteamUserstats, GetGlobalStatFLOAT);
            Createmethod(39, SteamUserstats, GetGlobalStatHistoryINT);
            Createmethod(40, SteamUserstats, GetGlobalStatHistoryFLOAT);
        };
    };
    struct SteamUserstats011 : Interface_t
    {
        SteamUserstats011()
        {
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, RequestUserStats);
            Createmethod(15, SteamUserstats, GetUserStatINT);
            Createmethod(16, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(37, SteamUserstats, GetGlobalStatINT);
            Createmethod(38, SteamUserstats, GetGlobalStatFLOAT);
            Createmethod(39, SteamUserstats, GetGlobalStatHistoryINT);
            Createmethod(40, SteamUserstats, GetGlobalStatHistoryFLOAT);
            Createmethod(41, SteamUserstats, InstallPS3Trophies);
            Createmethod(42, SteamUserstats, GetTrophySpaceRequiredBeforeInstall);
            Createmethod(43, SteamUserstats, SetUserStatsData);
            Createmethod(44, SteamUserstats, GetUserStatsData);

        };
    };
    struct SteamUserstats012 : Interface_t
    {
        SteamUserstats012()
        {
            Createmethod(0, SteamUserstats,  RequestCurrentStats1);
            Createmethod(1, SteamUserstats,  GetStatINT1);
            Createmethod(2, SteamUserstats,  GetStatFLOAT1);
            Createmethod(3, SteamUserstats,  SetStatINT1);
            Createmethod(4, SteamUserstats,  SetStatFLOAT1);
            Createmethod(5, SteamUserstats,  UpdateAvgRateStat1);
            Createmethod(6, SteamUserstats,  GetAchievement1);
            Createmethod(7, SteamUserstats,  SetAchievement1);
            Createmethod(8, SteamUserstats,  ClearAchievement1);
            Createmethod(9, SteamUserstats,  GetAchievementAndUnlockTime);
            Createmethod(10, SteamUserstats, StoreStats1);
            Createmethod(11, SteamUserstats, GetAchievementIcon1);
            Createmethod(12, SteamUserstats, GetAchievementDisplayAttribute1);
            Createmethod(13, SteamUserstats, IndicateAchievementProgress1);
            Createmethod(14, SteamUserstats, GetNumAchievements1);
            Createmethod(15, SteamUserstats, GetAchievementName1);
            Createmethod(16, SteamUserstats, RequestUserStats);
            Createmethod(17, SteamUserstats, GetUserStatINT);
            Createmethod(18, SteamUserstats, GetUserStatFLOAT);
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
            Createmethod(31, SteamUserstats, UploadLeaderboardScore);
            Createmethod(32, SteamUserstats, AttachLeaderboardUGC);
            Createmethod(33, SteamUserstats, GetNumberOfCurrentPlayers);
            Createmethod(34, SteamUserstats, RequestGlobalAchievementPercentages);
            Createmethod(35, SteamUserstats, GetMostAchievedAchievementInfo);
            Createmethod(36, SteamUserstats, GetNextMostAchievedAchievementInfo);
            Createmethod(37, SteamUserstats, GetAchievementAchievedPercent);
            Createmethod(38, SteamUserstats, RequestGlobalStats);
            Createmethod(39, SteamUserstats, GetGlobalStatINT);
            Createmethod(40, SteamUserstats, GetGlobalStatFLOAT);
            Createmethod(41, SteamUserstats, GetGlobalStatHistoryINT);
            Createmethod(42, SteamUserstats, GetGlobalStatHistoryFLOAT);
            Createmethod(43, SteamUserstats, GetAchievementProgressLimitsINT);
            Createmethod(44, SteamUserstats, GetAchievementProgressLimitsFLOAT);
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
            Register(Interfacetype_t::USERSTATS, "SteamUserstats012", SteamUserstats012);
        }
    };
    static Steamuserstatsloader Interfaceloader{};
}
