/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    static void setThreadname(std::string_view Name)
    {
        #pragma pack(push, 8)
        using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
        #pragma pack(pop)

        __try
        {
            THREADNAME_INFO Info{ 0x1000, Name.data(), 0xFFFFFFFF };
            RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static DWORD __stdcall Graphicsthread(void *)
    {
        // UI-thread, boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        if constexpr (Build::isDebug) setThreadname("Ayria_Graphics");

        // Initialize the subsystems.
        // TODO(tcn): Initialize pluginmenu.
        Overlay_t Ingameconsole({}, {});
        Console::Overlay::Createconsole(&Ingameconsole);

        // Optional console for developers, runs its own thread.
        if (std::strstr(GetCommandLineA(), "-DEVCON")) Console::Windows::Createconsole(GetModuleHandleW((NULL)));

        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe{};
        while (true)
        {
            // Track the frame-time, should be less than 33ms at worst.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
            Lastframe = Thisframe;

            // Notify all elements about the frame.
            Ingameconsole.doFrame(Deltatime);
            Console::Windows::doFrame();

            // Log frame-average every 5 seconds.
            if constexpr (Build::isDebug)
            {
                static std::array<uint32_t, 256> Timings{};
                static float Elapsedtime{};
                static size_t Index{};

                Timings[Index % 256] = 1000000 * Deltatime;
                Elapsedtime += Deltatime;
                Index++;

                if (Elapsedtime >= 5.0f)
                {
                    const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                    Debugprint(va("Average framerate: %5.2f FPS %5lu us", 1000000.0 / (Sum / 256), Sum / 256));
                    Elapsedtime = 0;
                }
            }

            // For reference, my workstations E5 hits 200FPS while laptops i5 gets 50FPS. Cap to 60.
            std::this_thread::sleep_until(Lastframe + std::chrono::milliseconds(16));
        }

        return 0;
    }
    static DWORD __stdcall Backgroundthread(void *)
    {
        // Mainly IO bound thread, might as well sleep a lot.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

        // Name this thread for easier debugging.
        if constexpr (Build::isDebug) setThreadname("Ayria_Background");

        // Main loop, runs until the application terminates or DLL unloads.
        while (true)
        {
            // Notify the subsystems about a new frame.
            Auxiliary::Updatenetworking();
            Clientinfo::doFrame();

            // TODO(tcn): Get async-job requests and process them here.
        }

        return 0;
    }

    static void Initialize()
    {
        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // Initialize subsystems that plugins may need.
        Clientinfo::API_Initialize();
        Auxiliary::API_Initialize();
        Clientinfo::Initialize();

        // Default network groups.
        Auxiliary::Joinmessagegroup(Auxiliary::Generalport);
        Auxiliary::Joinmessagegroup(Auxiliary::Pluginsport);
        Auxiliary::Joinmessagegroup(Auxiliary::Matchmakeport);
        Auxiliary::Joinmessagegroup(Auxiliary::Fileshareport);

        // Workers.
        CreateThread(NULL, NULL, Graphicsthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        CreateThread(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }
}

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
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Catch any unwanted exceptions.
        SetUnhandledExceptionFilter(onUnhandledexception);

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Initialize the backend; in-case plugins need access.
        Backend::Initialize();

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

// Entrypoint when running as a hostprocess.
int main(int, char **)
{
    printf("Host startup..\n");

    // Register the window.
    WNDCLASSEXW Windowclass{};
    Windowclass.cbSize = sizeof(WNDCLASSEXW);
    Windowclass.lpfnWndProc = DefWindowProcW;
    Windowclass.lpszClassName = L"Hostwindow";
    Windowclass.hInstance = GetModuleHandleA(NULL);
    Windowclass.hbrBackground = CreateSolidBrush(0xFF00FF);
    Windowclass.style = CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT;
    if (NULL == RegisterClassExW(&Windowclass)) return 2;

    const auto hDC = GetDC(GetDesktopWindow());
    const auto swidth = GetDeviceCaps(hDC, HORZRES);
    const auto sheight = GetDeviceCaps(hDC, VERTRES);
    ReleaseDC(GetDesktopWindow(), hDC);

    // Create a simple window.
    const auto Windowhandle = CreateWindowExW(NULL, Windowclass.lpszClassName, L"HOST", NULL, swidth / 4, sheight / 4,
        swidth / 2, sheight / 2, NULL, NULL, Windowclass.hInstance, NULL);
    if (!Windowhandle) return 1;
    ShowWindow(Windowhandle, SW_SHOW);

    // Initialize the backend to test features.
    Backend::Initialize();

    // Poll until quit.
    MSG Message;
    while (GetMessageW(&Message, Windowhandle, NULL, NULL))
    {
        TranslateMessage(&Message);

        if (Message.message == WM_QUIT) return 3;
        else DispatchMessageW(&Message);
    }

    return 0;
}
