/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-03-03
    License: MIT
*/

#include "Logging.hpp"

namespace Logging
{
    std::mutex Threadguard;

    void toFile(std::string_view Filename, const std::string_view Message)
    {
        std::lock_guard _(Threadguard);

        // Open the logfile on disk and push to it.
        if (const auto Filehandle = std::fopen(Filename.data(), "a"))
        {
            std::fwrite(Message.data(), Message.size(), 1, Filehandle);
            std::fclose(Filehandle);
        }
    }
    void toStream(const std::string_view Message)
    {
        std::lock_guard _(Threadguard);

        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);

        // Duplicate to NT.
        #if defined(_WIN32) && !defined(NDEBUG)
        OutputDebugStringA(Message.data());
        #endif
    }
    void toConsole(const std::string_view Message)
    {
        std::lock_guard _(Threadguard);

        constexpr auto Consolename = Build::is64bit ? "Ingame_GUI64" : "Ingame_GUI32";
        if (const auto Handle = GetModuleHandleA(Consolename))
        {
            if (const auto Address = GetProcAddress(Handle, "addConsolestring"))
            {
                reinterpret_cast<void (*__cdecl)(const char *, int)>(Address)(Message.data(), 0x121212);
            }
        }
    }
}
