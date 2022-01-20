/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-01
    License: MIT
*/

#include "AABackend.hpp"
#include <winioctl.h>

// 512-bit aligned storage.
Globalstate_t Global{};

// Generate an identifier for the current hardware.
inline std::pair<std::string, std::string> GenerateHWID();

namespace Backend
{
    using Task_t = struct { uint64_t Last, Period; void(__cdecl *Callback)(); };
    static Inlinedvector<Task_t, 8> Backgroundtasks{};
    static Defaultmutex Threadsafe{};

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)())
    {
        std::scoped_lock _(Threadsafe);
        Backgroundtasks.push_back({ 0, PeriodMS, Callback });
    }
    [[noreturn]] static unsigned __stdcall Backgroundthread(void *)
    {
        // Name this thread for easier debugging.
        setThreadname("Ayria_Background");

        // Runs until the application terminates or DLL unloads.
        while (true)
        {
            // Notify the subsystems about a new frame.
            const auto Currenttime = GetTickCount64();
            {
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

    // Set the global cryptokey from various sources.
    void setCryptokey_CRED(std::string_view Cred1, std::string_view Cred2)
    {
        uint8_t Seed[32]{};
        const auto Ret = PKCS5_PBKDF2_HMAC(Cred1.data(), (int)Cred1.size(),
            (uint8_t *)Cred2.data(), (int)Cred2.size(), 12345, EVP_sha256(), 32, Seed);
        if (!Ret) [[unlikely]] return;

        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(std::to_array(Seed));
    }
    void setCryptokey_TEMP()
    {
        uint8_t Seed[32]{};
        RAND_bytes(Seed, 32);
        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(std::to_array(Seed));
    }
    void setCryptokey_HWID()
    {
        const auto [MOBO, UUID] = GenerateHWID();
        const auto Seed = Hash::SHA256(Hash::SHA256(MOBO) + Hash::SHA256(UUID));
        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(Seed);
    }

    // For debugging, not static because MSVC < 17.0 does not like it.
    [[maybe_unused]] void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Interface with the client database.
    sqlite::Database_t Database()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database)
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems.
            auto Result = sqlite3_open_v2("./Ayria/Client.sqlite", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            assert(Result == SQLITE_OK);

            // Log errors in debug-mode.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.sqlite");

            // Close the DB at exit to ensure everything's flushed.
            Database = std::shared_ptr<sqlite3>(Ptr, [](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });

            // Basic initialization.
            sqlite::Database_t(Database) << "PRAGMA foreign_keys = ON;";
            sqlite::Database_t(Database) << "PRAGMA auto_vacuum = INCREMENTAL;";

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
                sqlite3_create_function(Database.get(), "WW32", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda32, nullptr, nullptr);
                sqlite3_create_function(Database.get(), "WW64", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, nullptr, Lambda64, nullptr, nullptr);
            }

            sqlite::Database_t(Database) <<
                "CREATE TABLE IF NOT EXISTS Account ("
                "Publickey TEXT NOT NULL PRIMARY KEY, "
                "Lastseen INTEGER DEFAULT 0 );";

            sqlite::Database_t(Database) <<
                "CREATE TABLE IF NOT EXISTS Messagestream ("
                "Sender TEXT REFERENCES Account (Publickey) ON DELETE CASCADE, "
                "Messagetype INTEGER NOT NULL, "
                "Timestamp INTEGER NOT NULL, "
                "Signature TEXT NOT NULL, "
                "Message TEXT NOT NULL, "
                "isProcessed BOOLEAN, "
                "UNIQUE (Sender, Messagetype, Message) );";  // Only save the last message in case of duplicates.

            // Perform cleanup on exit.
            std::atexit([]()
            {
                if (!!Global.Settings.pruneDB)
                {
                    const auto Timestamp = (std::chrono::system_clock::now() - std::chrono::hours(24)).time_since_epoch();
                    Backend::Database() << "DELETE FROM Messagestream WHERE Timestamp < ?;" << Timestamp.count();
                }

                Backend::Database() << "PRAGMA optimize;";
                Backend::Database() << "PRAGMA incremental_vacuum;";
            });
        }
        return sqlite::Database_t(Database);
    }

    // Save the configuration to disk.
    static void Saveconfig()
    {
        JSON::Object_t Object{};

        // Need to cast the bits to bool as otherwise they'd be uint16.
        Object["enableExternalconsole"] = (bool)Global.Settings.enableExternalconsole;
        Object["enableIATHooking"] = (bool)Global.Settings.enableIATHooking;
        Object["enableFileshare"] = (bool)Global.Settings.enableFileshare;
        Object["pruneDB"] = (bool)Global.Settings.pruneDB;
        Object["Username"] = *Global.Username;

        FS::Writefile(L"./Ayria/Settings.json", JSON::Dump(Object));
    }

    // Initialize the system.
    void Initialize()
    {
        // Load the last configuration from disk.
        const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
        Global.Settings.enableExternalconsole = Config.value<bool>("enableExternalconsole");
        Global.Settings.enableIATHooking = Config.value<bool>("enableIATHooking");
        Global.Settings.enableFileshare = Config.value<bool>("enableFileshare");
        Global.Settings.pruneDB = Config.value<bool>("pruneDB");
        *Global.Username = Config.value("Username", u8"AYRIA"s);

        // Select a source for crypto, credentials are used from the GUI.
        if (std::strstr(GetCommandLineA(), "--randID")) setCryptokey_TEMP();
        else setCryptokey_HWID();

        // Notify the user about the current settings.
        Infoprint("Loaded account:");
        Infoprint(va("ShortID: 0x%08X", Global.getShortID()));
        Infoprint(va("LongID: %s", Global.getLongID().c_str()));
        Infoprint(va("Username: %s", *Global.Username));

        // If there was no config, force-save one for the user instantly.
        std::atexit([]() { if (Global.Settings.modifiedConfig) Saveconfig(); });
        if (Config.empty()) Saveconfig();

        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // As of Windows 10 update 20H2 (v2004) we need to set the interrupt resolution for each process.
        timeBeginPeriod(1);

        // Initialize subsystems that plugins may need.
        Messageprocessing::Initialize();
        Messagebus::Initialize();
        Services::Initialize();
        Console::Initialize();
        Plugins::Initialize();

        // Create a worker-thread in the background.
        _beginthreadex(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }

    // Export functionality to the plugins.
    extern "C" EXPORT_ATTR void __cdecl Createperiodictask(unsigned int PeriodMS, void(__cdecl * Callback)())
    {
        if (PeriodMS && Callback) [[likely]] Enqueuetask(PeriodMS, Callback);
    }
}

