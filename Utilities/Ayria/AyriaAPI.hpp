/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-19
    License: MIT

    Helpers for common database operations.
    Errors return default constructed results, see Ayria.log for more info.
*/

#if defined (__cplusplus)
#pragma once
#include <Stdinclude.hpp>
#include "Ayriamodule.h"

namespace AyriaAPI
{
    #pragma region DOC
    /*
    These tables are the clients view of the global system, not the system itself.
    Some data may be encrypted for others (e.g. Messaging::Message)
    Some data is dependent on the clients view (e.g. ::Timestamp)

    Base58 is used for data that should be readable.
    Timestamps are nanoseconds since UTC 1970.
    Base85 is used for stored binary data.
    UTF8 strings are escaped to ASCII.

    // Separated to allow for easy (cascading) pruning.
    Account (
    Publickey TEXT PRIMARY KEY )    // Base58(qDSA::Publickey)
    [&](const Base58_t &Publickey)

    Client (
    ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey) ON DELETE CASCADE,
    GameID INTEGER,        // Set by Platformwrapper or equivalent.
    ModID INTEGER,         // Variation of the GameID, generally 0.
    Flags INTEGER,         // Clients public settings.
    Username TEXT )        // UTF8 escaped (\uXXXX) ASCII.
    [&](const Base58_t &ClientID, uint32_t GameID, uint32_t ModID, uint32_t Flags, const ASCII_t &Username)

    Relation (
    Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,    // Initiator.
    Target TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,    // Recipient.
    isFriend BOOLEAN,                                               // Source considers Target X
    isBlocked BOOLEAN,                                              // Source considers Target X
    UNIQUE (Source, Target) )
    [&](const Base58_t &Source, const Base58_t &Target, bool isFriend, bool isBlocked)

    Usermessages / Groupmessages (
    Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,    // Initiator.
    Target TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,    // Recipient, user or group.
    Messagetype INTEGER,                                            // Hash::WW32("Plaintext message type")
    Checksum INTEGER,                                               // Hash::WW32("Plaintext message")
    Received INTEGER,                                               // Time received / processed.
    Sent INTEGER,                                                   // Time sent.
    Message TEXT,                                                   // Base85 of the plaintext.
    UNIQUE (Source, Target, Sent, Messagetype) )                    // Ignore duplicates.
    [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint32_t Checksum, uint64_t Received, uint64_t Sent, const Base85_t &Message)

    Group (
    GroupID TEXT PRIMARY KEY REFERENCES Account(Publickey) ON DELETE CASCADE,   // Owners ID
    Groupname TEXT,         // UTF8 escaped (\uXXXX) ASCII.
    isPublic BOOLEAN,       // Can join without an invite, unencrypted chats.
    isFull BOOLEAN,         // Moderator decided they have enough members.
    Membercount INTEGER)    // Automatically generated from Groupmember
    [&](const Base58_t &GroupID, const ASCII_t &Groupname, bool isPublic, bool isFull, uint32_t Membercount)

    Groupmember (
    MemberID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,
    GroupID TEXT REFERENCES Group(GroupID) ON DELETE CASCADE,
    isModerator BOOLEAN DEFAULT false,
    UNIQUE (GroupID, MemberID) )
    [&](const Base58_t &GroupID, const Base58_t &MemberID, bool isModerator)

    Presence (
    OwnerID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE,
    Category TEXT NOT NULL,	// Often provider of sorts, e.g. "Steam" or "myPlugin"
    Key TEXT NOT NULL,      // Required.
    Value TEXT,             // Optional.
    UNIQUE (OwnerID, Category, Key) )
    [&](const Base58_t &OwnerID, const ASCII_t &Category, const ASCII_t &Key, const std::optional<ASCII_t> &Value)

    */
    #pragma endregion

    #pragma region Helpers
    namespace Internal
    {
        // The database should be read-only for plugins.
        inline std::shared_ptr<sqlite3> Database;
        inline sqlite::database OpenDB()
        {
            if (!Database) [[unlikely]]
            {
                sqlite3 * Ptr{};

                // :memory: should never fail unless the client has more serious problems..
                const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
                if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
                Database = std::shared_ptr<sqlite3>(Ptr, sqlite3_close_v2);
            }

            return sqlite::database(Database);
        }
    }

    // Evaluation of the prepared statement (>> operator) may throw, creation should not.
    template <typename ...Args> [[nodiscard]] auto Prepare(const std::string &SQL, Args&&... va)
    {
        auto PS = Internal::OpenDB() << SQL; if constexpr (sizeof...(va) > 0) { ((PS << va), ...); } return PS;
    }

