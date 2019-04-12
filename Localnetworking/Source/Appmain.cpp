/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#include "Stdinclude.hpp"

// Install the frontend hooks.
extern void InstallWinsock();

// Entrypoint when loaded as a plugin.
extern "C"
{
    EXPORT_ATTR void onStartup(bool)
    {
        // Hook the front-facing APIs.
        InstallWinsock();

        // Initialize the background thread.
        Localnetworking::Createbackend();
    }
    EXPORT_ATTR void onInitialized(bool) { /* Do .data edits */ }
    EXPORT_ATTR bool onMessage(const void *, uint32_t) { return false; }
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        _mkdir("./Ayria/");
        _mkdir("./Ayria/Plugins/");
        _mkdir("./Ayria/Assets/");
        _mkdir("./Ayria/Local/");
        _mkdir("./Ayria/Logs/");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
    }

    return TRUE;
}
#else
__attribute__((constructor)) void DllMain()
{
    // Ensure that Ayrias default directories exist.
    mkdir("./Ayria/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Plugins/", S_IRWXU | S_IRWXG);
    mkdir("./Ayria/Assets/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Local/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Logs/", S_IRWU | S_IRWG);

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif
