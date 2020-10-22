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
    #pragma message("Warning: No module name specified for the logging.")
    #endif

    // Set in Common.hpp
    #if !defined(LOGPATH)
    #define LOGPATH "."
    #endif

    // Compile-time evaluated path.
    constexpr auto Logfile = LOGPATH "/" MODULENAME ".log";

    // Logfile, stderr, and Ingame_GUI
    void toStream(std::u8string_view Message);
    void toConsole(std::u8string_view Message);
    void toFile(std::string_view Filename, std::u8string_view Message);

    // Formatted standard printing.
    inline void Print(const char Prefix, std::string_view Message)
    {
        char Buffer[16]{};
        const auto Now{ std::time(nullptr) };
        std::strftime(Buffer, 16, "%H:%M:%S", std::localtime(&Now));
        const auto Formatted = va("[%c][%-8s] %*s\n", Prefix, Buffer, Message.data(), Message.size());
        const auto Encoded = Encoding::toUTF8(Formatted);

        // Output.
        toFile(Logfile, Encoded);
        toConsole(Encoded);
        toStream(Encoded);
    }
    inline void Print(const char Prefix, std::wstring_view Message)
    {
        char Buffer[16]{};
        const auto Now{ std::time(nullptr) };
        std::strftime(Buffer, 16, "%H:%M:%S", std::localtime(&Now));
        const auto Formatted = va(L"[%c][%-8s] %*ls\n", Prefix, Buffer, Message.data(), Message.size());
        const auto Encoded = Encoding::toUTF8(Formatted);

        // Output.
        toFile(Logfile, Encoded);
        toConsole(Encoded);
        toStream(Encoded);
    }
    inline void Print(const char Prefix, std::u8string_view Message)
    {
        char Buffer[16]{};
        const auto Now{ std::time(nullptr) };
        std::strftime(Buffer, 16, "%H:%M:%S", std::localtime(&Now));
        auto Formatted = Encoding::toUTF8(va(L"[%c][%-8s] ", Prefix, Buffer));
        Formatted += Message;
        Formatted += u8'\n';

        // Output.
        toFile(Logfile, Formatted);
        toConsole(Formatted);
        toStream(Formatted);
    }

    // Remove the old logfile.
    inline void Clearlog()
    {
        #if defined(_WIN32)
        // We use UTF88 for the output, always.
        SetConsoleOutputCP(CP_UTF8);
        #endif

        std::remove(Logfile);
    }
}
