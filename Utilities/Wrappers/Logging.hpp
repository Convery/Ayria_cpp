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

    // NOTE(tcn): Not thread-safe but generally good enough.
    inline void toFile(const std::string_view Message)
    {
        // Open the logfile on disk and push to it.
        if (const auto Filehandle = std::fopen(Logfile, "a"))
        {
            std::fwrite(Message.data(), Message.size(), 1, Filehandle);
            std::fclose(Filehandle);
        }
    }
    inline void toStream(const std::string_view Message)
    {
        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);

        // Duplicate to NT.
        #if defined(_WIN32) && !defined(NDEBUG)
        OutputDebugStringA(Message.data());
        #endif
    }

    // Formatted standard printing.
    inline void Print(const char Prefix, const std::string_view Message)
    {
        const auto Now{ std::time(nullptr) };
        char Buffer[80]{};

        std::strftime(Buffer, 80, "%H:%M:%S", std::localtime(&Now));
        toFile(va("[%c][%-8s] %*s\n", Prefix, Buffer, Message.size(), Message.data()));

        #if !defined(NDEBUG)
        toStream(va("[%c][%-8s] %*s\n", Prefix, Buffer, Message.size(), Message.data()));
        #endif
    }

    // Remove the old logfile.
    inline void Clearlog()
    {
        std::remove(Logfile);
    }
}
