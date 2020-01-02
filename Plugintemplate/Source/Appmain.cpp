/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-01-02
    License: MIT
*/

#include "Stdinclude.hpp"

// Optional callbacks when loaded as a plugin.
extern "C"
{
    // NOTE(tcn): All of these are optional, simply comment them out if not needed.
    // See docs for usage examples.

    // Callbacks from the Bootstrapper module.
    EXPORT_ATTR void __cdecl onReload(void *Previousinstance) { (void)Previousinstance; }   // User-defined, called by devs for hotpatching plugins.
    EXPORT_ATTR bool __cdecl onEvent(const void *Data, uint32_t Length)                     // User-defined, returns if the event was handled.
    { (void)Data; (void)Length; return false; }
    EXPORT_ATTR void __cdecl onInitialized(bool) {}                                         // Do .data edits in this callback.
    EXPORT_ATTR void __cdecl onStartup(bool) {}                                             // Do .text edits in this callback.

    // Callbacks from the Localnetworking module.
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
