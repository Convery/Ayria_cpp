/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#include <Stdinclude.hpp>

// Keep the global state easily accessible.
Ayriamodule_t Ayria{};

// Exported initialization for the various subsystems.
namespace Steam { extern void Initialize(); }
namespace UPlay { extern void Initialize(); }
namespace Tencent { extern void Initialize(); }
namespace Arclight { extern void Initialize(); }

// Initialization when loaded as a plugin.
extern "C" EXPORT_ATTR void __cdecl onStartup(bool)
{
    static bool Initialized{};
    if (Initialized) return;
    Initialized = true;

    // Initialize the subsystems.
    Steam::Initialize();
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID lpvReserved)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // TODO(tcn): For 1.0, check profiler if _MM_HINT_T0 is the better choice (fill all cache-levels rather than 1-way tagging).
        // Although it should already be touched by the ctors, ensure it's propagated and prioritized.
        _mm_prefetch(reinterpret_cast<const char *>(&Ayria), _MM_HINT_NTA);

        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Storage");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // We were loaded via LoadLibrary rather than ntdll::ldr, so onStartup (likely) wont be called.
        if (lpvReserved != NULL) onStartup(false);
    }

    return TRUE;
}
#else
__attribute__((constructor)) void __stdcall DllMain()
{
    // TODO(tcn): For 1.0, check profiler if _MM_HINT_T0 is the better choice (fill all cache-levels rather than 1-way tagging).
    // Although it should already be touched by the ctors, ensure it's propagated and prioritized.
    _mm_prefetch(reinterpret_cast<const char *>(&Ayria), _MM_HINT_NTA);

    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Storage");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif


