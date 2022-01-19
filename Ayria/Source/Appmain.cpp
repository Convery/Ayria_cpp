/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-12
    License: MIT
*/

#include <Global.hpp>

// Some applications do not handle exceptions well.
static LONG __stdcall onUnhandledexception(PEXCEPTION_POINTERS Context)
{
    // OpenSSLs RAND_poll() causes Windows to throw if RPC services are down.
    if (Context->ExceptionRecord->ExceptionCode == RPC_S_SERVER_UNAVAILABLE)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // DirectSound does not like it if the Audio services are down.
    if (Context->ExceptionRecord->ExceptionCode == RPC_S_UNKNOWN_IF)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Semi-documented way for naming Windows threads.
    if (Context->ExceptionRecord->ExceptionCode == 0x406D1388)
    {
        if (Context->ExceptionRecord->ExceptionInformation[0] == 0x1000)
        {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // Probably crash.
    return EXCEPTION_CONTINUE_SEARCH;
}

// Entrypoint when loaded as a shared library.
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID lpvReserved)
{
    // On startup.
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Although it should already be touched by the ctors, ensure it's propagated and prioritized.
        _mm_prefetch(reinterpret_cast<const char *>(&Global), _MM_HINT_T0);

        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Storage");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Catch any unwanted exceptions.
        SetUnhandledExceptionFilter(onUnhandledexception);

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Initialize the backend and console; in-case plugins need access.
        Backend::Initialize();
        Console::Initialize();

        // If injected, we can't hook. So just load all plugins directly.
        if (lpvReserved == NULL) Plugins::Initialize();
        else
        {
            // We prefer TLS-hooks over EP.
            if (Plugins::InstallTLSHook()) return TRUE;

            // Fallback to EP hooking.
            if (!Plugins::InstallEPHook())
            {
                MessageBoxA(NULL, "Could not install a hook in the target application", "Fatal error", NULL);
                return FALSE;
            }
        }
    }

    return TRUE;
}

int main()
{
    _mm_prefetch(reinterpret_cast<const char *>(&Global), _MM_HINT_T0);

    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Storage");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();

    // Catch any unwanted exceptions.
    SetUnhandledExceptionFilter(onUnhandledexception);

    // Initialize the backend and console; in-case plugins need access.
    Backend::Initialize();
    Console::Initialize();

    // If injected, we can't hook. So just load all plugins directly.
    Plugins::Initialize();


    while (true) Sleep(10);

}
