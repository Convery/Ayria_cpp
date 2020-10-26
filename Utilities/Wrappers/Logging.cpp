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
        std::lock_guard _(Threadguard);

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
        std::lock_guard _(Threadguard);
        if (!Console) Console = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64d.dll" : L"./Ayria/Ayria32d.dll");
        if (!Console) Console = GetModuleHandleW(Build::is64bit ? L"./Ayria/Ayria64.dll" : L"./Ayria/Ayria32.dll");
        if (!Console) Console = GetModuleHandleW(NULL);

        if (Console)
        {
            if (const auto Address = GetProcAddress(Console, "addConsolemessage"))
            {
                // ASCII or UTF8 string.
                reinterpret_cast<void(__cdecl *)(const void *, unsigned int, unsigned int)>
                    (Address)(Message.data(), (unsigned int)Message.size(), 0);
            }
        }
    }
}
