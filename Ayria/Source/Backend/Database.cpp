/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

namespace Core
{
    static std::shared_ptr<sqlite3> DBConnection{};

    // For debugging, not static because MSVC < 17.0 does not like it.
    [[maybe_unused]] static void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Whenever the database is updated by our client, we need to sync with the network.
    static void Clientupdatehook(void *, sqlite3 *DB, int Operation, const char *, const char *Table, int64_t, int64_t)
    {
        // We do not track updates of internal tables.
        const auto Tablehash = Hash::WW32(std::string(Table));
        if (Tablehash == Hash::WW32("Accounts") || Tablehash == Hash::WW32("Syncpackets")) [[likely]]
            return;

        // Which interface is the relevant one.
        const auto getValue = (Operation == SQLITE_DELETE) ? sqlite3_preupdate_old : sqlite3_preupdate_new;

        // The public key / ID should always be the first column.
        sqlite3_value *PK; getValue(DB, 0, &PK);

        // Silly data is silly.
        if (sqlite3_value_type(PK) != SQLITE_TEXT) [[unlikely]]
            return;

        // We are only interested in our keys.
        const std::string Publickey = (char *)sqlite3_value_text(PK);
        if (Hash::WW64(Publickey) != Global.getShortID()) [[likely]]
            return;

        // Collect all parameters into JSON.
        JSON::Array_t Values{ Publickey };
        for (int i = 1; i < 20; ++i)
        {
            sqlite3_value *Value;
            if (SQLITE_OK != getValue(DB, i, &Value))
                break;

            switch (sqlite3_value_type(Value))
            {
                case SQLITE_TEXT: Values.emplace_back(std::string((char *)sqlite3_value_text(Value))); break;
                case SQLITE_INTEGER: Values.emplace_back(sqlite3_value_int64(Value)); break;
                case SQLITE_FLOAT: Values.emplace_back(sqlite3_value_double(Value)); break;
                case SQLITE_NULL: Values.emplace_back(JSON::Value_t()); break;
                case SQLITE_BLOB:
                {
                    const auto Size = sqlite3_value_bytes(Value);
                    const auto Data = sqlite3_value_blob(Value);

                    std::vector<uint8_t> Blob(Size, 0);
                    std::memcpy(Blob.data(), Data, Size);

                    Values.emplace_back(Blob);
                    break;
                }
            }
        }

        // Share the update with the world.
        Synchronization::sendSyncpacket(Operation, Table, Values);
    }

    // Manage core tables for the system.
    static void InitializeDB()
    {
        const sqlite::Database_t Database(DBConnection);

        // Database configuration.
        Database << "PRAGMA foreign_keys = ON;";
        Database << "PRAGMA temp_store = MEMORY;";
        Database << "PRAGMA auto_vacuum = INCREMENTAL;";

        // Helper functions for inline hashing.
        {
            static constexpr auto Lambda32 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
            {
                if (argc == 0) return;
                if (SQLITE3_TEXT != sqlite3_value_type(argv[0])) { sqlite3_result_null(context); return; }

                // SQLite may invalidate the pointer if _bytes is called after text.
                const auto Length = sqlite3_value_bytes(argv[0]);
                const auto Hash = Hash::WW32(sqlite3_value_text(argv[0]), Length);
                sqlite3_result_int(context, Hash);
            };
            static constexpr auto Lambda64 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
            {
                if (argc == 0) return;
                if (SQLITE3_TEXT != sqlite3_value_type(argv[0])) { sqlite3_result_null(context); return; }

                // SQLite may invalidate the pointer if _bytes is called after text.
                const auto Length = sqlite3_value_bytes(argv[0]);
                const auto Hash = Hash::WW64(sqlite3_value_text(argv[0]), Length);
                sqlite3_result_int64(context, Hash);
            };
            sqlite3_create_function(Database.Connection.get(), "WW32", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda32, nullptr, nullptr);
            sqlite3_create_function(Database.Connection.get(), "WW64", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda64, nullptr, nullptr);
        }

        // All tables use the accounts as primary identifier (so we only need to store a pointer).
        Database <<
            "CREATE TABLE IF NOT EXISTS Accounts ("
            "Publickey TEXT PRIMARY KEY, "
            "Timestamp INTEGER DEFAULT 0, "
            "Reputation INTEGER DEFAULT 0 );";

        // Clients may purge this table for size, servers should not.
        Database <<
            "CREATE TABLE IF NOT EXISTS Syncpackets ("
            "SenderID TEXT REFERENCES Accounts (Publickey) ON DELETE CASCADE, "
            "Timestamp INTEGER NOT NULL, "
            "Signature TEXT NOT NULL, "
            "Payload TEXT NOT NULL, "
            "isProcessed BOOLEAN DEFAULT FALSE, "
            "UNIQUE (SenderID, Signature) );";
    }
    static void CleanupDB()
    {
        const sqlite::Database_t Database(DBConnection);

        // Only remove packets if the user wants to.
        if (!!Global.Settings.pruneDB)
        {
            const auto Timestamp = (std::chrono::system_clock::now() - std::chrono::hours(24)).time_since_epoch();
            Database << "DELETE FROM Syncpackets WHERE (isProcessed = true AND Timestamp < ?);" << Timestamp.count();
        }

        Database << "PRAGMA incremental_vacuum;";
        Database << "PRAGMA optimize;";
    }

    // Interface with the client database.
    sqlite::Database_t QueryDB()
    {
        if (!DBConnection) [[unlikely]]
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems.
            auto Result = sqlite3_open_v2("./Ayria/Client.sqlite", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            assert(Result == SQLITE_OK);

            // Log errors in debug-mode.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.sqlite");

            // Track our changes to the DB.
            sqlite3_preupdate_hook(Ptr, Clientupdatehook, nullptr);

            // Close the DB at exit to ensure everything's flushed.
            DBConnection = std::shared_ptr<sqlite3>(Ptr, [](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });

            // Cleanup the DB on exit.
            std::atexit(CleanupDB);
            InitializeDB();
        }

        return sqlite::Database_t(DBConnection);
    }
}