    // The database generally keys on the long ID, short ID is more for reference where accuracy is not needed.
    using Base58_t = std::string; using Base85_t = std::string; using ASCII_t = std::string;
    using ShortID_t = uint64_t; using LongID_t = Base58_t;

    // Value_t default-constructs to NULL and returns (Type != NULL) for operator bool().
    using Result_t = JSON::Value_t;

    // Helpers to make everything more readable / save my hands.
    #define Trycatch(...) try { __VA_ARGS__ } catch (const sqlite::sqlite_exception &e) { (void)e; Debugprint(va("SQL error: %s (%s)", e.what(), e.get_sql().c_str())); }
    #define Messaginglambda     [&](const Base58_t &Source, const Base58_t &Target, uint32_t Messagetype, uint32_t Checksum, uint64_t Received, uint64_t Sent, const Base85_t &Message)
    #define Presencelambda      [&](const Base58_t &OwnerID, const ASCII_t &Category, const ASCII_t &Key, const std::optional<ASCII_t> &Value)
    #define Grouplambda         [&](const Base58_t &GroupID, const ASCII_t &Groupname, bool isPublic, bool isFull, uint32_t Membercount)
    #define Clientlambda        [&](const Base58_t &ClientID, uint32_t GameID, uint32_t ModID, uint32_t Flags, const ASCII_t &Username)
    #define Relationlambda      [&](const Base58_t &Source, const Base58_t &Target, bool isFriend, bool isBlocked)
    #define Groupmemberlambda   [&](const Base58_t &GroupID, const Base58_t &MemberID, bool isModerator)
    #define Groupinterestlambda [&](const Base58_t &MemberID, const Base58_t &GroupID)
    #define Accountlambda       [&](const Base58_t &Publickey)
    #pragma endregion

    // Disable warnings about unused parameters in the lambdas.
    #pragma warning(disable: 4100)

    // General information about the network.
    namespace Clientinfo
    {
        inline Result_t Count()
        {
            Trycatch(
                uint64_t Result{};
                Prepare("SELECT COUNT(*) FROM Client;") >> Result;
                if (Result) return Result;
            );

            return {};
        }
        inline Result_t Find(const LongID_t &ClientID)
        {
            Trycatch(
                JSON::Object_t Result{};
                Prepare("SELECT * FROM Client WHERE ClientID = ?;", ClientID) >> Clientlambda
                {
                    Result = JSON::Object_t({
                        { "ClientID", ClientID },
                        { "Username", Username },
                        { "GameID", GameID },
                        { "ModID", ModID },
                        { "Flags", Flags },

                        // NOTE(tcn): May change before v1.0
                        { "isPrivate",  Flags & (1 << 1) },
                        { "isAway",     Flags & (1 << 2) },
                        { "isHosting",  Flags & (1 << 3) },
                        { "isIngame",   Flags & (1 << 4) }
                    });
                };
                if (!Result.empty()) return Result;
            );

            return {};
        }
        inline Result_t Enumerate(const ASCII_t &Username)
        {
            Trycatch(
                Hashset<LongID_t> Clients{};

                Prepare("SELECT ClientID FROM Client WHERE Username = ?;", Username) >> [&](const Base58_t &ClientID)
                {
                    Clients.insert(ClientID);
                };

                JSON::Array_t Result{};
                Result.reserve(Clients.size());

                for (const auto &Item : Clients)
                    if (const auto TMP = Find(Item))
                        Result.emplace_back(TMP);

                if (!Result.empty()) return Result;
            );

            return {};
        }
        inline Result_t Enumerate(std::optional<uint32_t> byGameID, std::optional<uint32_t> byModID)
        {
            // We don't do full listings here.
            if (!byGameID && !byModID) [[unlikely]] return {};

            auto PS = [&]()
            {
                if (byGameID && byModID) return Prepare("SELECT ClientID FROM Client WHERE (GameID = ? AND ModID = ?);", byGameID.value(), byModID.value());
                if (byGameID) return Prepare("SELECT ClientID FROM Client WHERE GameID = ?;", byGameID.value());
                if (byModID) return Prepare("SELECT ClientID FROM Client WHERE ModID = ?;", byModID.value());
                return Prepare(";");
            }();

            Trycatch(
                Hashset<LongID_t> Clients{};
                PS >> [&](const Base58_t &ClientID) { Clients.insert(ClientID); };

                JSON::Array_t Result{};
                Result.reserve(Clients.size());

                for (const auto &Item : Clients)
                    if (const auto TMP = Find(Item))
                        Result.emplace_back(TMP);

                if (!Result.empty()) return Result;
            );

            return {};
        }

