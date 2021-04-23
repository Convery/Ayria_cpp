/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct Entry_t : LeaderboardEntry_t
    {
        std::vector<int32_t> Details;

        operator LeaderboardEntry_t()
        {
            m_cDetails = (int32_t)Details.size();
            return *this;
        }
    };
    static Hashmap<uint64_t, std::vector<Entry_t>> Downloadedentries;

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
            return k_ELeaderboardDisplayTypeNumeric;
        }
        ELeaderboardSortMethod GetLeaderboardSortMethod(SteamLeaderboard_t hSteamLeaderboard)
        {
            bool isDecending{};

            try
            {
                AyriaDB::Query()
                    << "SELECT isDecending FROM Leaderboard WHERE (LeaderboardID = ? AND GameID = ?);"
                    << hSteamLeaderboard << Global.ApplicationID >> isDecending;
            } catch (...) {}

            if (isDecending) return k_ELeaderboardSortMethodDescending;
            else return k_ELeaderboardSortMethodAscending;
        }
        ESteamUserStatType GetStatType(GameID_t nGameID, const char *pchName)
        {
            bool isFloat{};

            try
            {
                AyriaDB::Query()
                    << "SELECT isFloat FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?);"
                    << pchName << nGameID.AppID >> isFloat;
            } catch (...) {}

            if (isFloat) return k_ESteamUserStatTypeFLOAT;
            else return k_ESteamUserStatTypeINT;
        }

        SteamAPICall_t AttachLeaderboardUGC(SteamLeaderboard_t hSteamLeaderboard, UGCHandle_t hUGC)
        {
            return {};
        }
        SteamAPICall_t DownloadLeaderboardEntries(SteamLeaderboard_t hSteamLeaderboard, ELeaderboardDataRequest eLeaderboardDataRequest, int nRangeStart, int nRangeEnd);
        SteamAPICall_t DownloadLeaderboardEntriesForUsers(SteamLeaderboard_t hSteamLeaderboard, SteamID_t *prgUsers, int cUsers);
        SteamAPICall_t FindLeaderboard(const char *pchLeaderboardName)
        {
            uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID FROM Leaderboards WHERE Leaderboardname = ? LIMIT 1;"
                    << std::string(pchLeaderboardName)
                    >> LeaderboardID;
            }
            catch (...) {}

            const auto RequestID = Callbacks::Createrequest();
            auto Request = new LeaderboardFindResult_t();
            Request->m_hSteamLeaderboard = LeaderboardID;
            Request->m_bLeaderboardFound = !!LeaderboardID;

            Callbacks::Completerequest(RequestID, Types::LeaderboardFindResult_t, Request);
            return RequestID;
        }
        SteamAPICall_t FindOrCreateLeaderboard(const char *pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType)
        {
            uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID FROM Leaderboards WHERE Leaderboardname = ? LIMIT 1;"
                    << std::string(pchLeaderboardName)
                    >> LeaderboardID;
            }
            catch (...) {}

            try
            {
                if (!LeaderboardID)
                {
                    LeaderboardID = GetTickCount();

                    AyriaDB::Query()
                        << "INSERT INTO Leaderboards (LeaderboardID, Leaderboardname, isDecending, GameID, isFloat) VALUES (?, ?, ?, ?, ?);"
                        << LeaderboardID << std::string(pchLeaderboardName) << bool(eLeaderboardSortMethod != k_ELeaderboardSortMethodAscending)
                        << Global.ApplicationID << bool(eLeaderboardDisplayType != k_ELeaderboardDisplayTypeNumeric);
                }
            }
            catch (...) {}

            const auto RequestID = Callbacks::Createrequest();
            auto Request = new LeaderboardFindResult_t();
            Request->m_hSteamLeaderboard = LeaderboardID;
            Request->m_bLeaderboardFound = !!LeaderboardID;

            Callbacks::Completerequest(RequestID, Types::LeaderboardFindResult_t, Request);
            return RequestID;
        }
        SteamAPICall_t GetNumberOfCurrentPlayers()
        {
            uint32_t Count{};

            try { AyriaDB::Query() << "SELECT COUNT(*) FROM Clientinfo;" >> Count; }
            catch (...) {}

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
            bool Changed{ true };

            if (eLeaderboardUploadScoreMethod == k_ELeaderboardUploadScoreMethodKeepBest)
            {
                int32_t Currentscore{};
                try
                {
                    AyriaDB::Query()
                        << "SELECT Score FROM Leaderboardentry WHERE (LeaderboardID = ? AND ClientID = ? AND GameID = ?) LIMIT 1;"
                        << hSteamLeaderboard << Global.XUID.UserID << Global.ApplicationID >> Currentscore;
                }
                catch (...) {}

                Changed = nScore > Currentscore;
                nScore = std::max(Currentscore, nScore);
            }

            try
            {
                AyriaDB::Query()
                    << "REPLACE INTO Leaderboardentry (LeaderboardID, Timestamp, ClientID, GameID, iScore) VALUES (?,?,?,?,?);"
                    << hSteamLeaderboard << time(NULL) << Global.XUID.UserID << Global.ApplicationID << nScore;
            }
            catch (...) {}

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
            try
            {
                Database()
                    << "REPLACE INTO Achievementprogress (Currentprogress, Unlocktime) VALUES (0, 0) WHERE (AppID = ? AND API_Name = ? AND ClientID = ?);"
                    << nGameID.AppID<< std::string(pchName) << Global.XUID.UserID;

            } catch (...) {}

            return true;
        }
        bool ClearAchievement1(const char *pchName)
        {
            return ClearAchievement0(GameID_t{ .AppID = Global.ApplicationID }, pchName);
        }
        bool ClearGroupAchievement(GameID_t nGameID, const char *pchName)
        {
            return ClearAchievement0(nGameID, pchName);
        }
        bool GetAchievement0(GameID_t nGameID, const char *pchName, bool *pbAchieved)
        {
            uint32_t Unlocktime{};

            try
            {
                Database()
                    << "SELECT Unlocktime FROM Achievementprogress WHERE AppID = ? AND ClientID = ? AND API_Name = ?;"
                    << nGameID.AppID << Global.XUID.UserID << std::string(pchName) >> Unlocktime;
            }
            catch (...) {}

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

            try
            {
                Database()
                    << "SELECT Maxprogress FROM Achievement WHERE (AppID = ? AND API_Name = ?);"
                    << Global.ApplicationID << std::string(pchName) >> Maxprogress;

                Database()
                    << "SELECT Currentprogress FROM Achievementprogress WHERE (AppID = ? AND ClientID = ? AND API_Name = ?);"
                    << Global.ApplicationID << Global.XUID.UserID << std::string(pchName) >> Currentprogress;
            }
            catch (...) {}

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

            try
            {
                Database()
                    << "SELECT Maxprogress FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                    << Global.ApplicationID << std::string(pchName) >> Maxprogress;
            }
            catch (...) {}

            *pfMinProgress = 0;
            *pfMaxProgress = Maxprogress;

            return true;
        }
        bool GetAchievementProgressLimitsINT(const char *pchName, int32_t *pnMinProgress, int32_t *pnMaxProgress)
        {
            uint32_t Maxprogress{};

            try
            {
                Database()
                    << "SELECT Maxprogress FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                    << Global.ApplicationID << std::string(pchName) >> Maxprogress;
            }
            catch (...) {}

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
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = 0) LIMIT 1;"
                    << std::string(pchStatName)
                    >> std::tie(LeaderboardID, isDecending);

                if (isDecending)
                {
                    AyriaDB::Query()
                        << "SELECT fScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY fScore DESC LIMIT 1;"
                        << LeaderboardID
                        >> [&](double Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
                else
                {
                    AyriaDB::Query()
                        << "SELECT fScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY fScore ASC LIMIT 1;"
                        << LeaderboardID
                        >> [&](double Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
            }  catch (...) {}

            return hasResult;
        }
        bool GetGlobalStatINT(const char *pchStatName, int64_t *pData)
        {
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = 0) LIMIT 1;"
                    << std::string(pchStatName)
                    >> std::tie(LeaderboardID, isDecending);

                if (isDecending)
                {
                    AyriaDB::Query()
                        << "SELECT iScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY iScore DESC LIMIT 1;"
                        << LeaderboardID
                        >> [&](int64_t Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
                else
                {
                    AyriaDB::Query()
                        << "SELECT iScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY iScore ASC LIMIT 1;"
                        << LeaderboardID
                        >> [&](int64_t Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
            }  catch (...) {}

            return hasResult;
        }
        bool GetGroupAchievement(GameID_t nGameID, const char *pchName, bool *pbAchieved)
        {
            uint32_t Unlocktime{};

            try
            {
                Database()
                    << "SELECT Unlocktime FROM Achievementprogress WHERE (AppID = ? AND Name = ? AND ClientID = ?);"
                    << nGameID.AppID << std::string(pchName) << Global.XUID.UserID >> Unlocktime;
            }
            catch (...) {}

            *pbAchieved = !!Unlocktime;
            return true;
        }
        bool GetStatFLOAT0(GameID_t nGameID, const char *pchName, float *pData)
        {
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << nGameID.FullID
                    >> std::tie(LeaderboardID, isDecending);

                if (isDecending)
                {
                    AyriaDB::Query()
                        << "SELECT fScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY fScore DESC LIMIT 1;"
                        << LeaderboardID
                        >> [&](double Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
                else
                {
                    AyriaDB::Query()
                        << "SELECT fScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY fScore ASC LIMIT 1;"
                        << LeaderboardID
                        >> [&](double Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
            }  catch (...) {}

            return hasResult;
        }
        bool GetStatINT0(GameID_t nGameID, const char *pchName, int32_t *pData)
        {
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << nGameID.FullID
                    >> std::tie(LeaderboardID, isDecending);

                if (isDecending)
                {
                    AyriaDB::Query()
                        << "SELECT iScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY iScore DESC LIMIT 1;"
                        << LeaderboardID
                        >> [&](int64_t Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
                else
                {
                    AyriaDB::Query()
                        << "SELECT iScore FROM Leaderboardentry WHERE LeaderboardID = ? ORDER BY iScore ASC LIMIT 1;"
                        << LeaderboardID
                        >> [&](int64_t Value)
                        {
                            *pData = Value;
                            hasResult = true;
                        };
                }
            }  catch (...) {}

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

            try
            {
                Database()
                    << "SELECT Unlocktime FROM Achievementprogress WHERE (ClientID = ? AND Name = ?);"
                    << steamIDUser.UserID << std::string(pchName) >> Unlocktime;
            }
            catch (...) {}

            *punUnlockTime = Unlocktime;
            *pbAchieved = !!Unlocktime;
            return true;
        }
        bool GetUserStatFLOAT(SteamID_t steamIDUser, const char *pchName, float *pData)
        {
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << Global.ApplicationID
                    >> std::tie(LeaderboardID, isDecending);

                AyriaDB::Query()
                    << "SELECT fScore FROM Leaderboardentry WHERE (LeaderboardID = ? AND ClientID = ?) LIMIT 1;"
                    << LeaderboardID << Global.XUID.UserID
                    >> [&](double Value)
                    {
                        *pData = Value;
                        hasResult = true;
                    };
            }  catch (...) {}

            return hasResult;
        }
        bool GetUserStatINT(SteamID_t steamIDUser, const char *pchName, int32_t *pData)
        {
            bool hasResult{}, isDecending{}; uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID, isDecending FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << Global.ApplicationID
                    >> std::tie(LeaderboardID, isDecending);

                AyriaDB::Query()
                    << "SELECT iScore FROM Leaderboardentry WHERE (LeaderboardID = ? AND ClientID = ?) LIMIT 1;"
                    << LeaderboardID << Global.XUID.UserID
                    >> [&](int64_t Value)
                    {
                        *pData = Value;
                        hasResult = true;
                    };
            }  catch (...) {}

            return hasResult;
        }
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
            try
            {
                AyriaDB::Query()
                    << "DELETE FROM Leaderboardentry WHERE (GameID = ? AND ClientID = ?);"
                    << Global.ApplicationID << Global.XUID.UserID;

                if (bAchievementsToo)
                {
                    Database()
                        << "REPLACE INTO Achievementprogress (Currentprogress, Unlocktime) VALUES (0, 0) WHERE (AppID = ? AND ClientID = ?);"
                        << Global.ApplicationID << Global.XUID.UserID;
                }
            }
            catch (...) {}

            return true;
        }

        bool SetAchievement0(GameID_t nGameID, const char *pchName)
        {
            float Min{}, Max{};
            GetAchievementProgressLimitsFLOAT(pchName, &Min, &Max);

            try
            {
                Database()
                    << "REPLACE INTO Achievementprogress (AppID, ClientID, Currentprogress, Name, Unlocktime) VALUES (?,?,?,?,?);"
                    << nGameID.AppID << Global.XUID.UserID << Max << std::string(pchName) << uint32_t(time(NULL));
            }
            catch (...) {}

            return true;
        }
        bool SetAchievement1(const char *pchName)
        {
            return SetAchievement0(GameID_t{ .AppID = Global.ApplicationID }, pchName);
        }
        bool SetGroupAchievement(GameID_t nGameID, const char *pchName)
        {
            return SetAchievement0(nGameID, pchName);
        }
        bool SetStatFLOAT0(GameID_t nGameID, const char *pchName, float fData)
        {
            uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << nGameID.FullID
                    >> LeaderboardID;

                AyriaDB::Query()
                    << "REPLACE INTO Leaderboardentry (LeaderboardID, Timestamp, ClientID, GameID, fScore) VALUES (?, ?, ?, ?, ?);"
                    << LeaderboardID << time(NULL) << Global.XUID.UserID << nGameID.FullID << fData;
            }
            catch (...) {}

            return true;
        }
        bool SetStatINT0(GameID_t nGameID, const char *pchName, int32_t nData)
        {
            uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID FROM Leaderboard WHERE (Leaderboardname = ? AND GameID = ?) LIMIT 1;"
                    << std::string(pchName) << nGameID.FullID
                    >> LeaderboardID;

                AyriaDB::Query()
                    << "REPLACE INTO Leaderboardentry (LeaderboardID, Timestamp, ClientID, GameID, iScore) VALUES (?, ?, ?, ?, ?);"
                    << LeaderboardID << time(NULL) << Global.XUID.UserID << nGameID.FullID << nData;
            }
            catch (...) {}

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

            try
            {
                if (Hash::WW32(pchKey) == Hash::WW32("name"))
                {
                    Database()
                        << "SELECT Name FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                        << nGameID.AppID << std::string(pchName)
                        >> Result;

                    return Result.c_str();
                }

                if (Hash::WW32(pchKey) == Hash::WW32("desc"))
                {
                    Database()
                        << "SELECT Description FROM Achievement WHERE AppID = ? AND API_Name = ?;"
                        << nGameID.AppID << std::string(pchName)
                        >> Result;

                    return Result.c_str();
                }
            }
            catch (...) {}

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

            try
            {
                Database()
                    << "SELECT Name FROM Achievement WHERE AppID = ? LIMIT 1 OFFSET ?;"
                    << nGameID.AppID << iAchievement >> Result;
            }
            catch (...) {}

            return Result.c_str();
        }
        const char *GetAchievementName1(uint32_t iAchievement)
        {
            return GetAchievementName0(GameID_t{ .AppID = Global.ApplicationID }, iAchievement);
        }
        const char *GetGroupAchievementName(GameID_t nGameID, uint32_t iAchievement)
        {
            return GetAchievementName0(nGameID, iAchievement);
        }
        const char *GetLeaderboardName(SteamLeaderboard_t hSteamLeaderboard)
        {
            static std::string Result{};

            try
            {
                AyriaDB::Query()
                    << "SELECT Leaderboardname FROM Leaderboards WHERE (GameID = ? AND LeaderboardID = ?);"
                    << Global.ApplicationID << hSteamLeaderboard >> Result;
            }
            catch (...) {}

            return Result.c_str();
        }
        const char *GetStatName(GameID_t nGameID, uint32_t iStat)
        {
            static std::string Result{};
            uint64_t LeaderboardID{};

            try
            {
                AyriaDB::Query()
                    << "SELECT LeaderboardID FROM Leaderboardentry WHERE (GameID = ? AND ClientID = ?) LIMIT 1 OFFSET ?;"
                    << nGameID.FullID << Global.XUID.UserID << iStat >> LeaderboardID;

                AyriaDB::Query()
                    << "SELECT Leaderboardname FROM Leaderboards WHERE (GameID = ? AND LeaderboardID = ?);"
                    << Global.ApplicationID << LeaderboardID >> Result;
            }
            catch (...) {}

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

            try
            {
                AyriaDB::Query()
                    << "SELECT COUNT(*) FROM Leaderboardentry WHERE GameID = ? AND LeaderboardID = ?;"
                    << Global.ApplicationID << hSteamLeaderboard >> Count;
            }
            catch (...) {}

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

            try
            {
                Database()
                    << "SELECT COUNT(*) FROM Achievement WHERE AppID = ?;"
                    << nGameID.AppID >> Count;
            }
            catch (...) {}

            return Count;
        }
        uint32_t GetNumGroupAchievements(GameID_t nGameID)
        {
            return GetNumAchievements0(nGameID);
        }
        uint32_t GetNumStats(GameID_t nGameID)
        {
            uint32_t Count{};

            try
            {
                AyriaDB::Query()
                    << "SELECT COUNT(*) FROM Leaderboardentry WHERE (GameID = ? AND ClientID = ?);"
                    << nGameID.FullID << Global.XUID.UserID >> Count;
            }
            catch (...) {}

            return Count;
        }

        uint64_t GetTrophySpaceRequiredBeforeInstall()
        {
            return 0;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamUserstats001 : Interface_t<22>
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
            Createmethod(20, SteamUserstats, GetAchievementIcon0);
            Createmethod(21, SteamUserstats, GetAchievementDisplayAttribute0);
        };
    };
    struct SteamUserstats002 : Interface_t<18>
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
    struct SteamUserstats003 : Interface_t<13>
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
    struct SteamUserstats004 : Interface_t<17>
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
    struct SteamUserstats005 : Interface_t<27>
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
    struct SteamUserstats006 : Interface_t<28>
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
    struct SteamUserstats007 : Interface_t<30>
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
    struct SteamUserstats008 : Interface_t<31>
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
    struct SteamUserstats009 : Interface_t<32>
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
    struct SteamUserstats010 : Interface_t<41>
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
    struct SteamUserstats011 : Interface_t<45>
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
    struct SteamUserstats012 : Interface_t<45>
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
            Createmethod(31, SteamUserstats, UploadLeaderboardScore1);
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
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
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
