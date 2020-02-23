/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-19
    License: MIT
*/

#include "Stdinclude.hpp"
#pragma warning(push, 0)
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_FIXED_TYPES
#define NK_GDI_IMPLEMENTATION
#define NK_IMPLEMENTATION
#include "nuklear.h"
#include "Nuklear_GDI.h"
#pragma warning(pop)

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "Msimg32.lib")

// Somewhat object-orientated.
template<size_t Maxtabs = 6>
struct Console_t
{
    // Extended log.
    bool isExtended{};

    // Console tabs.
    uint32_t Tabcount{};
    uint32_t Currenttab{ Maxtabs };
    std::array<std::string, Maxtabs> Titles{};
    std::array<std::string, Maxtabs> Filters{};
    std::array<uint32_t, Maxtabs> Unreadcounts{};
    std::deque<std::pair<std::string, struct nk_color>> Rawdata{};

    // Window properties.
    struct nk_rect Region { 20, 50 };

    // Global input.
    std::array<char, 256> Inputstring;

    void Createtabs(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.button.normal, nk_style_item_color(nk_rgb(0x32, 0x3A, 0x45)));
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x26, 0x23, 0x26)));

        // Tab-row is 25px high: [Static log] [Dynamic tabs] [Add new tab button]
        nk_layout_row_begin(Context, NK_STATIC, 25, 1 + Tabcount + !!(Tabcount < Maxtabs));
        {
            // Unfiltered log, and default tab.
            nk_layout_row_push(Context, 50);
            if (nk_button_label(Context, "LOG"))
                Currenttab = Maxtabs;

            // Dynamic tabs, unordered.
            for (uint32_t i = 0; i < Maxtabs; ++i)
            {
                // Uninitialized slot.
                if (Titles[i].empty()) continue;

                // Notification for new lines displayed in the tab.
                auto Title = Unreadcounts[i] == 0 ? ""s : va("[%-2d]", Unreadcounts[i]);
                Title += Titles[i];

                // Save the area before adding the button.
                nk_layout_row_push(Context, 100);
                const auto Area = nk_widget_bounds(Context);

                // Highlight the currently selected tab.
                if (i != Currenttab) Currenttab = nk_button_label(Context, Title.c_str()) ? i : Currenttab;
                else
                {
                    nk_style_push_style_item(Context, &Context->style.button.normal, Context->style.button.active);
                    Currenttab = nk_button_label(Context, Title.c_str()) ? i : Currenttab;
                    nk_style_pop_style_item(Context);
                }

                // Right-click on the tab.
                if (nk_contextual_begin(Context, 0, nk_vec2(100, 35 * 1 /* Dropdown count */), Area))
                {
                    nk_layout_row_dynamic(Context, 25, 1);
                    if (nk_contextual_item_label(Context, "Close", NK_TEXT_CENTERED))
                    {
                        Tabcount--;
                        Titles[i].clear();
                        if (i == Currenttab)
                            Currenttab = Maxtabs;
                    }

                    nk_contextual_end(Context);
                }
            }

            // Add tabs if we have unused slots.
            if (Tabcount < Maxtabs)
            {
                static char Input[2][64]; static int Length[2];
                const auto Area = nk_widget_bounds(Context);
                static bool Showpopup = false;

                // Fixed size.
                nk_layout_row_push(Context, 50);
                if (nk_button_symbol(Context, NK_SYMBOL_PLUS)) Showpopup = true;

                // Latched value.
                if (Showpopup)
                {
                    nk_style_push_vec2(Context, &Context->style.window.spacing, nk_vec2(0, 5));
                    if (nk_popup_begin(Context, NK_POPUP_DYNAMIC, "Add a new tab",
                                       NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE, nk_rect(Area.x - 20, 25, 300, 300)))
                    {
                        // Two widgets per row.
                        nk_layout_row_template_begin(Context, 25);
                        nk_layout_row_template_push_static(Context, 50);
                        nk_layout_row_template_push_dynamic(Context);
                        nk_layout_row_template_end(Context);
                        {
                            nk_label(Context, "Name:", NK_TEXT_LEFT);
                            nk_edit_string(Context, NK_EDIT_SIMPLE, Input[0], &Length[0], 64, nk_filter_default);

                            nk_label(Context, "Filter:", NK_TEXT_LEFT);
                            nk_edit_string(Context, NK_EDIT_SIMPLE, Input[1], &Length[1], 64, nk_filter_default);
                        }

                        // Full-row widget.
                        nk_layout_row_dynamic(Context, 25, 1);
                        if (Length[0])
                        {
                            if (nk_button_label(Context, "Submit"))
                            {
                                Input[0][Length[0]] = '\0';
                                Input[1][Length[1]] = '\0';
                                Length[0] = 0;
                                Length[1] = 0;

                                // Find an unused slot.
                                for (uint32_t i = 0; i < Maxtabs; ++i)
                                {
                                    // Already initialized slot.
                                    if (!Titles[i].empty()) continue;
                                    Filters[i] = Input[1];
                                    Titles[i] = Input[0];
                                    Unreadcounts[i] = 0;
                                    Tabcount++;
                                    break;
                                }

                                nk_popup_close(Context);
                                Showpopup = false;
                            }
                        }
                        else
                        {
                            // Inactive button.
                            const auto Button = Context->style.button;
                            Context->style.button.normal = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.active = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.hover = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.text_background = nk_rgb(60, 60, 60);
                            Context->style.button.border_color = nk_rgb(60, 60, 60);
                            Context->style.button.text_normal = nk_rgb(60, 60, 60);
                            Context->style.button.text_active = nk_rgb(60, 60, 60);
                            Context->style.button.text_hover = nk_rgb(60, 60, 60);
                            nk_button_label(Context, "Submit");
                            Context->style.button = Button;
                        }
                    }
                    else Showpopup = false;
                    nk_style_pop_vec2(Context);
                    nk_popup_end(Context);
                }
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_style_item(Context);
        nk_style_pop_style_item(Context);
    }
    void Createlog(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_color(Context, &Context->style.window.background, nk_rgb(0x29, 0x26, 0x29));
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));

        // Sized between 15% and 60% of the window.
        nk_layout_row_begin(Context, NK_STATIC, Region.h * (isExtended ? 0.6f : 0.15f), 1);
        {
            nk_layout_row_push(Context, Region.w);
            if (nk_group_begin(Context, "Console.Log", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
            {
                uint32_t Scrolloffset = 0;
                nk_layout_row_dynamic(Context, 20, 1);
                for (const auto &[String, Color] : Rawdata)
                {
                    // No filtering.
                    if(Currenttab == Maxtabs || std::strstr(String.c_str(), Filters[Currenttab].c_str()))
                    {
                        nk_label_colored(Context, String.c_str(), NK_TEXT_LEFT, Color);
                        Scrolloffset += 20;
                    }
                }

                const auto Y = std::max(0.0f, Scrolloffset - (Region.h * (isExtended ? 0.6f : 0.15f)) + 5);
                nk_group_set_scroll(Context, "Console.Log", 0, uint32_t(std::round(Y)));
            }
            nk_group_end(Context);
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }
    void Createinput(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));

        nk_layout_row_dynamic(Context, 25, 1);
        {
            // Auto-focus.
            static int Inputlength;
            nk_edit_focus(Context, 0);
            auto Active = nk_edit_string(Context, NK_EDIT_SELECTABLE | NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE,
                                         Inputstring.data(), &Inputlength, Inputstring.size(), nk_filter_default);

            // On enter pressed.
            if (Active & NK_EDIT_COMMITED)
            {
                if (Inputlength)
                {
                    Inputstring.data()[Inputlength] = 0;
                    Rawdata.push_back({ va("> %s", Inputstring.data()), nk_rgb(0xD6, 0xB7, 0x49) });
                    Inputlength = 0;

                    // TODO(tcn): Do something fun with the input.
                }
            }
        }

        // Restore the context.
        nk_style_pop_style_item(Context);
    }

    void onRender(nk_context *Context, struct nk_vec2 Windowsize)
    {
        Region.w = Windowsize.x - Region.x * 2;
        Region.h = Windowsize.y - Region.y;

        if (nk_begin(Context, "Console", Region, NK_WINDOW_NO_SCROLLBAR))
        {
            Context->style.window.spacing = nk_vec2(0, 0);
            Context->style.window.padding = nk_vec2(0, 0);
            Context->style.button.rounding = 0;

            Createtabs(Context);
            Createlog(Context);
            Createinput(Context);
        }
        nk_end(Context);
    }
};


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
        if (nk_gdi_handle_event(wnd, msg, wparam, lparam)) return 0;
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
    auto Defaultfont = nk_gdifont_create("Arial", 14);
    auto Context = nk_gdi_init(Defaultfont, GetDC((HWND)Global.Windowhandle), 0, 0);

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
            const auto Keystate = GetAsyncKeyState(VK_F1);
            const auto Keydown = Keystate & (1 << 31);
            const auto Newkey = Keystate & (1 << 0);
            if (Newkey && Keydown)
            {
                Debugprint("Toggle key down");

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

            // Clear the frame-buffer to white (chroma-keyed to transparent).
            Context->style.window.fixed_background = nk_style_item_color(nk_rgb(0xFF, 0xFF, 0xFF));
            Console.onRender(Context, nk_vec2(Width, Height));
            nk_gdi_render(nk_rgb(0xFF, 0xFF, 0xFF));

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