        // Internal table, do not expect Hyrums law to be respected.
        inline uint32_t Lastmessage(const LongID_t &ClientID)
        {
            uint64_t Timestamp{};
            auto PS = Prepare("SELECT Timestamp FROM Messagestream WHERE Sender = ? LIMIT 1 ORDER BY Timestamp;", ClientID);
            Trycatch(PS >> Timestamp; );

            if (0 == Timestamp) [[unlikely]] return 0x7FFFFFFF;

            const auto Duration = std::chrono::utc_clock::now().time_since_epoch() - std::chrono::utc_clock::duration(Timestamp);
            return std::chrono::duration_cast<std::chrono::seconds>(Duration).count();
        }
    }

    // Client-client relationships.
    namespace Relations
    {
        using isFriend = bool; using isBlocked = bool;
        using Relation = std::pair<isFriend, isBlocked>;

        inline Relation Get(const LongID_t &Source, const LongID_t &Target)
        {
            Relation Result{};

            auto PS = Prepare("SELECT isFriend, isBlocked FROM Relation WHERE (Source = ? AND Target = ?);", Source, Target);
            Trycatch(PS >> std::tie(Result.first, Result.second););
            return Result;
        }
        inline bool areFriends(const LongID_t &ClientA, const LongID_t &ClientB)
        {
            const auto A = Get(ClientA, ClientB);
            const auto B = Get(ClientB, ClientA);
            return A.first && B.first;
        }
        inline bool areBlocked(const LongID_t &ClientA, const LongID_t &ClientB)
        {
            const auto A = Get(ClientA, ClientB);
            const auto B = Get(ClientB, ClientA);
            return A.second || B.second;
        }
        inline std::unordered_set<LongID_t> getBlocked(const LongID_t &Source)
        {
            std::unordered_set<LongID_t> Result{};

            auto PS = Prepare("SELECT Target FROM Relation WHERE (Source = ? AND isBlocked = true);", Source);
            Trycatch(PS >> [&](LongID_t ID) { Result.insert(ID); };);
            return Result;
        }
        inline std::unordered_set<LongID_t> getFriends(const LongID_t &Source)
        {
            std::unordered_set<LongID_t> Temp{}, Result{};

            auto PS = Prepare("SELECT Target FROM Relation WHERE (Source = ? AND isFriend = true);", Source);
            Trycatch(PS >> [&](LongID_t ID) { Temp.insert(ID); };);

            for (const auto &Item : Temp)
                if (Get(Item, Source).first)
                    Result.insert(Item);

            return Result;
        }
        inline std::unordered_set<LongID_t> getFriendrequests(const LongID_t &Target)
        {
            std::unordered_set<LongID_t> Result{};

            auto PS = Prepare("SELECT Source FROM Relation WHERE (Target = ? AND isFriend = true);", Target);
            Trycatch(PS >> [&](LongID_t ID) { Result.insert(ID); };);
            return Result;
        }
        inline std::unordered_set<LongID_t> getFriendproposals(const LongID_t &Source)
        {
            std::unordered_set<LongID_t> Result{};

            auto PS = Prepare("SELECT Target FROM Relation WHERE (Source = ? AND isFriend = true);", Source);
            Trycatch(PS >> [&](LongID_t ID) { Result.insert(ID); };);
            return Result;
        }
    }

