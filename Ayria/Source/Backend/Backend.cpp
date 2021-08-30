/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-01
    License: MIT
*/

#include "AABackend.hpp"
#include <openssl/curve25519.h>
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
            auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            assert(Result == SQLITE_OK);

            // Intercept updates from plugins writing to the DB.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Client.db");
            sqlite3_extended_result_codes(Ptr, false);

            // Close the DB at exit to ensure everything's flushed.
            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
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
    static bool Initialauth()
    {
        const auto Commandline = std::wstring(GetCommandLineW());
        if (Commandline.find(L"--Auth") == std::wstring::npos) return false;

        const auto &[MOBO, UUID] = GenerateHWID();
        const auto Hash1 = Hash::SHA256(MOBO);
        const auto Hash2 = Hash::SHA256(UUID);

        uint8_t Seed[32]{};
        if (NULL == PKCS5_PBKDF2_HMAC(Hash1.data(), Hash1.size(), (uint8_t *)Hash2.data(),
                                      Hash2.size(), 123456, EVP_sha512(), 32, Seed)) [[unlikely]] return false;

        ED25519_keypair_from_seed(Global.SigningkeyPublic->data(), Global.SigningkeyPrivate->data(), Seed);
        const auto Hash = Hash::SHA256(*Global.SigningkeyPrivate);

        // The encryption-key is RFC 7748 compliant.
        std::memcpy(Global.EncryptionkeyPrivate->data(), Hash.data(), Hash.size());
        (*Global.EncryptionkeyPrivate)[0] |= ~248;
        (*Global.EncryptionkeyPrivate)[31] &= ~64;
        (*Global.EncryptionkeyPrivate)[31] |= ~127;

        // TODO(tcn): Notify Ayria server.
        return true;
    }

    // Initialize the system.
    void Initialize()
    {
        // Initialize the signing key-pair with random data.
        ED25519_keypair(Global.SigningkeyPublic->data(), Global.SigningkeyPrivate->data());
        const auto Hash = Hash::SHA256(*Global.SigningkeyPrivate);

        // The encryption-key is RFC 7748 compliant.
        std::memcpy(Global.EncryptionkeyPrivate->data(), Hash.data(), Hash.size());
        (*Global.EncryptionkeyPrivate)[0] |= ~248;
        (*Global.EncryptionkeyPrivate)[31] &= ~64;
        (*Global.EncryptionkeyPrivate)[31] |= ~127;

        // Try to authenticate and get proper keys.
        Global.Settings.isAuthenticated = Initialauth();

        // Load the last configuration from disk.
        const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
        Global.Settings.enableExternalconsole = Config.value<bool>("enableExternalconsole");
        Global.Settings.enableIATHooking = Config.value<bool>("enableIATHooking");
        Global.Settings.enableFileshare = Config.value<bool>("enableFileshare");
        *Global.Username = Config.value("Username", u8"AYRIA"s);

        // Notify the user about the current settings.
        Infoprint("Loaded settings:");
        Infoprint(va("Username: %*s", Global.Username->size(), Global.Username->data()));
        Infoprint(va("LongID: %s", Global.getLongID().c_str()));
        Infoprint(va("ShortID: 0x08X", Global.getShortID()));

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
            const auto Handle = Consume(uint16_t);

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
