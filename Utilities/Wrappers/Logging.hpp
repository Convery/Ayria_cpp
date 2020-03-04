/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Logging
{
    // Usually set by CMAKE.
    #if !defined(MODULENAME)
    #define MODULENAME "NoModuleName"
    #warning No module name specified for the logging.
    #endif

    // Set in Common.hpp
    #if !defined(LOGPATH)
    #define LOGPATH "."
    #endif

    // Compile-time evaluated path.
    constexpr auto Logfile = LOGPATH "/" MODULENAME ".log";

    // Logfile, stderr, and Ingame_GUI
    void toStream(const std::string_view Message);
    void toConsole(const std::string_view Message);
    void toFile(std::string_view Filename, const std::string_view Message);

    // Formatted standard printing.
    inline void Print(const char Prefix, const std::string_view Message)
    {
        const auto Now{ std::time(nullptr) };
        char Buffer[80]{};

        std::strftime(Buffer, 80, "%H:%M:%S", std::localtime(&Now));
        const auto Formatted = va("[%c][%-8s] %*s\n", Prefix, Buffer, Message.size(), Message.data());

        // Output.
        toFile(Logfile, Formatted);
        #if !defined(NDEBUG)
        toStream(Formatted);
        toConsole(Formatted);
        #endif
    }

    // Remove the old logfile.
    inline void Clearlog()
    {
        std::remove(Logfile);
    }
}
