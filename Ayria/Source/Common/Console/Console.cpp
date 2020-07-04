/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include <Global.hpp>

// Stolen from https://github.com/futurist/CommandLineToArgvA
LPSTR *WINAPI CommandLineToArgvA_wine(LPSTR lpCmdline, int *numargs);

struct Functioncallback_t
{
    std::string Friendlyname;
    void(__cdecl *Callback)(int Argc, const char **Argv);
};

namespace Console
{
    struct nk_rect Gamearea;
    void *Gamewindowhandle;
    bool isExtended;
    bool isVisible;

    Spinlock Writelock;

    uint32_t Tabcount{};
    constexpr size_t Maxtabs = 7;
    uint32_t Currenttab{ Maxtabs };
    std::array<std::string, Maxtabs> Titles{};
    std::array<std::string, Maxtabs> Filters{};
    std::array<uint32_t, Maxtabs> Unreadcounts{};
    std::unordered_map<std::string, Functioncallback_t> Functions;

    // Rawdata should not be directly modified.
    std::array<std::pair<std::shared_ptr<std::string>, struct nk_color>, 64> Rawdata{};
    nonstd::ring_span<std::pair<std::shared_ptr<std::string>, struct nk_color>> Messages
    { Rawdata.data(), Rawdata.data() + Rawdata.size(), Rawdata.data(), Rawdata.size() };

