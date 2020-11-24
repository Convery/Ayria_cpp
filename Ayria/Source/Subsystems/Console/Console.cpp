/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-18
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    // Console output.
    #pragma region Consoleoutput
    static Spinlock Writelock;
    constexpr size_t Logsize = 256;
    static std::array<Logline_t, Logsize> Rawbuffer{};
    static nonstd::ring_span<Logline_t> Consolelog
    { Rawbuffer.data(), Rawbuffer.data() + Logsize, Rawbuffer.data(), Logsize };
    static std::wstring Currentfilter{};

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::string &Message, Color_t Colour)
    {
        // Scan for keywords to detect a colour if not specified.
        if (Colour == 0)
        {
            Colour = [&]() -> uint32_t
            {
                // Common keywords.
                if (std::strstr(Message.c_str(), "[E]") || std::strstr(Message.c_str(), "rror")) return 0xBE282A;
                if (std::strstr(Message.c_str(), "[W]") || std::strstr(Message.c_str(), "arning")) return 0x2AC0BE;

                // Ayria default.
                if (std::strstr(Message.c_str(), "[I]")) return 0xBD8F21;
                if (std::strstr(Message.c_str(), "[D]")) return 0x3E967F;
                if (std::strstr(Message.c_str(), "[>]")) return 0x7F963E;

                // Default.
                return 0x315571;
            }();
        }

        // Safety per parse, rather than per line.
        const std::scoped_lock _(Writelock);

        // Split by newline.
        for (const auto &String : absl::StrSplit(Message, '\n'))
            if (!String.empty())
                Consolelog.push_back({ Encoding::toWide(String), Colour });
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
    static std::vector<std::pair<std::string, Callback_t>> Functions;

    // Evaluate the string, optionally add to the history.
    void execCommandline(std::string_view Commandline, bool logCommand)
    {
        // Why would you do this?
        if (Commandline.empty() || Commandline.front() == '\r' || Commandline.front() == '\n') [[unlikely]]
            return;

        // Remove line-break characters.
        while (Commandline.back() == '\r' || Commandline.back() == '\n')
            Commandline.remove_suffix(1);

        // Split into arguments.
        const absl::InlinedVector<std::string, 4> Split = absl::StrSplit(Commandline, absl::ByAnyChar("\" "));

        // Format as C-array.
        const auto Array = (const char **)alloca((Split.size() + 1) * sizeof(char *));
        for (size_t i = 0; i < Split.size(); i++)
            Array[i] = Split[i].c_str();

        // Find the function we are interested in.
        const auto Callback = std::find_if(std::execution::par_unseq, Functions.begin(), Functions.end(), [&](const auto &Pair)
        {
            if (Pair.first.size() != Split[0].size()) return false;
            for (size_t i = 0; i < Pair.first.size(); i++)
                if (std::toupper(Pair.first[i]) != std::toupper(Split[0][i]))
                    return false;
            return true;
        });

        // Print the command to the console and evaluate.
        if (logCommand)
        {
            addConsolemessage("> "s + Commandline.data(), Color_t(0xD6, 0xB7, 0x49));
        }
        if (Callback != Functions.end())
        {
            Callback->second(Split.size(), Array);
        }
    }

    // Add a new command to the internal list.
    void addConsolecommand(std::string_view Name, Callback_t Callback)
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

        static const auto Quit = [](int, const char **)
        {
            std::exit(0);
        };
        addConsolecommand("Quit", Quit);
        addConsolecommand("Exit", Quit);

        static const auto List = [](int, const char **)
        {
            std::string Commands; Commands.reserve(16 * Functions.size());
            for (const auto &[Index, Pair] : Enumerate(Functions, 1))
            {
                Commands += "    ";
                Commands += Pair.first;
                if (Index % 4 == 0) Commands += "\n";
            }

            addConsolemessage("Available commands:", 0xBD8F21U);
            addConsolemessage(Commands, 0x715531U);
        };
        addConsolecommand("List", List);
        addConsolecommand("Help", List);

        static const auto Changefilter = [](int ArgC, const char **ArgV)
        {
            if (ArgC == 1) Currentfilter = L"";
            else Currentfilter = Encoding::toWide(ArgV[1]);
        };
        addConsolecommand("setFilter", Changefilter);
        addConsolecommand("Filter", Changefilter);
    }

    // Provide a C-API for external code.
    namespace API
    {
        // UTF8 escaped ASCII strings.
        // using Callback_t = void(__cdecl *)(int Argc, const char **Argv);
        extern "C" EXPORT_ATTR void __cdecl addConsolecommand(const char *Name, unsigned int Length, const void *Callback)
        {
            assert(Name); assert(Callback);
            Console::addConsolecommand({ Name, Length }, (Callback_t)Callback);
        }
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Length, unsigned int Colour)
        {
            assert(String);
            Console::addConsolemessage({String, Length }, uint32_t(Colour));
        }
        extern "C" EXPORT_ATTR void __cdecl execCommandline(const char *String, unsigned int Length)
        {
            assert(String);
            Console::execCommandline({ String, Length }, false);
        }
    }
}
