/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-03-03
    License: MIT
*/

#include "Logging.hpp"

namespace Logging
{
    Defaultmutex Threadguard;

    void toFile(std::string_view Filename, std::string_view Message)
    {
        std::lock_guard _(Threadguard);

        // Open the logfile on disk and push to it.
        if (const auto Filehandle = std::fopen(Filename.data(), "a"))
        {
            std::fwrite(Message.data(), Message.size(), 1, Filehandle);
            std::fclose(Filehandle);
        }
    }
    void toStream(std::string_view Message)
    {
        std::lock_guard _(Threadguard);

        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);

        // Duplicate to NT.
        #if defined(_WIN32) && !defined(NDEBUG)
        OutputDebugStringA(Message.data());
        #endif
    }

    static HMODULE Console = nullptr;
    void toConsole(std::string_view Message)
    {
        std::lock_guard _(Threadguard);
        if (!Console) Console = GetModuleHandleA(Build::is64bit ? "./Ayria/Ayria64d.dll" : "./Ayria/Ayria32d.dll");
        if (!Console) Console = GetModuleHandleA(Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll");
        if (!Console) Console = GetModuleHandleA(NULL);

        if (Console)
        {
            if (const auto Address = GetProcAddress(Console, "addConsolemessage"))
            {
                reinterpret_cast<void (__cdecl *)(const wchar_t *, int)>(Address)(toWide(Message).c_str(), 0);
            }
        }
    }
}