    // User-managed groups, one per account ID.
    namespace Groups
    {
        inline Result_t Find(const LongID_t &GroupID)
        {
            Trycatch(
                JSON::Object_t Result{};
                Prepare("SELECT * FROM Groups WHERE GroupID = ?;", GroupID) >> Grouplambda
                {
                    Result = JSON::Object_t({
                        { "Membercount", Membercount },
                        { "Groupname", Groupname },
                        { "isPublic", isPublic },
                        { "GroupID", GroupID },
                        { "isFull", isFull }
                    });
                };
                if (!Result.empty()) return Result;
            );

            return {};
        }
        inline Result_t Enumerate(const ASCII_t &Groupname)
        {
            Trycatch(
                Hashset<LongID_t> Groups{};

                Prepare("SELECT GroupID FROM Client WHERE Groupname = ?;", Groupname) >> [&](const Base58_t &GroupID)
                {
                    Groups.insert(GroupID);
                };

                JSON::Array_t Result{};
                Result.reserve(Groups.size());

                for (const auto &Item : Groups)
                    if (const auto TMP = Find(Item))
                        Result.emplace_back(TMP);

                if (!Result.empty()) return Result;
            );

            return {};
        }
        inline std::vector<LongID_t> getMembers(const LongID_t &GroupID)
        {
            std::set<LongID_t> Result{ GroupID };

            Trycatch(
                Prepare("SELECT MemberID FROM Groupmember WHERE GroupID = ?;", GroupID) >> [&](const Base58_t &MemberID)
                {
                    Result.insert(MemberID);
                };
            );

            return { Result.begin(), Result.end() };
        }
        inline std::vector<LongID_t> getModerators(const LongID_t &GroupID)
        {
            std::set<LongID_t> Result{ GroupID };

            Trycatch(
                Prepare("SELECT MemberID FROM Groupmember WHERE (isModerator = true AND GroupID = ?);", GroupID) >> [&](const Base58_t &MemberID)
                {
                    Result.insert(MemberID);
                };
            );

            return { Result.begin(), Result.end() };
        }
        inline std::vector<LongID_t> getMemberships(const LongID_t &UserID)
        {
            std::set<LongID_t> Result{};

            Trycatch(
                Prepare("SELECT GroupID FROM Groupmember WHERE MemberID = ?;", UserID) >> [&](const Base58_t &GroupID)
                {
                    Result.insert(GroupID);
                };
            );

            return { Result.begin(), Result.end() };
        }
        inline std::vector<LongID_t> Enumerate(std::optional<bool> isPublic, std::optional<bool> isFull)
        {
            // Not going to list all groups in one call.
            if (!isPublic && !isFull) [[unlikely]] return {};

            auto PS = [&]()
            {
                if (isPublic && isFull) return Prepare("SELECT GroupID FROM Group WHERE (isPublic = ? AND isFull = ?);", isPublic.value(), isFull.value());
                if (isPublic) return Prepare("SELECT GroupID FROM Group WHERE isPublic = ?;", isPublic.value());
                if (isFull) return Prepare("SELECT GroupID FROM Group WHERE isFull = ?;", isFull.value());
                return Prepare(";");
            }();

            std::set<LongID_t> Result{};
            Trycatch(PS >> [&](const Base58_t &GroupID) { Result.insert(GroupID); }; );

            return { Result.begin(), Result.end() };
        }
    }

    // Key-value store for the client, usually for social presence.
    namespace Presence
    {
        using Category_t = ASCII_t;
        using Keyvalue_t = std::pair<ASCII_t, ASCII_t>;

        inline std::optional<std::map<ASCII_t, ASCII_t>> Dump(const LongID_t &ClientID, const ASCII_t &Category)
        {
            Trycatch(
                std::map<ASCII_t, ASCII_t> Result{};
                Prepare("SELECT * FROM Presence WHERE (OwnerID = ? AND Category = ?);", ClientID, Category) >> Presencelambda
                {
                    Result.emplace(Key, Value.value_or(""));
                };
                if (!Result.empty()) return Result;
            );

            return {};

        }
        inline std::optional<std::unordered_map<Category_t, Keyvalue_t>> Dump(const LongID_t &ClientID)
        {
            Trycatch(
                std::unordered_map<Category_t, Keyvalue_t> Result{};
                Prepare("SELECT * FROM Presence WHERE OwnerID = ?;", ClientID) >> Presencelambda
                {
                    Result[Category] = { Key, Value.value_or("") };
                };
                if (!Result.empty()) return Result;
            );

            return {};
        }

        inline Result_t Value(const LongID_t &ClientID, const ASCII_t &Key, std::optional<ASCII_t> Category = {})
        {
            auto PS = Category
                ? Prepare("SELECT Value FROM Presence WHERE (OwnerID = ? AND Key = ? AND Category = ?) LIMIT 1;", ClientID, Key, Category.value())
                : Prepare("SELECT Value FROM Presence WHERE (OwnerID = ? AND Key = ?) LIMIT 1;", ClientID, Key);

            Trycatch(
                std::optional<ASCII_t> Result{};
                PS >> [&](std::optional<ASCII_t> TMP) { Result = TMP; };
                if (Result) return Result.value();
            );

            return {};
        }
        inline std::unordered_set<LongID_t> Enumerate(const ASCII_t &Key, std::optional<ASCII_t> Category = {}, std::optional<ASCII_t> Value = {})
        {
            std::unordered_set<LongID_t> Result{};
            auto PS = [&]()
            {
                if (Category && Value) return Prepare("SELECT OwnerID FROM Presence WHERE (Key = ? AND Value = ? AND Category = ?);", Key, Value.value(), Category.value());
                if (Category) return Prepare("SELECT OwnerID FROM Presence WHERE (Key = ? AND Category = ?);", Key, Category.value());
                if (Value) return Prepare("SELECT OwnerID FROM Presence WHERE (Key = ? AND Value = ?);", Key, Category.value());
                return Prepare("SELECT OwnerID FROM Presence WHERE Key = ?;", Key);
            }();

            Trycatch(
                PS >> [&](const Base58_t &OwnerID) { Result.insert(OwnerID); };
                if (!Result.empty()) return Result;
            );

            return Result;
        }
    }

