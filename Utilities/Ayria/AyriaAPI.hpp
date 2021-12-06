/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-19
    License: MIT

    The database utility will transform string in/output to the requested type (string, wstring, u8string).
    The JSON utilities will do the same, to give the developer more freedom.

    The Query macro can catch some common errors at compiletime, Queryunsafe can be used for synamic strings.
    The database utility makes use of C-asserts to indicate errors, but will also log to Ayria.log.
*/

#if defined (__cplusplus)
#pragma once
#include <Stdinclude.hpp>
#include "Ayriamodule.h"

namespace AyriaAPI
{
    #pragma region DOC
    /*

    These tables are the clients view of the system, not the system itself.
    So some data (e.g. messages) may be encrypted for others.

    Base58 is used for IDs that can be used for input (e.g. ClientIDs).
    Base85 (RFC1924) is used for storing binary data.
    Timestamps are nanoseconds since UTC 1970.
    UTF8 strings are used for plaintext data.

    ===========================================================================

    // Accounts are separate to allow for cascading deletions.
    Account (
        Publickey TEXT PRIMARY KEY,     // Base58(qDSA::Publickey);
        Lastseen INTEGER DEFAULT 0 )
    > [&](const Base58_t &Publickey, uint64_t Lastseen)

    ===========================================================================

    // Display information about a client.
    Clientsocial (
        ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey),
        Username TEXT,      // Optional.
        isAway BOOLEAN )
    > [&](const Base58_t &ClientID, const std::optional<UTF8_t> &Username, bool isAway)

    // Game-state for applications where it's relevant.
    Clientgaming (
        ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey),
        GameID INTEGER,
        ModID INTEGER,
        isIngame BOOLEAN )
    > [&](const Base58_t &ClientID, uint32_t GameID, uint32_t ModID, bool isIngame)

    // What a client considers another, mutual relationships result in two rows.
    Clientrelation (
        Source TEXT REFERENCES Account(Publickey),
        Target TEXT REFERENCES Account(Publickey),
        isBlocked BOOLEAN,
        isFriend BOOLEAN,
        UNIQUE (Source, Target) )
    > [&](const Base58_t &Source, const Base58_t &Target, bool isBlocked, bool isFriend)

    ===========================================================================

    // General key-value store, but most often used for social presence.
    Keyvalues (
        ClientID TEXT REFERENCES Account(Publickey),
        Category INTEGER,   // Hash::WW32("Some string");
        Key TEXT,
        Value TEXT,        // Optional.
        UNIQUE (ClientID, Category, Key) )
    > [&](const Base58_t &ClientID, uint32_t Category, const UTF8_t &Key, const std::optional<UTF8_t> &Value)

    ===========================================================================

    // A groups ID is the same as the owner / creator of it.
    Group (
        GroupID TEXT PRIMARY KEY REFERENCES Account(Publickey),
        Groupname TEXT,     // Optional.
        Maxmembers INTEGER,
        isPublic BOOLEAN )
    > [&](const Base58_t &GroupID, const std::optional<UTF8_t> &Groupname, uint32_t Maxmembers, bool isPublic)

    // Members are added by moderators, moderators are added by the group's creator.
    Groupmember (
        GroupID TEXT REFERENCES Group(GroupID),
        MemberID TEXT REFERENCES Account(Publickey),
        isModerator BOOLEAN,
        UNIQUE (GroupID, MemberID) )
    > [&](const Base58_t &GroupID, const Base58_t &MemberID, bool isModerator)

    ===========================================================================

    // Encrypted messages by the clients PK, only ones we have decrypted gets saved.
    Clientmessage (
        Source TEXT REFERENCES Account(Publickey),
        Target TEXT REFERENCES Account(Publickey),
        Messagetype INTEGER,    // Hash::WW32("Some string");
        Timestamp INTEGER,      // Time sent.
        Message TEXT )          // Base85 (RFC1924 is JSON compatible).
    > [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint64_t Timestamp, const Base85_t &Message)

    // Encrypted messages by the groups choosen key, only ones we have decrypted gets saved.
    Groupmessage (
        Source TEXT REFERENCES Account(Publickey),
        Target TEXT REFERENCES Group(GroupID),
        Messagetype INTEGER,    // Hash::WW32("Some string");
        Timestamp INTEGER,      // Time sent.
        Message TEXT )          // Base85 (RFC1924 is JSON compatible).
    > [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint64_t Timestamp, const Base85_t &Message)

    ===========================================================================

    //
    Matchmaking (
        GroupID TEXT PRIMARY KEY REFERENCES Group(GroupID),
        Hostaddress TEXT UNIQUE,    // IPv4:Port format.
        Providers BLOB,             // Where to look for more information, e.g. std::vector<WW32("Steam")>.
        GameID INTEGER )            // Should match Clientgaming::GameID
    > [&](const Base58_t &GroupID, const UTF8_t &Hostaddress, const std::vector<uint32_t> &Providers, uint32_t GameID)

    ===========================================================================

    Thread (
        ClientID TEXT REFERENCES Account(Publickey),
        ThreadID INTEGER,
        Title TEXT,
        bool isPublic )
    Post (
        ThreadID INTEGER REFERENCES Thread(ThreadID),
        ClientID TEXT REFERENCES Account(Publickey),
        Timestamp INTEGER,
        Message TEXT )

    */
    #pragma endregion

    #pragma region Helpers
    namespace Internal
    {
        // The database should be read-only for plugins.
        inline std::shared_ptr<sqlite3> Database;
        inline sqlite::Database_t OpenDB()
        {
            if (!Database) [[unlikely]]
            {
                sqlite3 *Ptr{};

                // :memory: should never fail unless the client has more serious problems..
                const auto Result = sqlite3_open_v2("./Ayria/Client.sqlite", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
                if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
                Database = std::shared_ptr<sqlite3>(Ptr, sqlite3_close_v2);

                // This should already have been called from Ayria.dll, but just in case.
                sqlite::Database_t(Database) << "PRAGMA foreign_keys = ON;";
                sqlite::Database_t(Database) << "PRAGMA auto_vacuum = INCREMENTAL;";

                // Helper functions for inline hashing, should fail, but just in case..
                {
                    static const auto Lambda32 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
                    {
                        if (argc == 0) return;
                        if (SQLITE3_TEXT != sqlite3_value_type(argv[0])) { sqlite3_result_null(context); return; }

                        // SQLite may invalidate the pointer if _bytes is called after text.
                        const auto Length = sqlite3_value_bytes(argv[0]);
                        const auto Hash = Hash::WW32(sqlite3_value_text(argv[0]), Length);
                        sqlite3_result_int(context, Hash);
                    };
                    static const auto Lambda64 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
                    {
                        if (argc == 0) return;
                        if (SQLITE3_TEXT != sqlite3_value_type(argv[0])) { sqlite3_result_null(context); return; }

                        // SQLite may invalidate the pointer if _bytes is called after text.
                        const auto Length = sqlite3_value_bytes(argv[0]);
                        const auto Hash = Hash::WW64(sqlite3_value_text(argv[0]), Length);
                        sqlite3_result_int64(context, Hash);
                    };
                    sqlite3_create_function(Database.get(), "WW32", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda32, nullptr, nullptr);
                    sqlite3_create_function(Database.get(), "WW64", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda64, nullptr, nullptr);
                }
            }

            return sqlite::Database_t(Database);
        }

        constexpr size_t Semicolons(std::string_view SQL) { return std::ranges::count(SQL, ';'); }
        constexpr size_t Bindings(std::string_view SQL) { return std::ranges::count(SQL, '?'); }

        // Creation of a prepared statement, evaluates with the '>>' operator or when it goes out of scope.
        template <bool Single, size_t Bindings, typename ...Args> requires(Bindings >= sizeof...(Args) && Single)
        [[nodiscard]] inline auto Querychecked(std::string_view SQL, Args&&... va) noexcept
        {
            auto PS = Internal::OpenDB() << SQL;
            if constexpr (sizeof...(Args) > 0)
            {
                ((PS << va), ...);
            }
            return PS;
        }
    }
    #define Query(x, ...) Internal::Querychecked<Internal::Semicolons(x) == 1, Internal::Bindings(x)>(x, __VA_ARGS__)

    // Creation of a prepared statement, evaluates with the '>>' operator or when it goes out of scope.
    template <typename ...Args> [[nodiscard]] inline auto Queryunsafe(std::string_view SQL, Args&&... va) noexcept
    {
        assert(Internal::Bindings(SQL) >= sizeof...(Args));
        assert(Internal::Semicolons(SQL) == 1);

        auto PS = Internal::OpenDB() << SQL;
        if constexpr (sizeof...(Args) > 0)
        {
            ((PS << va), ...);
        }
        return PS;
    }

    // The database generally keys on the long ID, short ID is more for reference where accuracy is not needed.
    using Base58_t = std::string; using Base85_t = std::string; using UTF8_t = std::u8string;
    using ShortID_t = uint64_t; using LongID_t = std::string;

    // Value_t default-constructs to NULL and returns (Type != NULL) for operator bool().
    using Result_t = JSON::Value_t;

    // Helpers to make everything more readable / save my hands.
    #define Accountlambda           [&](const Base58_t &Publickey, uint64_t Lastseen)
    #define Clientsociallambda      [&](const Base58_t &ClientID, const std::optional<UTF8_t> &Username, bool isAway)
    #define Clientgaminglambda      [&](const Base58_t &ClientID, uint32_t GameID, uint32_t ModID, bool isIngame)
    #define Clientrelationlambda    [&](const Base58_t &Source, const Base58_t &Target, bool isBlocked, bool isFriend)
    #define Keyvaluelambda          [&](const Base58_t &ClientID, uint32_t Category, const UTF8_t &Key, const std::optional<UTF8_t> &Value)
    #define Grouplambda             [&](const Base58_t &GroupID, const std::optional<UTF8_t> &Groupname, uint32_t Maxmembers, bool isPublic)
    #define Groupmemberlambda       [&](const Base58_t &GroupID, const Base58_t &MemberID, bool isModerator)
    #define Clientmessagelambda     [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint64_t Timestamp, const Base85_t &Message)
    #define Groupmessagelambda      [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint64_t Timestamp, const Base85_t &Message)
    #define Matchmakinglambda       [&](const Base58_t &GroupID, const UTF8_t &Hostaddress, const std::vector<uint32_t> &Providers, uint32_t GameID)
    #pragma endregion

    // Disable warnings about unused parameters in the lambdas.
    #pragma warning(disable: 4100)

    // General information about the clients.
    namespace Clientinfo
    {
        // Active clients only (last seen > 60 seconds ago).
        inline uint32_t Count(bool IncludeAFK = false)
        {
            const auto Timestamp = (std::chrono::utc_clock::now() - std::chrono::seconds(60)).time_since_epoch().count();
            uint32_t Count{};

            if (IncludeAFK)
            {
                Query("SELECT COUNT(*) FROM Acccount WHERE Lastseen >= ?;", Timestamp) >> Count;
            }
            else
            {
                Hashset<LongID_t> Accounts{};
                Query("SELECT Publickey FROM Acccount WHERE Lastseen >= ?;", Timestamp)
                    >> [&](const Base58_t &Publickey) { Accounts.insert(Publickey); };

                for (const auto &Item : Accounts)
                {
                    Query("SELECT isAway FROM Clientsocial WHERE ClientID = ?;", Item)
                        >> [&](bool isAway) { Count += !isAway; };
                }
            }

            return Count;
        }

