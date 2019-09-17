/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "Common/Common.hpp"
#include "Stdinclude.hpp"

// Keep the global state together.
namespace Ayria { Globalstate_t Global{}; }

// Exported from the various platforms.
extern void Arclight_init();
extern void Tencent_init();
extern void Steam_init();

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

        // Although useless for most users, we create a console if we get a switch.
        #if defined(NDEBUG)
        if (std::strstr(GetCommandLineA(), "-devcon"))
        #endif
        {
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            SetConsoleTitleA("Platformwrapper Console");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8);
        }

        // Start tracking availability.
        Ayria::Global.Startuptimestamp = time(NULL);

        // If there's a local bootstrap module, we'll load it and trigger TLS.
        if(LoadLibraryA("./Ayria/Bootstrapper64d.dll") || LoadLibraryA("./Ayria/Bootstrapper32d.dll"))
            std::thread([]() { volatile bool NOP; }).detach();
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

// Entrypoint when loaded as a plugin.
extern "C"
{
    EXPORT_ATTR void onStartup(bool)
    {
        // Initialize the various platforms.
        Steam_init();

    }
    EXPORT_ATTR void onInitialized(bool) { /* Do .data edits */ }
    EXPORT_ATTR bool onMessage(const void *, uint32_t) { return false; }
}