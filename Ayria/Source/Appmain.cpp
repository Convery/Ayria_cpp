/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include "Global.hpp"


// Track if we need to use fallback methods.
static bool TLSFallback = false;

// Some games use do not handle exceptions well, so we'll have to catch them.
LONG __stdcall onUnhandledexception(PEXCEPTION_POINTERS Info)
{
    // MSVC thread naming exception for debuggers.
    if (Info->ExceptionRecord->ExceptionCode == 0x406D1388)
    {
        // Double-check, and allow any debugger to handle it if available.
        if (Info->ExceptionRecord->ExceptionInformation[0] == 0x1000 && !IsDebuggerPresent())
        {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // OpenSSLs RAND_poll() causes Windows to throw if RPC services are down.
    if (Info->ExceptionRecord->ExceptionCode == RPC_S_SERVER_UNAVAILABLE)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// Entrypoint when loaded as a shared library.
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

        // Catch any errors.
        SetUnhandledExceptionFilter(onUnhandledexception);

        // Prefer TLS over EP-hooking.
        if (Loaders::InstallTLSCallback())
        {
            // Opt out of further notifications.
            DisableThreadLibraryCalls(hDllHandle);
            return TRUE;
        }

        TLSFallback = true;
        // TODO EP-hook
    }

    // Alternative for games without TLS support.
    if (nReason == DLL_THREAD_ATTACH && TLSFallback)
    {
        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
        TLSFallback = false; // Disable.
        Loaders::Loadplugins();
    }

    return TRUE;
}

// Initialize our subsystems.
static bool Initialized = false;
void Ayriastartup()
{
    if (Initialized) return;
    Initialized = true;

    // Main loop.
    std::thread([]() -> void
    {
        while(true)
        {
            Networking::onFrame();
            Console::onFrame();
        }
    }).detach();
}
