/*
    Initial author: Convery (tcn@ayria.se)
    Started: 14-03-2019
    License: MIT
*/

#pragma once
#include "Variadicstring.hpp"
#include <cstdio>
#include <ctime>

namespace Logging
{
    #if !defined(LOGPATH)
    #define LOGPATH "."
    #endif

    #if !defined(MODULENAME)
    #warning No module name specified for the logging.
        constexpr char Logfile[] = "./NoModuleName.log";
    #else
    constexpr char Logfile[] = LOGPATH "/" MODULENAME ".log";
    #endif

    // NOTE(tcn): Not threadsafe but good enough.
    inline void toFile(const std::basic_string_view<char> Message)
    {
        // Open the logfile on disk and push to it.
        if (const auto Filehandle = std::fopen(Logfile, "a"))
        {
            std::fwrite(Message.data(), Message.size(), 1, Filehandle);
            std::fclose(Filehandle);
        }
    }
    inline void toStream(const std::basic_string_view<char> Message)
    {
        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);
    }

    // Formatted standard printing.
    inline void Print(const char Prefix, const std::basic_string_view<char> Message)
    {
        const auto Now{ std::time(NULL) };
        char Buffer[80]{};

        std::strftime(Buffer, 80, "%H:%M:%S", std::localtime(&Now));
        toFile(va("[%c][%-8s] %*s", Prefix, Buffer, Message.size(), Message.data()));

        #if !defined(NDEBUG)
        toStream(va("[%c][%-8s] %*s", Prefix, Buffer, Message.size(), Message.data()));
        #endif
    }

    // Remove the old logfile.
    inline void Clearlog()
    {
        std::remove(Logfile);
    }
}
