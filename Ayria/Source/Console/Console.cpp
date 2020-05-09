/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include "../Global.hpp"

// Stolen from https://github.com/futurist/CommandLineToArgvA
LPSTR *WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int *numargs);

struct Functioncallback_t
{
    std::string Friendlyname;
    void(__cdecl *Callback)(int Argc, const char **Argv);
};

namespace Console
{
    void *Overlaywindowhandle;
    struct nk_rect Gamearea;
    void *Gamewindowhandle;
    bool isExtended;
    bool isVisible;

    uint32_t Tabcount{};
    constexpr size_t Maxtabs = 7;
    uint32_t Currenttab{ Maxtabs };
    std::array<std::string, Maxtabs> Titles{};
    std::array<std::string, Maxtabs> Filters{};
    std::array<uint32_t, Maxtabs> Unreadcounts{};
    std::unordered_map<std::string, Functioncallback_t> Functions;

    // User input.
    std::array<char, 512> Inputstring;

    // Rawdata should not be directly modified.
    std::array<std::pair<std::string, struct nk_color>, 256> Rawdata{};
    nonstd::ring_span<std::pair<std::string, struct nk_color>> Messages
    { Rawdata.data(), Rawdata.data() + Rawdata.size(), Rawdata.data(), Rawdata.size() };

    // Subsections.
    void addTab(nk_context *Context)
    {
        static char Input[2][64]; static int Length[2];
        static bool Showpopup = false;

        // Fixed size.
        nk_layout_row_push(Context, 50);
        if (nk_button_symbol(Context, NK_SYMBOL_PLUS)) Showpopup = true;

        // Latched value.
        if (Showpopup)
        {
            nk_style_push_vec2(Context, &Context->style.window.spacing, nk_vec2(0, 5));
            if (nk_popup_begin(Context, NK_POPUP_DYNAMIC, "Add a new tab",
                NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE,
                nk_rect(Gamearea.w / 2 - 150, Gamearea.h * (isExtended ? 0.6f : 0.15f) * 0.5, 300, 300)))
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
    void Create_tabrow(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_color(Context, &Context->style.button.text_normal, nk_color(nk_rgb(0x61, 0xcf, 0xee)));
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
            if (Tabcount < Maxtabs) addTab(Context);
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_style_item(Context);
        nk_style_pop_style_item(Context);
        nk_style_pop_color(Context);
    }
    void Create_logarea(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_color(Context, &Context->style.window.background, nk_rgb(0x27, 0x26, 0x23));
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));

        // Sized between 15% and 60% of the window.
        nk_layout_row_begin(Context, NK_STATIC, Gamearea.h * (isExtended ? 0.6f : 0.15f), 1);
        {
            nk_layout_row_push(Context, Gamearea.w - 40);
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

                const auto Y = std::max(0.0f, Scrolloffset - (Gamearea.h * (isExtended ? 0.6f : 0.15f)) + 5);
                nk_group_set_scroll(Context, "Console.Log", 0, uint32_t(std::round(Y)));
                nk_group_end(Context);
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }
    void Create_inputarea(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));
        nk_style_push_color(Context, &Context->style.edit.border_color, nk_rgb(0x32, 0x3A, 0x45));
        nk_style_push_float(Context, &Context->style.edit.border, 3);

        nk_layout_row_begin(Context, NK_STATIC, 25, 1);
        {
            nk_layout_row_push(Context, Gamearea.w - 40);

            // Auto-focus.
            static int Inputlength;
            nk_edit_focus(Context, 0);
            const auto Active = nk_edit_string(Context, NK_EDIT_SELECTABLE | NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE,
                Inputstring.data(), &Inputlength, (int)Inputstring.size(), [](const struct nk_text_edit *, nk_rune unicode) -> int
                {
                    return unicode != L'§' && unicode != L'½' && unicode != L'~';
                });

            // On enter pressed.
            if (Active & NK_EDIT_COMMITED)
            {
                if (Inputlength)
                {
                    Inputstring[Inputlength] = 0;
                    Inputlength = 0;
                    int Argc;

                    // Log the input before executing any functions.
                    Messages.push_back({ "> "s + Inputstring.data(), nk_rgb(0xD6, 0xB7, 0x49) });

                    // Parse the input using the same rules as command-lines.
                    if (const auto Argv = CommandLineToArgvA_wine(Inputstring.data(), &Argc))
                    {
                        std::string Functionname = Argv[0];
                        std::transform(Functionname.begin(), Functionname.end(), Functionname.begin(), [](auto a) { return (char)std::tolower(a); });
                        if (const auto Callback = Functions.find(Functionname); Callback != Functions.end())
                        {
                            Callback->second.Callback(Argc, (const char **)Argv);
                        }

                        LocalFree(Argv);
                    }
                }
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_float(Context);
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }

    // Immediate mode, so do everything.
    void Consolewindow(nk_context *Context)
    {
        if (!isVisible) [[likely]] return;
        RECT Gamewindow, Overlaywindow;

        // Save the context-handle.
        if (!Overlaywindowhandle) Overlaywindowhandle = Context->userdata.ptr;

        // Follow the main window.
        GetWindowRect((HWND)Gamewindowhandle, &Gamewindow);
        GetWindowRect((HWND)Overlaywindowhandle, &Overlaywindow);
        const auto Offsetx = Gamewindow.left - Overlaywindow.left;
        const auto Offsety = Gamewindow.top - Overlaywindow.top;
        const auto Height = Gamewindow.bottom - Gamewindow.top;
        const auto Width = Branchless::min(
                            static_cast<long>(Gamewindow.right - Gamewindow.left - 40),
                            static_cast<long>(1440));

        // We only need part of the window, so crop if large to save resources.
        Gamearea = nk_recti(Offsetx + (Gamewindow.right - Gamewindow.left) / 2 - Width / 2,
            Offsety + 50, Width, Height);

        // Only impose limits when maxed out.
        if(Width != 1440)
            Graphics::include(Gamewindow.left, Gamewindow.top, Gamewindow.right, Gamewindow.bottom);
        else
        {
            Graphics::include( Gamewindow.left + Width / 2, Gamewindow.top, Gamewindow.right - Width / 2,
                Height <= 1024 ? Gamewindow.bottom : Gamewindow.top + Height * (isExtended ? 0.7f : 0.25f));
        }

        // Bound to the game-window.
        if (nk_begin(Context, "Console", Gamearea, NK_WINDOW_NO_SCROLLBAR))
        {
            // Ensure a clean state.
            Context->style.window.spacing = nk_vec2(0, 0);
            Context->style.window.padding = nk_vec2(0, 0);
            Context->style.button.rounding = 0;
            Context->current->bounds = Gamearea;

            // Build the console.
            Create_tabrow(Context);
            Create_logarea(Context);
            Create_inputarea(Context);
            // TODO(tcn): Suggestion area?
        }
        nk_end(Context);
    }

    // Internal callbacks.
    void onStartup()
    {
        isExtended = isVisible = false;
        Graphics::Registerwindow(Consolewindow);
    }
    void onFrame()
    {
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
                    isExtended ^= true;
                    Graphics::spoil();
                }
                else
                {
                    if (isVisible)
                    {
                        if (Gamewindowhandle)
                        {
                            if(Overlaywindowhandle == GetForegroundWindow())
                                SetForegroundWindow((HWND)Gamewindowhandle);
                            EnableWindow((HWND)Gamewindowhandle, TRUE);
                        }
                    }
                    else
                    {
                        EnumWindows([](HWND Handle, LPARAM) -> BOOL
                        {
                            DWORD ProcessID;
                            const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

                            if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
                            {
                                RECT Gamewindow{};
                                if (GetWindowRect(Handle, &Gamewindow))
                                {
                                    if (Gamewindow.top != 0 || Gamewindow.left != 0)
                                    {
                                        Gamewindowhandle = Handle;
                                        return FALSE;
                                    }
                                }
                            }

                            return TRUE;
                        }, NULL);

                        if (Gamewindowhandle) EnableWindow((HWND)Gamewindowhandle, FALSE);
                    }

                    isVisible ^= true;
                    Graphics::spoil();
                }
            }
        }
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Colour)
        {
            // Scan for keywords to select the colour.
            if (Colour == 0)
            {
                do
                {
                    // Ayria default.
                    if (std::strstr(String, "[E]")) { Colour = 0xBE282A; break; }
                    if (std::strstr(String, "[W]")) { Colour = 0xBEC02A; break; }
                    if (std::strstr(String, "[I]")) { Colour = 0x218FBD; break; }
                    if (std::strstr(String, "[D]")) { Colour = 0x7F963E; break; }
                    if (std::strstr(String, "[>]")) { Colour = 0x7F963E; break; }

                    // Take a stab in the dark.
                    if (std::strstr(String, "rror")) { Colour = 0xBE282A; break; }
                    if (std::strstr(String, "arning")) { Colour = 0xBEC02A; break; }

                    // Default.
                    Colour = 0x315571;
                } while (false);
            }

            Console::Messages.push_back({ String, nk_rgb(
                Colour >> 16 & 0xFF,
                Colour >> 8 & 0xFF,
                Colour >> 0 & 0xFF
                )});
        }
        EXPORT_ATTR void __cdecl addConsolefunction(const char *Name, void *Callback)
        {
            std::string Functionname = Name;
            std::transform(Functionname.begin(), Functionname.end(), Functionname.begin(), [](auto a) {return (char)std::tolower(a); });
            Console::Functions[Functionname].Callback = static_cast<void(__cdecl *)(int, const char **)>(Callback);
            Console::Functions[Functionname].Friendlyname = Name;
        }
    }
}

#pragma region Thirdparty
// Stolen from https://github.com/futurist/CommandLineToArgvA
LPSTR *WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int *numargs)
{
    LPSTR d;
    int bcount;

    if (!numargs || *lpCmdline == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* --- First count the arguments */
    DWORD argc = 1;
    LPSTR s = lpCmdline;
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
    int qcount = bcount = 0;
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
    LPSTR* argv = (LPSTR*)LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (strlen(lpCmdline) + 1) * sizeof(char));
    if (!argv)
        return NULL;
    LPSTR cmdline = (LPSTR)(argv + argc + 1);
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
#pragma endregion
