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
    static Ringbuffer<Logline_t, Logsize> Consolelog{};

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
        for (const auto &String : Tokenizestring_s(Message, '\n'))
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
    void execCommandline(std::string_view Commandline, bool logCommand)
    {
        // Why would you do this?
        if (Commandline.empty() || Commandline.front() == '\r' || Commandline.front() == '\n') [[unlikely]]
            return;

        // Remove line-break characters.
        while (Commandline.back() == '\r' || Commandline.back() == '\n')
            Commandline.remove_suffix(1);

        // Split into arguments.
        bool isQuoted = false;
        Inlinedvector<std::string, 8> Arguments{ "" };
        for (const auto &Char : Commandline)
            if (Char == ' ' && !isQuoted) Arguments.push_back("");
            else if (Char == '\"') { isQuoted ^= 1; Arguments.push_back(""); }
            else Arguments.back().push_back(Char);

        // Format as a C-array with the last pointer being null.
        const auto Array = (const char **)alloca((Arguments.size() + 1) * sizeof(char *));
        for (size_t i = 0; i < Arguments.size(); i++)
            Array[i] = Arguments[i].c_str();

        // Find the function we are interested in.
        const auto Callback = std::find_if(std::execution::par_unseq, Functions.begin(), Functions.end(), [&](const auto &Pair)
        {
            if (Pair.first.size() != Arguments[0].size()) return false;
            for (size_t i = 0; i < Pair.first.size(); i++)
                if (std::toupper(Pair.first[i]) != std::toupper(Arguments[0][i]))
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
            Callback->second((int)Arguments.size(), Array);
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

    // API exports.
    extern "C"  EXPORT_ATTR void __cdecl addConsolecommand(const char *Command, const void *Callback)
    {
        if (!Command || !Callback) return;
        addConsolecommand(std::string_view(Command), Callback_t(Callback));
    }
    static std::string __cdecl execCommand(const char *JSONString)
    {
        const auto Request = JSON::Parse(JSONString);
        const auto Commandline = Request.value<std::string>("Commandline");

        execCommandline(Commandline, false);
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

        Backend::API::addHandler("Console::Exec", execCommand);
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
