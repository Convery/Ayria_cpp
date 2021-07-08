/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-24
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::DB
{
    // Prepare a query for the DB. Creates and initialize it if needed.
    sqlite::database Query()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database) [[unlikely]]
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems..
            const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if constexpr (Build::isDebug)
            {
                sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, [](void *DBName, int Errorcode, const char *Errorstring)
                {
                    Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
                }, "Client.db");
            }

            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });

            // Initialize the database.
            if (Database)
            {
                auto DB = sqlite::database(Database);
                DB << "PRAGMA foreign_keys = ON;";
                DB << "DROP TABLE IF EXISTS Clientpresence;";

                DB << "CREATE TABLE IF NOT EXISTS Knownclients ("
                      "Sharedkeyhash integer primary key unique not null, "
                      "AccountID integer not null, "
                      "Username text not null, "
                      "Validated bool not null, "
                      "Timestamp integer not null);";

                DB << "CREATE TABLE IF NOT EXISTS Clientinfo ("
                      "AyriaID integer primary key unique not null, "
                      "Sharedkey text not null, "
                      "InternalIP integer, "
                      "ExternalIP integer, "
                      "GameID integer, "
                      "ModID integer, "
                      "FOREIGN KEY (AyriaID) REFERENCES Knownclients(Sharedkeyhash));";

                DB << "CREATE TABLE IF NOT EXISTS Clientpresence ("
                      "AyriaID integer not null, "
                      "Key text not null, "
                      "Value text not null, "
                      "FOREIGN KEY (AyriaID) REFERENCES Knownclients(Sharedkeyhash) "
                      "PRIMARY KEY (AyriaID, Key) );";

                DB << "CREATE TABLE IF NOT EXISTS Clientrelations ("
                      "SourceID integer primary key not null, "
                      "TargetID integer primary key not null, "
                      "isBlocked bool not null, "
                      "isFriend bool not null, "
                      "FOREIGN KEY (SourceID, TargetID) REFERENCES Knownclients(Sharedkeyhash));";

                DB << "CREATE TABLE IF NOT EXISTS Clientmessages ("
                      "SourceID integer not null, "
                      "TargetID integer not null, "
                      "Checksum integer not null, "
                      "Timestamp integer not null, "
                      "Message text not null);";

                DB << "CREATE TABLE IF NOT EXISTS Groupmessages ("
                      "SourceID integer not null, "
                      "GroupID integer not null, "
                      "Checksum integer not null, "
                      "Timestamp integer not null, "
                      "Message text not null);";


            }
        }
        return sqlite::database(Database);
    }

    // Get the primary key for a client.
    uint32_t getKeyhash(uint32_t AccountID)
    {
        uint32_t Keyhash = 0;

        try { Query() << "SELECT Sharedkeyhash FROM Knownclients WHERE AccountID = ?;"
                      << AccountID >> Keyhash;
        } catch (...) {}

        return Keyhash;
    }
}

namespace Services
{
    static Hashmap<uint32_t, std::unordered_set<Eventcallback_t>> Subscriptions{};

    // Allow plugins to listen to events from the services.
    void Postevent(std::string_view Identifier, const char *Message, unsigned int Length)
    {
        return Postevent(Hash::WW32(Identifier), Message, Length);
    }
    void Subscribetoevent(std::string_view  Identifier, Eventcallback_t Callback)
    {
        return Subscribetoevent(Hash::WW32(Identifier), Callback);
    }
    void Unsubscribe(std::string_view  Identifier, Eventcallback_t Callback)
    {
        return Unsubscribe(Hash::WW32(Identifier), Callback);
    }

    void Postevent(uint32_t Identifier, const char *Message, unsigned int Length)
    {
        for (const auto &Callback : Subscriptions[Identifier])
            if (Callback) Callback(Identifier, Message, Length);
    }
    void Subscribetoevent(uint32_t Identifier, Eventcallback_t Callback)
    {
        Subscriptions[Identifier].insert(Callback);
    }
    void Unsubscribe(uint32_t Identifier, Eventcallback_t Callback)
    {
        Subscriptions[Identifier].erase(Callback);
    }
}
