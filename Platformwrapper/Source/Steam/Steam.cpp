/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#include <Steam.hpp>

namespace Steam
{
    // Keep the global state together.
    Globalstate_t Global{};

    // Steam uses 32-bit IDs for the account, so we need to do some conversions.
    static Hashmap<uint64_t, std::string> Accountmap;
    SteamID_t toSteamID(const std::string &LongID)
    {
        const auto SessionID = Hash::WW64(LongID) & 0x0FFF;
        const auto UserID = Hash::WW32(LongID);

        const SteamID_t SteamID{ 0x0110000000000000ULL | UserID | SessionID << 32 };
        if (!Accountmap.contains(SteamID)) [[unlikely]]
            Accountmap[SteamID] = LongID;

        return SteamID;
    }
    std::string fromSteamID(SteamID_t AccountID)
    {
        // Generally used for bots and such temp accounts.
        if (AccountID.Accounttype == SteamID_t::Accounttype_t::Anonymous)
            return "";

        // Really fun and expensive to search for..
        if (!Accountmap.contains(AccountID)) [[unlikely]]
        {
            std::unordered_set<std::string> Keys{};
            AyriaAPI::Prepare("SELECT * FROM Account WHERE WW32(Publickey) = ?;", uint32_t(AccountID.UserID))
                              >> [&](const std::string &LongID) { Keys.insert(LongID); };

            // Filter by SessionID.
            std::erase_if(Keys, [ID = AccountID.SessionID](const auto &LongID) { return ID != (Hash::WW64(LongID) & 0x0FFF); });

            // General case, less than 1 / E^9 risk to fail.
            if (Keys.empty()) [[unlikely]] return "";
            if (Keys.size() == 1) [[likely]]
            {
                Accountmap[AccountID] = *Keys.begin();
                return *Keys.begin();
            }

            // Try to see if any is in the friendslist.
            const auto Friends = AyriaAPI::Relations::getFriends(Global.LongID->c_str());
            for (const auto &Item : Keys)
            {
                if (Friends.contains(Item))
                {
                    Accountmap[AccountID] = Item;
                    return Item;
                }
            }

            // If not a friend, check the groups we are in.
            const auto Groups = AyriaAPI::Groups::getMemberships(std::string(*Global.LongID));
            for (const auto &Item : Groups)
            {
                const auto Members = AyriaAPI::Groups::getMembers(Item);
                for (const auto &ID : Members)
                {
                    if (Keys.contains(ID))
                    {
                        Accountmap[AccountID] = ID;
                        return ID;
                    }
                }
            }

            // There's no other logic here, just use the first ID and pray.
            Accountmap[AccountID] = *Keys.begin();
            return *Keys.begin();
        }

        return Accountmap[AccountID];
    }

    // For debugging, not static because MSVC < 17.0 does not like it.
    [[maybe_unused]] void SQLErrorlog(void *DBName, int Errorcode, const char *Errorstring)
    {
        (void)DBName; (void)Errorcode; (void)Errorstring;
        Debugprint(va("SQL error %i in %s: %s", DBName, Errorcode, Errorstring));
    }

