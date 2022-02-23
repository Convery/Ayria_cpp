/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-20
    License: MIT
*/

// C++ only, for now.
#if defined (__cplusplus)
#pragma once
#include <Stdinclude.hpp>
#include "Ayriamodule.h"

namespace AyriaDB
{
    #pragma region DOC
    /*
        They Ayria database contains the clients view of the larger system.
        This is not guaranteed to be accurate or up to date.
        The only source of truth is the Syncpackets table.
        UTF8 strings are used for plaintext data.

        =======================================================================

        // All tables references the Accounts table for their identifier.
        CREATE TABLE IF NOT EXISTS Accounts (
        Publickey TEXT PRIMARY KEY,             // Base58 encoded.
        Timestamp INTEGER DEFAULT 0,            // UTC nanoseconds since epoch of last update.
        Reputation INTEGER DEFAULT 0 );         // An amalgamation of user reports and other sources.

        // Syncpackets may be pruned by clients, Notifications (often) contain the RowID that triggered it.
        CREATE TABLE IF NOT EXISTS Syncpackets (
        SenderID TEXT REFERENCES Accounts (Publickey),
        Timestamp INTEGER NOT NULL,             // UTC nanoseconds since epoch.
        Signature TEXT NOT NULL,                // Base85 signature of the payload + timestamp.
        Payload TEXT NOT NULL,                  // Base85 encoded.
        isProcessed BOOLEAN DEFAULT FALSE );    // If the system has included it in the clients view.

        =======================================================================

        // Social information, mainly to prettify the UI.
        CREATE TABLE IF NOT EXISTS Clientsocial (
        ClientID TEXT REFERENCES Accounts (Publickey),
        Username TEXT NOT NULL,                 // UTF8 string, defaults to "AYRIA".
        isAway BOOLEAN DEFAULT FALSE );         // Client-specified status, for online status check Accounts.

        // Telemetry about activity, mainly for the UI.
        CREATE TABLE IF NOT EXISTS Clientgaming (
        ClientID TEXT REFERENCES Accounts (Publickey),
        GameID INTEGER NOT NULL,                // Set by Platformwrapper or similar plugins.
        ModID INTEGER,                          // Optional.
        isIngame BOOLEAN NOT NULL );            // Client-specified status, for online status check Accounts.

        // Relationships is what one client considers another, mutual friends have two entries.
        CREATE TABLE IF NOT EXISTS Clientrelation (
        Source TEXT REFERENCES Account(Publickey),  // Client A
        Target TEXT REFERENCES Account(Publickey),  // Client B
        isBlocked BOOLEAN NOT NULL,
        isFriend BOOLEAN NOT NULL );

        =======================================================================

    */
    #pragma endregion

    #pragma region Helpers
    // Open the database on read-only mode.
    inline sqlite::Database_t OpenDB()
    {
        static std::shared_ptr<sqlite3> DBConnection{};
        if (!DBConnection) [[unlikely]]
        {
            sqlite3 *Ptr{};

        // :memory: should never fail unless the client has more serious problems.
        auto Result = sqlite3_open_v2("./Ayria/Client.sqlite", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        assert(Result == SQLITE_OK);

        // Log errors in debug-mode.
        if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.sqlite");

        // Track our changes to the DB.
        sqlite3_preupdate_hook(Ptr, Clientupdatehook, nullptr);

        // Close the DB at exit to ensure everything's flushed.
        DBConnection = std::shared_ptr<sqlite3>(Ptr, [](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
        }

        return sqlite::Database_t(Database);
    }

    // Creation of a prepared statement, evaluates with the '>>' operator or when it goes out of scope.
    template <typename ...Args> [[nodiscard]] inline auto Query(std::string_view SQL, Args&&... va) noexcept
    {
        auto PS = OpenDB() << SQL;
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

    // Helpers when fetching a whole table.
    #define Accountslambda          [&](const Base58_t &Publickey, int64_t Timestamp, int64_t Reputation)
    #define Clientsociallambda      [&](const Base58_t &ClientID, const UTF8_t &Username, bool isAway)
    #define Clientgaminglambda      [&](const Base58_t &ClientID, uint32_t GameID, uint32_t ModID, bool isIngame)
    #define Clientrelationlambda    [&](const Base58_t &Source, const Base58_t &Target, bool isBlocked, bool isFriend)
    #pragma endregion

    // General information about clients.
    namespace Clientinfo
    {
        // Active clients being defined as last seen < 60 seconds ago.
        inline size_t Count(bool incudeAFK = false)
        {
            const auto Timestamp = (std::chrono::system_clock::now() - std::chrono::seconds(60)).time_since_epoch().count();
            size_t Result{};

            if (includeAFK)
            {
                Query("SELECT COUNT(*) FROM Accounts WHERE Timestamp > ?;", Timestamp) >> Result;
            }
            else
            {
                Query("SELECT COUNT(*) FROM Accounts WHERE (Timestamp > ? AND Accounts.Publickey NOT IN "
                      "(SELECT ClientID FROM Clientsocial WHERE isAway = true));", Timestamp) >> Result;
            }

            return Result;
        }

        // Collect commonly used information about a client.
        inline Result_t Fetch(const LongID_t &ClientID)
        {
            JSON::Object_t Result{};

            Query("SELECT * FROM Account WHERE Publickey = ?;", ClientID) >> Accountslambda
            {
                Result["Publickey"] = Publickey;
                Result["Lastseen"] = Timestamp;
            };

            Query("SELECT * FROM Clientsocial WHERE ClientID = ?;", ClientID) >> Clientsociallambda
            {
                Result["Username"] = Username;
                Result["isAway"] = isAway;
            };

            Query("SELECT * FROM Clientgaming WHERE ClientID = ?;", ClientID) >> Clientgaminglambda
            {
                Result["isIngame"] = isIngame;
                Result["GameID"] = GameID;
                Result["ModID"] = ModID;
            };

            // Return NULL on failure.
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
                if (byGameID && byModID) return Query("SELECT ClientID FROM Clientgaming WHERE (GameID = ? AND ModID = ?);", *byGameID, *byModID);
                if (byGameID) return Query("SELECT ClientID FROM Clientgaming WHERE GameID = ?;", *byGameID);
                else return Query("SELECT ClientID FROM Clientgaming WHERE ModID = ?;", *byModID);
            }() >> [&](const Base58_t &ClientID) { Result.insert(ClientID); };

            return Result;
        }
    }




}

#endif
