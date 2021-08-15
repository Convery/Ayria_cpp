/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-01
    License: MIT
*/

#include "AABackend.hpp"
#include <openssl/curve25519.h>

// 512-bit aligned storage.
Globalstate_t Global{};

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
    [[noreturn]] static DWORD __stdcall Backgroundthread(void *)
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

    // For debugging.
    static void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Interface with the client database, remember try-catch.
    sqlite::database Database()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database)
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems.
            auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
            if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
            assert(Result == SQLITE_OK);

            // Intercept updates from plugins writing to the DB.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.db");
            sqlite3_extended_result_codes(Ptr, false);

            // Close the DB at exit to ensure everything's flushed.
            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });

            /*
                INIT CLIENT DB?
            */
        }
        return sqlite::database(Database);
    }

    // Save the configuration to disk.
    static void Saveconfig()
    {
        JSON::Object_t Object{};
        Object["enableExternalconsole"] = Global.Settings.enableExternalconsole;
        Object["enableIATHooking"] = Global.Settings.enableIATHooking;
        Object["enableFileshare"] = Global.Settings.enableFileshare;
        Object["Username"] = *Global.Username;

        FS::Writefile(L"./Ayria/Settings.json", JSON::Dump(Object));
    }

    // Perform startup authentication if credentials have been provided.
    static void Initialauth()
    {
        const auto Commandline = std::string(GetCommandLineA());
        static const std::regex rxCommands(R"(("[^"]+"|[^\s"]+))", std::regex_constants::optimize);
        const auto Start = std::sregex_iterator(Commandline.cbegin(), Commandline.cend(), rxCommands);
        const auto End = std::sregex_iterator();

        // Extract the credentials.
        bool hasE{}, hasP{}; std::string Email{}, Password{};
        for (auto It = Start; It != End; ++It)
        {
            // Hackery because STL regex doesn't support lookahead/behind.
            std::string Temp((It++)->str());
            if (Temp.back() == '\"') Temp.pop_back();
            if (Temp.front() == '\"') Temp.erase(0, 1);

            if (hasE) { Email = Temp; hasE = false; continue; }
            if (hasP) { Password = Temp; hasP = false; continue; }
            if (Temp.starts_with("--E")) { hasE = true; continue; }
            if (Temp.starts_with("--P")) { hasP = true; continue; }
        }

        // If nothing is provided, we can't do much.
        if (Email.empty() || Password.empty()) return;

        // Derive keys from the credentials.
        uint8_t Seed[32]{};
        if (NULL == PKCS5_PBKDF2_HMAC(Password.c_str(), Password.size(), (uint8_t *)Email.c_str(), Email.size(), 123456, EVP_sha512(), 32, Seed))
            [[unlikely]] return;
        ED25519_keypair_from_seed(Global.SigningkeyPublic->data(), Global.SigningkeyPrivate->data(), Seed);

        // Update the ID for the new keys.
        Global.AccountID.KeyID = Hash::WW32(*Global.SigningkeyPublic);

        // TODO(tcn): Perform HTTPS auth to get the AyriaID.
    }

    // Initialize the system.
    void Initialize()
    {
        // Initialize the cryptographic-keys with random data, RFC 7748 compliant.
        ED25519_keypair(Global.SigningkeyPublic->data(), Global.SigningkeyPrivate->data());
        RAND_bytes(Global.EncryptionkeyPrivate->data(), 32);
        (*Global.EncryptionkeyPrivate)[0] |= ~248;
        (*Global.EncryptionkeyPrivate)[31] &= ~64;
        (*Global.EncryptionkeyPrivate)[31] |= ~127;

        // Try to authenticate and get proper IDs.
        Global.AccountID.KeyID = Hash::WW32(*Global.SigningkeyPublic);
        Initialauth();

        // Load the last configuration from disk.
        const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
        Global.Settings.enableExternalconsole = Config.value<bool>("enableExternalconsole");
        Global.Settings.enableIATHooking = Config.value<bool>("enableIATHooking");
        Global.Settings.enableFileshare = Config.value<bool>("enableFileshare");
        *Global.Username = Config.value("Username", u8"AYRIA"s);

        // If there was no config, force-save one for the user instantly.
        std::atexit([]() { if (Global.Settings.modifiedConfig) Saveconfig(); });
        if (Config.empty()) Saveconfig();

        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // As of Windows 10 update 20H2 (v2004) we need to set the interrupt resolution for each process.
        timeBeginPeriod(1);

        // Initialize subsystems that plugins may need.
        Networking::Initialize();
        //Services::Initialize();

        // Create a worker-thread in the background.
        CreateThread(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }

    // Export functionality to the plugins.
    extern "C" EXPORT_ATTR void __cdecl Createperiodictask(unsigned int PeriodMS, void(__cdecl * Callback)())
    {
        if (PeriodMS && Callback) [[likely]] Enqueuetask(PeriodMS, Callback);
    }
}
