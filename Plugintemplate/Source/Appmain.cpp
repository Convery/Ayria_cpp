/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-01-02
    License: MIT
*/

#include "Stdinclude.hpp"

// Optional callbacks when loaded as a plugin.
extern "C"
{
    // Callbacks from the Bootstrapper module. All of these are optional, see /Docs/ for usage examples.
    EXPORT_ATTR bool __cdecl onEvent(const void *, uint32_t) { return false; }  // User-defined, returns if the event was handled.
    EXPORT_ATTR void __cdecl onReload(const void *) {}                          // User-defined, called by devs for hotpatching plugins.
    EXPORT_ATTR void __cdecl onInitialized(bool) {}                             // Do .data edits in this callback.
    EXPORT_ATTR void __cdecl onStartup(bool) {}                                 // Do .text edits in this callback.

    // Callbacks from the Localnetworking module when enabled.
    EXPORT_ATTR IServer * __cdecl Createserver(const char *Hostname) { (void)Hostname; return nullptr; }
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
    }

    return TRUE;
}
#else
__attribute__((constructor)) void __stdcall DllMain()
{
    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Assets");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif
