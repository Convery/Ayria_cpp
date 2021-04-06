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
    Globalstate_t Global{};

    // Contained in SteamGameserver.cpp
    extern bool Initgameserver(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort,
        uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t unServerFlags, const char *pchGameDir, const char *pchVersion);
    extern bool Initgameserver(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort,
        uint16_t usQueryPort, uint32_t unServerFlags, AppID_t nAppID, const char *pchVersion);

    // Exported Steam interface.
    extern "C"
    {
        // Initialization and shutdown.
        static bool isInitialized{};
        EXPORT_ATTR bool SteamAPI_Init()
        {
            if (isInitialized) return true;
            isInitialized = true;

            // For usage tracking.
            Global.Startuptime = uint32_t(time(NULL));

            // Legacy compatibility.
            Redirectmodulehandle();

            // Start processing the IPC separately.
            CreateThread(NULL, NULL, InitializeIPC, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

            // Ensure that interfaces are loaded, this should only be relevant for developers.
            if (Getinterfaceversion(Interfacetype_t::APPS) == 0) Initializeinterfaces();

            // Modern games provide the ApplicationID via SteamAPI_RestartAppIfNecessary.
            // While legacy/dedis have hardcoded steam_apis. Thus we need a configuration file.
            if (Global.ApplicationID == 0)
            {
                // Check for configuration files.
                if (const auto Filehandle = std::fopen("steam_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Global.ApplicationID);
                    std::fclose(Filehandle);
                }
                if (const auto Filehandle = std::fopen("ayria_appid.txt", "r"))
                {
                    std::fscanf(Filehandle, "%u", &Global.ApplicationID);
                    std::fclose(Filehandle);
                }

                // Else we need to at least warn the user, although it should be an error.
                if (Global.ApplicationID == 0)
                {
                    Errorprint("Platformwrapper could not find the games Application ID.");
                    Errorprint("This may cause errors, contact the developer if you experience issues.");
                    Errorprint("Alternatively provide a \"steam_appid.txt\" or \"ayria_appid.txt\" with the ID");

                    Global.ApplicationID = Hash::FNV1a_32("Ayria");
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
                            Global.Installpath = std::make_unique<std::string>(Encoding::toNarrow(std::wstring(Buffer)));
                    }
                    {
                        wchar_t Buffer[260]{}; DWORD Size{ 260 };
                        if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"Language", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                            Global.Locale = std::make_unique<std::string>(Encoding::toNarrow(std::wstring(Buffer)));
                    }
                    RegCloseKey(Registrykey);
                }
            }
            #endif

            // Ask Ayria nicely for data on the client.
            {
                uint32_t AccountID = 0xDEADC0DE;
                auto Username = "Ayria"s;
                auto Locale = "english"s;

                if (const auto Callback = Ayria.JSONRequest) [[likely]]
                {
                    const auto Object = JSON::Parse(Callback("Clientinfo::getAccountinfo", nullptr));
                    AccountID = Object.value("AccountID", 0xDEADC0DE);
                    Username = Object.value("Username", "Ayria"s);
                    Locale = Object.value("Locale", "english"s);
                }

                std::memcpy(Global.Username, Username.data(), std::min(sizeof(Global.Username), Username.size()));
                Global.XUID = { 0x0110000100000000ULL | AccountID };
                Global.Locale = std::make_unique<std::string>(Locale);

                // Ensure that we have a path available.
                if (!Global.Installpath)
                {
                    wchar_t Buffer[260]{};
                    const auto Size = GetCurrentDirectoryW(260, Buffer);
                    Global.Installpath = std::make_unique<std::string>(Encoding::toNarrow(std::wstring(Buffer, Size)));
                }
            }

            // Some legacy applications query the application info from the environment.
            SetEnvironmentVariableW(L"SteamAppId", va(L"%u", Global.ApplicationID).c_str());
            SetEnvironmentVariableW(L"SteamGameId", va(L"%lu", Global.ApplicationID & 0xFFFFFF).c_str());
            #if defined(_WIN32)
            {
                {
                    HKEY Registrykey;
                    constexpr auto Steamregistry = Build::is64bit ? L"Software\\Wow6432Node\\Valve\\Steam\\ActiveProcess" : L"Software\\Valve\\Steam\\ActiveProcess";
                    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, Steamregistry, &Registrykey))
                    {
                        const auto ProcessID = GetCurrentProcessId();
                        const auto UserID = Global.XUID.UserID;

                        // Legacy wants the dlls loaded.
                        const auto Clientpath64 = Encoding::toWide(*Global.Installpath + "\\steamclient64.dll"s);
                        const auto Clientpath32 = Encoding::toWide(*Global.Installpath + "\\steamclient.dll"s);
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
                        L"Software\\Wow6432Node\\Valve\\Steam\\Apps\\"s + va(L"%u", Global.ApplicationID) :
                        L"Software\\Valve\\Steam\\Apps\\"s + va(L"%u", Global.ApplicationID);
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
                constexpr auto Gameoverlay = Build::is64bit ? "gameoverlayrenderer64.dll" : "gameoverlayrenderer.dll";
                constexpr auto Clientlibrary = Build::is64bit ? "steamclient64.dll" : "steamclient.dll";

                LoadLibraryA((*Global.Installpath + Clientlibrary).c_str());
                LoadLibraryA((*Global.Installpath + Gameoverlay).c_str());
            }
            #endif

            // Notify the game that it's properly connected.
            const auto RequestID = Callbacks::Createrequest();
            Callbacks::Completerequest(RequestID, Callbacks::Types::SteamServersConnected_t, nullptr);

            // Notify the plugins that we are initialized.
            if (const auto Callback = Ayria.onInitialized) Callback(false);

            return true;
        }
        EXPORT_ATTR void SteamAPI_Shutdown() { Traceprint(); }
        EXPORT_ATTR bool SteamAPI_IsSteamRunning() { return true; }
        EXPORT_ATTR bool SteamAPI_InitSafe() { return SteamAPI_Init(); }
        EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID) { Global.ApplicationID = unOwnAppID; return false; }
        EXPORT_ATTR const char *SteamAPI_GetSteamInstallPath() { return Global.Installpath->c_str(); }

        // Callback management.
        EXPORT_ATTR void SteamAPI_RunCallbacks()
        {
            Callbacks::Runcallbacks();
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
        EXPORT_ATTR uint64_t SteamGameServer_GetSteamID() { return Global.XUID.FullID; }

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

                    Initgameserver(unIP, usSteamPort, usGamePort, usSpectatorPort, usQueryPort, eServerMode,
                        pchGameDir, pchVersion);
                }
                if (Version == 11 || Version == 12 || Version == 13)
                {
                    const uint16_t usQueryPort = va_arg(Args, uint16_t);
                    const uint32_t eServerMode = va_arg(Args, uint32_t);
                    const char *pchVersion = va_arg(Args, char *);

                    Initgameserver(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, Global.ApplicationID, pchVersion);
                }

                va_end(Args);
            }

            return true;
        }
        EXPORT_ATTR bool SteamGameServer_InitSafe(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchGameDir, const char *pchVersion)
        {
            // Ensure that we are initialized.
            SteamAPI_Init();

            return Initgameserver(unIP, usSteamPort, usGamePort, usSpectatorPort, usQueryPort, eServerMode,
                pchGameDir, pchVersion);
        }
        EXPORT_ATTR bool SteamInternal_GameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersion)
        {
            // Ensure that we are initialized.
            SteamAPI_Init();

            return Initgameserver(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, Global.ApplicationID, pchVersion);
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
