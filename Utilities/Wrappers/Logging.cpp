/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-03-03
    License: MIT
*/

#include "Logging.hpp"

namespace Logging
{
    // Usually set by CMAKE.
    #if !defined(MODULENAME)
        #define MODULENAME "NoModuleName"
        #pragma message("Warning: No module name specified for the logging.")
    #endif

    // Logging to disk is slow as fuck, let's batch it.
    std::string Pendingwrite{};
    uint8_t Pendingcount{};

    // Compile-time evaluated path.
    constexpr auto Logfile = LOG_PATH "/" MODULENAME ".log";

    // Only our process should write to this log, so thread it.
    Defaultmutex Threadguard;

    // UTF8 escaped ASCII strings.
    inline void toConsole(const std::string &Message)
    {
        static const FARPROC Console = []() -> FARPROC
        {
            HMODULE Handle = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64d.dll" : L"./Ayria/Ayria32d.dll");
            if (!Handle) Handle = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64.dll" : L"./Ayria/Ayria32.dll");
            if (!Handle) Handle = GetModuleHandleW(NULL);

            return GetProcAddress(Handle, "addConsolemessage");
        }();

        // Console does thread-safety.
        if (Console) [[likely]]
        {
            reinterpret_cast<void(__cdecl *)(const char *, unsigned int)>(Console)(Message.data(), 0);
        }
    }
    inline void toLogfile(const std::string &Message)
    {
        std::scoped_lock _(Threadguard);
        Pendingwrite += Message;
        Pendingcount++;

        if (Pendingcount >= 5)
        {
            // Open the logfile on disk and push to it.
            if (const auto Filehandle = std::fopen(Logfile, "ab"))
            {
                #if defined (_WIN32)
                _fwrite_nolock(Pendingwrite.data(), Pendingwrite.size(), 1, Filehandle);
                _fclose_nolock(Filehandle);
                #else
                std::fwrite(Pendingwrite.data(), Pendingwrite.size(), 1, Filehandle);
                std::fclose(Filehandle);
                #endif
            }

            Pendingwrite.clear();
            Pendingcount = 0;
        }
    }
    inline void toDebugstream(const std::string &Message)
    {
        // Standard error, should do for most consoles.
        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);

        // Duplicate to NT.
        #if defined (_WIN32)
        OutputDebugStringA(Message.c_str());
        #endif
    }

    // Remove the old logfile so we only have this session.
    void Clearlog()
    {
        std::remove(Logfile);
        std::atexit([]()
        {
            Pendingcount = 99;
            toLogfile("");
        });
    }
}
