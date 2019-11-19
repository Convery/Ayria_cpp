/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-11
    License: MIT
*/

#include "Stdinclude.hpp"

// Install the frontend hooks.
extern void InstallWinsock();
namespace Localnetworking { extern void Createbackend(uint16_t TCPPort = 4200, uint16_t UDPPort = 4201); };

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
__attribute__((constructor)) void DllMain()
{
    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Assets");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif
