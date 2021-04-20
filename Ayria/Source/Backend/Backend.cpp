/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-15
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    using Task_t = struct { uint32_t Last, Period; void(__cdecl *Callback)(); };
    static Inlinedvector<Task_t, 8> Backgroundtasks{};
    static Defaultmutex Threadsafe;

    // Intercept update operations and process them later, might be some minor chance of a data-race.
    static Hashmap<std::string, std::unordered_set<uint64_t>> Databasechanges;
    static void Updatehook(void *, int Operation, char const *, char const *Table, sqlite3_int64 RowID)
    {
        // Someone reported a bug with SQLite, probably fixed but better check.
        assert(Table);

        if (Operation != SQLITE_DELETE) Databasechanges[Table].insert(RowID);
    }
    std::unordered_set<uint64_t> getDatabasechanges(const std::string &Tablename)
    {
        std::unordered_set<uint64_t> Clean{};
        std::swap(Clean, Databasechanges[Tablename]);
        return Clean;
    }

    // For debugging.
    static void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
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

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)())
    {
        std::scoped_lock _(Threadsafe);
        Backgroundtasks.push_back({ 0, PeriodMS, Callback });
    }

    // TODO(tcn): Investigate if we can merge these threads.
    [[noreturn]] static DWORD __stdcall Backgroundthread(void *)
    {
        // Name this thread for easier debugging.
        setThreadname("Ayria_Background");

        // Main loop, runs until the application terminates or DLL unloads.
        while (true)
        {
            // Notify the subsystems about a new frame.
            {
                const auto Currenttime = GetTickCount();
                std::scoped_lock _(Threadsafe);

                for (auto &Task : Backgroundtasks)
                {
                    if ((Task.Last + Task.Period) < Currenttime) [[unlikely]]
                    {
                        Task.Last = Currenttime;
                        Task.Callback();
                    }
                }
            }

            // Most tasks run with periods in seconds.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    [[noreturn]] static DWORD __stdcall Graphicsthread(void *)
    {
        // UI-thread, boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        setThreadname("Ayria_Graphics");

        // Disable DPI scaling on Windows 10.
        if (const auto Callback = GetProcAddress(GetModuleHandleA("User32.dll"), "SetThreadDpiAwarenessContext"))
            reinterpret_cast<size_t (__stdcall *)(size_t)>(Callback)(static_cast<size_t>(-2));

        // Initialize the subsystems.
        // TODO(tcn): Initialize pluginmenu, move the overlay storage somewhere.
        Overlay_t<false> Ingameconsole({}, {});
        Console::Overlay::Createconsole(&Ingameconsole);

        // Optional console for developers.
        if (Global.Applicationsettings.enableExternalconsole) Console::Windows::Createconsole(GetModuleHandleW(NULL));

        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe{};
        while (true)
        {
            // Track the frame-time, should be less than 33ms at worst.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = Thisframe - Lastframe;
            Lastframe = Thisframe;

            // Notify all elements about the frame.
            Ingameconsole.doFrame(std::chrono::duration<float>(Deltatime).count());
            Console::Windows::doFrame();

            // Log frame-average every 5 seconds.
            if constexpr (Build::isDebug)
            {
                static std::array<uint64_t, 256> Timings{};
                static float Elapsedtime{};
                static size_t Index{};

                Timings[Index % 256] = std::chrono::duration_cast<std::chrono::nanoseconds>(Deltatime).count();
                Elapsedtime += std::chrono::duration<float>(Deltatime).count();
                Index++;

                if (Elapsedtime >= 5.0f)
                {
                    const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                    constexpr auto NPS = 1000000000.0;
                    const auto Avg = Sum / 256;

                    Debugprint(va("Average framerate: %5.2f FPS %8lu ns", double(NPS / Avg), Avg));
                    Elapsedtime = 0;
                }
            }

            // Cap the FPS to ~60, as we only render if dirty we can get thousands of FPS.
            std::this_thread::sleep_until(Thisframe + std::chrono::milliseconds(1000 / 60));
        }
    }

    // Save the configuration to disk.
    static void Saveconfig()
    {
        JSON::Object_t Config{};
        Config["enableExternalconsole"] = Global.Applicationsettings.enableExternalconsole;
        Config["enableIATHooking"] = Global.Applicationsettings.enableIATHooking;
        Config["Username"] = std::u8string(Global.Username);
        Config["UserID"] = Global.ClientID;

        FS::Writefile(L"./Ayria/Settings.json", JSON::Dump(Config));
    }

    // Initialize the global state from disk settings.
    static void Initializeglobals()
    {
        const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
        Global.Applicationsettings.enableExternalconsole = Config.value<bool>("enableExternalconsole");
        Global.Applicationsettings.enableIATHooking = Config.value<bool>("enableIATHooking");
        Global.ClientID = Config.value("UserID", 0xDEADC0DE);

        const auto Username = Config.value("Username", u8"AYRIA"s);;
        std::memcpy(Global.Username, Username.data(), std::min(Username.size(), sizeof(Global.Username) - 1));

        // Sanity checking, force save if nothing was parsed.
        if (!FS::Fileexists(L"./Ayria/Settings.json")) Saveconfig();

        // Save the configuration on exit.
        std::atexit([]() { if (Global.Applicationsettings.modifiedConfig) Saveconfig(); });
    }

    // Ensure that all tables for the database is available.
    static void Initializedatabase()
    {
        // Clients, optionally free-range and locally sourced.
        Database() << "CREATE TABLE IF NOT EXISTS Clientinfo ("
                      "ClientID integer primary key unique not null, "
                      "Lastupdate integer, "
                      "B64Authticket text, "
                      "B64Sharedkey text, "
                      "PublicIP integer, "
                      "GameID integer, "
                      "Username text, "
                      "isLocal bool )";

        // Client-info providers, can be queried for Clientinfo by ID.
        Database() << "DROP TABLE IF EXISTS Remoteclients;";
        Database() << "CREATE TABLE Remoteclients ("
                      "Relayaddress integer primary key unique not null, "
                      "Relayport integer not null, "
                      "ClientIDs blob )";

        // Stats-tracking definition per game.
        Database() << "CREATE TABLE IF NOT EXISTS Leaderboard ("
                      "LeaderboardID integer not null, "
                      "Leaderboardname text not null, "
                      "isDecending bool not null, "
                      "GameID integer not null, "
                      "PRIMARY KEY (LeaderboardID, GameID) )";

        // Stats-entry, optionally validated by some authority.
        Database() << "CREATE TABLE IF NOT EXISTS Leaderboardentry ("
                      "LeaderboardID integer not null, "
                      "Timestamp integer not null, "
                      "ClientID integer not null, "
                      "GameID integer not null, "
                      "Score integer not null, "
                      "isValid bool not null"
                      "PRIMARY KEY (LeaderboardID, GameID, ClientID) )";

        // Transient messages are cleared on restart.
        Database() << "CREATE TABLE IF NOT EXISTS Usermessages ("
                      "Messagetype integer not null, "
                      "Timestamp integer not null, "
                      "SenderID integer not null, "
                      "TargetID integer not null, "
                      "B64Message text not null, "
                      "Transient bool not null )";

        // Optionally cleared when the user leaves a group.
        Database() << "CREATE TABLE IF NOT EXISTS Groupmessages ("
                      "Messagetype integer not null, "
                      "Timestamp integer not null, "
                      "SenderID integer not null, "
                      "GroupID integer not null, "
                      "B64Message text not null, "
                      "Transient bool not null )";

        // Simply represents a collection of users, base for derived tables.
        Database() << "CREATE TABLE IF NOT EXISTS Usergroups ("
                      "GroupID integer primary key unique not null, "
                      "GameID integer not null, "
                      "Lastupdate integer, "
                      "MemberIDs blob, "
                      "Memberdata text, " // JSON data.
                      "Groupdata text )"; // JSON data.

        // A request to join a group, extra data can contain passwords or such.
        Database() << "CREATE TABLE IF NOT EXISTS Grouprequests ("
                      "ClientID integer not null,"
                      "GroupID integer not null,"
                      "Timestamp integer,"
                      "Extradata text," // JSON data.
                      "PRIMARY KEY (ClientID, GroupID) )";

        // If a user is blocked, they may not receive other types of messages.
        Database() << "CREATE TABLE IF NOT EXISTS Relationships ("
                      "SourceID integer not null, "
                      "TargetID integer not null, "
                      "isBlocked bool not null, "
                      "isFriend bool not null, "
                      "PRIMARY KEY (SourceID, TargetID) )";

        // Session-related presence info, mainly for friends.
        Database() << "DROP TABLE IF EXISTS Userpresence;";
        Database() << "CREATE TABLE Userpresence ("
                      "ClientID integer not null, "
                      "Key text not null, "
                      "Value text, "
                      "PRIMARY KEY (ClientID, Key) )";

        // Simply track active sessions, best practices for gamedata to come.
        Database() << "DROP TABLE IF EXISTS Matchmakingsessions;";
        Database() << "CREATE TABLE Matchmakingsessions ("
                      "HostID integer primary key unique not null, "
                      "GameID integer not null, "
                      "Hostaddress integer, "
                      "Lastupdate integer, "
                      "Hostport integer, "
                      "Gamedata text )"; // JSON data.

        // Clean up the database from the last session, and some some old tables.
        Database() << "DELETE FROM Matchmakingsessions WHERE Lastupdate < ?;" << time(NULL) - 1800; // 15 minutes.
        Database() << "DELETE FROM Grouprequests WHERE Timestamp < ?;" << time(NULL) - 1800;        // 15 minutes.
        Database() << "DELETE FROM Clientinfo WHERE Lastupdate < ?;" << time(NULL) - 2592000;       // 30 days of inactivity.
        Database() << "DELETE FROM Usergroups WHERE Lastupdate < ?;" << time(NULL) - 2592000;       // 30 days of inactivity.
        Database() << "DELETE FROM Groupmessages WHERE Transient = true;";
        Database() << "DELETE FROM Usermessages WHERE Transient = true;";
        Database() << "PRAGMA VACUUM;";
    }

    // Initialize the system.
    void Initialize()
    {
        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // As of Windows 10 update 20H2 (v2004) we need to set the interrupt resolution for each process.
        timeBeginPeriod(1);

        // Initialize the global state from disk settings.
        Initializeglobals();

        // Create / clear the database if needed.
        Initializedatabase();

        // Initialize subsystems that plugins may need.
        Services::Initialize();

        /*
            TODO(tcn):
            If the user has authenticated in the past, get 'Authhash' from the config.
            Saved as Base64::Encode(AES::Encrypt(bcrypt/argon2(Password), Clientinfo::getHWID()));

            Then authenticate in the background.
        */

        // Workers.
        CreateThread(NULL, NULL, Graphicsthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        CreateThread(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }

    // Export functionality to the plugins.
    extern "C" EXPORT_ATTR void __cdecl Createperiodictask(unsigned int PeriodMS, void(__cdecl *Callback)())
    {
        if (PeriodMS && Callback) Enqueuetask(PeriodMS, Callback);
    }
}
