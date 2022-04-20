/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-12
    License: MIT
*/

#include <Global.hpp>

namespace Console
{
    uint32_t LastmessageID{};
    static Debugmutex Writelock{};
    constexpr size_t Loglimit = 128;
    static Ringbuffer_t<Logline_t, Loglimit> Consolelog{};
    static Hashmap<std::wstring, Hashset<Functioncallback_t>> Commands{};

    // Threadsafe injection and fetching from the global log.
    void addMessage(Logline_t &&Message)
    {
        const auto Lines = Tokenizestring(Message.first, L'\n');

        std::scoped_lock Threadguard(Writelock);
        for (const auto &String : Lines)
        {
            // Ensure that we have some color.
            if (Message.second == 0)
            {
                const auto Newcolor = [&]() -> uint32_t
                {
                    // Ayria common prefixes.
                    if (String.find(L"[E]") != String.npos) return 0xBE282A;
                    if (String.find(L"[W]") != String.npos) return 0x2AC0BE;
                    if (String.find(L"[I]") != String.npos) return 0xBD8F21;
                    if (String.find(L"[D]") != String.npos) return 0x3E967F;
                    if (String.find(L"[>]") != String.npos) return 0x7F963E;

                    // Hail Mary..
                    if (String.find(L"rror") != String.npos) return 0xBE282A;
                    if (String.find(L"arning") != String.npos) return 0x2AC0BE;

                    // Default.
                    return 0x315571;
                }();

                Consolelog.emplace_back(String, Newcolor);
            }
            else
            {
                Consolelog.emplace_back(String, Message.second);
            }
        }

        LastmessageID = RNG::Next<uint32_t>();
    }
    std::vector<Logline_t> getMessages(size_t Maxcount, std::wstring_view Filter)
    {
        auto Clamped = std::min(Maxcount, Loglimit);
        std::vector<Logline_t> Result;
        Result.reserve(Clamped);

        std::scoped_lock Threadguard(Writelock);
        std::copy_if(Consolelog.rbegin(), Consolelog.rend(), std::back_inserter(Result),
                    [&](const auto &Tuple) -> bool
                    {
                        if (0 == Clamped) return false;

                        const auto &[String, Color] = Tuple;
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
    static std::optional<Hashset<Functioncallback_t>> Findcommand(std::wstring_view Functionname)
    {
        for (const auto &[Name, Callbacks] : Commands)
        {
            if (Name.size() != Functionname.size()) continue;

            const auto Range = lz::zip(Name, Functionname);
            if (std::any_of(Range.begin(), Range.end(), [](const auto &Tuple) -> bool
            {
                const auto &[Char1, Char2] = Tuple;
                return std::toupper(Char1) != std::toupper(Char2);
            })) continue;

            return Callbacks;
        }

        return {};
    }

    // Helper for parsing UTF8 escaped ASCII.
    static std::wstring toWIDE(std::string_view Input)
    {
        return Encoding::toUNICODE(Encoding::toUTF8(Input));
    }

    // Manage and execute the commandline, with optional logging.
    void execCommand(std::wstring_view Commandline, bool Log)
    {
        // Can't think of an easier way to parse the commandline.
        static const std::wregex Regex(LR"(("[^"]+"|[^\s"]+))", std::regex_constants::optimize);
        auto It = std::regex_iterator<std::wstring_view::const_iterator>(Commandline.begin(), Commandline.end(), Regex);
        const auto Size = std::distance(It, std::regex_iterator<std::wstring_view::const_iterator>());

        // Why would you do this to me? =(
        if (Size == 0) [[unlikely]] return;

        // Format as a C-array with the last pointer being null.
        const auto Arguments = std::make_unique<const wchar_t *[]>(Size + 1);
        std::vector<std::wstring> Heapstorage; Heapstorage.reserve(Size);
        for (ptrdiff_t i = 0; i < Size; ++i)
        {
            auto Temp = (*It++).str();
            if (Temp.back() == L'\"' && Temp.front() == L'\"') Temp = Temp.substr(1, Temp.size() - 2);
            Arguments[i] = Heapstorage.emplace_back(std::move(Temp)).c_str();
        }

        // Find the command by name (first argument).
        const auto Callbacks = Findcommand(Heapstorage.front());
        if (!Callbacks) [[unlikely]]
        {
            Errorprint(va(L"No command named: %s", Heapstorage.front().c_str()));
            return;
        }

        // Notify the user about what was executed.
        if (Log) [[likely]]
        {
            addMessage({L"> " + std::wstring{Commandline.data(), Commandline.size()}, Color_t(0xD6, 0xB7, 0x49)});
        }

        // And finally evaluate.
        for (const auto &Callback : *Callbacks)
        {
            if (Callback) [[likely]]
            {
                Callback(Size - 1, &Arguments[1]);
            }
        }
    }
    void addCommand(std::wstring_view Name, Functioncallback_t Callback)
    {
        if (!Callback || Name.empty()) [[unlikely]] return;
        Commands[{Name.data(), Name.size()}].insert(Callback);
    }

    // Interactions with other plugins.
    namespace API
    {
        // UTF8 escaped ASCII strings.
        extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int RGBColor)
        {
            if (!String) [[unlikely]] return;
            addMessage({ toWIDE(String), uint32_t(RGBColor) });
        }
        extern "C" EXPORT_ATTR void __cdecl addConsolecommand(const char *Name, void(__cdecl *Callback)(int Argc, const wchar_t **Argv))
        {
            if (!Name || !Callback) [[unlikely]] return;
            addCommand(toWIDE(Name), Callback);
        }

        // JSON endpoints.
        static std::string __cdecl printLine(JSON::Value_t &&Request)
        {
            const auto Color = std::max(Request.value<uint32_t>("Color"), Request.value<uint32_t>("Colour"));
            const auto Message = Request.value<std::string>("Message");

            addMessage({ toWIDE(Message), Color });
            return "{}";
        }
        static std::string __cdecl execCommand(JSON::Value_t &&Request)
        {
            const auto Commandline = Request.value<std::string>("Commandline");
            const auto Shouldlog = Request.value<bool>("Log");

            Console::execCommand(toWIDE(Commandline), Shouldlog);
            return "{}";
        }
    }

    // Add common commands to the backend.
    void Initialize()
    {
        static bool Initialized{};
        if (Initialized) return;
        Initialized = true;

        static constexpr auto Quit = [](int, const wchar_t **)
        {
            Core::Terminate();
        };
        addCommand(L"Quit", Quit);
        addCommand(L"Exit", Quit);

        static constexpr auto List = [](int, const wchar_t **)
        {
            std::wstring Output; Output.reserve(20 * Commands.size());
            for (const auto &[Index, Tuple] : lz::enumerate(Commands, 1))
            {
                Output += L"    ";
                Output += Tuple.first;
                if (Index % 4 == 0) Output += L'\n';
            }

            addMessage({ L"Available commands:" , 0xBD8F21U });
            addMessage({ Output, 0x715531U });
        };
        addCommand(L"List", List);
        addCommand(L"Help", List);

        JSONAPI::addEndpoint("Console::Exec", API::execCommand);
        JSONAPI::addEndpoint("Console::Print", API::printLine);
    }
}
