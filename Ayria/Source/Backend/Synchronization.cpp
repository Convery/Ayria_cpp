/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-20
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend::Sync
{




    // Set the last-updated timestamp for a client.
    inline void Touchclient(uint32_t ID)
    {
        try { DBQuery() << "REPLACE INTO Knownclients (Timestamp) VALUES (?) WHERE (AccountID = ? OR Sharedkeyhash = ?);"
            << time(NULL) << ID << ID; } catch (...) {}
    }



















    static Hashmap<Eventtype_t, std::unordered_set<Eventcallback_t>> Subscriptions{};

    // Create a database-connection for this session.
    static sqlite::database Database()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database)
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
                      "AyriaID integer primary key unique not null, "
                      "Key text not null, "
                      "Value text not null, "
                      "FOREIGN KEY (AyriaID) REFERENCES Knownclients(Sharedkeyhash));";

                DB << "CREATE TABLE IF NOT EXISTS Clientrelations ("
                      "SourceID integer primary key not null, "
                      "TargetID integer primary key not null, "
                      "isBlocked bool not null, "
                      "isFriend bool not null, "
                      "FOREIGN KEY (SourceID, TargetID) REFERENCES Knownclients(Sharedkeyhash));";
            }
        }
        return sqlite::database(Database);
    }

    // Update the last-modified timestamp.
    static void Touch(uint32_t ID)
    {
        try
        {
            Database()
                << "REPLACE INTO Knownclients (Timestamp) VALUES (?) WHERE (AccountID = ? OR Sharedkeyhash = ?);"
                << time(NULL) << ID << ID;
        }
        catch (...) {}
    }

    // Internal helpers for the tables.
    static JSON::Object_t getKnownclients(uint32_t Keyhash)
    {
        JSON::Object_t Result{};
        try
        {
            Database()
                << "SELECT * FROM Knownclients WHERE Sharedkeyhash = ?;" << Keyhash
                >> [&](uint32_t Sharedkeyhash, uint32_t AccountID, const std::string &Username, bool Validated, uint32_t Timestamp)
            {
                Result = JSON::Object_t({
                    {"Sharedkeyhash", Sharedkeyhash},
                    {"AccountID", AccountID},
                    {"Username", Username},
                    {"Validated", Validated},
                    {"Timestamp", Timestamp}
                });
            };
        }
        catch (...) {}

        return Result;
    }
    static bool setKnownclients(JSON::Value_t &&Keyvalues)
    {
        // Without a primary key we can't do much.
        if (!Keyvalues.contains("Sharedkeyhash")) return false;

        const auto Sharedkeyhash = Keyvalues.value<uint32_t>("Sharedkeyhash");
        const auto Username = Keyvalues.value<std::string>("Username");
        const auto AccountID = Keyvalues.value<uint32_t>("AccountID");
        const auto Validated = Keyvalues.value<bool>("Validated");
        const auto Timestamp = time(NULL);

        // Can we do a full insert or do we need to update each value?
        if (!Keyvalues.contains("AccountID") || !Keyvalues.contains("Username") || !Keyvalues.contains("Validated"))
        {
            try
            {
                if (Keyvalues.contains("AccountID"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, AccountID, Timestamp) VALUES (?, ?, ?);"
                        << Sharedkeyhash << AccountID << Timestamp;
                }
                if (Keyvalues.contains("Username"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, Username, Timestamp) VALUES (?, ?, ?);"
                        << Sharedkeyhash << Username << Timestamp;
                }
                if (Keyvalues.contains("Validated"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, Validated, Timestamp) VALUES (?, ?, ?);"
                        << Sharedkeyhash << Validated << Timestamp;
                }

                return true;
            }
            catch (...) {}
        }
        else
        {
            try
            {
                Database()
                    << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, AccountID, Username, Validated, Timestamp) VALUES (?, ?, ?, ?, ?);"
                    << Sharedkeyhash << AccountID << Username << Validated << Timestamp;
                return true;
            }
            catch (...) {}
        }

        return false;
    }

    static JSON::Object_t getClientinfo(uint32_t Keyhash)
    {
        JSON::Object_t Result{};
        try
        {
            Database()
                << "SELECT * FROM Clientinfo WHERE AyriaID = ?;" << Keyhash
                >> [&](uint32_t AyriaID, const std::string &Sharedkey, uint32_t InternalIP, uint32_t ExternalIP, uint32_t GameID, uint32_t ModID)
            {
                Result = JSON::Object_t({
                    {"AyriaID", AyriaID},
                    {"Sharedkey", Sharedkey},
                    {"InternalIP", InternalIP},
                    {"ExternalIP", ExternalIP},
                    {"GameID", GameID},
                    {"ModID", ModID}
                });
            };
        }
        catch (...) {}

        return Result;
    }
    static bool setClientinfo(JSON::Value_t &&Keyvalues)
    {
        // Without a primary key we can't do much.
        if (!Keyvalues.contains("AyriaID")) return false;

        const auto Sharedkey = Keyvalues.value<std::string>("Sharedkey");
        const auto InternalIP = Keyvalues.value<uint32_t>("InternalIP");
        const auto ExternalIP = Keyvalues.value<uint32_t>("ExternalIP");
        const auto AyriaID = Keyvalues.value<uint32_t>("AyriaID");
        const auto GameID = Keyvalues.value<uint32_t>("GameID");
        const auto ModID = Keyvalues.value<uint32_t>("ModID");

        // Can we do a full insert or do we need to update each value?
        if (!Keyvalues.contains("Sharedkey") || !Keyvalues.contains("InternalIP") || !Keyvalues.contains("ExternalIP") ||
            !Keyvalues.contains("GameID") || !Keyvalues.contains("ModID"))
        {
            try
            {
                if (Keyvalues.contains("Sharedkey"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, Sharedkey) VALUES (?, ?);"
                        << AyriaID << Sharedkey;
                }
                if (Keyvalues.contains("InternalIP"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, InternalIP) VALUES (?, ?);"
                        << AyriaID << InternalIP;
                }
                if (Keyvalues.contains("ExternalIP"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, ExternalIP) VALUES (?, ?);"
                        << AyriaID << ExternalIP;
                }
                if (Keyvalues.contains("GameID"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, GameID) VALUES (?, ?);"
                        << AyriaID << GameID;
                }
                if (Keyvalues.contains("ModID"))
                {
                    Database()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, ModID) VALUES (?, ?);"
                        << AyriaID << ModID;
                }

                Touch(AyriaID);
                return true;
            }
            catch (...) {}
        }
        else
        {
            try
            {
                Database()
                    << "INSERT OR REPLACE INTO Clientinfo (AyriaID, Sharedkey, InternalIP, ExternalIP, GameID, ModID) VALUES (?, ?, ?, ?, ?, ?);"
                    << AyriaID << Sharedkey << InternalIP << ExternalIP << GameID << ModID;

                Touch(AyriaID);
                return true;
            }
            catch (...) {}
        }

        return false;
    }

    static JSON::Array_t getClientpresence(uint32_t Keyhash)
    {
        JSON::Array_t Result{};
        try
        {
            Database()
                << "SELECT * FROM Clientpresence WHERE AyriaID = ?;" << Keyhash
                >> [&](uint32_t AyriaID, const std::string &Key, const std::string &Value)
            {
                Result.emplace_back(JSON::Object_t({
                    {"AyriaID", AyriaID},
                    {"Key", Key},
                    {"Value", Value}
                }));
            };
        }
        catch (...) {}

        return Result;
    }
    static bool setClientpresence(JSON::Array_t &&Keyvalues)
    {
        try
        {
            for (const auto &Item : Keyvalues)
            {
                const auto AyriaID = Item.value<uint32_t>("AyriaID");
                const auto Value = Item.value<std::string>("Value");
                const auto Key = Item.value<std::string>("Key");

                Database()
                    << "INSERT OR REPLACE INTO Clientpresence (AyriaID, Key, Value) VALUES (?, ?, ?);"
                    << AyriaID << Key << Value;
            }

            return true;
        }
        catch (...) {}

        return false;
    }



    // Synchronisation callbacks.
    static void __cdecl AYRIA_DELETE(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = 0;

    }
    static void __cdecl AYRIA_UPDATE(unsigned int NodeID, const char *Message, unsigned int Length);
    static void __cdecl AYRIA_DUMP(unsigned int NodeID, const char *Message, unsigned int Length);
    static void __cdecl AYRIA_QUERY(unsigned int NodeID, const char *Message, unsigned int Length);












    static Spinlock Databaselock{};

    // Debug any errors that come up.
    static void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Create a database-connection for this session.
    static sqlite::database Database()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database)
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems..
            const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if constexpr (Build::isDebug) { sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.db"); }

            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
        }
        return sqlite::database(Database);
    }

    // Create the tables we want to synchronize.
    static bool InitializeDB()
    {
        try
        {
            Database() << "PRAGMA foreign_keys = ON;";

            Database() << "CREATE TABLE IF NOT EXISTS Knownclients ("
                "Sharedkeyhash integer primary key unique not null, "
                "AccountID integer not null, "
                "Username text not null, "
                "Validated bool, "
                "Timestamp integer not null);";

            Database() <<
                "CREATE TABLE IF NOT EXISTS Clientinfo ("
                "AyriaID integer primary key unique not null, "
                "Sharedkey text not null, "
                "InternalIP integer, "
                "ExternalIP integer, "
                "GameID integer, "
                "ModID integer, "
                "FOREIGN KEY (AyriaID) REFERENCES Knownclients(Sharedkeyhash));";

            Database() << "DROP TABLE IF EXISTS Clientpresence;";
            Database() <<
                "CREATE TABLE IF NOT EXISTS Clientpresence ("
                "AyriaID integer primary key unique not null, "
                "Key text not null, "
                "Value text, "
                "FOREIGN KEY (AyriaID) REFERENCES Knownclients(Sharedkeyhash));";

            Database() <<
                "CREATE TABLE IF NOT EXISTS Clientrelations ("
                "SourceID integer primary key not null, "
                "TargetID integer primary key not null, "
                "isBlocked bool not null, "
                "isFriend bool not null, "
                "FOREIGN KEY (SourceID, TargetID) REFERENCES Knownclients(Sharedkeyhash));";


            Database()
                << "CREATE TRIGGER IF NOT EXISTS Presence AFTER UPDATE ON Clientpresence"
                "BEGIN UPDATE Knownclients SET Timestamp = datetime(\'now\', \'unixepoch\') "
                "WHERE Sharedkeyhash = AyriaID END;";


            return true;
        }
        catch (...) {}

        return false;
    }

    // Share state changes with the other plugins.
    void Subscribetoevent(Eventtype_t Eventtype, Eventcallback_t Callback)
    {
        Subscriptions[Eventtype].insert(Callback);
    }
    void Unsubscribe(Eventtype_t Eventtype, Eventcallback_t Callback)
    {
        Subscriptions[Eventtype].erase(Callback);
    }
    void Postevent(Eventtype_t Eventtype, std::string_view JSONData)
    {
        for (const auto &Callback : Subscriptions[Eventtype])
            if (Callback) Callback(Eventtype, JSONData);
    }

    // Internal state updates.
    bool Insert(const std::string &Tablename, JSON::Value_t &&Keyvalues)
    {
        // Verify that the insert has all the keys required.
        switch (Hash::WW32(Tablename))
        {
            case Hash::WW32("Knownclients"):
            {
                if (!Keyvalues.contains("Sharedkeyhash") || !Keyvalues.contains("AccountID") ||
                    !Keyvalues.contains("Username")) return false;
                break;
            }
            case Hash::WW32("Clientinfo"):
            {
                if (!Keyvalues.contains("AyriaID") || !Keyvalues.contains("Sharedkey")) return false;
                break;
            }
            case Hash::WW32("Clientpresence"):
            {
                if (!Keyvalues.contains("AyriaID") || !Keyvalues.contains("Key")) return false;
                break;
            }
            case Hash::WW32("Clientrelations"):
            {
                if (!Keyvalues.contains("SourceID") || !Keyvalues.contains("TargetID") ||
                    !Keyvalues.contains("isBlocked") || !Keyvalues.contains("isFriend")) return false;
                break;
            }
        }

        // And finally insert the new entry.
        try
        {
            switch (Hash::WW32(Tablename))
            {
                case Hash::WW32("Knownclients"):
                {
                    const auto Sharedkeyhash = Keyvalues.value<uint32_t>("Sharedkeyhash");
                    const auto Username = Keyvalues.value<std::string>("Username");
                    const auto AccountID = Keyvalues.value<uint32_t>("AccountID");
                    const auto Validated = Keyvalues.value<bool>("Validated");
                    const auto Timestamp = time(NULL);

                    Database()
                        << "INSERT INTO Knownclients (Sharedkeyhash, AccountID, Username, Validated, Timestamp) VALUES (?, ?, ?, ?, ?);"
                        << Sharedkeyhash << AccountID << Username << Validated << Timestamp;
                    return true;
                }

                case Hash::WW32("Clientinfo"):
                {
                    const auto Sharedkey = Keyvalues.value<std::string>("Sharedkey");
                    const auto InternalIP = Keyvalues.value<uint32_t>("InternalIP");
                    const auto ExternalIP = Keyvalues.value<uint32_t>("ExternalIP");
                    const auto AyriaID = Keyvalues.value<uint32_t>("AyriaID");
                    const auto GameID = Keyvalues.value<uint32_t>("GameID");
                    const auto ModID = Keyvalues.value<uint32_t>("ModID");

                    Database()
                        << "INSERT INTO Clientinfo (AyriaID, Sharedkey, InternalIP, ExternalIP, GameID, ModID) VALUES (?, ?, ?, ?, ?, ?);"
                        << AyriaID << Sharedkey << InternalIP << ExternalIP << GameID << ModID;
                    return true;
                    break;
                }

            }
        }
        catch (...) {}

        return false;
    }
    void Erase(const std::string &Tablename, JSON::Object_t &&Keyvalues);
    void Query(const std::string &Tablename, uint32_t AccountID);
    void Dump(const std::string &Tablename, uint32_t AccountID)
    {

    }
    void Touch(uint32_t AccountID)
    {
        try
        {
            Database() << "REPLACE INTO Knownclients (Timestamp) VALUES (?) WHERE AccountID = ?;"
                << time(NULL) << AccountID;
        }
        catch (...) {}
    }


    // Notify the other systems about database changes.
    void Subscribe(Tablename_t Table, Updatecallback_t Callback)
    {
        Subscriptions[Table].insert(Callback);
    }
    void Unsubscribe(Tablename_t Table, Updatecallback_t Callback)
    {
        Subscriptions[Table].erase(Callback);
    }

    // Process the database-changes each frame.
    static void Processchanges()
    {
        // Check if there's any changes.
        {
            std::scoped_lock Threadguard(Databaselock);
            const auto Empty = Databasechanges.empty() || std::ranges::all_of(Databasechanges, [](const auto &Tuple)
            {
                const auto &[Key, Value] = Tuple;
                return Value.empty();
            });

            // Nothing to do here.
            if (Empty) return;
        }

        // Take a copy of the data..
        Hashmap<std::string, std::unordered_set<std::pair<uint32_t, uint64_t>>> Changes{};
        {
            std::scoped_lock Threadguard(Databaselock);
            Changes.swap(Databasechanges);
        }

        // Helper for JSON.
        const auto toString = [](uint32_t Operation)
        {
            if (Operation == SQLITE_DELETE) return "DELETE"s;
            if (Operation == SQLITE_INSERT) return "INSERT"s;
            return "UPDATE"s;
        };
        const auto Process = [&](const std::string &Tablename, Tablename_t TableID)
        {
            if (Changes.contains(Tablename))
            {
                JSON::Array_t Array{};
                for (const auto &[Operation, RowID] : Changes[Tablename])
                {
                    Array.emplace_back(JSON::Object_t({
                        { "Operation", toString(Operation) },
                        { "RowID", RowID }
                    }));
                }

                const auto JSONString = JSON::Dump(Array);
                for (const auto &Callback : Subscriptions[TableID])
                {
                    Callback(TableID, JSONString);
                }
            }
        };

        // Process table by table.
        Process("Clientpresence", Clientpresence);
        Process("Activeclients", Activeclients);
        Process("Relationships", Relationships);
        Process("Clientinfo", Clientinfo);
    }



    // Process the database-changes each frame.
    static void Processclientinfo(uint64_t Sharedkeyhash, uint32_t AccountID, const std::string &Sharedkey, uint32_t GameID, const std::string &Username, uint32_t PublicIP)
    {
        const auto JSON = JSON::Dump(JSON::Object_t({
            { "Sharedkeyhash", Sharedkeyhash },
            { "AccountID", AccountID },
            { "Sharedkey", Sharedkey},
            { "Username", Username },
            { "PublicIP", PublicIP },
            { "GameID", GameID }
        }));

        for (const auto &Callback : Subscriptions[CLIENT_UPDATE])
        {

        }
    }
    static void Processchanges()
    {
        {
            std::scoped_lock Threadguard(Databaselock);
            const auto Empty = std::ranges::all_of(Databasechanges, [](const auto &Tuple)
            {
                const auto &[Key, Value] = Tuple;
                return Value.empty();
            });

            // Nothing to do here.
            if (Empty) return;
        }

        // Take a copy of the data..
        Hashmap<std::string, std::unordered_set<uint64_t>> Changes{};
        {
            std::scoped_lock Threadguard(Databaselock);
            Changes.swap(Databasechanges);
        }

        // Process table by table.
        if (Changes.contains("Clientinfo"))
        {
            try
            {
                for (const auto &ID : Changes["Clientinfo"])
                {
                    Database()
                        << "SELECT * FROM Clientinfo WHERE rowid = ?;" << ID
                        >> [](uint64_t Sharedkeyhash, uint32_t AccountID, const std::string &Sharedkey, uint32_t GameID, const std::string &Username, uint32_t PublicIP)
                    {


                    };
                }

            }
            catch (...) {};
        }


        if (Changes.contains("Activeclients"))
        {
            try
            {
                for (const auto &ID : Changes["Activeclients"])
                {
                    Database() << "SELECT * FROM Activeclients WHERE rowid = ?;";
                }

            }
            catch (...) {};
        }


    }





    constexpr auto Updateclient = Hash::WW32("AYRIA_CLIENT_UPDATE");
    constexpr auto Dumpclient = Hash::WW32("AYRIA_CLIENT_DUMP");

    // Intercept update operations and process them later, might be some minor chance of a data-race.
    static void Updatehook(void *, int Operation, char const *, char const *Table, sqlite3_int64 RowID)
    {
        // Someone reported a bug with SQLite, probably fixed but better check.
        assert(Table);

        if (Operation != SQLITE_DELETE) Databasechanges[Table].insert(RowID);
    }

    // For debugging.
    static void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Get the client-database.
    sqlite::database Database()
    {
    static std::shared_ptr<sqlite3> Database{};
        if (!Database)
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems.
            const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);

            // Intercept updates from plugins writing to the DB.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.db");
            sqlite3_update_hook(Ptr, Updatehook, nullptr);
            sqlite3_extended_result_codes(Ptr, false);

            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
        }
        return sqlite::database(Database);
    }

    void InitDB()
    {

    }
}
