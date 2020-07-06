/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Console.hpp"

namespace Console
{
    constexpr size_t Maxtabs = 10;
    int32_t Tabcount{1}, Selectedtab{ -1 };
    std::array<std::string, Maxtabs> Tabtitles;
    std::array<std::string, Maxtabs> Tabfilters;

    // Which user-selected filter is active?
    std::string_view getCurrentfilter()
    {
        // Default tab doesn't filter anything.
        if (Selectedtab == -1) return "";
        else return Tabfilters[Selectedtab];
    }

    // Filter the input based on 'tabs'.
    void addConsoletab(const std::string &Name, const std::string &Filter)
    {
        if (Tabcount >= Maxtabs) return;
        const auto String = std::string(Filter);

        // Sanity-check to avoid redundant filters.
        for (size_t i = 0; i < Maxtabs; ++i)
        {
            if (Tabfilters[i] == String)
            {
                // Already added.
                if (std::strstr(Tabtitles[i].c_str(), Name.c_str()))
                    return;

                Tabtitles[i] += " | "s + String;
                return;
            }
        }

        // Insert the requested filter.
        Tabfilters[Tabcount] = String;
        Tabtitles[Tabcount] = Name;
        Tabcount++;
    }

    /* TODO(tcn): Rendering */
}

// Provide a C-API for external code.
namespace API
{
    extern "C" EXPORT_ATTR void __cdecl addConsoletab(const char *Name, const char *Filter)
    {
        assert(Name);
        assert(Filter);
        Console::addConsoletab(Name, Filter);
    }
}
