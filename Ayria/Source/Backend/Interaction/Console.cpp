/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-12
    License: MIT
*/

#include <Global.hpp>

namespace Console
{
    uint32_t LastmessageID{};
    static Spinlock Writelock{};
    constexpr size_t Loglimit = 256;
    static Ringbuffer_t<Logline_t, Loglimit> Consolelog{};
    static Hashmap<std::string, Functioncallback_t> Commands{};

    // Threadsafe injection into and fetching from the global log.
    template <typename T> void addMessage(const std::basic_string<T> &Message, Color_t RGBColor)
    {
        return addMessage(std::basic_string_view<T> {Message}, RGBColor);
    }
    template <typename T> void addMessage(std::basic_string_view<T> Message, Color_t RGBColor)
    {
        // Passthrough for T = char
        const auto Compat = Encoding::toNarrow(Message);
        std::scoped_lock Threadguard(Writelock);

        for (const auto &String : lz::split(Compat, '\n'))
        {
            // Ensure that we have some color.
            if (RGBColor == 0)
            {
                const auto Newcolor = [&]() -> uint32_t
                {
                    // Ayria common prefixes.
                    if (String.find("[E]") != String.npos) return 0xBE282A;
                    if (String.find("[W]") != String.npos) return 0x2AC0BE;
                    if (String.find("[I]") != String.npos) return 0xBD8F21;
                    if (String.find("[D]") != String.npos) return 0x3E967F;
                    if (String.find("[>]") != String.npos) return 0x7F963E;

                    // Hail Mary..
                    if (String.find("rror") !=  String.npos) return 0xBE282A;
                    if (String.find("arning") !=  String.npos) return 0x2AC0BE;

                    // Default.
                    return 0x315571;
                }();

                // Passthrough for T = wchar_t
                Consolelog.emplace_back(Encoding::toWide(String), Newcolor);
            }
            else
            {
                // Passthrough for T = wchar_t
                Consolelog.emplace_back(Encoding::toWide(String), RGBColor);
            }
        }

        // Just need a new number.
        LastmessageID = rand();
    }
    std::vector<Logline_t> getMessages(size_t Maxcount, std::wstring_view Filter)
    {
        auto Clamped = std::min(Maxcount, Loglimit);
        std::vector<Logline_t> Result;
        Result.reserve(Clamped);

        std::scoped_lock Threadguard(Writelock);
        std::copy_if(Consolelog.rbegin(), Consolelog.rend(), std::back_inserter(Result),
                     [&](const auto &Tuple)
                     {
                         if (0 == Clamped) return false;

                         const auto &[String, Colot] = Tuple;
                         if (String.find(Filter) != String.npos)
                         {
                             Clamped--;
                             return true;
                         }

                         return false;
                     });

        // Reverse the vector to simplify other code.
        std::ranges::reverse(Result);
        return Result;
    }

    // Helper to manage the commands.
    static Functioncallback_t Findcommand(std::string_view Functionname)
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
        // Passthrough for T = char
        const auto Compatible = Encoding::toNarrow(Commandline);

        // Can't think of an easier way to parse the commandline.
        static const std::regex Regex(R"(("[^"]+"|[^\s"]+))", std::regex_constants::optimize);
        auto It = std::sregex_iterator(Compatible.cbegin(), Compatible.cend(), Regex);
        const auto Size = std::distance(It, std::sregex_iterator());

        // Why would you do this to me? =(
        if (Size == 0) [[unlikely]] return;

        // Format as a C-array with the last pointer being null.
        const auto Arguments = std::make_unique<const char *[]>(Size + 1);
        std::vector<std::string> Heapstorage;
        Heapstorage.reserve(Size);

        for (ptrdiff_t i = 0; i < Size; ++i)
        {
            auto Temp = (*It++).str();
            if (Temp.back() == '\"' && Temp.front() == '\"') Temp = Temp.substr(1, Temp.size() - 2);
            Arguments[i] = Heapstorage.emplace_back(std::move(Temp)).c_str();
        }

        // Find the command by name (first argument).
        const auto Callback = Findcommand(Heapstorage.front());
        if (!Callback) [[unlikely]]
        {
            Errorprint(va("No command named: %s", Heapstorage.front().c_str()));
            return;
        }

        // Notify the user about what was executed.
        if (Log) [[likely]]
        {
            addMessage("> "s + Compatible, Color_t(0xD6, 0xB7, 0x49));
        }

        // And finally evaluate.
        Callback(Size - 1, &Arguments[1]);
    }
    template <typename T> void execCommand(const std::basic_string<T> &Commandline, bool Log)
    {
        return execCommand(std::basic_string_view<T>{Commandline}, Log);
    }
    template <typename T> void addCommand(std::basic_string_view<T> Name, Functioncallback_t Callback)
    {
        if (!Callback || Name.empty()) [[unlikely]] return;
        if (!Findcommand(Encoding::toNarrow(Name)))
        {
            Commands.emplace(Encoding::toNarrow(Name), Callback);
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
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int RGBColor)
        {
            if (!String) [[unlikely]] return;
            addMessage(std::string_view{ String }, uint32_t(RGBColor));
        }
        extern "C" EXPORT_ATTR void __cdecl addConsolecommand(const char *Name, void(__cdecl *Callback)(int Argc, const char **Argv))
        {
            if (!Name || !Callback) [[unlikely]] return;
            addCommand(std::string_view{ Name }, Callback);
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
            std::string Output; Output.reserve(16 * Commands.size());
            for (const auto &[Index, Tuple] : lz::enumerate(Commands, 1))
            {
                Output += "    ";
                Output += Tuple.first;
                if (Index % 4 == 0) Output += '\n';
            }

            addMessage("Available commands:"sv , 0xBD8F21U);
            addMessage(Output, 0x715531U);
        };
        addCommand("List"sv, List);
        addCommand("Help"sv, List);

        ::API::addEndpoint("Console::Exec", API::execCommand);
        ::API::addEndpoint("Console::Print", API::printLine);
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
