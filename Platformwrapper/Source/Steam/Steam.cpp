/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Stdinclude.hpp"
#include "Steam.hpp"

// Exported Steam interface.
extern "C"
{
    // Initialization and shutdown.
    EXPORT_ATTR bool SteamAPI_Init()
    {
        Traceprint();

        // Start processing the IPC separately.
        std::thread(Steam::InitializeIPC).detach();






        return true;
    }
    EXPORT_ATTR bool SteamAPI_InitSafe() { return SteamAPI_Init(); }
    EXPORT_ATTR void SteamAPI_Shutdown() { Traceprint(); }
    EXPORT_ATTR bool SteamAPI_IsSteamRunning() { Traceprint(); return true; }
    EXPORT_ATTR const char *SteamAPI_GetSteamInstallPath() { Traceprint(); return nullptr; }
    EXPORT_ATTR bool SteamAPI_RestartAppIfNecessary(uint32_t unOwnAppID) { Traceprint(); return false; }

    // Callback management.
    EXPORT_ATTR void SteamAPI_RunCallbacks() { Traceprint(); }
    EXPORT_ATTR void SteamAPI_RegisterCallback(void *pCallback, int iCallback) { Traceprint(); }
    EXPORT_ATTR void SteamAPI_UnregisterCallback(void *pCallback, int iCallback) { Traceprint(); }
    EXPORT_ATTR void SteamAPI_RegisterCallResult(void *pCallback, uint64_t hAPICall) { Traceprint(); }
    EXPORT_ATTR void SteamAPI_UnregisterCallResult(void *pCallback, uint64_t hAPICall) { Traceprint(); }

    // Steam proxy.
    EXPORT_ATTR int32_t SteamAPI_GetHSteamUser() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamAPI_GetHSteamPipe() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamUser() { Traceprint(); return { }; }
    EXPORT_ATTR int32_t SteamGameServer_GetHSteamPipe() { Traceprint(); return { }; }
    EXPORT_ATTR bool SteamGameServer_BSecure() { Traceprint(); return { }; }
    EXPORT_ATTR void SteamGameServer_Shutdown() { Traceprint();}
    EXPORT_ATTR void SteamGameServer_RunCallbacks() { Traceprint();}
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
