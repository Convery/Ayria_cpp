/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-18
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    // Stolen from https://source.winehq.org/git/wine.git/blob/HEAD:/dlls/shcore/main.c
    wchar_t **__stdcall CommandLineToArgvW_wine(const wchar_t *cmdline, int *numargs);

    // Console output.
    #pragma region Consoleoutput
    static Spinlock Writelock;
    constexpr size_t Logsize = 256;
    static std::array<Logline_t, Logsize> Rawbuffer{};
    static nonstd::ring_span<Logline_t> Consolelog
    { Rawbuffer.data(), Rawbuffer.data() + Logsize, Rawbuffer.data(), Logsize };
    static std::wstring Currentfilter{};

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::wstring &Message, COLORREF Colour)
    {
        // Scan for keywords to detect a colour if not specified.
        if (Colour == 0)
        {
            Colour = [&]()
            {
                // Common keywords.
                if (std::wcsstr(Message.c_str(), L"[E]") || std::wcsstr(Message.c_str(), L"rror")) return 0xBE282A;
                if (std::wcsstr(Message.c_str(), L"[W]") || std::wcsstr(Message.c_str(), L"arning")) return 0xBEC02A;

                // Ayria default.
                if (std::wcsstr(Message.c_str(), L"[I]")) return 0x218FBD;
                if (std::wcsstr(Message.c_str(), L"[D]")) return 0x7F963E;
                if (std::wcsstr(Message.c_str(), L"[>]")) return 0x7F963E;

                // Default.
                return 0x315571;
            }();
        }

        // Safety per parse, rather than per line.
        const std::scoped_lock _(Writelock);

        // Split by newline.
        std::wstring_view Input(Message);
        while (!Input.empty())
        {
            if (const auto Pos = Input.find(L'\n'); Pos != std::string_view::npos)
            {
                if (Pos != 0)
                {
                    Consolelog.push_back({ {Input.data(), Pos}, Colour });
                }
                Input.remove_prefix(Pos + 1);
                continue;
            }

            Consolelog.push_back({ Input.data(), Colour });
            break;
        }
    }

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::wstring_view Filter)
    {
        std::vector<Logline_t> Result;
        Result.reserve(std::clamp(Count, size_t(1), Logsize));

        // Safety per fetch, rather than per line.
        const std::scoped_lock _(Writelock);

        std::for_each(Consolelog.rbegin(), Consolelog.rend(), [&](const auto &Item)
        {
            if (Count)
            {
                if (std::wcsstr(Item.first.c_str(), Filter.data()))
                {
                    Result.push_back(Item);
                    Count--;
                }
            }
        });

        std::reverse(Result.begin(), Result.end());
        return Result;
    }

    // Track the current filter.
    std::wstring_view getFilter()
    {
        return Currentfilter;
    }

    #pragma endregion

    // Console input.
    #pragma region Consoleinput
    static std::vector<std::pair<std::wstring, Callback_t>> Functions;

    // Evaluate the string, optionally add to the history.
    void execCommandline(std::wstring Commandline, bool logCommand)
    {
        int Argc{};

        // Remove line-break characters.
        while (Commandline.back() == L'\r' || Commandline.back() == L'\n')
            Commandline.pop_back();

        // Print the command to the console.
        if (logCommand)
        {
            addConsolemessage(L"> "s + Commandline, Color_t(0xD6, 0xB7, 0x49));
        }

        // Parse the input using the same rules as command-lines.
        if (const auto Argv = CommandLineToArgvW_wine(Commandline.c_str(), &Argc))
        {
            std::wstring Name(Argv[0]);

            // Check if we have this command.
            const auto Callback = std::find_if(std::execution::par_unseq, Functions.begin(), Functions.end(), [&](const auto &Pair)
            {
                if (Pair.first.size() != Name.size()) return false;
                for (size_t i = 0; i < Pair.first.size(); i++)
                    if (std::toupper(Pair.first[i]) != std::toupper(Name[i]))
                        return false;
                return true;
            });

            if (Callback != Functions.end())
            {
                Callback->second(Argc, Argv);
            }

            LocalFree(Argv);
        }
    }

    // Add a new command to the internal list.
    void addConsolecommand(std::wstring_view Name, Callback_t Callback)
    {
        // Check if we already have this command.
        if (std::any_of(std::execution::par_unseq, Functions.begin(), Functions.end(), [&](const auto &Pair)
        {
            if (Pair.first.size() != Name.size()) return false;
            for (size_t i = 0; i < Pair.first.size(); i++)
                if (std::toupper(Pair.first[i]) != std::toupper(Name[i]))
                    return false;
            return true;
        })) return;

        Functions.emplace_back(Name, Callback);
    }

    #pragma endregion

    // Add common commands.
    void Initializebackend()
    {
        static bool Initialized{};
        if (Initialized) return;
        Initialized = true;

        static const auto Quit = [](int, wchar_t **)
        {
            std::exit(0);
        };
        addConsolecommand(L"Quit", Quit);
        addConsolecommand(L"Exit", Quit);

        static const auto List = [](int, wchar_t **)
        {
            std::wstring Commands; Commands.reserve(16 * Functions.size());
            for (const auto &[Index, Pair] : Enumerate(Functions, 1))
            {
                Commands += L"    ";
                Commands += Pair.first;
                if (Index % 4 == 0) Commands += L"\n";
            }

            addConsolemessage(L"Available commands:", 0x218FBD);
            addConsolemessage(Commands, 0x315571);
        };
        addConsolecommand(L"List", List);
        addConsolecommand(L"Help", List);

        static const auto Changefilter = [](int ArgC, wchar_t **ArgV)
        {
            if (ArgC == 1) Currentfilter = L"";
            else Currentfilter = ArgV[1];
        };
        addConsolecommand(L"setFilter", Changefilter);
        addConsolecommand(L"Filter", Changefilter);
    }

    // Provide a C-API for external code.
    namespace API
    {
        // ASCII or UTF8 strings.
        // using Callback_t = void(__cdecl *)(int Argc, wchar_t **Argv);
        extern "C" EXPORT_ATTR void __cdecl addConsolecommand(const void *Name, unsigned int Length, const void *Callback)
        {
            assert(Name); assert(Callback);
            Console::addConsolecommand(Encoding::toWide({ (char8_t *)Name, Length }), (Callback_t)Callback);
        }
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const void *String, unsigned int Length, unsigned int Colour)
        {
            assert(String);
            Console::addConsolemessage(Encoding::toWide({ (char8_t *)String, Length }), Colour);
        }
        extern "C" EXPORT_ATTR void __cdecl execCommandline(const void *String, unsigned int Length)
        {
            assert(String);
            Console::execCommandline(Encoding::toWide({ (char8_t *)String, Length }), false);
        }
    }

    #pragma region Thirdparty
    // Stolen from https://source.winehq.org/git/wine.git/blob/HEAD:/dlls/shcore/main.c
    wchar_t** __stdcall CommandLineToArgvW_wine(const wchar_t *cmdline, int *numargs)
    {
        int qcount, bcount;
        const wchar_t *s;
        wchar_t **argv;
        DWORD argc;
        wchar_t *d;

        if (!numargs)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        if (*cmdline == 0)
        {
            return NULL;
        }

        /* --- First count the arguments */
        argc = 1;
        s = cmdline;
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
        argv = (wchar_t **)LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(wchar_t *) + (lstrlenW(cmdline) + 1) * sizeof(wchar_t));
        if (!argv)
            return NULL;

        /* --- Then split and copy the arguments */
        argv[0] = d = lstrcpyW((wchar_t *)(argv + argc + 1), cmdline);
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
                do {
                    s++;
                } while (*s == ' ' || *s == '\t');
                if (*s)
                    argv[argc++] = d;
            }
            else if (*s=='\\')
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
}
