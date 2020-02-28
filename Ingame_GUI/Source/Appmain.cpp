/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-19
    License: MIT
*/

#include "Stdinclude.hpp"
#define NK_IMPLEMENTATION
#include "Nuklear_GDI.hpp"
#include "Console.hpp"

enum Colors_t : uint32_t
{
    Log_BG_def =        0x292629,
    Input_BG_def =      0x262326,
    Input_value_def =   0xFCFB97,
    Input_text_def =    0xFCFCFC,
    Tab_def =           0x323A45,

    Dirty_Yellow =  0xBEC02A,
    Dirty_Orange =  0xBE542A,
    Dirty_Red =     0xBE282A,
    Dirty_Blue =    0x218FBD,

    Gray = 0x121212,
    Blue = 0x315571,
    Orange = 0xD6B749,
};

struct
{
    void *Gamewindowhandle;
    void *Windowhandle;
    void *Threadhandle;
    void *Modulehandle;
    DWORD MainthreadID;

    bool isVisible;
} Global{};

// Create a centred window chroma-keyed on 0xFFFFFF.
void *Createwindow()
{
    // Register the window.
    WNDCLASSEXA Windowclass{};
    Windowclass.cbSize = sizeof(WNDCLASSEXA);
    Windowclass.lpszClassName = "Ingame_GUI";
    Windowclass.style = CS_VREDRAW | CS_OWNDC;
    Windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    Windowclass.hInstance = (HINSTANCE)Global.Modulehandle;
    Windowclass.lpfnWndProc = [](HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (NK_GDI::onEvent(wnd, msg, wparam, lparam)) return 0;
        return DefWindowProcW(wnd, msg, wparam, lparam);
    };
    if (NULL == RegisterClassExA(&Windowclass)) return nullptr;

    if (auto Windowhandle = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED, "Ingame_GUI", NULL, WS_POPUP,
                                            NULL, NULL, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL))
    {
        // Use a pixel-value of [0xFF, 0xFF, 0xFF] to mean transparent rather than Alpha.
        // Because using Alpha is slow and we should not use pure white anyway.
        SetLayeredWindowAttributes(Windowhandle, 0xFFFFFF, 0, LWA_COLORKEY);
        return Windowhandle;
    }

    return nullptr;
}

// Poll for new messages and repaint the main window in the background.
DWORD __stdcall Windowthread(void *)
{
    // As we are single-threaded (in release), boost our priority.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Windows tracks message-queues by thread ID, so we need to create the window
    // from this new thread to prevent issues. Took like 8 hours of hackery to find that..
    Global.Windowhandle = Createwindow();
    Console_t<7> Console{};

    // DEV
    auto Context = NK_GDI::Initialize(GetDC((HWND)Global.Windowhandle));

    // Main loop.
    while (true)
    {
        // Track the frame-time, should be less than 33ms.
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        // Update the context for smooth scrolling.
        Context->delta_time_seconds = Deltatime;
        Lastframe = Thisframe;

        // Process window-messages.
        nk_input_begin(Context);
        {
            MSG Message;
            while (PeekMessageW(&Message, (HWND)Global.Windowhandle, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }
        }
        nk_input_end(Context);

        // Check if we should toggle the console (tilde).
        {
            const auto Keystate = GetAsyncKeyState(VK_OEM_5);
            const auto Keydown = Keystate & (1 << 31);
            const auto Newkey = Keystate & (1 << 0);
            if (Newkey && Keydown)
            {
                // Shift modifier extends the log.
                if (GetAsyncKeyState(VK_SHIFT) & (1 << 31))
                {
                    Console.isExtended ^= true;
                }
                else
                {
                    // The first time we display the log, capture the main handle.
                    if (!Global.Gamewindowhandle)
                    {
                        GUITHREADINFO Info{ sizeof(GUITHREADINFO) };
                        GetGUIThreadInfo(Global.MainthreadID, &Info);
                        Global.Gamewindowhandle = Info.hwndActive;
                    }

                    if (Global.isVisible)
                    {
                        ShowWindowAsync((HWND)Global.Windowhandle, SW_HIDE);
                        EnableWindow((HWND)Global.Gamewindowhandle, TRUE);
                    }
                    else
                    {
                        EnableWindow((HWND)Global.Gamewindowhandle, FALSE);
                        SetActiveWindow((HWND)Global.Windowhandle);
                        SetFocus((HWND)Global.Windowhandle);
                    }

                    Global.isVisible ^= true;
                }
            }
        }

        // Only redraw when visible.
        if (Global.isVisible)
        {
            // Follow the main window.
            RECT Gamewindow{};
            GetWindowRect((HWND)Global.Gamewindowhandle, &Gamewindow);
            const auto Height = Gamewindow.bottom - Gamewindow.top;
            const auto Width = Gamewindow.right - Gamewindow.left;
            SetWindowPos((HWND)Global.Windowhandle, 0, Gamewindow.left, Gamewindow.top, Width, Height, NULL);

            // Set the default background to white (chroma-keyed to transparent).
            Context->style.window.fixed_background = nk_style_item_color(nk_rgb(0xFF, 0xFF, 0xFF));
            Console.onRender(Context, nk_vec2(Width, Height));
            NK_GDI::Render();

            // Ensure that the window is marked as visible.
            ShowWindow((HWND)Global.Windowhandle, SW_SHOWNORMAL);
        }
    }

    return 0;
}

// Optional callbacks when loaded as a plugin.
extern "C"
{
    EXPORT_ATTR void __cdecl onInitialized(bool)
    {
        Global.Threadhandle = CreateThread(0, 0, Windowthread, 0, 0, 0);
    }
}

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

        // Save our info for later.
        Global.Modulehandle = hDllHandle;
        Global.MainthreadID = GetCurrentThreadId();


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
