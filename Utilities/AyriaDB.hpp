/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-19
    License: MIT

    Prepared statements for common database operations.
    Errors return default constructed results, see Ayria.log for more info.
    NOTE(tcn): Using IDs other than those provided by Ayria is generally a no-op.
*/

#pragma once
#include <Stdinclude.hpp>

#if defined (__cplusplus)
namespace AyriaDB
{
    // For unfiltered access to the DB, use with care.
    inline sqlite::database Query()
    {
        try
        {
            sqlite::sqlite_config Config{};
            Config.flags = sqlite::OpenFlags::CREATE | sqlite::OpenFlags::READWRITE | sqlite::OpenFlags::FULLMUTEX;
            return sqlite::database("./Ayria/Client.db", Config);
        }
        catch (std::exception &e)
        {
            Errorprint(va("Could not connect to the database: %s", e.what()).c_str());
            return sqlite::database(":memory:");
        }
    }

    // Information about the active clients on the network.
    namespace Clientinfo
    {
        namespace Get
        {
            inline JSON::Object_t byID(uint32_t ClientID)
            {
                JSON::Object_t Result{};

                try
                {
                    Query() << "SELECT * FROM Clientinfo WHERE ClientID = ?;" << ClientID
                        >> [&](uint32_t, uint32_t Lastupdate, std::string &&B64Authticket,
                            std::string &&B64Sharedkey, uint32_t PublicIP, uint32_t GameID,
                            std::string &&Username, bool isLocal)
                        {
                            Result = JSON::Object_t({
                                { "B64Authticket", B64Authticket },
                                { "B64Sharedkey", B64Sharedkey },
                                { "Lastupdate", Lastupdate },
                                { "ClientID", ClientID },
                                { "Username", Username },
                                { "PublicIP", PublicIP },
                                { "isLocal", isLocal },
                                { "GameID", GameID }
                            });
                        };
                }
                catch (...) {}

                return Result;
            }
            inline JSON::Object_t byName(const std::string &Name)
            {
                JSON::Object_t Result{};

                try
                {
                    Query()
                        << "SELECT * FROM Clientinfo WHERE Username = ? LIMIT 1;" << Name
                        >> [&](uint32_t ClientID, uint32_t Lastupdate, std::string &&B64Authticket,
                            std::string &&B64Sharedkey, uint32_t PublicIP, uint32_t GameID,
                            std::string &&Username, bool isLocal)
                        {
                            Result = JSON::Object_t({
                                { "B64Authticket", B64Authticket },
                                { "B64Sharedkey", B64Sharedkey },
                                { "Lastupdate", Lastupdate },
                                { "ClientID", ClientID },
                                { "Username", Username },
                                { "PublicIP", PublicIP },
                                { "isLocal", isLocal },
                                { "GameID", GameID }
                                });
                        };
                } catch (...) {}

                return Result;
            }
            inline std::unordered_set<uint32_t> byGame(uint32_t GameID)
            {
                std::unordered_set<uint32_t> Result{};

                try
                {
                    Query() << "SELECT ClientID FROM Clientinfo WHERE GameID = ?;" << GameID
                            >> [&](uint32_t ClientID) { Result.insert(ClientID); };
                }
                catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void GameID(uint32_t ClientID, uint32_t GameID)
            {
                try
                {
                    Query() << "REPLACE INTO Clientinfo (ClientID, GameID) VALUES (?, ?);"
                            << ClientID << GameID;
                }
                catch (...) {}
            }
        }
    }

    // Key-value storage of what the user is doing.
    namespace Userpresence
    {
        inline void Clear(uint32_t ClientID)
        {
            try { Query() << "DELETE FROM Userpresence WHERE ClientID = ?;" << ClientID; }
            catch (...) {}
        }

        namespace Get
        {
            inline std::unordered_map<std::string, std::string> All(uint32_t ClientID)
            {
                std::unordered_map<std::string, std::string> Result{};

                try
                {
                    Query() << "SELECT Key, Value FROM Userpresence WHERE ClientID = ?;" << ClientID
                            >> [&](std::string &&Key, std::string &&Value) { Result.emplace(Key, Value); };
                } catch (...) {}

                return Result;
            }
            inline std::string byKey(uint32_t ClientID, const std::string &Key)
            {
                std::string Result{};

                try
                {
                    Query() << "SELECT Value FROM Userpresence WHERE ClientID = ? AND Key = ?;" << ClientID << Key
                            >> Result;
                } catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void Single(uint32_t ClientID, const std::string &Key, const std::string &Value)
            {
                try
                {
                    Query() << "REPLACE INTO Userpresence (ClientID, Key, Value) VALUES (?, ?, ?);"
                            << ClientID << Key << Value;
                } catch (...) {}
            }
            inline void Multiple(uint32_t ClientID, std::unordered_map<std::string, std::string> Keyvalues)
            {
                for (const auto &[Key, Value] : Keyvalues) Single(ClientID, Key, Value);
            }
        }
    }

    // Simple friendslist.
    namespace Relationships
    {
        inline bool isBlocked(uint32_t SourceID, uint32_t TargetID)
        {
            bool isBlocked{};

            try
            {
                Query() << "SELECT isBlocked FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                        << SourceID << TargetID >> isBlocked;
            } catch (...) {}

            return isBlocked;
        }
        inline bool isFriend(uint32_t SourceID, uint32_t TargetID)
        {
            bool isFriend{};

            try
            {
                Query() << "SELECT isFriend FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                        << SourceID << TargetID >> isFriend;
            } catch (...) {}

            return isFriend;
        }
        inline void Clear(uint32_t SourceID, uint32_t TargetID)
        {
            try { Query() << "DELETE FROM Relationships WHERE SourceID = ? AND TargetID = ?;" << SourceID << TargetID; }
            catch (...) {}
        }

        namespace Get
        {
            inline std::unordered_set<uint32_t> Friends(uint32_t ClientID)
            {
                std::unordered_set<uint32_t> Result{};

                try
                {
                    Query() << "SELECT TargetID FROM Relationships WHERE SourceID = ? AND isFriend = true;" << ClientID
                            >> [&](uint32_t FriendID) { Result.insert(FriendID); };
                }
                catch (...) {}

                return Result;
            }
            inline std::unordered_set<uint32_t> Blocked(uint32_t ClientID)
            {
                std::unordered_set<uint32_t> Result{};

                try
                {
                    Query() << "SELECT TargetID FROM Relationships WHERE SourceID = ? AND isBlocked = true;" << ClientID
                            >> [&](uint32_t BlockedID) { Result.insert(BlockedID); };
                }
                catch (...) {}

                return Result;
            }

            inline std::unordered_set<uint32_t> Friendrequests(uint32_t ClientID)
            {
                std::unordered_set<uint32_t> Result{};

                try
                {
                    Query()
                        << "SELECT TargetID FROM Relationships WHERE TargetID = ? AND isFriend = true;" << ClientID
                        >> [&](uint32_t FriendID) { Result.insert(FriendID); };
                } catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void Blocked(uint32_t SourceID, uint32_t TargetID)
            {
                try
                {
                    Query() << "REPLACE INTO Relationships (SourceID, TargetID, isBlocked, isFriend) VALUES (?, ?, ?, ?);"
                            << SourceID << TargetID << true << false;
                }
                catch (...) {}
            }
            inline void Friend(uint32_t SourceID, uint32_t TargetID)
            {
                try
                {
                    Query() << "REPLACE INTO Relationships (SourceID, TargetID, isBlocked, isFriend) VALUES (?, ?, ?, ?);"
                            << SourceID << TargetID << false << true;
                }
                catch (...) {}
            }
        }
    }

    // Base building block for other services.
    namespace Usergroups
    {
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