// Generate an identifier for the current hardware.
inline std::pair<std::string, std::string> GenerateHWID()
{
    #if defined (_WIN32)
    std::string MOBOSerial{};
    std::string SystemUUID{};

    const auto Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    const auto Buffer = std::make_unique<char8_t[]>(Size);
    GetSystemFirmwareTable('RSMB', 0, Buffer.get(), Size);

    const auto Tablelength = *(DWORD *)(Buffer.get() + 4);
    const auto Version_major = *(uint8_t *)(Buffer.get() + 1);
    auto Table = std::u8string_view(Buffer.get() + 8, Tablelength);

    #define Consume(x) *(x *)Table.data(); Table.remove_prefix(sizeof(x));
    if (Version_major == 0 || Version_major >= 2)
    {
        while (!Table.empty())
        {
            const auto Type = Consume(uint8_t);
            const auto Length = Consume(uint8_t);
            /*const auto Handle =*/ (void)Consume(uint16_t);

            if (Type == 1)
            {
                SystemUUID.reserve(16);
                for (size_t i = 0; i < 16; ++i)
                    SystemUUID.append(std::format("{:02X}", (uint8_t)Table[4 + i]));

                Table.remove_prefix(Length);
            }
            else if (Type == 2)
            {
                const auto Index = *(const uint8_t *)(Table.data() + 3);
                Table.remove_prefix(Length);

                for (uint8_t i = 1; i < Index; ++i)
                    Table.remove_prefix(std::strlen((char *)Table.data()) + 1);

                MOBOSerial = std::string((char *)Table.data());
            }
            else Table.remove_prefix(Length);

            // Have all the information we want?
            if (!MOBOSerial.empty() && !SystemUUID.empty()) break;

            // Skip to next entry.
            while (!Table.empty())
            {
                const auto Value = Consume(uint16_t);
                if (Value == 0) break;
            }
        }
    }
    #undef Consume

    return { MOBOSerial, SystemUUID };
    #else
    static_assert(false, "NIX function is not yet implemented.");
    return {};
    #endif
}
