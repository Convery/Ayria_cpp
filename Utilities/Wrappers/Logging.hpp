/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Logging
{
    // UTF8 escaped ASCII strings.
    inline void toConsole(const std::string &Message);
    inline void toLogfile(const std::string &Message);
    inline void toDebugstream(const std::string &Message);

    // Let std::fmt deal with type specializations.
    template <typename T> void Print(char Prefix, const T &Message)
    {
        const auto Now{ std::time(nullptr) };
        const auto Local = localtime(&Now);

        const auto Formatted = std::format("[{}][{:02}:{:02}:{:02}] {}\n", Prefix,
            Local->tm_hour, Local->tm_min, Local->tm_sec,
            Encoding::toASCII(Message));

        toDebugstream(Formatted);
        toConsole(Formatted);
        toLogfile(Formatted);
    }

    // Remove the old logfile so we only have this session.
    void Clearlog();
}