        namespace Get
        {
            inline JSON::Object_t byID(uint64_t GroupID)
            {
                JSON::Object_t Result{};

                try
                {
                    Query() << "SELECT * FROM Usergroups WHERE GroupID = ?;" << GroupID
                            >> [&](uint64_t, uint32_t GameID, uint32_t Lastupdate, std::vector<uint32_t> MemberIDs,
                                   std::string &&Memberdata, std::string &&Groupdata)
                            {
                                Result = JSON::Object_t({
                                    { "Memberlimit", GroupID_t{ GroupID }.Limit },
                                    { "RoomID",  GroupID_t{ GroupID }.RoomID },
                                    { "Lastupdate", Lastupdate },
                                    { "Memberdata", Memberdata },
                                    { "Groupdata", Groupdata },
                                    { "MemberIDs", MemberIDs },
                                    { "GroupID", GroupID },
                                    { "GameID", GameID }
                                });
                            };
                } catch (...) {}

                return Result;
            }
            inline std::unordered_set<uint64_t> byGame(uint32_t GameID)
            {
                std::unordered_set<uint64_t> Result{};

                try
                {
                    Query() << "SELECT GroupID FROM Usergroups WHERE GameID = ?;" << GameID
                            >> [&](uint64_t GroupID) { Result.insert(GroupID); };
                } catch (...) {}

                return Result;
            }
            inline std::unordered_set<uint64_t> byType(uint8_t Type)
            {
                std::unordered_set<uint64_t> Result{};

                try
                {
                    Query() << "SELECT GroupID FROM Usergroups WHERE (GroupID & ?) = true;" << Type
                        >> [&](uint64_t GroupID) { Result.insert(GroupID); };
                } catch (...) {}

                return Result;
            }

            inline std::vector<uint32_t> MemberIDs(uint64_t GroupID)
            {
                std::vector<uint32_t> Result{};

                try
                {
                    Query() << "SELECT MemberIDs FROM Usergroups WHERE GroupID = ?;" << GroupID >> Result;
                } catch (...) {}

                return Result;
            }
            inline std::string Memberdata(uint64_t GroupID)
            {
                std::string Result{};

                try
                {
                    Query() << "SELECT Memberdata FROM Usergroups WHERE GroupID = ?;" << GroupID >> Result;
                } catch (...) {}

                return Result;
            }
            inline std::string Groupdata(uint64_t GroupID)
            {
                std::string Result{};

                try
                {
                    Query() << "SELECT Groupdata FROM Usergroups WHERE GroupID = ?;" << GroupID >> Result;
                } catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void MemberIDs(uint64_t GroupID, const std::vector<uint32_t> &MemberIDs)
            {
                try
                {
                    Query() << "REPLACE INTO Usergroups (GroupID, MemberIDs) VALUES (?, ?);"
                            << GroupID << MemberIDs;
                } catch (...) {}
            }
            inline void Memberdata(uint64_t GroupID, const std::string &Memberdata)
            {
                try
                {
                    Query() << "REPLACE INTO Usergroups (GroupID, Memberdata) VALUES (?, ?);"
                            << GroupID << Memberdata;
                } catch (...) {}
            }
            inline void Groupdata(uint64_t GroupID, const std::string &Groupdata)
            {
                try
                {
                    Query() << "REPLACE INTO Usergroups (GroupID, Groupdata) VALUES (?, ?);"
                            << GroupID << Groupdata;
                } catch (...) {}
            }
        }
    }

    namespace Matchmakingsessions
    {
        namespace Get
        {
            inline std::unordered_set<uint32_t> byGame(uint32_t GameID)
            {
                std::unordered_set<uint32_t> Result{};

                try
                {
                    Query()
                        << "SELECT HostID FROM Matchmakingsessions WHERE GameID = ?;" << GameID
                        >> [&](uint32_t HostID) { Result.insert(HostID); };
                }
                catch (...) {}

                return Result;
            }
            inline JSON::Object_t byID(uint32_t HostID)
            {
                JSON::Object_t Result{};

                try
                {
                    Query()
                        << "SELECT * FROM Matchmakingsessions WHERE HostID = ?;" << HostID
                        >> [&](uint32_t, uint32_t Lastupdate, uint32_t GameID, uint32_t Hostaddress, uint16_t Hostport, std::string &&Gamedata)
                        {
                            Result = JSON::Object_t({
                                { "Gamedata", std::move(Gamedata) },
                                { "Hostaddress", Hostaddress },
                                { "Lastupdate", Lastupdate },
                                { "Hostport", Hostport },
                                { "HostID", HostID },
                                { "GameID", GameID }
                            });
                        };
                } catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void Gamedata(uint32_t HostID, std::string &&Gamedata)
            {
                try
                {
                    Query() << "REPLACE INTO Matchmakingsessions (HostID, Gamedata) VALUES (?, ?);" << HostID << Gamedata;
                }
                catch (...) {}
            }
        }
    }

}
#endif
