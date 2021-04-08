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
    static std::wstring Currentfilter{};
    static Ringbuffer_t<Logline_t, Logsize> Consolelog{};

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
        for (const auto Results = Tokenizestring(Message, "\r\n"); const auto &String : Results)
            if (!String.empty())
                Consolelog.push_back(Logline_t{ Encoding::toWide(String), Colour });
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
    void execCommandline(const std::string &Commandline, bool logCommand)
    {
        static const std::regex rxCommands("(\"[^\"]+\"|[^\\s\"]+)", std::regex_constants::optimize);
        auto It = std::sregex_iterator(Commandline.cbegin(), Commandline.cend(), rxCommands);
        const auto Size = std::distance(It, std::sregex_iterator());

        // Why would you do this?
        if (Size == 0) return;

        // Format as a C-array with the last pointer being null.
        std::vector<std::string> Heapstorage; Heapstorage.reserve(Size);
        const auto Arguments = (const char **)alloca((Size + 1) * sizeof(char *));
        for (ptrdiff_t i = 0; i < Size; ++i)
        {
            // Hackery because STL regex doesn't support lookahead/behind.
            std::string Temp((It++)->str());
            if (Temp.back() == '\"') Temp.pop_back();
            if (Temp.front() == '\"') Temp.erase(0, 1);
            Arguments[i] = Heapstorage.emplace_back(std::move(Temp)).c_str();
        }
        Arguments[Size] = NULL;

        // Find the function we are interested in.
        const auto Callback = std::find_if(std::execution::par_unseq, Functions.begin(), Functions.end(),
                              [&](const auto &Pair) { return 0 == _strnicmp(Pair.first.c_str(), Arguments[0], Pair.first.size()); });

        // Print the command to the console and evaluate.
        if (logCommand)
        {
            addConsolemessage("> "s + Commandline.data(), Color_t(0xD6, 0xB7, 0x49));
        }
        if (Callback != Functions.end())
        {
            Callback->second(Size - 1, Arguments + 1);
        }
    }

    // Add a new command to the internal list.
    void addConsolecommand(std::string_view Name, Callback_t Callback)
    {
        // Check if we already have this command.
        if (std::any_of(std::execution::par_unseq, Functions.begin(), Functions.end(),
                        [&](const auto &Pair) { return 0 == _strnicmp(Pair.first.c_str(), Name.data(), Pair.first.size()); }))
            return;

        Functions.emplace_back(Name, Callback);
    }
    #pragma endregion

    // API exports.
    extern "C"  EXPORT_ATTR void __cdecl addConsolecommand(const char *Command, const void *Callback)
    {
        if (!Command || !Callback) return;
        addConsolecommand(std::string_view(Command), Callback_t(Callback));
    }
    static std::string __cdecl execCommand(JSON::Value_t &&Request)
    {
        const auto Commandline = Request.value<std::string>("Commandline");
        execCommandline(Commandline, false);
        return "{}";
    }
    static std::string __cdecl printLine(JSON::Value_t &&Request)
    {
        const auto Color = std::max(Request.value<uint32_t>("Color"), Request.value<uint32_t>("Colour"));
        const auto Message = Request.value<std::string>("Message");

        addConsolemessage(Message, Color);
        return "{}";
    }

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
            for (auto Items = Enumerate(Functions, 1); const auto &[Index, Pair] : Items)
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

        Backend::API::addEndpoint("Console::Exec", execCommand);
        Backend::API::addEndpoint("Console::Print", printLine);
    }

    // Provide a C-API for external code.
    namespace API
    {
        // UTF8 escaped ASCII strings.
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Length, unsigned int Colour)
        {
            assert(String);
            Console::addConsolemessage({String, Length }, uint32_t(Colour));
        }
    }
}
