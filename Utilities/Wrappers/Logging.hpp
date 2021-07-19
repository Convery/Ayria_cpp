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
    void toConsole(const std::string &Message);
    void toLogfile(const std::string &Message);
    void toDebugstream(const std::string &Message);

    // Let std::fmt deal with type specializations.
    template <typename T> void Print(char Prefix, T Message)
    {
        char Buffer[16]{};
        const auto Now{ std::time(nullptr) };
        std::strftime(Buffer, 16, "%H:%M:%S", std::localtime(&Now));

        const auto Formatted = std::format("[{}][{}] {}\n", Prefix, Buffer, Encoding::toNarrow(Message));
        toDebugstream(Formatted);
        toConsole(Formatted);
        toLogfile(Formatted);
    }

    // Remove the old logfile so we only have this session.
    void Clearlog();
}
