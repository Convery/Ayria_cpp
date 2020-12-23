/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    // Global state.
    Steaminfo_t Steam{};

    // Exported Steam interface.
    extern "C"
    {
        // Initialization and shutdown.
        bool isInitialized{};
        EXPORT_ATTR bool SteamAPI_Init()
        {
            if (isInitialized) return true;
            isInitialized = true;

            // For usage tracking.
            Steam.Startuptime = time(NULL);

            // Legacy compatibility.
            Redirectmodulehandle();

            // Start processing the IPC separately.
            CreateThread(NULL, NULL, InitializeIPC, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

            // Ensure that interfaces are loaded, this should only be relevant for developers.
            if (Getinterfaceversion(Interfacetype_t::APPS) == 0) Initializeinterfaces();

            // Modern games provide the ApplicationID via SteamAPI_RestartAppIfNecessary.
            // While legacy/dedis have hardcoded steam_apis. Thus we need a configuration file.
            if (Steam.ApplicationID == 0)
            {
                // Check for configuration files.
                if (const auto Filehandle = std::fopen("steam_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Steam.ApplicationID);
                    std::fclose(Filehandle);
                }
                if (const auto Filehandle = std::fopen("ayria_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Steam.ApplicationID);
                    std::fclose(Filehandle);
                }

                // Else we need to at least warn the user, although it should be an error.
                if (Steam.ApplicationID == 0)
                {
                    Errorprint("Platformwrapper could not find the games Application ID.");
                    Errorprint("This may cause errors, contact the developer if you experience issues.");
                    Errorprint("Alternatively provide a \"steam_appid.txt\" or \"ayria_appid.txt\" with the ID");

                    Steam.ApplicationID = Hash::FNV1a_32("Ayria");
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
                        wchar_t Buffer[260]{}; long Size{ 260 };
                        if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"InstallPath", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                            Steam.Installpath = std::wstring(Buffer);
                    }
                    {
                        wchar_t Buffer[260]{}; long Size{ 260 };
                        if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"Language", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                            Steam.Steamlocale = std::wstring(Buffer);
                    }
                    RegCloseKey(Registrykey);
                }
            }
            #endif

            // Ask Ayria nicely for data on the client.
            {
                Steam.Username = u8"Ayria"s;
                Steam.Ayrialocale = u8"english"s;
                Steam.XUID = 0x1100001DEADC0DEULL;

                if (const auto Callback = Ayria.API_Client) [[likely]]
                {
                    const auto Object = ParseJSON(Callback(Ayria.toFunctionID("Accountinfo"), nullptr));
                    const auto AccountID = Object.value("AccountID", 0xDEADC0DE);
                    Steam.Ayrialocale = Object.value("Locale", u8"english"s);
                    Steam.Username = Object.value("Username", u8"Ayria"s);
                    Steam.XUID = 0x0110000100000000ULL | AccountID;

                    // Ensure that a locale is set.
                    if (Steam.Steamlocale.size() == 0)
                        Steam.Steamlocale = Steam.Ayrialocale;
                }
            }

            // Some legacy applications query the application info from the environment.
            SetEnvironmentVariableW(L"SteamAppId", va(L"%u", Steam.ApplicationID).c_str());
            SetEnvironmentVariableW(L"SteamGameId", va(L"%lu", Steam.ApplicationID & 0xFFFFFF).c_str());
            #if defined(_WIN32)
            {
                {
                    HKEY Registrykey;
                    constexpr auto Steamregistry = Build::is64bit ? L"Software\\Wow6432Node\\Valve\\Steam\\ActiveProcess" : L"Software\\Valve\\Steam\\ActiveProcess";
                    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, Steamregistry, &Registrykey))
                    {
                        const auto ProcessID = GetCurrentProcessId();
                        const auto UserID = Steam.XUID.GetAccountID();

                        // Legacy wants the dlls loaded.
                        const auto Clientpath64 = Steam.Installpath.asWCHAR() + L"\\steamclient64.dll";
                        const auto Clientpath32 = Steam.Installpath.asWCHAR() + L"\\steamclient.dll";
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
                        L"Software\\Wow6432Node\\Valve\\Steam\\Apps\\"s + va(L"%u", Steam.ApplicationID) :
                        L"Software\\Valve\\Steam\\Apps\\"s + va(L"%u", Steam.ApplicationID);
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
                constexpr auto Gameoverlay = Build::is64bit ? L"gameoverlayrenderer64.dll" : L"gameoverlayrenderer.dll";
                constexpr auto Clientlibrary = Build::is64bit ? L"steamclient64.dll" : L"steamclient.dll";

                LoadLibraryW((Steam.Installpath.asWCHAR() + Clientlibrary).c_str());
                LoadLibraryW((Steam.Installpath.asWCHAR() + Gameoverlay).c_str());
            }
            #endif

            // Notify the game that it's properly connected.
            const auto RequestID = Callbacks::Createrequest();
            Callbacks::Completerequest(RequestID, Callbacks::k_iSteamUserCallbacks + 1, nullptr);

            // Notify the plugins that we are initialized.
            const auto Callback = GetProcAddress(Ayria.Modulehandle, "onInitialized");
            if (Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);

            return true;
        }
        EXPORT_ATTR void SteamAPI_Shutdown() { Traceprint(); }
        EXPORT_ATTR bool SteamAPI_IsSteamRunning() { return true; }
        EXPORT_ATTR bool SteamAPI_InitSafe() { return SteamAPI_Init(); }
        EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID) { Steam.ApplicationID = unOwnAppID; return false; }
        EXPORT_ATTR const char *SteamAPI_GetSteamInstallPath() { static auto Path = Steam.Installpath.asUTF8(); return (char *)Path.c_str(); }

        // Callback management.
        EXPORT_ATTR void SteamAPI_RunCallbacks()
        {
            Callbacks::Runcallbacks();
            Matchmaking::doFrame();
            Social::doFrame();
        }
        EXPORT_ATTR void SteamAPI_RegisterCallback(void *pCallback, int iCallback)
        {
            // Broadcasting callback.
            Callbacks::Registercallback(pCallback, iCallback);
        }
        EXPORT_ATTR void SteamAPI_UnregisterCallback(void *pCallback, int iCallback) { }
        EXPORT_ATTR void SteamAPI_RegisterCallResult(void *pCallback, uint64_t hAPICall)
        {
            // One-off callback (though we cheat and implement it as a normal one).
            Callbacks::Registercallback(pCallback, -1);
        }
        EXPORT_ATTR void SteamAPI_UnregisterCallResult(void *pCallback, uint64_t hAPICall) { }

        // Steam proxy.
        EXPORT_ATTR int32_t SteamAPI_GetHSteamUser() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamAPI_GetHSteamPipe() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamGameServer_GetHSteamUser() { Traceprint(); return { }; }
        EXPORT_ATTR int32_t SteamGameServer_GetHSteamPipe() { Traceprint(); return { }; }
        EXPORT_ATTR bool SteamGameServer_BSecure() { Traceprint(); return { }; }
        EXPORT_ATTR void SteamGameServer_Shutdown() { Traceprint(); }
        EXPORT_ATTR void SteamGameServer_RunCallbacks() { Callbacks::Runcallbacks(); }
        EXPORT_ATTR uint64_t SteamGameServer_GetSteamID() { return Steam.XUID.ConvertToUint64(); }
        EXPORT_ATTR bool SteamGameServer_Init(uint32_t unIP, uint16_t usPort, uint16_t usGamePort, ...)
        {
            // For AppID.
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
                    const char *pchVersionString = va_arg(Args, char *);

                    Infoprint(va("Starting a Steam-gameserver\n> Address: %u.%u.%u.%u\n> Auth-port: %u\n> Game-port: %u\n> Spectator-port: %u\n> Query-port: %u\n> Version \"%s\"",
                        ((uint8_t *)&unIP)[3], ((uint8_t *)&unIP)[2], ((uint8_t *)&unIP)[1], ((uint8_t *)&unIP)[0],
                        usPort, usGamePort, usSpectatorPort, usQueryPort, pchVersionString));

                    // New session for the server.
                    auto Session = Matchmaking::getLocalsession();
                    Session->Steam.Gamemod = Encoding::toUTF8(pchGameDir);
                    Session->Steam.Spectatorport = usSpectatorPort;
                    Session->Steam.Queryport = usQueryPort;
                    Session->Steam.Gameport = usGamePort;
                    Session->Steam.Authport = usPort;
                    Session->Steam.IPAddress = unIP;

                    uint32_t a{}, b{}, c{}, d{};
                    std::sscanf(pchVersionString, "%u.%u.%u.%u", &a, &b, &c, &d);
                    Session->Steam.Versionint = d + (c * 10) + (b * 100) + (a * 1000);
                    Session->Steam.Versionstring = Encoding::toUTF8(pchVersionString);
                    Matchmaking::Invalidatesession();
                }
                if (Version == 11 || Version == 12)
                {
                    const uint16_t usQueryPort = va_arg(Args, uint16_t);
                    const uint32_t eServerMode = va_arg(Args, uint32_t);
                    const char *pchVersionString = va_arg(Args, char *);

                    Infoprint(va("Starting a Steam-gameserver\n> Address: %u.%u.%u.%u\n> Auth-port: %u\n> Game-port: %u\n> Query-port: %u\n> Version \"%s\"",
                        ((uint8_t *)&unIP)[3], ((uint8_t *)&unIP)[2], ((uint8_t *)&unIP)[1], ((uint8_t *)&unIP)[0],
                        usPort, usGamePort, usQueryPort, pchVersionString));

                    // New session for the server.
                    auto Session = Matchmaking::getLocalsession();
                    Session->Steam.Queryport = usQueryPort;
                    Session->Steam.Gameport = usGamePort;
                    Session->Steam.Authport = usPort;
                    Session->Steam.IPAddress = unIP;

                    uint32_t a{}, b{}, c{}, d{};
                    std::sscanf(pchVersionString, "%u.%u.%u.%u", &a, &b, &c, &d);
                    Session->Steam.Versionint = d + (c * 10) + (b * 100) + (a * 1000);
                    Session->Steam.Versionstring = Encoding::toUTF8(pchVersionString);
                    Matchmaking::Invalidatesession();
                }

                va_end(Args);
            }

            return true;
        }
        EXPORT_ATTR bool SteamGameServer_InitSafe(uint32_t unIP, uint16_t usPort, uint16_t usGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchGameDir, const char *pchVersionString)
        {
            return SteamGameServer_Init(unIP, usPort, usGamePort, usSpectatorPort, usQueryPort, eServerMode, pchGameDir, pchVersionString);
        }
        EXPORT_ATTR bool SteamInternal_GameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersionString)
        {
            return SteamGameServer_Init(unIP, usSteamPort, usGamePort, 0, usQueryPort, eServerMode, "", pchVersionString);
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
    void Initializeinterfaces()
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
