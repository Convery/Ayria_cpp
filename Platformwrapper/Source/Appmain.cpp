/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Common.hpp"
#include "Steam/Steam.hpp"

// Keep the global state together.
Ayriamodule_t Ayria{};

// Exported from the various platforms.
extern void Arclight_init();
extern void Tencent_init();

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

// Callback when loaded as a plugin.
extern "C" EXPORT_ATTR void __cdecl onStartup(bool)
{
    // Initialize the various platforms.
    Steam::Initializeinterfaces();
    Tencent_init();
}