    // Exists for a frame.
    std::vector<std::pair<std::shared_ptr<std::string>, struct nk_color>> Messagecache;

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
                nk_rect(Gamearea.w / 2 - 150, Gamearea.h / 2 - 75, 300, 150)))
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
        nk_layout_row_begin(Context, NK_STATIC, Gamearea.h - 60, 1);
        {
            nk_layout_row_push(Context, Gamearea.w);
            if (nk_group_begin(Context, "Console.Log", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
            {
                nk_layout_row_dynamic(Context, 20, 1);

                auto Count = int32_t((Gamearea.h - 60) / 20);
                Messagecache.clear(); Messagecache.reserve(Count);

                std::for_each(Messages.rbegin(), Messages.rend(), [&](const auto &Item)
                {
                    if (Count-- > 0) Messagecache.push_back({ Item });
                });
                std::reverse(Messagecache.begin(), Messagecache.end());

                for (const auto &[String, Color] : Messagecache)
                {
                    // No filtering for maxtabs.
                    if (Currenttab == Maxtabs || std::strstr(String->c_str(), Filters[Currenttab].c_str()))
                    {
                        nk_label_colored(Context, String->c_str(), NK_TEXT_LEFT, Color);
                    }
                }
                nk_group_end(Context);
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }
    void Create_inputarea(nk_context *Context, HWND Windowhandle)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));
        nk_style_push_color(Context, &Context->style.edit.border_color, nk_rgb(0x32, 0x3A, 0x45));
        nk_style_push_float(Context, &Context->style.edit.border, 3);

        nk_layout_row_begin(Context, NK_STATIC, 30, 1);
        {
            nk_layout_row_push(Context, Gamearea.w);

            // Reset focus on every frame.
            const auto Flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
            nk_edit_focus(Context, Flags);

            static std::string Lastcommand;
            static std::array<char, 512> Inputstring;
            const auto State = nk_edit_string_zero_terminated(Context, Flags, Inputstring.data(), Inputstring.size(),
                [](const struct nk_text_edit *, nk_rune Unicode) -> int
                {
                    // Drop non-asci.
                    if (Unicode > 126) return false;

                    // Drop toggle-chars.
                    return Unicode != L'§' && Unicode != L'½' && Unicode != L'~' && Unicode != L'`';
                });

            // On enter pressed.
            if (State & NK_EDIT_COMMITED && Inputstring[0] != '\0')
            {
                int Argc;

                // Log the input before executing any functions.
                {
                    std::scoped_lock _(Writelock);
                    Messages.push_back({ std::make_shared<std::string>("> "s + Inputstring.data()), nk_rgb(0xD6, 0xB7, 0x49) });
                }
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

                // Save the buffer for later.
                Lastcommand = Inputstring.data();
                std::memset(Inputstring.data(), '\0', Inputstring.size());
            }

            // Visit previous command.
            if (nk_input_is_key_pressed(&Context->input, NK_KEY_UP))
            {
                std::memset(Inputstring.data(), '\0', Inputstring.size());
                std::memcpy(Inputstring.data(), Lastcommand.c_str(), Lastcommand.size());

                // Hackery to ensure that the cursor gets moved to the end of the input.
                PostMessageA(Windowhandle, WM_KEYDOWN, VK_END, NULL);
            }
            else if (nk_input_is_key_pressed(&Context->input, NK_KEY_DOWN))
            {
                std::memset(Inputstring.data(), '\0', Inputstring.size());
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_float(Context);
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }

    // Callbacks from Graphics.cpp
    bool onFrame(Graphics::Surface_t *This)
    {
        // Fixup for modern Windows having different behaviour than previous.
        static auto Lastpress{ GetTickCount64() };

        const auto Keydown = (GetAsyncKeyState(VK_OEM_5) & (1 << 31)) | (GetKeyState(VK_OEM_5) & (1 << 31));
        if (Keydown && GetTickCount64() > (Lastpress + 256))
        {
            Lastpress = GetTickCount64();

            // Shift modifier extends the log.
            if (GetAsyncKeyState(VK_SHIFT) & (1 << 31))
            {
                isExtended ^= true;
            }
            else
            {
                isVisible ^= true;

                // Hide the window.
                if (!isVisible)
                {
                    if (Gamewindowhandle)
                    {
                        EnableWindow((HWND)Gamewindowhandle, TRUE);
                        SetForegroundWindow((HWND)Gamewindowhandle);
                    }
                }
                else
                {
                    RECT Gamewindow{};
                    EnumWindows([](HWND Handle, LPARAM Previouswindow) -> BOOL
                    {
                        DWORD ProcessID;
                        const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

                        if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
                        {
                            RECT Gamewindow{};
                            if (GetWindowRect(Handle, &Gamewindow))
                            {
                                auto Previous = (RECT *)Previouswindow;
                                if (Gamewindow.right - Gamewindow.left > Previous->right - Previous->left ||
                                    Gamewindow.bottom - Gamewindow.top > Previous->bottom - Previous->top)
                                {
                                    Gamewindowhandle = Handle;
                                    *Previous = Gamewindow;
                                }
                            }
                        }

                        return TRUE;
                    }, (LPARAM)&Gamewindow);

                    // Let's not activate the console if the game isn't active.
                    if (GetForegroundWindow() == (HWND)Gamewindowhandle)
                    {
                        EnableWindow((HWND)Gamewindowhandle, FALSE);
                        SetForegroundWindow(This->Windowhandle);
                        SetFocus(This->Windowhandle);
                    }
                    else isVisible = false;

                    // Hackery to ensure that the cursor gets moved to the end of the input.
                    if (isVisible) PostMessageA(This->Windowhandle, WM_KEYDOWN, VK_END, NULL);
                }
            }

            return true;
        }

        return false;
    }
    bool onRender(Graphics::Surface_t *This)
    {
        if (!isVisible) [[likely]] return false;
        RECT Gamewindow;

        // Get the main window.
        GetWindowRect((HWND)Gamewindowhandle, &Gamewindow);
        const auto Width = Gamewindow.right - Gamewindow.left;
        const auto Widthcropped = Branchless::min(1440L, Width);
        const auto Height = (Gamewindow.bottom - Gamewindow.top) * (isExtended ? 0.7 : 0.3);

        // Limit the size to save resources (as we draw on the CPU).
        if (Width < 1440) SetWindowPos(This->Windowhandle, 0, Gamewindow.left, Gamewindow.top, Width, Height, 0);
        else SetWindowPos(This->Windowhandle, 0, Gamewindow.left + (Width - 1440) / 2, Gamewindow.top, Widthcropped, Height, 0);

        // Add offsets now to simplify maths later.
        Gamearea = nk_recti(20, 50, Widthcropped - 40, Height - 50);

        // Bound to the game-window.
        if (nk_begin(&This->Context, "Console", Gamearea, NK_WINDOW_NO_SCROLLBAR))
        {
            // Ensure a clean state.
            This->Context.style.window.spacing = nk_vec2(0, 0);
            This->Context.style.window.padding = nk_vec2(0, 0);
            This->Context.style.button.rounding = 0;
            This->Context.current->bounds = Gamearea;

            // Build the console.
            Create_tabrow(&This->Context);
            Create_logarea(&This->Context);
            Create_inputarea(&This->Context, This->Windowhandle);
            // TODO(tcn): Suggestion area?
        }
        nk_end(&This->Context);

        return true;
    }

    // Callback from Ayria-core.
    void onStartup()
    {
        isExtended = isVisible = false;
        Graphics::Createsurface("Ayria_console", onFrame, onRender);
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
                    // Common keywords.
                    if (std::strstr(String, "[E]") || std::strstr(String, "rror")) { Colour = 0xBE282A; break; }
                    if (std::strstr(String, "[W]") || std::strstr(String, "arning")) { Colour = 0xBEC02A; break; }

                    // Ayria default.
                    if (std::strstr(String, "[I]")) { Colour = 0x218FBD; break; }
                    if (std::strstr(String, "[D]")) { Colour = 0x7F963E; break; }
                    if (std::strstr(String, "[>]")) { Colour = 0x7F963E; break; }

                    // Default.
                    Colour = 0x315571;
                } while (false);
            }

            // Split messages containing newlines.
            std::string_view Input(String);
            while (Input.size())
            {
                if (const auto Pos = Input.find('\n'); Pos != Input.npos)
                {
                    if (Pos != 0)
                    {
                        std::scoped_lock _(Console::Writelock);
                        Console::Messages.push_back({ std::make_shared<std::string>(Input.data(), Pos),
                            nk_rgb(Colour >> 16 & 0xFF, Colour >> 8 & 0xFF, Colour >> 0 & 0xFF) });
                    }
                    Input.remove_prefix(Pos + 1);
                }
                else
                {
                    std::scoped_lock _(Console::Writelock);
                    Console::Messages.push_back({ std::make_shared<std::string>(Input.data(), Input.size()),
                        nk_rgb(Colour >> 16 & 0xFF, Colour >> 8 & 0xFF, Colour >> 0 & 0xFF) });
                    break;
                }
            }
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
