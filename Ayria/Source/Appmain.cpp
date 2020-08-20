/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

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

    // DirectSound does not like it if the Audio services are down.
    if (Info->ExceptionRecord->ExceptionCode == RPC_S_UNKNOWN_IF)
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
        if (Pluginloader::InstallTLSCallback())
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
        Pluginloader::Loadplugins();
    }

    return TRUE;
}

// Entrypoint when running as a hostprocess.
int main(int, char **)
{
    printf("Host startup..\n");

    if (!LoadLibraryA(("Ayria"s + (Build::is64bit ? "64"s : "32"s) + "d.dll"s).c_str()))
        return 3;

    // Register the window.
    WNDCLASSEXA Windowclass{};
    Windowclass.cbSize = sizeof(WNDCLASSEXA);
    Windowclass.lpfnWndProc = DefWindowProcW;
    Windowclass.lpszClassName = "Hostwindow";
    Windowclass.hbrBackground = CreateSolidBrush(0);
    Windowclass.hInstance = GetModuleHandleA(NULL);
    Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
    if (NULL == RegisterClassExA(&Windowclass)) return 2;

    // Topmost, optionally transparent, no icon on the taskbar.
    const auto Windowhandle = CreateWindowExA(NULL, Windowclass.lpszClassName, "HOST", NULL, 1080, 720,
        1280, 720, NULL, NULL, Windowclass.hInstance, NULL);
    if (!Windowhandle) return 1;
    ShowWindow(Windowhandle, SW_SHOW);

    // Trigger TLS.
    std::thread([] {}).detach();

    MSG Message;
    while (GetMessageW(&Message, Windowhandle, NULL, NULL))
    {
        TranslateMessage(&Message);
        DispatchMessageW(&Message);
    }

    return 0;
}

// Initialize our subsystems.
void onStartup()
{
    /*
        TODO(tcn):
        1. Initialize client info.
        2. Initialize networking.
    */




    //Graphics::Createsurface("Console", Graphics::Createconsole());
    // TODO(tcn): Init client information.

    std::thread([]() -> void
    {
        // Name the thread for easier debugging.
        {
            #pragma pack(push, 8)
            using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
            #pragma pack(pop)

            __try
            {
                THREADNAME_INFO Info{ 0x1000, "Ayria_Mainthread", 0xFFFFFFFF };
                RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // As we are single-threaded (in release), boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Initialize the subsystems.
        //Console::Createconsole();
        //Networking::Core::onStartup();
        //Console::onStartup();

        // Depending on system resources, this may still result in 100% utilisation.
        static bool Shouldquit{}; std::atexit([]() { Shouldquit = true; });
        while (!Shouldquit) [[likely]] { onFrame(); std::this_thread::yield(); }

    }).detach();
}
void onFrame()
{
    //Console::onFrame();

    //Networking::Core::onFrame();
    //Graphics::Processsurfaces();
}
