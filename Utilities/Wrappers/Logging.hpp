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
        const auto Time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
        const auto Formatted = std::format("[{}][{:%T}] {}\n", Prefix, Time, Encoding::toNarrow(Message));

        toDebugstream(Formatted);
        toConsole(Formatted);
        toLogfile(Formatted);
    }

    // Remove the old logfile so we only have this session.
    void Clearlog();
}