        // Collect relevant information about the client.
        inline Result_t Fetch(const LongID_t &ClientID)
        {
            JSON::Object_t Result{};

            Query("SELECT * FROM Account WHERE Publickey = ?;", ClientID) >> Accountlambda
            {
                Result["Publickey"] = Publickey;
                Result["Lastseen"] = Lastseen;
            };

            Query("SELECT * FROM Clientsocial WHERE ClientID = ?;", ClientID) >> Clientsociallambda
            {
                // If the client has no username, give it one.
                Result["Username"] = Username ? *Username : Encoding::toUTF8(ClientID);
                Result["isAway"] = isAway;
            };

            Query("SELECT * FROM Clientgaming WHERE ClientID = ?;", ClientID) >> Clientgaminglambda
            {
                Result["isIngame"] = isIngame;
                Result["GameID"] = GameID;
                Result["ModID"] = ModID;
            };

            uint64_t Timestamp{};
            Query("SELECT Lastseen FROM Account WHERE Publickey = ?;", ClientID) >> Timestamp;
            Result["Lastseen"] = Timestamp;

            if (Result.empty()) return {};
            else return Result;
        }

        // Find clients by criteria.
        inline std::unordered_set<LongID_t> Find(const UTF8_t &Username)
        {
            std::unordered_set<LongID_t> Result{};

            Query("SELECT ClientID FROM Clientsocial WHERE Username = ?;", Username)
                >> [&](const LongID_t &ClientID) { Result.insert(ClientID); };

            return Result;
        }
        inline std::unordered_set<LongID_t> Find(std::optional<uint32_t> byGameID, std::optional<uint32_t> byModID)
        {
            // We don't do full listings here.
            if (!byGameID && !byModID) [[unlikely]] return {};

            std::unordered_set<LongID_t> Result{};
            [&]()
            {
                if (byGameID && byModID) return Query("SELECT ClientID FROM Clientgaming WHERE (GameID = ? AND ModID = ?);", byGameID.value(), byModID.value());
                if (byGameID) return Query("SELECT ClientID FROM Clientgaming WHERE GameID = ?;", byGameID.value());
                else return Query("SELECT ClientID FROM Clientgaming WHERE ModID = ?;", byModID.value());
            }() >> [&](const Base58_t &ClientID) { Result.insert(ClientID); };

            return Result;
        }
    }

    // Client <-> client relationships.
    namespace Clientrelations
    {
        using isFriend = bool; using isBlocked = bool;
        using Relation = std::pair<isFriend, isBlocked>;

        // Get the status of one user.
        inline Relation Get(const LongID_t &Source, const LongID_t &Target)
        {
            Relation Result{};
            Query("SELECT isFriend, isBlocked FROM Clientrelation WHERE (Source = ? AND Target = ?);", Source, Target)
                >> std::tie(Result.first, Result.second);
            return Result;
        }

        // Get a list of users by criteria.
        inline std::unordered_set<LongID_t> getFriends(const LongID_t &ClientID)
        {
            std::unordered_set<LongID_t> Candidates{}, Results{};

            Query("SELECT Target FROM Clientrelation WHERE (Source = ? AND isFriend = true);", ClientID)
                >> [&](const Base58_t &Target) { Candidates.insert(Target); };

            for (const auto &Item : Candidates)
            {
                Query("SELECT Source FROM Clientrelation WHERE (Source = ? AND Target = ? AND isFriend = true);", Item, ClientID)
                    >> [&](const Base58_t &Source) { Results.insert(Source); };
            }

            return Results;
        }
        inline std::unordered_set<LongID_t> getBlocked(const LongID_t &ClientID)
        {
            std::unordered_set<LongID_t> Candidates{}, Results{};

            Query("SELECT Target FROM Clientrelation WHERE (Source = ? AND isBlocked = true);", ClientID)
                >> [&](const Base58_t &Target) { Candidates.insert(Target); };

            for (const auto &Item : Candidates)
            {
                Query("SELECT Source FROM Clientrelation WHERE (Source = ? AND Target = ? AND isBlocked = true);", Item, ClientID)
                    >> [&](const Base58_t &Source) { Results.insert(Source); };
            }

            return Results;
        }

