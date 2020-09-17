/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

struct Overlay2_t
{
    std::vector<int> Elements;
    vec2_t Position, Size;
    HWND Windowhandle;

    LRESULT __stdcall Eventhandler(UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
            // Default Windows BS.
            case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
            case WM_NCCREATE: return Windowhandle != NULL;

            // Preferred over WM_MOVE/WM_SIZE for performance.
            case WM_WINDOWPOSCHANGED:
            {
                const auto Info = (WINDOWPOS *)lParam;
                Position = { Info->x, Info->y };
                Size = { Info->cx, Info->cy };
                return NULL;
            }

        }

        return DefWindowProcA(Windowhandle, Message, wParam, lParam);
    }
    static LRESULT __stdcall Windowproc(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        Overlay2_t *This;

        if (Message == WM_NCCREATE)
        {
            This = (Overlay2_t *)((LPCREATESTRUCTA)lParam)->lpCreateParams;
            SetWindowLongPtr(Windowhandle, GWLP_USERDATA, (LONG_PTR)This);
            This->Windowhandle = Windowhandle;
        }
        else
        {
            This = (Overlay2_t *)GetWindowLongPtrA(Windowhandle, GWLP_USERDATA);
        }

        if (This) return This->Eventhandler(Message, wParam, lParam);
        return DefWindowProcA(Windowhandle, Message, wParam, lParam);
    }

    void setWindowposition(vec2_t XY, bool Delta = false)
    {
        if (Delta) XY += Position;
        SetWindowPos(Windowhandle, NULL, XY.x, XY.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
    }
    void setWindowsize(vec2_t WH, bool Delta = false)
    {
        if (Delta) WH += Size;
        SetWindowPos(Windowhandle, NULL, Position.x, Position.y, WH.x, WH.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
    }
    void setVisible(bool Visible = true)
    {
        ShowWindowAsync(Windowhandle, Visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }

    void doPresent()
    {
        // Only render when the overlay is visible.
        if (IsWindowVisible(Windowhandle))
        {
            const auto Device = GetDC(Windowhandle);
            RECT Screen{ 0, 0, Size.x, Size.y };

            // Ensure that the device is 'clean'.
            SelectObject(Device, GetStockObject(DC_PEN));
            SelectObject(Device, GetStockObject(DC_BRUSH));
            SelectObject(Device, GetStockObject(SYSTEM_FONT));

            // TODO(tcn): Investigate if there's a better way than to clean the whole surface at high-resolution.
            static auto Color = 0;
            SetBkColor(Device, Color++);
            ExtTextOutW(Device, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

            // Paint the overlay.
            for (const auto &Element : Elements)
            {
                // TODO
            }

            // Cleanup.
            ReleaseDC(Windowhandle, Device);
        }
    }
    void doFrame(float Deltatime)
    {
        // Process events.
        {
            MSG Message;
            while (PeekMessageA(&Message, Windowhandle, NULL, NULL, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
        }

        // Notify the elements about the frame.
        std::for_each(std::execution::par_unseq, Elements.begin(), Elements.end(), [=](const auto &Element)
        {
            // TODO
        });

        // Repaint if needed.
        doPresent();
    }


    explicit Overlay2_t(vec2_t _Position, vec2_t _Size) : Position(_Position)
    {
        // Register the overlay class.
        WNDCLASSEXA Windowclass{};
        Windowclass.lpfnWndProc = Windowproc;
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.style = CS_SAVEBITS | CS_OWNDC;
        Windowclass.lpszClassName = "Ayria_Overlay";
        Windowclass.cbWndExtra = sizeof(Overlay2_t *);
        Windowclass.hbrBackground = CreateSolidBrush(Clearcolor);
        if (NULL == RegisterClassExA(&Windowclass)) assert(false);

        // Generic overlay style.
        DWORD Style = WS_POPUP | CS_BYTEALIGNWINDOW;
        DWORD StyleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
        if constexpr (Build::isDebug) Style |= WS_BORDER;

        // Topmost, optionally transparent, no icon on the taskbar, zero size so it's not shown.
        if (!CreateWindowExA(StyleEx, Windowclass.lpszClassName, NULL, Style, Position.x, Position.y,
            0, 0, NULL, NULL, NULL, this)) assert(false);

        // To keep static-analysis happy, is set via Windowproc.
        assert(Windowhandle);

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, Clearcolor, 0, LWA_COLORKEY);

        // If we have a size, resize to show the window.
        if (_Size) setWindowsize(_Size);
    }
};

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

    static DWORD __stdcall Mainthread(void *)
    {
        // As we are single-threaded (in release), boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        if constexpr (Build::isDebug) setThreadname("Ayria_Main");

        // Initialize the subsystems.
        // TODO(tcn): Initialize console overlay.
        // TODO(tcn): Initialize networking.
        Overlay2_t Console({ 1080, 720 }, { 600, 720 });
        Console.setVisible();


        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe;
        while (true)
        {
            // Track the frame-time, should be around 33ms.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
            Lastframe = Thisframe;

            // Notify the subsystems about a new frame.
            // TODO(tcn): Console overlay frame.
            // TODO(tcn): Network frame.
            // TODO(tcn): Client frame.
            Console.doFrame(Deltatime);

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
                    Debugprint(va("Average frametime[us]: %lu", Sum / 256));
                    Elapsedtime = 0;
                }
            }

            // Even with software-rendering, we can still hit 600 FPS, so slow down.
            std::this_thread::sleep_until(Lastframe + std::chrono::milliseconds(33));
        }

        return 0;
    }

    static void Initialize()
    {
        // TODO(tcn): Init client info.

        CreateThread(NULL, NULL, Mainthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
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
    WNDCLASSEXA Windowclass{};
    Windowclass.cbSize = sizeof(WNDCLASSEXA);
    Windowclass.lpfnWndProc = DefWindowProcW;
    Windowclass.lpszClassName = "Hostwindow";
    Windowclass.hInstance = GetModuleHandleA(NULL);
    Windowclass.hbrBackground = CreateSolidBrush(0xFF00FF);
    Windowclass.style = CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT;
    if (NULL == RegisterClassExA(&Windowclass)) return 2;

    // Create a simple window.
    const auto Windowhandle = CreateWindowExA(NULL, Windowclass.lpszClassName, "HOST", NULL, 1080, 720,
        1280, 720, NULL, NULL, Windowclass.hInstance, NULL);
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
