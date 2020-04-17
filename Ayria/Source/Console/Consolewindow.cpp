/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/


#include "../Global.hpp"
#define NK_IMPLEMENTATION
#include "Nuklear_GDI.hpp"
#include <nonstd/ring_span.hpp>

namespace Console
{
    // Stolen from https://github.com/futurist/CommandLineToArgvA
    LPSTR *WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int *numargs);
    using Consolecallback_t = void(__cdecl *)(int Argc, const char **Argv);
    constexpr size_t Maxtabs = 6;

    // Global input.
    std::array<char, 512> Inputstring;

    // Console tabs.
    uint32_t Tabcount{};
    uint32_t Currenttab{ Maxtabs };
    std::array<std::string, Maxtabs> Titles{};
    std::array<std::string, Maxtabs> Filters{};
    std::array<uint32_t, Maxtabs> Unreadcounts{};

    // Rawdata should not be directly modified.
    std::array<std::pair<std::string, struct nk_color>, 256> Rawdata{};
    nonstd::ring_span<std::pair<std::string, struct nk_color>> Messages
    { Rawdata.data(), Rawdata.data() + Rawdata.size(), Rawdata.data(), Rawdata.size() };

    // Functionality.
    std::unordered_map<std::string, Consolecallback_t> Functions;

    // Window properties.
    struct nk_rect Region { 20, 50 };

    struct
    {
        void *Gamewindowhandle;
        nk_context *Context;
        void *Windowhandle;
        bool isExtended;
        bool isVisible;
    } Global{};

    // Create a centred window chroma-keyed on 0xFFFFFF.
    void *Createwindow()
    {
        // Register the window.
        WNDCLASSEXA Windowclass{};
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.style = CS_VREDRAW | CS_OWNDC;
        Windowclass.lpszClassName = "Ayria Console";
        Windowclass.hInstance = GetModuleHandleA(NULL);
        Windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        Windowclass.lpfnWndProc = [](HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (NK_GDI::onEvent(wnd, msg, wparam, lparam)) return 0;
            return DefWindowProcW(wnd, msg, wparam, lparam);
        };
        if (NULL == RegisterClassExA(&Windowclass)) return nullptr;

        if (auto Windowhandle = CreateWindowExA(WS_EX_LAYERED, "Ayria Console", NULL, WS_POPUP,
            NULL, NULL, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL))
        {
            // Use a pixel-value of [0xFF, 0xFF, 0xFF] to mean transparent rather than Alpha.
            // Because using Alpha is slow and we should not use pure white anyway.
            SetLayeredWindowAttributes(Windowhandle, 0xFFFFFF, 0, LWA_COLORKEY);
            return Windowhandle;
        }

        return nullptr;
    }

    // Setup the system.
    static void Initialize()
    {
        // As we are single-threaded (in release), boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Windows tracks message-queues by thread ID, so we need to create the window
        // from this new thread to prevent issues. Took like 8 hours of hackery to find that..
        Global.Windowhandle = Createwindow();

        Global.Context = NK_GDI::Initialize(GetDC((HWND)Global.Windowhandle));

        // TODO(tcn): More init!
    }

    // Create the layout every frame.
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
        nk_layout_row_begin(Context, NK_STATIC, Region.h * (Global.isExtended ? 0.6f : 0.15f), 1);
        {
            nk_layout_row_push(Context, Region.w);
            if (nk_group_begin(Context, "Console.Log", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
            {
                uint32_t Scrolloffset = 0;
                nk_layout_row_dynamic(Context, 20, 1);
                for (const auto &[String, Color] : Messages)
                {
                    // No filtering for maxtabs.
                    if (Currenttab == Maxtabs || std::strstr(String.c_str(), Filters[Currenttab].c_str()))
                    {
                        nk_label_colored(Context, String.c_str(), NK_TEXT_LEFT, Color);
                        Scrolloffset += 20;
                    }
                }

                const auto Y = std::max(0.0f, Scrolloffset - (Region.h * (Global.isExtended ? 0.6f : 0.15f)) + 5);
                nk_group_set_scroll(Context, "Console.Log", 0, uint32_t(std::round(Y)));
                nk_group_end(Context);
            }
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
                    Inputlength = 0;
                    int Argc;

                    // Log the input before executing any functions.
                    Messages.push_back({ "> "s + Inputstring.data(), nk_rgb(0xD6, 0xB7, 0x49) });

                    // Parse the input using the same rules as command-lines.
                    if (const auto Argv = CommandLineToArgvA_wine(Inputstring.data(), &Argc))
                    {
                        std::string Functionname = Argv[0];
                        std::transform(Functionname.begin(), Functionname.end(), Functionname.begin(), [](auto a) { return (char)std::tolower(a); });
                        if (auto Callback = Functions.find(Functionname); Callback != Functions.end())
                        {
                            Callback->second(Argc, (const char **)Argv);
                        }

                        LocalFree(Argv);
                    }

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

    static bool Initialized = false;
    static bool Dirtyframe = true;
    void onFrame()
    {
        if (!Initialized) Initialize();
        Initialized = true;

        // Track the frame-time, should be less than 33ms.
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        // Update the context for smooth scrolling.
        Global.Context->delta_time_seconds = Deltatime;
        Lastframe = Thisframe;

        // Process window-messages.
        nk_input_begin(Global.Context);
        {
            MSG Message;
            while (PeekMessageW(&Message, (HWND)Global.Windowhandle, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
                Dirtyframe = true;
            }
        }
        nk_input_end(Global.Context);

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
                    Global.isExtended ^= true;
                    Dirtyframe = true;
                }
                else
                {
                    // The first time we display the console, capture the game-handle.
                    if (!Global.Gamewindowhandle)
                    {
                        EnumWindows([](HWND Handle, LPARAM) -> BOOL
                        {
                            DWORD ProcessID;
                            auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

                            if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
                            {
                                RECT Gamewindow{};
                                if (GetWindowRect(Handle, &Gamewindow))
                                {
                                    if (Gamewindow.top >= 0 && Gamewindow.left >= 0)
                                    {
                                        Global.Gamewindowhandle = Handle;
                                        return FALSE;
                                    }
                                }
                            }

                            return TRUE;
                        }, NULL);
                    }

                    if (Global.isVisible)
                    {
                        ShowWindowAsync((HWND)Global.Windowhandle, SW_HIDE);
                        if (Global.Gamewindowhandle) SetForegroundWindow((HWND)Global.Gamewindowhandle);
                        if (Global.Gamewindowhandle) EnableWindow((HWND)Global.Gamewindowhandle, TRUE);
                    }
                    else
                    {
                        if (Global.Gamewindowhandle) EnableWindow((HWND)Global.Gamewindowhandle, FALSE);
                        SetForegroundWindow((HWND)Global.Windowhandle);
                        SetActiveWindow((HWND)Global.Windowhandle);
                        SetFocus((HWND)Global.Windowhandle);
                    }

                    Global.isVisible ^= true;
                    Dirtyframe = true;
                }
            }
        }

        // Only redraw when dirty.
        if (Dirtyframe)
        {
            RECT Gamewindow{};
            Dirtyframe = false;

            // Follow the main window.
            GetWindowRect((HWND)Global.Gamewindowhandle, &Gamewindow);
            const auto Height = Gamewindow.bottom - Gamewindow.top;
            const auto Width = Gamewindow.right - Gamewindow.left;
            SetWindowPos((HWND)Global.Windowhandle, 0, Gamewindow.left, Gamewindow.top, Width, Height, NULL);

            // Set the default background to white (chroma-keyed to transparent).
            Global.Context->style.window.fixed_background = nk_style_item_color(nk_rgb(0xFF, 0xFF, 0xFF));
            onRender(Global.Context, nk_vec2i(Width, Height));
            NK_GDI::Render();

            // Ensure that the window is marked as visible.
            ShowWindow((HWND)Global.Windowhandle, SW_SHOWNORMAL);
        }
    }

    // Stolen from https://github.com/futurist/CommandLineToArgvA
    LPSTR *WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int *numargs)
    {
        DWORD argc;
        LPSTR *argv;
        LPSTR s;
        LPSTR d;
        LPSTR cmdline;
        int qcount, bcount;

        if (!numargs || *lpCmdline == 0)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        /* --- First count the arguments */
        argc = 1;
        s = lpCmdline;
        /* The first argument, the executable path, follows special rules */
        if (*s == '"')
        {
            /* The executable path ends at the next quote, no matter what */
            s++;
            while (*s)
                if (*s++ == '"')
                    break;
        }
        else
        {
            /* The executable path ends at the next space, no matter what */
            while (*s && *s != ' ' && *s != '\t')
                s++;
        }
        /* skip to the first argument, if any */
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s)
            argc++;

        /* Analyze the remaining arguments */
        qcount = bcount = 0;
        while (*s)
        {
            if ((*s == ' ' || *s == '\t') && qcount == 0)
            {
                /* skip to the next argument and count it if any */
                while (*s == ' ' || *s == '\t')
                    s++;
                if (*s)
                    argc++;
                bcount = 0;
            }
            else if (*s == '\\')
            {
                /* '\', count them */
                bcount++;
                s++;
            }
            else if (*s == '"')
            {
                /* '"' */
                if ((bcount & 1) == 0)
                    qcount++; /* unescaped '"' */
                s++;
                bcount = 0;
                /* consecutive quotes, see comment in copying code below */
                while (*s == '"')
                {
                    qcount++;
                    s++;
                }
                qcount = qcount % 3;
                if (qcount == 2)
                    qcount = 0;
            }
            else
            {
                /* a regular character */
                bcount = 0;
                s++;
            }
        }

        /* Allocate in a single lump, the string array, and the strings that go
        * with it. This way the caller can make a single LocalFree() call to free
        * both, as per MSDN.
        */
        argv = (LPSTR *)LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (strlen(lpCmdline) + 1) * sizeof(char));
        if (!argv)
            return NULL;
        cmdline = (LPSTR)(argv + argc + 1);
        strcpy(cmdline, lpCmdline);

        /* --- Then split and copy the arguments */
        argv[0] = d = cmdline;
        argc = 1;
        /* The first argument, the executable path, follows special rules */
        if (*d == '"')
        {
            /* The executable path ends at the next quote, no matter what */
            s = d + 1;
            while (*s)
            {
                if (*s == '"')
                {
                    s++;
                    break;
                }
                *d++ = *s++;
            }
        }
        else
        {
            /* The executable path ends at the next space, no matter what */
            while (*d && *d != ' ' && *d != '\t')
                d++;
            s = d;
            if (*s)
                s++;
        }
        /* close the executable path */
        *d++ = 0;
        /* skip to the first argument and initialize it if any */
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
        {
            /* There are no parameters so we are all done */
            argv[argc] = NULL;
            *numargs = argc;
            return argv;
        }

        /* Split and copy the remaining arguments */
        argv[argc++] = d;
        qcount = bcount = 0;
        while (*s)
        {
            if ((*s == ' ' || *s == '\t') && qcount == 0)
            {
                /* close the argument */
                *d++ = 0;
                bcount = 0;

                /* skip to the next one and initialize it if any */
                do
                {
                    s++;
                }
                while (*s == ' ' || *s == '\t');
                if (*s)
                    argv[argc++] = d;
            }
            else if (*s == '\\')
            {
                *d++ = *s++;
                bcount++;
            }
            else if (*s == '"')
            {
                if ((bcount & 1) == 0)
                {
                    /* Preceded by an even number of '\', this is half that
                    * number of '\', plus a quote which we erase.
                    */
                    d -= bcount / 2;
                    qcount++;
                }
                else
                {
                    /* Preceded by an odd number of '\', this is half that
                    * number of '\' followed by a '"'
                    */
                    d = d - bcount / 2 - 1;
                    *d++ = '"';
                }
                s++;
                bcount = 0;
                /* Now count the number of consecutive quotes. Note that qcount
                * already takes into account the opening quote if any, as well as
                * the quote that lead us here.
                */
                while (*s == '"')
                {
                    if (++qcount == 3)
                    {
                        *d++ = '"';
                        qcount = 0;
                    }
                    s++;
                }
                if (qcount == 2)
                    qcount = 0;
            }
            else
            {
                /* a regular character */
                *d++ = *s++;
                bcount = 0;
            }
        }
        *d = '\0';
        argv[argc] = NULL;
        *numargs = argc;

        return argv;
    }
}