    // The messages have already been decrypted, but remains Base85.
    namespace Messaging
    {
        inline Result_t groupCount(const LongID_t &GroupID, std::optional<uint32_t> Messagetype)
        {
            auto PS = Messagetype
                ? Prepare("SELECT COUNT(*) FROM Groupmessages WHERE (Target = ? AND Messagetype = ?);", GroupID, Messagetype.value())
                : Prepare("SELECT COUNT(*) FROM Groupmessages WHERE Target = ?;", GroupID);

            Trycatch(
                uint64_t Result{};
                PS >> Result;
                if (Result) return Result;
            );

            return {};
        }
        inline Result_t userCount(const LongID_t &ClientID, std::optional<uint32_t> Messagetype)
        {
            auto PS = Messagetype
                ? Prepare("SELECT COUNT(*) FROM Usermessages WHERE (Target = ? AND Messagetype = ?);", ClientID, Messagetype.value())
                : Prepare("SELECT COUNT(*) FROM Usermessages WHERE Target = ?;", ClientID);

            Trycatch(
                uint64_t Result{};
                PS >> Result;
                if (Result) return Result;
            );

            return {};
        }

        inline Result_t getGroupmessage(const LongID_t &GroupID, std::optional<uint32_t> Messagetype, std::optional<uint32_t> Offset = {})
        {
            std::string SQL = "SELECT * FROM Groupmessages WHERE (Target = ?";
            if (Messagetype) SQL += " AND Messagetype = ?";
            if (Offset) SQL += ") OFFSET ?";
            else SQL += ")";
            SQL += " SORT BY Sent LIMIT 1;";

            auto PS = Prepare(SQL, GroupID);
            if (Messagetype) PS << Messagetype.value();
            if (Offset) PS << Offset.value();

            Trycatch(
                JSON::Object_t Result{};
                PS >> Messaginglambda
                {
                    Result = JSON::Object_t({
                        { "Messagetype", Messagetype },
                        { "Received", Received },
                        { "Message", Message },
                        { "GroupID", Target },
                        { "From", Source },
                        { "Sent", Sent }
                    });
                };
                if (!Result.empty()) return Result;
            );

            return {};
        }
        inline Result_t getMessage(const LongID_t &To, std::optional<LongID_t> From, std::optional<uint32_t> Messagetype, std::optional<uint32_t> Offset = {})
        {
            std::string SQL = "SELECT * FROM Usermessages WHERE (Target = ?";
            if (Messagetype) SQL += " AND Messagetype = ?";
            if (From) SQL += " AND Source = ?";
            if (Offset) SQL += ") OFFSET ?";
            else SQL += ")";
            SQL += " SORT BY Sent LIMIT 1;";

            auto PS = Prepare(SQL, To);
            if (Messagetype) PS << Messagetype.value();
            if (From) PS << From.value();
            if (Offset) PS << Offset.value();

            Trycatch(
                JSON::Object_t Result{};
                PS >> Messaginglambda
                {
                    Result = JSON::Object_t({
                        { "Messagetype", Messagetype },
                        { "Received", Received },
                        { "Message", Message },
                        { "From", Source },
                        { "To", Target },
                        { "Sent", Sent }
                    });
                };
                if (!Result.empty()) return Result;
            );

            return {};
        }
    }

    #pragma region Cleanup
    // Restore warning to default.
    #pragma warning(default: 4100)

    // Avoid leakage.
    #undef Trycatch
    #undef Grouplambda
    #undef Clientlambda
    #undef Accountlambda
    #undef Relationlambda
    #undef Presencelambda
    #undef Messaginglambda
    #undef Groupmemberlambda
    #undef Groupinterestlambda
    #pragma endregion
}
#endif