        // One-sided relations =(
        inline std::unordered_set<LongID_t> getFriendproposals(const LongID_t &ClientID)
        {
            std::unordered_set<LongID_t> Results{};

            Query("SELECT Target FROM Clientrelation WHERE (Source = ? AND isFriend = true);", ClientID)
                >> [&](const Base58_t &Target) { Results.insert(Target); };

            for (const auto &Item : Results)
            {
                Query("SELECT Source FROM Clientrelation WHERE (Source = ? AND Target = ? AND isFriend = true);", Item, ClientID)
                    >> [&](const Base58_t &Source) { Results.erase(Source); };
            }

            return Results;
        }
        inline std::unordered_set<LongID_t> getFriendrequests(const LongID_t &ClientID)
        {
            std::unordered_set<LongID_t> Results{};

            Query("SELECT Target FROM Clientrelation WHERE (Target = ? AND isFriend = true);", ClientID)
                >> [&](const Base58_t &Target) { Results.insert(Target); };

            for (const auto &Item : Results)
            {
                Query("SELECT Source FROM Clientrelation WHERE (Source = ? AND Target = ? AND isFriend = true);", Item, ClientID)
                    >> [&](const Base58_t &Source) { Results.erase(Source); };
            }

            return Results;
        }

        // Simplified check.
        inline bool areFriends(const LongID_t &Source, const LongID_t &Target)
        {
            const auto A = Get(Source, Target);
            const auto B = Get(Target, Source);
            return A.first && B.first;
        }
    }

    // Client's key-value (usually presence) storage.
    namespace Keyvalue
    {
        using Key_t = UTF8_t;
        using Category_t = uint32_t;
        using Value_t = std::optional<UTF8_t>;
        using Entry_t = std::tuple<Category_t, Key_t, Value_t>;

        // Find clients by criteria.
        inline std::unordered_set<LongID_t> Find(Category_t Category, const Key_t &Key, Value_t Value = {})
        {
            std::unordered_set<LongID_t> Results{};

            if (Value)
            {
                Query("SELECT ClientID FROM Keyvalues WHERE (Category = ? AND Key = ? AND Value = ?);", Category, Key, *Value)
                    >> [&](const Base58_t &ClientID) { Results.insert(ClientID); };
            }
            else
            {
                Query("SELECT ClientID FROM Keyvalues WHERE (Category = ? AND Key = ?);", Category, Key)
                    >> [&](const Base58_t &ClientID) { Results.insert(ClientID); };
            }

            return Results;
        }

        // Dump all entries for a given user.
        inline std::vector<Entry_t> Dump(const LongID_t &ClientID, std::optional<Category_t> Category = {})
        {
            std::vector<Entry_t> Results{};

            if (Category)
            {
                Query("SELECT * FROM Keyvalues WHERE (ClientID = ? AND Category = ?);", ClientID, *Category) >> Keyvaluelambda
                {
                    Results.emplace_back(Category, Key, Value);
                };
            }
            else
            {
                Query("SELECT * FROM Keyvalues WHERE ClientID = ?;", ClientID) >> Keyvaluelambda
                {
                    Results.emplace_back(Category, Key, Value);
                };
            }

            return Results;
        }

        // Simple lookup of a given key-value.
        inline Value_t Get(const LongID_t &ClientID, Category_t Category, const Key_t &Key)
        {
            Value_t Result{};

            Query("SELECT Value FROM Keyvalues WHERE (ClientID = ? AND Category = ? AND Key = ?);", ClientID, Category, Key)
                >> Result;

            return Result;
        }
    }

