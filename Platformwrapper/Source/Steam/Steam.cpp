/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Steam.hpp"

// Keep the global state together.
namespace Steam { Globalstate_t Global{}; }

// Exported Steam interface.
extern "C"
{
    // Initialization and shutdown.
    EXPORT_ATTR bool SteamAPI_Init()
    {
        Traceprint();

        // Modern games provide the ApplicationID via SteamAPI_RestartAppIfNecessary.
        // While legacy/dedis have hardcoded steam_apis. Thus we need a configuration file.
        if (Steam::Global.ApplicationID == 0)
        {
            // Check for configuration files.
            if (auto Filehandle = std::fopen("steam_appid.txt", "r"))
            {
                std::fscanf(Filehandle, "%u", &Steam::Global.ApplicationID);
                std::fclose(Filehandle);
            }
            if (auto Filehandle = std::fopen("ayria_appid.txt", "r"))
            {
                std::fscanf(Filehandle, "%u", &Steam::Global.ApplicationID);
                std::fclose(Filehandle);
            }

            // Else we need to at least warn the user, although it should be an error.
            if (Steam::Global.ApplicationID == 0)
            {
                Errorprint("Platformwrapper could not find the games Application ID.");
                Errorprint("This can cause a lot of errors, contact the developer.");
                Errorprint("Alternatively provide a \"steam_appid.txt\" or \"ayria_appid.txt\"");

                Steam::Global.ApplicationID = Hash::FNV1a_32("Ayria");
            }
        }

        // Query the Ayria platform for the account information.
        if (true /* TODO(tcn): Update when Ayria is done. */)
        {
            Steam::Global.Username = "Ayria";
            Steam::Global.Language = "english";
            Steam::Global.UserID = 0x1100001DEADC0DE;
        }

        // Query the Steam platform for installation-location.
        #if defined(_WIN32)
        {
            HKEY Registrykey;
            constexpr auto Steamregistry = Build::is64bit ? "Software\\Wow6432Node\\Valve\\Steam" : "Software\\Valve\\Steam";

            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Steamregistry, 0, KEY_QUERY_VALUE, &Registrykey) == ERROR_SUCCESS)
            {
                unsigned long Size = 260; unsigned char Buffer[260]{};
                RegQueryValueExA(Registrykey, "InstallPath", NULL, NULL, Buffer, &Size);
                Steam::Global.Path = (char *)Buffer;
                RegCloseKey(Registrykey);
            }
        }
        #endif

        // Some legacy applications query the application info from the environment.
        SetEnvironmentVariableA("SteamAppId", va("%lu", Steam::Global.ApplicationID).c_str());
        SetEnvironmentVariableA("SteamGameId", va("%llu", Steam::Global.ApplicationID & 0xFFFFFF).c_str());
        #if defined(_WIN32)
        {
            HKEY Registrykey;
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_QUERY_VALUE, &Registrykey) == ERROR_SUCCESS)
            {
                DWORD ProcessID = GetCurrentProcessId();
                DWORD UserID = Steam::Global.UserID & 0xFFFFFFFF;

                std::string Clientpath32 = va("%s\\steamclient.dll", Steam::Global.Path.c_str());
                std::string Clientpath64 = va("%s\\steamclient64.dll", Steam::Global.Path.c_str());

                RegSetValueExA(Registrykey, "pid", NULL, REG_DWORD, (LPBYTE)&ProcessID, sizeof(DWORD));
                RegSetValueExA(Registrykey, "ActiveUser", NULL, REG_DWORD, (LPBYTE)&UserID, sizeof(DWORD));
                RegSetValueExA(Registrykey, "Universe", NULL, REG_SZ, (LPBYTE)"Public", (DWORD)std::strlen("Public") + 1);
                RegSetValueExA(Registrykey, "SteamClientDll", NULL, REG_SZ, (LPBYTE)Clientpath32.c_str(), (DWORD)Clientpath32.length() + 1);
                RegSetValueExA(Registrykey, "SteamClientDll64", NULL, REG_SZ, (LPBYTE)Clientpath64.c_str(), (DWORD)Clientpath64.length() + 1);

                RegCloseKey(Registrykey);
            }

            DWORD Disposition;
            if (RegCreateKeyExA(HKEY_CURRENT_USER, va("Software\\Valve\\Steam\\Apps\\%i", Steam::Global.ApplicationID).c_str(), 0, NULL, 0, KEY_WRITE, NULL, &Registrykey, &Disposition) == ERROR_SUCCESS)
            {
                DWORD Running = TRUE;

                RegSetValueExA(Registrykey, "Installed", NULL, REG_DWORD, (LPBYTE)&Running, sizeof(DWORD));
                RegSetValueExA(Registrykey, "Running", NULL, REG_DWORD, (LPBYTE)&Running, sizeof(DWORD));

                RegCloseKey(Registrykey);
            }
        }
        #endif

        // We don't emulate Steams overlay, but users will have to request it.
        #if defined(_WIN32)
        if (std::strstr(GetCommandLineA(), "-overlay"))
        {
            constexpr auto *Gameoverlay = Build::is64bit ? "gameoverlayrenderer64.dll" : "gameoverlayrenderer.dll";
            constexpr auto *Clientlibrary = Build::is64bit ? "steamclient64.dll" : "steamclient.dll";

            SetDllDirectoryA(Steam::Global.Path.c_str());
            LoadLibraryA(va("%s\\%s", Steam::Global.Path.c_str(), Gameoverlay).c_str());
            LoadLibraryA(va("%s\\%s", Steam::Global.Path.c_str(), Clientlibrary).c_str());
        }
        #endif

        // Finally initialize the interfaces by module.
        if (!Steam::Scanforinterfaces("interfaces.txt") && /* TODO(tcn): Parse interfaces from cahce. */
            !Steam::Scanforinterfaces("steam_api.bak")  && !Steam::Scanforinterfaces("steam_api64.bak") &&
            !Steam::Scanforinterfaces("steam_api.dll")  && !Steam::Scanforinterfaces("steam_api64.dll") &&
            !Steam::Scanforinterfaces("steam_api.so")   && !Steam::Scanforinterfaces("steam_api64.so"))
        {
            Errorprint("Platformwrapper could not find the games interface version.");
            Errorprint("This can cause a lot of errors, contact the developer.");
            Errorprint("Alternatively provide a \"interfaces.txt\"");
        }

        return true;
    }
    EXPORT_ATTR bool SteamAPI_InitSafe() { return SteamAPI_Init(); }
    EXPORT_ATTR void SteamAPI_Shutdown() { Traceprint(); }
    EXPORT_ATTR bool SteamAPI_IsSteamRunning() { return true; }
    EXPORT_ATTR const char *SteamAPI_GetSteamInstallPath() { return Steam::Global.Path.c_str(); }
    EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID) { Steam::Global.ApplicationID = unOwnAppID; return false; }

    // Callback management.
    EXPORT_ATTR void SteamAPI_RunCallbacks() { Steam::Callbacks::Runcallbacks(); }
    EXPORT_ATTR void SteamAPI_RegisterCallback(void *pCallback, int iCallback)
    {
        Steam::Callbacks::Registercallback(pCallback, iCallback);
    }
    EXPORT_ATTR void SteamAPI_UnregisterCallback(void *pCallback, int iCallback) { }
    EXPORT_ATTR void SteamAPI_RegisterCallResult(void *pCallback, uint64_t hAPICall) { Traceprint(); }
    EXPORT_ATTR void SteamAPI_UnregisterCallResult(void *pCallback, uint64_t hAPICall) { }

    // Steam proxy.
    EXPORT_ATTR int32_t SteamAPI_GetHSteamUser() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamAPI_GetHSteamPipe() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamUser() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamPipe() { Traceprint(); return { }; }
    EXPORT_ATTR bool SteamGameServer_BSecure() { Traceprint(); return { }; }
    EXPORT_ATTR void SteamGameServer_Shutdown() { Traceprint();}
    EXPORT_ATTR void SteamGameServer_RunCallbacks() { Steam::Callbacks::Runcallbacks(); }
    EXPORT_ATTR uint64_t SteamGameServer_GetSteamID() { Traceprint(); return { }; }
    EXPORT_ATTR bool SteamGameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersionString) { Traceprint(); return true; }
    EXPORT_ATTR bool SteamGameServer_InitSafe(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersionString) { Traceprint(); return true; }
    EXPORT_ATTR bool SteamInternal_GameServer_Init(uint32_t unIP, uint16_t usSteamPort, uint16_t usGamePort, uint16_t usQueryPort, uint32_t eServerMode, const char *pchVersionString) { Traceprint(); return true; }

    // Interface access.
    EXPORT_ATTR void *SteamAppList() { return Steam::Fetchinterface(Steam::Interfacetype_t::APPLIST);  }
    EXPORT_ATTR void *SteamApps() { return Steam::Fetchinterface(Steam::Interfacetype_t::APPS);  }
    EXPORT_ATTR void *SteamClient() { return Steam::Fetchinterface(Steam::Interfacetype_t::CLIENT);  }
    EXPORT_ATTR void *SteamController() { return Steam::Fetchinterface(Steam::Interfacetype_t::CONTROLLER);  }
    EXPORT_ATTR void *SteamFriends() { return Steam::Fetchinterface(Steam::Interfacetype_t::FRIENDS);  }
    EXPORT_ATTR void *SteamGameServer() { return Steam::Fetchinterface(Steam::Interfacetype_t::GAMESERVER);  }
    EXPORT_ATTR void *SteamGameServerHTTP() { return Steam::Fetchinterface(Steam::Interfacetype_t::HTTP);  }
    EXPORT_ATTR void *SteamGameServerInventory() { return Steam::Fetchinterface(Steam::Interfacetype_t::INVENTORY);  }
    EXPORT_ATTR void *SteamGameServerNetworking() { return Steam::Fetchinterface(Steam::Interfacetype_t::NETWORKING);  }
    EXPORT_ATTR void *SteamGameServerStats() { return Steam::Fetchinterface(Steam::Interfacetype_t::GAMESERVERSTATS);  }
    EXPORT_ATTR void *SteamGameServerUGC() { return Steam::Fetchinterface(Steam::Interfacetype_t::UGC);  }
    EXPORT_ATTR void *SteamGameServerUtils() { return Steam::Fetchinterface(Steam::Interfacetype_t::UTILS);  }
    EXPORT_ATTR void *SteamHTMLSurface() { return Steam::Fetchinterface(Steam::Interfacetype_t::HTMLSURFACE);  }
    EXPORT_ATTR void *SteamHTTP() { return Steam::Fetchinterface(Steam::Interfacetype_t::HTTP);  }
    EXPORT_ATTR void *SteamInventory() { return Steam::Fetchinterface(Steam::Interfacetype_t::INVENTORY);  }
    EXPORT_ATTR void *SteamMatchmaking() { return Steam::Fetchinterface(Steam::Interfacetype_t::MATCHMAKING);  }
    EXPORT_ATTR void *SteamMatchmakingServers() { return Steam::Fetchinterface(Steam::Interfacetype_t::MATCHMAKINGSERVERS);  }
    EXPORT_ATTR void *SteamMusic() { return Steam::Fetchinterface(Steam::Interfacetype_t::MUSIC);  }
    EXPORT_ATTR void *SteamMusicRemote() { return Steam::Fetchinterface(Steam::Interfacetype_t::MUSICREMOTE);  }
    EXPORT_ATTR void *SteamNetworking() { return Steam::Fetchinterface(Steam::Interfacetype_t::NETWORKING);  }
    EXPORT_ATTR void *SteamRemoteStorage() { return Steam::Fetchinterface(Steam::Interfacetype_t::REMOTESTORAGE);  }
    EXPORT_ATTR void *SteamScreenshots() { return Steam::Fetchinterface(Steam::Interfacetype_t::SCREENSHOTS);  }
    EXPORT_ATTR void *SteamUnifiedMessages() { return Steam::Fetchinterface(Steam::Interfacetype_t::UNIFIEDMESSAGES);  }
    EXPORT_ATTR void *SteamUGC() { return Steam::Fetchinterface(Steam::Interfacetype_t::UGC);  }
    EXPORT_ATTR void *SteamUser() { return Steam::Fetchinterface(Steam::Interfacetype_t::USER); }
    EXPORT_ATTR void *SteamUserStats() { return Steam::Fetchinterface(Steam::Interfacetype_t::USERSTATS);  }
    EXPORT_ATTR void *SteamUtils() { return Steam::Fetchinterface(Steam::Interfacetype_t::UTILS);  }
    EXPORT_ATTR void *SteamVideo() { return Steam::Fetchinterface(Steam::Interfacetype_t::VIDEO);  }
    EXPORT_ATTR void *SteamMasterServerUpdater() { return Steam::Fetchinterface(Steam::Interfacetype_t::MASTERSERVERUPDATER);  }
    EXPORT_ATTR void *SteamInternal_CreateInterface(const char *Interfacename) { return Steam::Fetchinterface(Interfacename);  }
}
