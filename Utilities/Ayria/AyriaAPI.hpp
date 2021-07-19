/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-19
    License: MIT

    Prepared statements for common database operations.
    Errors return default constructed results, see Ayria.log for more info.
    NOTE(tcn): Using IDs other than those provided by Ayria is generally a no-op.
*/

#if defined (__cplusplus)
#pragma once
#include <Stdinclude.hpp>
#include "Ayriamodule.h"

namespace AyriaAPI
{
    // For unfiltered access to the DB, use with care.
    inline sqlite::database Query()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database) [[unlikely]]
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems..
            const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
        }

        return sqlite::database(Database);
    }

    // Get the primary key for a client.
    inline uint32_t getKeyhash(uint32_t AccountID)
    {
        uint32_t Keyhash = 0;

        try { Query() << "SELECT Sharedkeyhash FROM Knownclients WHERE AccountID = ? LIMIT 1;"
                      << AccountID >> Keyhash;
        } catch (...) {}

        return Keyhash;
    }
    inline uint32_t getAccountID(uint32_t Keyhash)
    {
        uint32_t AccountID = 0;

        try { Query() << "SELECT AccountID FROM Knownclients WHERE Sharedkeyhash = ? LIMIT 1;"
                      << Keyhash >> AccountID;
        } catch (...) {}

        return AccountID;
    }

    // Information about the active clients on the network.
    namespace Clientinfo
    {
        namespace Get
        {
            inline JSON::Value_t Basicinfo(uint32_t AccountID)
            {
                JSON::Object_t Result{};
                try
                {
                    Query() << "SELECT * FROM Knownclients WHERE Sharedkeyhash = ?;" << getKeyhash(AccountID)
                            >> [&](uint32_t Sharedkeyhash, uint32_t AccountID, const std::string &Username, bool Validated, uint32_t Timestamp)
                            {
                                Result = JSON::Object_t({
                                    { "Sharedkeyhash", Sharedkeyhash },
                                    { "AccountID", AccountID },
                                    { "Username", Username },
                                    { "Validated", Validated },
                                    { "Timestamp", Timestamp }
                                });
                            };
                } catch (...) {}

                return Result;
            }
            inline JSON::Value_t Extendedinfo(uint32_t AccountID)
            {
                JSON::Object_t Result{};
                try
                {
                    Query() << "SELECT * FROM Clientinfo WHERE AyriaID = ?;" << getKeyhash(AccountID)
                            >> [&](uint32_t AyriaID, const std::string &Sharedkey, uint32_t InternalIP, uint32_t ExternalIP, uint32_t GameID, uint32_t ModID)
                            {
                                Result = JSON::Object_t({
                                    { "AyriaID", AyriaID},
                                    { "Sharedkey", Sharedkey},
                                    { "InternalIP", InternalIP},
                                    { "ExternalIP", ExternalIP},
                                    { "GameID", GameID},
                                    { "ModID", ModID}
                                });
                            };
                } catch (...) {}

                return Result;
            }

            inline uint32_t Lastmessage(uint32_t AccountID)
            {
                uint32_t Timestamp{};

                try { Query() << "SELECT Timestamp FROM Knownclients WHERE AccountID = ? LIMIT 1;"
                              << AccountID >> Timestamp;
                } catch (...) {}

                return uint32_t(time(NULL)) - Timestamp;
            }
            inline uint32_t AccountID(const std::string &Username)
            {
                uint32_t AccountID{};

                try
                {
                    Query() << "SELECT AccountID FROM Knownclients WHERE Username = ? LIMIT 1;"
                            << Username >> AccountID;
                } catch (...) {}

                return AccountID;
            }

            inline std::unordered_set<uint32_t> byGame(std::optional<uint32_t> GameID, std::optional<uint32_t> ModID)
            {
                std::unordered_set<uint32_t> AccountIDs{}, AyriaIDs{};

                try
                {
                    // Both by game and mod.
                    if (GameID && ModID)
                    {
                        Query() << "SELECT AyriaID FROM Clientinfo WHERE (GameID = ? AND ModID = ?);"
                                << GameID.value() << ModID.value()
                                >> [&](uint32_t AyriaID) { AyriaIDs.insert(AyriaID); };
                    }

                    // Only by game.
                    else if (GameID)
                    {
                        Query() << "SELECT AyriaID FROM Clientinfo WHERE GameID = ?;"
                                << GameID.value()
                                >> [&](uint32_t AyriaID) { AyriaIDs.insert(AyriaID); };
                    }

                    // Only by mod.
                    else if (ModID)
                    {
                        Query() << "SELECT AyriaID FROM Clientinfo WHERE ModID = ?;"
                                << ModID.value()
                                >> [&](uint32_t AyriaID) { AyriaIDs.insert(AyriaID); };
                    }

                    // Any and all.
                    else
                    {
                        Query() << "SELECT AccountID FROM Knownclients;"
                                >> [&](uint32_t AccountID) { AccountIDs.insert(AccountID); };
                    }

                } catch (...) {}


                for (const auto &ID : AyriaIDs) AccountIDs.insert(getAccountID(ID));

                return AccountIDs;
            }
        }

        namespace Set
        {
            inline void Socialstate(std::optional<bool> isPrivate, std::optional<bool> isAway)
            {
                JSON::Object_t Request{};
                if (isPrivate) Request["isPrivate"] = isPrivate.value();
                if (isAway) Request["isAway"] = isAway.value();

                Ayria.doRequest("setSocialstate", Request);
            }
            inline void Gamestate(std::optional<bool> isHosting, std::optional<bool> isIngame, std::optional<uint32_t> GameID, std::optional<uint32_t> ModID)
            {
                JSON::Object_t Request{};
                if (isHosting) Request["isHosting"] = isHosting.value();
                if (isIngame) Request["isIngame"] = isIngame.value();
                if (GameID) Request["GameID"] = GameID.value();
                if (ModID) Request["ModID"] = ModID.value();

                Ayria.doRequest("setGamestate", Request);
            }
        }
    }

    // Key-value storage of what the user is doing.
    namespace Clientpresence
    {
        namespace Get
        {
            inline std::string byKey(const std::string &Key, uint32_t AccountID)
            {
                std::string Value{};

                try
                {
                    Query() << "SELECT Value FROM Clientpresence WHERE (AyriaID = ? AND Key = ?) LIMIT 1;"
                            << getKeyhash(AccountID) << Key >> Value;
                } catch (...) {}

                return Value;
            }
            inline std::vector<std::pair<uint32_t, std::string>> byKey(const std::string &Key)
            {
                std::vector<std::pair<uint32_t, std::string>> Result{};

                try
                {
                    Query() << "SELECT AccountID, Value FROM Clientpresence WHERE Key = ?;"
                            << Key >> [&](uint32_t AccountID, const std::string &Value)
                                      { Result.emplace_back(AccountID, Value); };
                } catch (...) {}

                return Result;
            }
            inline std::vector<std::pair<std::string, std::string>> byAccountID(uint32_t AccountID)
            {
                std::vector<std::pair<std::string, std::string>> Result{};

                try
                {
                    Query() << "SELECT Key, Value FROM Clientpresence WHERE AyriaID = ?;"
                            << getKeyhash(AccountID) >> [&](const std::string &Key, const std::string &Value)
                                                        { Result.emplace_back(Key, Value); };
                } catch (...) {}

                return Result;
            }
        }

        namespace Set
        {
            inline void Single(const std::string &Key, const std::string &Value)
            {
                Ayria.doRequest("setClientpresence", JSON::Object_t({
                    { "Value", Value },
                    { "Key", Key }
                }));
            }
            inline void Multiple(const std::vector<std::pair<std::string, std::string>> &Keyvalues)
            {
                JSON::Array_t Array{}; Array.reserve(Keyvalues.size());

                for (const auto &[Key, Value] : Keyvalues)
                {
                    Array.emplace_back(JSON::Object_t({
                        { "Value", Value },
                        { "Key", Key }
                    }));
                }

                Ayria.doRequest("setClientpresence", Array);
            }
        }
    }

    // Chat messages between clients.
    namespace Messaging
    {
        inline bool sendClientmessage(uint32_t toAccountID, const std::string &Message, bool Encrypt = false)
        {
            const auto Response = Ayria.doRequest("Sendmessage", JSON::Object_t({
                { "toAccountID", toAccountID },
                { "Message", Message },
                { "Encrypt", Encrypt }
            }));

            return !Response.contains("Error");
        }
        inline bool sendGroupmessage(uint32_t toGroupID, const std::string &Message)
        {
            const auto Response = Ayria.doRequest("Sendmessage", JSON::Object_t({
                { "toGroupID", toGroupID },
                { "Message", Message }
            }));

            return !Response.contains("Error");
        }
    }







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
