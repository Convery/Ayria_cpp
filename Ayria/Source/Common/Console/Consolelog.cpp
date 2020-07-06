/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Console.hpp"

namespace Console
{
    constexpr size_t Logsize = 256;
    std::array<Logline_t, Logsize> Internalbuffer;
    nonstd::ring_span <Logline_t> Consolelog{ Internalbuffer.data(),
                                              Internalbuffer.data() + Logsize,
                                              Internalbuffer.data(), Logsize };

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::string_view Filter)
    {
        std::vector<Logline_t> Result;
        Result.reserve(Count);

        std::for_each(Consolelog.rbegin(), Consolelog.rend(), [&](const auto &Item)
        {
            if (Count)
            {
                if (std::strstr(Item.first.c_str(), Filter.data()))
                {
                    Result.push_back(Item);
                    Count--;
                }
            }
        });

        std::reverse(Result.begin(), Result.end());
        return Result;
    }

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::string &Message, COLORREF Colour)
    {
        // Scan for keywords to detect a colour if not specified.
        if (Colour == 0)
        {
            Colour = [&]()
            {
                // Common keywords.
                if (std::strstr(Message.c_str(), "[E]") || std::strstr(Message.c_str(), "rror")) return 0xBE282A;
                if (std::strstr(Message.c_str(), "[W]") || std::strstr(Message.c_str(), "arning")) return 0xBEC02A;

                // Ayria default.
                if (std::strstr(Message.c_str(), "[I]")) return 0x218FBD;
                if (std::strstr(Message.c_str(), "[D]")) return 0x7F963E;
                if (std::strstr(Message.c_str(), "[>]")) return 0x7F963E;

                // Default.
                return 0x315571;
            }();
        }

        // Safety per parse, rather than line.
        static Spinlock Writelock;
        const std::scoped_lock _(Writelock);

        // Split by newline.
        std::string_view Input(Message);
        while (!Input.empty())
        {
            if (const auto Pos = Input.find('\n'); Pos != std::string_view::npos)
            {
                if (Pos != 0)
                {
                    Consolelog.push_back({ Input.data(), Colour });
                }
                Input.remove_prefix(Pos + 1);
                continue;
            }

            Consolelog.push_back({ Input.data(), Colour });
            break;
        }
    }

    /* TODO(tcn): Rendering */
}

// Provide a C-API for external code.
namespace API
{
    extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Colour)
    {
        assert(String);
        Console::addConsolemessage(String, Colour);
    }
}
