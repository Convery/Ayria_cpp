/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-03-03
    License: MIT
*/

#include "Logging.hpp"

namespace Logging
{
    Defaultmutex Threadguard;

    void toFile(std::string_view Filename, std::u8string_view Message)
    {
        std::lock_guard _(Threadguard);

        // Open the logfile on disk and push to it.
        if (const auto Filehandle = std::fopen(Filename.data(), "a+b"))
        {
            std::fwrite(Message.data(), Message.size(), 1, Filehandle);
            std::fclose(Filehandle);
        }
    }
    void toStream(std::u8string_view Message)
    {
        std::fwrite(Message.data(), Message.size(), 1, stderr);
        std::fflush(stderr);

        // Duplicate to NT.
        #if defined(_WIN32) && !defined(NDEBUG)
        OutputDebugStringW(Encoding::toWide(Message).c_str());
        #endif
    }

    static HMODULE Console = nullptr;
    void toConsole(std::u8string_view Message)
    {
        if (!Console) Console = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64d.dll" : L"./Ayria/Ayria32d.dll");
        if (!Console) Console = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64.dll" : L"./Ayria/Ayria32.dll");
        if (!Console) Console = GetModuleHandleW(NULL);

        if (Console)
        {
            if (const auto Address = GetProcAddress(Console, "addConsolemessage"))
            {
                // UTF8 escaped ASCII strings.
                const auto ASCII = Encoding::toNarrow(Message);
                reinterpret_cast<void(__cdecl *)(const char *, unsigned int, unsigned int)>
                    (Address)(ASCII.c_str(), (unsigned int)Message.size(), 0);
            }
        }
    }
}