    // User-managed groups, one per client.
    namespace Groups
    {
        // The groups a user is a member of.
        inline std::vector<LongID_t> getMemberships(const LongID_t &ClientID)
        {
            std::vector<LongID_t> Result{};

            Query("SELECT GroupID FROM Groupmember WHERE ClientID = ?;", ClientID)
                >> [&](const LongID_t &GroupID) { Result.emplace_back(GroupID); };

            return Result;
        }

        // Vector is used as some consumers want process by offset.
        inline std::vector<LongID_t> getMembers(const LongID_t &GroupID)
        {
            std::vector<LongID_t> Result{};

            Query("SELECT MemberID FROM Groupmember WHERE GroupID = ?;", GroupID)
                >> [&](const LongID_t &MemberID) { Result.emplace_back(MemberID); };

            return Result;
        }
        inline std::vector<LongID_t> getModerators(const LongID_t &GroupID)
        {
            std::vector<LongID_t> Result{};

            Query("SELECT MemberID FROM Groupmember WHERE (GroupID = ? AND isModerator = true);", GroupID)
                >> [&](const LongID_t &MemberID) { Result.emplace_back(MemberID); };

            return Result;
        }

        // Fetch the relevant information about the group.
        inline Result_t Fetch(const LongID_t &GroupID)
        {
            JSON::Object_t Result{};

            Query("SELECT * FROM Group WHERE GroupID = ?;", GroupID) >> Grouplambda
            {
                Result["GroupID"] = GroupID;
                Result["isPublic"] = isPublic;
                Result["Maxmembers"] = Maxmembers;
                if (Groupname) Result["Groupname"] = *Groupname;
            };

            Result["Membercount"] = getMembers(GroupID).size();
            return Result;
        }

        // Find groups by criteria.
        inline std::vector<LongID_t> Find(bool isPublic)
        {
            std::vector<LongID_t> Result{};

            Query("SELECT GroupID FROM Group WHERE isPublic = ?;", isPublic)
                >> [&](const LongID_t &GroupID) { Result.emplace_back(GroupID); };

            return Result;
        }
        inline std::vector<LongID_t> Find(const UTF8_t &Groupname)
        {
            std::vector<LongID_t> Result{};

            Query("SELECT GroupID FROM Group WHERE Groupname = ?;", Groupname)
                >> [&](const LongID_t &GroupID) { Result.emplace_back(GroupID); };

            return Result;
        }
    }

    // The messages have already been decrypted.
    namespace Messaging
    {
        // Get a count to check if there's new messages / chunk processing.
        inline uint32_t groupCount(const LongID_t &GroupID, std::optional<uint32_t> Messagetype)
        {
            uint32_t Result{};

            if (Messagetype)
            {
                Query("SELECT COUNT (*) FROM Groupmessage WHERE (Target = ? AND Messagetype = ?);", GroupID, *Messagetype)
                    >> Result;
            }
            else
            {
                Query("SELECT COUNT (*) FROM Groupmessage WHERE (Target = ?);", GroupID)
                    >> Result;
            }

            return Result;
        }
        inline uint32_t clientCount(const LongID_t &ClientID, std::optional<uint32_t> Messagetype)
        {
            uint32_t Result{};

            if (Messagetype)
            {
                Query("SELECT COUNT (*) FROM Clientmessage WHERE (Target = ? AND Messagetype = ?);", ClientID, *Messagetype)
                    >> Result;
            }
            else
            {
                Query("SELECT COUNT (*) FROM Clientmessage WHERE (Target = ?);", ClientID)
                    >> Result;
            }

            return Result;
        }

        // Fetch a single message by criteria and offset.
        inline Result_t getClientmessage(const std::optional<LongID_t> &To, const std::optional<LongID_t> &From,
            std::optional<uint32_t> Messagetype, std::optional<uint32_t> Offset = {})
        {
            JSON::Object_t Result{};

            std::string SQL = "SELECT * FROM Clientmessage WHERE ( ";
            if (Messagetype) SQL += "Messagetype = ? AND ";
            if (From) SQL += "Source = ? AND ";
            if (To) SQL += "Target = ? AND ";
            SQL += "true ) ";

            if (Offset) SQL += "OFFSET ? LIMIT 1;";
            else SQL += "LIMIT 1;";

            auto PS = Queryunsafe(SQL);
            if (Messagetype) PS << *Messagetype;
            if (From) PS << *From;
            if (To) PS << *To;
            if (Offset) PS << *Offset;

            PS >> Clientmessagelambda
            {
                Result = JSON::Object_t({
                    { "To", Target },
                    { "From", Source },
                    { "Message", Message },
                    { "Messagetype", Messagetype }
                });
            };

            return Result;
        }
        inline Result_t getGroupmessage(const std::optional<LongID_t> &GroupID, const std::optional<LongID_t> &From,
            std::optional<uint32_t> Messagetype, std::optional<uint32_t> Offset = {})
        {
            JSON::Object_t Result{};

            std::string SQL = "SELECT * FROM Clientmessage WHERE ( ";
            if (Messagetype) SQL += "Messagetype = ? AND ";
            if (GroupID) SQL += "Target = ? AND ";
            if (From) SQL += "Source = ? AND ";
            SQL += "true ) ";

            if (Offset) SQL += "OFFSET ? LIMIT 1;";
            else SQL += "LIMIT 1;";

            auto PS = Queryunsafe(SQL);
            if (Messagetype) PS << *Messagetype;
            if (GroupID) PS << *GroupID;
            if (From) PS << *From;
            if (Offset) PS << *Offset;

            PS >> Groupmessagelambda
            {
                Result = JSON::Object_t({
                    { "From", Source },
                    { "GroupID", Target },
                    { "Message", Message },
                    { "Messagetype", Messagetype }
                });
            };

            return Result;
        }
    }

    // Let users know which groups are actively hosting.
    namespace Matchmaking
    {
        // Collect information about a host.
        inline Result_t Fetch(const Base58_t &GroupID)
        {
            JSON::Object_t Result{};

            // Fetch the base info.
            Query("SELECT * FROM Matchmaking WHERE GroupID = ?;", GroupID) >> Matchmakinglambda
            {
                Result["Hostaddress"] = Hostaddress;
                Result["Providers"] = Providers;
                Result["GroupID"] = GroupID;
                Result["GameID"] = GameID;
            };

            // Nothing found in the DB.
            if (Result.empty()) [[unlikely]]
                return {};

            // Combine with group info, can't really fail.
            JSON::Object_t Group = Groups::Fetch(GroupID);
            Result.merge(Group);

            return Result;
        }
        inline Result_t Fetch(const UTF8_t &Hostaddress)
        {
            LongID_t GroupID{};
            Query("SELECT GroupID FROM Matchmaking WHERE Hostaddress = ?;", Hostaddress)
                >> GroupID;

            // Nothing found in the DB.
            if (GroupID.empty()) [[unlikely]]
                return {};

            return Fetch(GroupID);
        }

        // Find hosts by criteria.
        inline std::vector<LongID_t> Find(uint32_t GameID)
        {
            std::vector<LongID_t> Result{};
            Query("SELECT GroupID FROM Matchmaking WHERE GameID = ?;", GameID)
                >> [&](const LongID_t &ID) { Result.emplace_back(ID); };
            return Result;
        }
    }

    #pragma region Cleanup
    // Restore warning to default.
    #pragma warning(default: 4100)

    // Avoid leakage.
    #undef Accountlambda
    #undef Clientsociallambda
    #undef Clientgaminglambda
    #undef Clientrelationlambda
    #undef Keyvaluelambda
    #undef Grouplambda
    #undef Groupmemberlambda
    #undef Clientmessagelambda
    #undef Groupmessagelambda
    #undef Matchmakinglambda
    #pragma endregion
}
#endif