    // Interface with the client database, remember try-catch.
    sqlite::Database_t Database()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database)
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems.
            auto Result = sqlite3_open_v2("./Ayria/Steam.sqlite", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) Result = sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            assert(Result == SQLITE_OK);

            // Intercept updates from plugins writing to the DB.
            if constexpr (Build::isDebug) sqlite3_db_config(Ptr, SQLITE_CONFIG_LOG, SQLErrorlog, "Steam.sqlite");
            sqlite3_extended_result_codes(Ptr, false);

            // Close the DB at exit to ensure everything's flushed.
            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });

            // Basic initialization.
            try
            {
                sqlite::Database_t(Database) << "PRAGMA foreign_keys = ON;";
                sqlite::Database_t(Database) << "PRAGMA auto_vacuum = INCREMENTAL;";

                // Helper functions for inline hashing.
                const auto Lambda32 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
                {
                    if (argc == 0) return;
                    if (SQLITE3_TEXT != sqlite3_value_type(argv[0])) { sqlite3_result_null(context); return; }

                    // SQLite may invalidate the pointer if _bytes is called after text.
                    const auto Length = sqlite3_value_bytes(argv[0]);
                    const auto Hash = Hash::WW32(sqlite3_value_text(argv[0]), Length);
                    sqlite3_result_int(context, Hash);
                };
                const auto Lambda64 = [](sqlite3_context *context, int argc, sqlite3_value **argv) -> void
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

                // Steamapps tables.
                {
                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Apps ("
                        "AppID INTEGER PRIMARY KEY, "
                        "Name TEXT NOT NULL );";

                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Languagesupport ("
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "Language TEXT NOT NULL );";

                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS DLCInfo ("
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "DLCID INTEGER NOT NULL, "
                        "Checkfile TEXT NOT NULL, "
                        "Name TEXT NOT NULL, "
                        "PRIMARY KEY (DLCID, AppID) );";

                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS DLCData ("
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "Key TEXT NOT NULL, "
                        "Value TEXT, "
                        "PRIMARY KEY (AppID, Key) );";
                }

                // Steam remote-storage.
                {
                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Clientfiles ("
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "FileID INTEGER NOT NULL, "
                        "Visibility INTEGER, "
                        "Thumbnailfile TEXT, "
                        "Description TEXT, "
                        "Changelog TEXT, "
                        "Filename TEXT, "
                        "Title TEXT, "
                        "Tags TEXT, "
                        "PRIMARY KEY (AppID, FileID) );";
                }

                // Steam achievements and stats.
                {
                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Achievement ("
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "Maxprogress INTEGER NOT NULL, "
                        "API_Name TEXT NOT NULL, "
                        "Name TEXT NOT NULL, "
                        "Description TEXT, "
                        "Icon TEXT, "
                        "PRIMARY_KEY(API_Name, AppID) );";

                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Achievementprogress ("
                        "Name TEXT REFERENCES Achievement (Name) ON DELETE CASCADE, "
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "ClientID INTEGER NOT NULL, "
                        "Currentprogress INTEGER, "
                        "Unlocktime INTEGER, "
                        "PRIMARY KEY (Name, ClientID, AppID) );";
                }

                // Steam gameserver.
                {
                    sqlite::Database_t(Database) <<
                        "CREATE TABLE IF NOT EXISTS Gameserver ("

                        // Required properties.
                        "AppID INTEGER REFERENCES Apps (AppID) ON DELETE CASCADE, "
                        "ServerID INTEGER NOT NULL PRIMARY KEY, "

                        // Less used properties.
                        "Gamedescription TEXT NOT NULL, "
                        "Spectatorname TEXT NOT NULL, "
                        "Servername TEXT NOT NULL, "
                        "Gamedata TEXT NOT NULL, "
                        "Gametags TEXT NOT NULL, "
                        "Gametype TEXT NOT NULL, "
                        "Gamedir TEXT NOT NULL, "
                        "Mapname TEXT NOT NULL, "
                        "Product TEXT NOT NULL, "
                        "Version TEXT NOT NULL, "
                        "Moddir TEXT NOT NULL, "
                        "Region TEXT NOT NULL, "

                        "Keyvalues TEXT NOT NULL, "
                        "Userdata TEXT NOT NULL, "
                        "PublicIP TEXT NOT NULL, "

                        "Spectatorport INTEGER NOT NULL "
                        "Queryport INTEGER NOT NULL, "
                        "Gameport INTEGER NOT NULL, "
                        "Authport INTEGER NOT NULL, "

                        "Playercount INTEGER NOT NULL, "
                        "Serverflags INTEGER NOT NULL, "
                        "Spawncount INTEGER NOT NULL, "
                        "Playermax INTEGER NOT NULL, "
                        "Botcount INTEGER NOT NULL, "

                        "isPasswordprotected BOOLEAN NOT NULL, "
                        "isDedicated BOOLEAN NOT NULL, "
                        "isActive BOOLEAN NOT NULL, "
                        "isSecure BOOLEAN NOT NULL );";
                }

            } catch (...) {}

            // Perform cleanup on exit.
            std::atexit([]()
            {
                try
                {
                    Steam::Database() << "PRAGMA optimize;";
                    Steam::Database() << "PRAGMA incremental_vacuum;";
                } catch (...) {}
            });
        }
        return sqlite::Database_t(Database);
    }

    // Exported Steam interface.
    void Initialize();
    extern "C"
    {
        // Initialization and shutdown.
        EXPORT_ATTR bool SteamAPI_Init()
        {
            static bool Initialized{};
            if (Initialized) return true;
            Initialized = true;

            // Legacy compatibility.
            Redirectmodulehandle();

            // Start processing the IPC separately.
            CreateThread(NULL, NULL, InitializeIPC, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

            // Ensure that interfaces are loaded, this should only be relevant for developers.
            if (Getinterfaceversion(Interfacetype_t::APPS) == 0) Initialize();

            // Modern games provide the AppID via SteamAPI_RestartAppIfNecessary.
            // While legacy/dedis have hardcoded steam_apis. Thus we need a configuration file.
            if (Global.AppID == 0)
            {
                // Check for configuration files.
                if (const auto Filehandle = std::fopen("steam_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Global.AppID);
                    std::fclose(Filehandle);
                }
                if (const auto Filehandle = std::fopen("ayria_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Global.AppID);
                    std::fclose(Filehandle);
                }

                // Else we need to at least warn the user, although it should be an error.
                if (Global.AppID == 0)
                {
                    Errorprint("Platformwrapper could not find the games Application ID.");
                    Errorprint("This may cause errors, contact the developer if you experience issues.");
                    Errorprint("Alternatively provide a \"steam_appid.txt\" or \"ayria_appid.txt\" with the ID");

                    Global.AppID = Hash::FNV1a_32("Ayria");
                }
            }

            // Fetch Steam information from the registry.
            #if defined(_WIN32)
            {
                HKEY Registrykey;
                constexpr auto Steamregistry = Build::is64bit ? L"Software\\Wow6432Node\\Valve\\Steam" : L"Software\\Valve\\Steam";
                if (ERROR_SUCCESS == RegOpenKeyW(HKEY_LOCAL_MACHINE, Steamregistry, &Registrykey))
                {
                    {
                        wchar_t Buffer[260]{}; DWORD Size{ 260 };
                        if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"InstallPath", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                            *Global.Installpath = Encoding::toUTF8(std::wstring(Buffer));
                    }
                    {
                        wchar_t Buffer[260]{}; DWORD Size{ 260 };
                        if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"Language", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                            *Global.Locale = Encoding::toNarrow(std::wstring(Buffer));
                    }
                    RegCloseKey(Registrykey);
                }
            }
            #endif

            // Ask Ayria nicely for data on the client.
            {
                const auto Object = Ayria.doRequest("Client::getLocalclient", {});
                Global.XUID = toSteamID(Object.value<std::string>("LongID", "G06He"));
                *Global.Username = Object.value("Username", "Steam_user"s);
                if (Global.Locale->empty()) *Global.Locale = "english";

                // Ensure that we have a path available.
                if (Global.Installpath->empty())
                {
                    wchar_t Buffer[260]{};
                    const auto Size = GetCurrentDirectoryW(260, Buffer);
                    *Global.Installpath = Encoding::toUTF8(std::wstring(Buffer, Size));
                }
            }

            // Notify Ayria about our game-info.
            Ayria.doRequest("Client::setGameinfo", JSON::Object_t({ {"GameID", Global.AppID} }));

            // Some legacy applications query the application info from the environment.
            SetEnvironmentVariableW(L"SteamAppId", va(L"%u", Global.AppID).c_str());
            SetEnvironmentVariableW(L"SteamGameId", va(L"%lu", Global.AppID & 0xFFFFFF).c_str());
            #if defined(_WIN32)
            {
                {
                    HKEY Registrykey;
                    constexpr auto Steamregistry = Build::is64bit ? L"Software\\Wow6432Node\\Valve\\Steam\\ActiveProcess" : L"Software\\Valve\\Steam\\ActiveProcess";
                    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, Steamregistry, &Registrykey))
                    {
                        const auto ProcessID = GetCurrentProcessId();
                        const auto UserID = Global.XUID.UserID;

                        // Legacy games wants the dlls loaded.
                        const auto Clientpath64 = Encoding::toWide(std::u8string(*Global.Installpath) + u8"\\steamclient64.dll"s);
                        const auto Clientpath32 = Encoding::toWide(std::u8string(*Global.Installpath) + u8"\\steamclient.dll"s);
                        if (Build::is64bit) LoadLibraryW(Clientpath64.c_str());
                        else LoadLibraryW(Clientpath32.c_str());

                        RegSetValueW(Registrykey, L"SteamClientDll64", REG_SZ, Clientpath64.c_str(), (DWORD)Clientpath64.length() + 1);
                        RegSetValueW(Registrykey, L"SteamClientDll", REG_SZ, Clientpath32.c_str(), (DWORD)Clientpath32.length() + 1);
                        RegSetValueW(Registrykey, L"ActiveUser", REG_DWORD, (LPCWSTR)&UserID, sizeof(DWORD));
                        RegSetValueW(Registrykey, L"pid", REG_DWORD, (LPCWSTR)&ProcessID, sizeof(DWORD));
                        RegSetValueW(Registrykey, L"Universe", REG_SZ, L"Public", 7);
                        RegCloseKey(Registrykey);
                    }
                }
                {
                    HKEY Registrykey;
                    auto Steamregistry = Build::is64bit ?
                        L"Software\\Wow6432Node\\Valve\\Steam\\Apps\\"s + va(L"%u", Global.AppID) :
                        L"Software\\Valve\\Steam\\Apps\\"s + va(L"%u", Global.AppID);
                    if (ERROR_SUCCESS == RegCreateKeyW(HKEY_CURRENT_USER, Steamregistry.c_str(), &Registrykey))
                    {
                        DWORD Running = TRUE;

                        RegSetValueW(Registrykey, L"Installed", REG_DWORD, (LPCWSTR)&Running, sizeof(DWORD));
                        RegSetValueW(Registrykey, L"Running", REG_DWORD, (LPCWSTR)&Running, sizeof(DWORD));
                        RegCloseKey(Registrykey);
                    }
                }
            }
            #endif

            // We don't emulate Steams overlay, so users will have to request it.
            #if defined(_WIN32)
            if (std::strstr(GetCommandLineA(), "-overlay"))
            {
                constexpr auto Gameoverlay = Build::is64bit ? "\\gameoverlayrenderer64.dll" : "\\gameoverlayrenderer.dll";
                constexpr auto Clientlibrary = Build::is64bit ? "\\steamclient64.dll" : "\\steamclient.dll";

                LoadLibraryA((Encoding::toNarrow(*Global.Installpath) + Clientlibrary).c_str());
                LoadLibraryA((Encoding::toNarrow(*Global.Installpath) + Gameoverlay).c_str());
            }
            #endif

            // Initialize subsystems.
            Gameserver::Initialize();

            // Notify the game that it's properly connected.
            const auto RequestID = Tasks::Createrequest();
            Tasks::Completerequest(RequestID, Tasks::ECallbackType::SteamServersConnected_t, nullptr);

            // Notify the plugins that we are initialized.
            if (const auto Callback = Ayria.onInitialized) Callback(false);

            return true;
        }
        EXPORT_ATTR void SteamAPI_Shutdown() { Traceprint(); }
        EXPORT_ATTR bool SteamAPI_IsSteamRunning() { return true; }
        EXPORT_ATTR bool SteamAPI_InitSafe() { return SteamAPI_Init(); }
        EXPORT_ATTR const char8_t *SteamAPI_GetSteamInstallPath() { return Global.Installpath->c_str(); }
        EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID) { Global.AppID = unOwnAppID; return false; }

        // Callback management.
        EXPORT_ATTR void SteamAPI_UnregisterCallback(void *, int) { }
        EXPORT_ATTR void SteamAPI_RunCallbacks() { Tasks::Runcallbacks(); }
        EXPORT_ATTR void SteamAPI_UnregisterCallResult(void *, uint64_t) { }
        EXPORT_ATTR void SteamAPI_RegisterCallResult(void *pCallback, uint64_t)
        {
            // One-off callback (though we cheat and implement it as a normal one).
            Tasks::Registercallback(pCallback, -1);
        }
        EXPORT_ATTR void SteamAPI_RegisterCallback(void *pCallback, int iCallback)
        {
            // Broadcasting callback.
            Tasks::Registercallback(pCallback, iCallback);
        }

        // Steam proxy.
        EXPORT_ATTR bool SteamGameServer_BSecure() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamAPI_GetHSteamUser() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamAPI_GetHSteamPipe() { Traceprint(); return { }; }
        EXPORT_ATTR void SteamGameServer_RunCallbacks() { Tasks::Runcallbacks(); }
        EXPORT_ATTR uint64_t SteamGameServer_GetSteamID() { return Global.XUID.FullID; }
        EXPORT_ATTR int32_t SteamGameServer_GetHSteamUser() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamGameServer_GetHSteamPipe() { Traceprint(); return { }; }
        EXPORT_ATTR void SteamGameServer_Shutdown() { Traceprint(); Gameserver::Terminate(); }

        // Gameservers need some extra hackery to support legacy games.
        EXPORT_ATTR bool SteamGameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, ...)
        {
            // Ensure that we are initialized.
            SteamAPI_Init();

            // Per-version initialization.
            if (const auto Version = Getinterfaceversion(Interfacetype_t::GAMESERVER))
            {
                va_list Args;
                va_start(Args, usGamePort);

                if (Version == 10)
                {
                    const uint16_t usSpectatorPort = va_arg(Args, uint16_t);
                    const uint16_t usQueryPort = va_arg(Args, uint16_t);
                    const uint32_t eServerMode = va_arg(Args, uint32_t);
                    const char *pchGameDir = va_arg(Args, char *);
                    const char *pchVersion = va_arg(Args, char *);

                    Gameserver::Start(unIP, usSteamPort, usGamePort, usSpectatorPort, usQueryPort, eServerMode, pchGameDir, pchVersion);
                }
                if (Version == 11 || Version == 12 || Version == 13)
                {
                    const uint16_t usQueryPort = va_arg(Args, uint16_t);
                    const uint32_t eServerMode = va_arg(Args, uint32_t);
                    const char *pchVersion = va_arg(Args, char *);

                    Gameserver::Start(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, Global.AppID, pchVersion);
                }

                va_end(Args);
            }

            return true;
        }
        EXPORT_ATTR bool SteamGameServer_InitSafe(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchGameDir, const char *pchVersion)
        {
            // Ensure that we are initialized.
            SteamAPI_Init();

            return Gameserver::Start(unIP, usSteamPort, usGamePort, usSpectatorPort, usQueryPort, eServerMode, pchGameDir, pchVersion);
        }
        EXPORT_ATTR bool SteamInternal_GameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersion)
        {
            // Ensure that we are initialized.
            SteamAPI_Init();

            return Gameserver::Start(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, Global.AppID, pchVersion);
        }

        // For debugging interface access.
        #if 0
        #define Printcaller() { Debugprint(va("%s from 0x%X", __func__, (size_t)_ReturnAddress())); }
        #else
        #define Printcaller()
        #endif

        // Interface access.
        EXPORT_ATTR void *SteamAppList() { Printcaller(); return Fetchinterface(Interfacetype_t::APPLIST); }
        EXPORT_ATTR void *SteamApps() { Printcaller(); return Fetchinterface(Interfacetype_t::APPS); }
        EXPORT_ATTR void *SteamClient() { Printcaller(); return Fetchinterface(Interfacetype_t::CLIENT); }
        EXPORT_ATTR void *SteamController() { Printcaller(); return Fetchinterface(Interfacetype_t::CONTROLLER); }
        EXPORT_ATTR void *SteamFriends() { Printcaller(); return Fetchinterface(Interfacetype_t::FRIENDS); }
        EXPORT_ATTR void *SteamGameSearch() { Printcaller(); return Fetchinterface(Interfacetype_t::GAMESEARCH); }
        EXPORT_ATTR void *SteamGameServer() { Printcaller(); return Fetchinterface(Interfacetype_t::GAMESERVER); }
        EXPORT_ATTR void *SteamGameServerHTTP() { Printcaller(); return Fetchinterface(Interfacetype_t::HTTP); }
        EXPORT_ATTR void *SteamGameServerInventory() { Printcaller(); return Fetchinterface(Interfacetype_t::INVENTORY); }
        EXPORT_ATTR void *SteamGameServerNetworking() { Printcaller(); return Fetchinterface(Interfacetype_t::NETWORKING); }
        EXPORT_ATTR void *SteamGameServerStats() { Printcaller(); return Fetchinterface(Interfacetype_t::GAMESERVERSTATS); }
        EXPORT_ATTR void *SteamGameServerUGC() { Printcaller(); return Fetchinterface(Interfacetype_t::UGC); }
        EXPORT_ATTR void *SteamGameServerUtils() { Printcaller(); return Fetchinterface(Interfacetype_t::UTILS); }
        EXPORT_ATTR void *SteamHTMLSurface() { Printcaller(); return Fetchinterface(Interfacetype_t::HTMLSURFACE); }
        EXPORT_ATTR void *SteamHTTP() { Printcaller(); return Fetchinterface(Interfacetype_t::HTTP); }
        EXPORT_ATTR void *SteamInventory() { Printcaller(); return Fetchinterface(Interfacetype_t::INVENTORY); }
        EXPORT_ATTR void *SteamMatchmaking() { Printcaller(); return Fetchinterface(Interfacetype_t::MATCHMAKING); }
        EXPORT_ATTR void *SteamMatchmakingServers() { Printcaller(); return Fetchinterface(Interfacetype_t::MATCHMAKINGSERVERS); }
        EXPORT_ATTR void *SteamMusic() { Printcaller(); return Fetchinterface(Interfacetype_t::MUSIC); }
        EXPORT_ATTR void *SteamMusicRemote() { Printcaller(); return Fetchinterface(Interfacetype_t::MUSICREMOTE); }
        EXPORT_ATTR void *SteamNetworking() { Printcaller(); return Fetchinterface(Interfacetype_t::NETWORKING); }
        EXPORT_ATTR void *SteamRemoteStorage() { Printcaller(); return Fetchinterface(Interfacetype_t::REMOTESTORAGE); }
        EXPORT_ATTR void *SteamScreenshots() { Printcaller(); return Fetchinterface(Interfacetype_t::SCREENSHOTS); }
        EXPORT_ATTR void *SteamUnifiedMessages() { Printcaller(); return Fetchinterface(Interfacetype_t::UNIFIEDMESSAGES); }
        EXPORT_ATTR void *SteamUGC() { Printcaller(); return Fetchinterface(Interfacetype_t::UGC); }
        EXPORT_ATTR void *SteamUser() { Printcaller(); return Fetchinterface(Interfacetype_t::USER); }
        EXPORT_ATTR void *SteamUserStats() { Printcaller(); return Fetchinterface(Interfacetype_t::USERSTATS); }
        EXPORT_ATTR void *SteamUtils() { Printcaller(); return Fetchinterface(Interfacetype_t::UTILS); }
        EXPORT_ATTR void *SteamVideo() { Printcaller(); return Fetchinterface(Interfacetype_t::VIDEO); }
        EXPORT_ATTR void *SteamMasterServerUpdater() { Printcaller(); return Fetchinterface(Interfacetype_t::MASTERSERVERUPDATER); }
        EXPORT_ATTR void *SteamInternal_CreateInterface(const char *Interfacename) { Printcaller(); return Fetchinterface(Interfacename); }
    }

    // Initialize the interfaces by hooking.
    void Initialize()
    {
        // Proxy Steams exports with our own information.
        const auto Lambda = [&](const char *Name, void *Target) -> uint32_t
        {
            const auto Address = GetProcAddress(GetModuleHandleA(Build::is64bit ? "steam_api64.dll" : "steam_api.dll"), Name);
            if (Address)
            {
                // If a developer has loaded the plugin as a DLL, ignore it.
                if (Address == Target) return 0;

                return !!Hooking::Stomphook(Address, Target);
            }

            return 0;
        };
        #define Hook(x) Hookcount += Lambda(#x, (void *)x)

        // Count the number of hooks.
        uint32_t Hookcount{};

        // Initialization and shutdown.
        Hook(SteamAPI_Init);
        Hook(SteamAPI_InitSafe);
        Hook(SteamAPI_Shutdown);
        Hook(SteamAPI_IsSteamRunning);
        Hook(SteamAPI_GetSteamInstallPath);
        Hook(SteamAPI_RestartAppIfNecessary);

        // Callback management.
        Hook(SteamAPI_RunCallbacks);
        Hook(SteamAPI_RegisterCallback);
        Hook(SteamAPI_UnregisterCallback);
        Hook(SteamAPI_RegisterCallResult);
        Hook(SteamAPI_UnregisterCallResult);

        // Steam proxy.
        Hook(SteamAPI_GetHSteamUser);
        Hook(SteamAPI_GetHSteamPipe);
        Hook(SteamGameServer_GetHSteamUser);
        Hook(SteamGameServer_GetHSteamPipe);
        Hook(SteamGameServer_BSecure);
        Hook(SteamGameServer_Shutdown);
        Hook(SteamGameServer_RunCallbacks);
        Hook(SteamGameServer_GetSteamID);
        Hook(SteamGameServer_Init);
        Hook(SteamGameServer_InitSafe);
        Hook(SteamInternal_GameServer_Init);

        // Interface access.
        Hook(SteamAppList);
        Hook(SteamApps);
        Hook(SteamClient);
        Hook(SteamController);
        Hook(SteamFriends);
        Hook(SteamGameSearch);
        Hook(SteamGameServer);
        Hook(SteamGameServerHTTP);
        Hook(SteamGameServerInventory);
        Hook(SteamGameServerNetworking);
        Hook(SteamGameServerStats);
        Hook(SteamGameServerUGC);
        Hook(SteamGameServerUtils);
        Hook(SteamHTMLSurface);
        Hook(SteamHTTP);
        Hook(SteamInventory);
        Hook(SteamMatchmaking);
        Hook(SteamMatchmakingServers);
        Hook(SteamMusic);
        Hook(SteamMusicRemote);
        Hook(SteamNetworking);
        Hook(SteamRemoteStorage);
        Hook(SteamScreenshots);
        Hook(SteamUnifiedMessages);
        Hook(SteamUGC);
        Hook(SteamUser);
        Hook(SteamUserStats);
        Hook(SteamUtils);
        Hook(SteamVideo);
        Hook(SteamMasterServerUpdater);
        Hook(SteamInternal_CreateInterface);
        #undef Hook

        // Verify that we are in Steam mode.
        if (Hookcount)
        {
            // Finally initialize the interfaces by module.
            if (!Scanforinterfaces("interfaces.txt") && /* TODO(tcn): Parse interfaces from cache. */
                !Scanforinterfaces("steam_api.bak") && !Scanforinterfaces("steam_api64.bak") &&
                !Scanforinterfaces("steam_api.dll") && !Scanforinterfaces("steam_api64.dll") &&
                !Scanforinterfaces("steam_api.so") && !Scanforinterfaces("steam_api64.so"))
            {
                Errorprint("Platformwrapper could not find the games interface version.");
                Errorprint("This can cause a lot of errors, contact the developer.");
                Errorprint("Alternatively provide a \"interfaces.txt\"");
            }

            // Initialize the Steam system
            SteamAPI_Init();
        }
    }
}
