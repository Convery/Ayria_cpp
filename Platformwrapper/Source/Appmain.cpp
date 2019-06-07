/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Steam/Steam.hpp"

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
        Steam::Global.Startuptimestamp = time(NULL);

        // Legacy compatibility.
        Steam::Redirectmodulehandle();

        // Start processing the IPC separately.
        std::thread(Steam::InitializeIPC).detach();

        // If there's a local bootstrap module, we'll load it.
        LoadLibraryA("Developerbootstrap.dll");

        // Finally initialize the interfaces by module.
        if (!Steam::Scanforinterfaces("interfaces.txt") && /* TODO(tcn): Parse interfaces from cache. */
            !Steam::Scanforinterfaces("steam_api.bak") && !Steam::Scanforinterfaces("steam_api64.bak") &&
            !Steam::Scanforinterfaces("steam_api.dll") && !Steam::Scanforinterfaces("steam_api64.dll") &&
            !Steam::Scanforinterfaces("steam_api.so") && !Steam::Scanforinterfaces("steam_api64.so"))
        {
            Errorprint("Platformwrapper could not find the games interface version.");
            Errorprint("This can cause a lot of errors, contact the developer.");
            Errorprint("Alternatively provide a \"interfaces.txt\"");
        }
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
    EXPORT_ATTR void onStartup(bool) { /* Do .text edits */ }
    EXPORT_ATTR void onInitialized(bool) { /* Do .data edits */ }
    EXPORT_ATTR bool onMessage(const void *, uint32_t) { return false; }
}