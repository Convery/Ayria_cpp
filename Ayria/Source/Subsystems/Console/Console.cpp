/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    // UTF8 escaped ACII strings are passed to Argv for compatibility.
    // using Functioncallback_t = void(__cdecl *)(int Argc, const char *Argv);
    // using Logline_t = std::pair<std::wstring, Color_t>;

    static Spinlock Writelock{};
    constexpr size_t Loglimit = 256;
    static Ringbuffer_t<Logline_t, Loglimit> Consolelog{};
    static Hashmap<std::wstring, Functioncallback_t> Commands{};

    // Threadsafe injection into and fetching from the global log.
    template <typename T> void addMessage(const std::basic_string<T> &Message, Color_t Color)
    {
        return addMessage(std::basic_string_view<T> {Message}, Color);
    }
    template <typename T> void addMessage(std::basic_string_view<T> Message, Color_t Color)
    {
        std::scoped_lock Threadguard(Writelock);
        const auto Compat = Encoding::toNarrow(Message);

        for (const auto &String : lz::split(Compat, '\n'))
        {
            // Ensure that we have some color.
            if (Color == 0)
            {
                const auto Newcolor = [&]() -> uint32_t
                {
                    // Ayria common prefixes.
                    if (String.find("[E]") != String.npos) return 0xBE282A;
                    if (String.find("[W]") != String.npos) return 0x2AC0BE;
                    if (String.find("[I]") != String.npos) return 0xBD8F21;
                    if (String.find("[D]") != String.npos) return 0x3E967F;
                    if (String.find("[>]") != String.npos) return 0x7F963E;

                    // Just a guess..
                    if (String.find("rror") !=  String.npos) return 0xBE282A;
                    if (String.find("arning") !=  String.npos) return 0x2AC0BE;

                    // Default.
                    return 0x315571;
                }();

                Consolelog.emplace_back(Encoding::toWide(String), Newcolor);
            }
            else
            {
                Consolelog.emplace_back(Encoding::toWide(String), Color);
            }
        }
    }
    std::vector<Logline_t> getMessages(size_t Maxcount, std::wstring_view Filter)
    {
        std::vector<Logline_t> Result;
        Result.reserve(std::clamp(Maxcount, size_t(1), Loglimit));

        std::scoped_lock Threadguard(Writelock);
        std::copy_if(Consolelog.rbegin(), Consolelog.rend(), std::back_inserter(Result),
        [&](const auto &Tuple)
        {
            if (0 == Maxcount) return false;

            const auto &[String, Colot] = Tuple;
            if (String.find(Filter) != String.npos)
            {
                Maxcount--;
                return true;
            }

            return false;
        });

        // Reverse the vector to simplify other code.
        std::ranges::reverse(Result);
        return Result;
    }

    // Helper to manage the commands.
    Functioncallback_t Findcommand(std::wstring_view Functionname)
    {
        for (const auto &[Name, Callback] : Commands)
        {
            if (Name.size() != Functionname.size()) continue;

            const auto Range = lz::zip(Name, Functionname);
            if (std::any_of(Range.begin(), Range.end(), [](const auto &Tuple) -> bool
            {
                const auto &[Char1, Char2] = Tuple;
                return std::toupper(Char1) != std::toupper(Char2);
            })) continue;

            return Callback;
        }

        return nullptr;
    }

    // Manage and execute the commandline, with optional logging.
    template <typename T> void execCommand(std::basic_string_view<T> Commandline, bool Log)
    {
        const auto Compat = Encoding::toNarrow(Commandline);

        static const std::regex Regex("(\"[^\"]+\"|[^\\s\"]+)", std::regex_constants::optimize);
        auto It = std::sregex_iterator(Compat.cbegin(), Compat.cend(), Regex);
        const auto Size = std::distance(It, std::sregex_iterator());

        // Why would you do this to me? =(
        if (Size == 0) return;

        // Format as a C-array with the last pointer being null.
        const char **Arguments = new const char *[Size + 1]();
        std::vector<std::string> Heapstorage;
        Heapstorage.reserve(Size);

        for (ptrdiff_t i = 0; i < Size; ++i)
        {
            auto Temp = (*It++).str();
            if (Temp.back() == '\"' && Temp.front() == '\"')
                Temp = Temp.substr(1, Temp.size() - 2);
            Arguments[i] = Heapstorage.emplace_back(std::move(Temp)).c_str();
        }

        // Find the command by name (first argument).
        const auto Callback = Findcommand(Encoding::toWide(Heapstorage.front()));
        if (!Callback)
        {
            Errorprint(va("No command named: %s", Heapstorage.front().c_str()));
            delete[] Arguments;
            return;
        }

        // Notify the user about what was executed.
        if (Log)
        {
            addMessage("> "s + Compat, Color_t(0xD6, 0xB7, 0x49));
        }

        // And finally evaluate.
        Callback(Size - 1, &Arguments[1]);
        delete[] Arguments;
    }
    template <typename T> void execCommand(const std::basic_string<T> &Commandline, bool Log)
    {
        return execCommand(std::basic_string_view<T>{Commandline}, Log);
    }
    template <typename T> void addCommand(std::basic_string_view<T> Name, Functioncallback_t Callback)
    {
        assert(Callback); assert(!Name.empty());

        if (!Findcommand(Encoding::toWide(Name)))
        {
            Commands.emplace(Encoding::toWide(Name), Callback);
        }
    }
    template <typename T> void addCommand(const std::basic_string<T> &Name, Functioncallback_t Callback)
    {
        return addCommand(std::basic_string_view<T>{Name}, Callback);
    }

    // Interactions with other plugins.
    namespace API
    {
        // UTF8 escaped ASCII strings.
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Length, unsigned int Colour)
        {
            assert(String);
            addMessage(std::string_view{ String, Length }, uint32_t(Colour));
        }
        extern "C" EXPORT_ATTR void __cdecl addConsolecommand(const char *String, unsigned int Length, const void *Callback)
        {
            assert(String); assert(Callback);
            addCommand(std::string_view{ String, Length }, Functioncallback_t(Callback));
        }

        // JSON endpoints.
        static std::string __cdecl printLine(JSON::Value_t &&Request)
        {
            const auto Color = std::max(Request.value<uint32_t>("Color"), Request.value<uint32_t>("Colour"));
            const auto Message = Request.value<std::string>("Message");

            addMessage(Message, Color);
            return "{}";
        }
        static std::string __cdecl execCommand(JSON::Value_t &&Request)
        {
            const auto Commandline = Request.value<std::string>("Commandline");
            const auto Shouldlog = Request.value<bool>("Log");

            Console::execCommand(Commandline, Shouldlog);
            return "{}";
        }
    }

    // Add common commands to the backend.
    void Initialize()
    {
        static bool Initialized{};
        if (Initialized) return;
        Initialized = true;

        static const auto Quit = [](int, const char **)
        {
            std::exit(0);
        };
        addCommand("Quit"sv, Quit);
        addCommand("Exit"sv, Quit);

        static const auto List = [](int, const char **)
        {
            std::wstring Output; Output.reserve(16 * Commands.size());
            for (const auto &[Index, Tuple] : lz::enumerate(Commands, 1))
            {
                Output += L"    ";
                Output += Tuple.first;
                if (Index % 4 == 0) Output += L'\n';
            }

            addMessage("Available commands:"sv , 0xBD8F21U);
            addMessage(Output, 0x715531U);

            // Totally not a hack to make the last line unique.
            static bool Ticktock{}; Ticktock ^= true;
            if (Ticktock) addMessage("--------------------------------"sv, 0xBD8F21U);
            else addMessage("================================"sv, 0xBD8F21U);
        };
        addCommand("List"sv, List);
        addCommand("Help"sv, List);

        Backend::API::addEndpoint("Console::Exec", API::execCommand);
        Backend::API::addEndpoint("Console::Print", API::printLine);
    }

    // Instantiate the templates to make MSVC happy.
    void Totally_real_func()
    {
        addMessage(std::wstring(), {});
        addCommand(std::wstring(), {});
        execCommand(std::wstring(), {});
        addMessage(std::wstring_view(), {});
        addCommand(std::wstring_view(), {});
        execCommand(std::wstring_view(), {});

        addMessage(std::string(), {});
        addCommand(std::string(), {});
        execCommand(std::string(), {});
        addMessage(std::string_view(), {});
        addCommand(std::string_view(), {});
        execCommand(std::string_view(), {});
    }
}