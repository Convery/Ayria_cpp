/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once

// Our configuration-, define-, macro-options.
#include "Common.hpp"

// Ignore warnings from third-party code.
#pragma warning(push, 0)

// Standard-library includes for all projects in this repository.
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <execution>
#include <cassert>
#include <cstdint>
#include <numbers>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdio>
#include <future>
#include <memory>
#include <ranges>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <queue>
#include <tuple>
#include <span>
#include <any>

// Platform-specific libraries.
#if defined(_WIN32)
#include <Ws2Tcpip.h>
#include <WinSock2.h>
#include <windowsx.h>
#include <Windows.h>
#include <timeapi.h>
#include <intrin.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#endif

// Restore warnings.
#pragma warning(pop)

// Third-party includes, usually included via VCPKG.
#include "Thirdparty.hpp"

// Extensions to the language.
using namespace std::literals;

// Global utilities.
#include <Utilities/Datatypes.hpp>
#include <Utilities/Containers/Ringbuffer.hpp>
#include <Utilities/Crypto/FNV1Hash.hpp>
#include <Utilities/Crypto/CRC32Hash.hpp>
#include <Utilities/Crypto/OpenSSLWrappers.hpp>
#include <Utilities/Crypto/Tiger192Hash.hpp>
#include <Utilities/Crypto/WWHash.hpp>
#include <Utilities/Encoding/Base64.hpp>
#include <Utilities/Encoding/Bitbuffer.hpp>
#include <Utilities/Encoding/Bytebuffer.hpp>
#include <Utilities/Encoding/JSON.hpp>
#include <Utilities/Encoding/Stringconv.hpp>
#include <Utilities/Encoding/Variadicstring.hpp>
#include <Utilities/Hacking/Hooking.hpp>
#include <Utilities/Hacking/Memprotect.hpp>
#include <Utilities/Hacking/Patternscan.hpp>
#include <Utilities/Wrappers/Logging.hpp>
#include <Utilities/Wrappers/Filesystem.hpp>
#include <Utilities/Localnetservers.hpp>

// Temporary includes.
#include <Utilities/Internal/Misc.hpp>
#include <Utilities/Internal/Spinlock.hpp>
#include <Utilities/Internal/Debugmutex.hpp>
#include <Utilities/Internal/Compressedstring.hpp>

// Ayria module used throughout the projects.
// Exports as struct for easier plugin initialization.
struct Ayriamodule_t
{
    // Call the exported JSON functions, pass NULL as function to list all.
    const char *(__cdecl *JSONAPI)(const char *Function, const char *JSONString);

    // Callback = void(__cdecl *)(unsigned int NodeID, const char *Message, unsigned int Length);
    void (__cdecl *addMessagehandler)(const char *Messagetype, const void *Callback);

    // UTF8 escaped ASCII strings, Callback = void(__cdecl *)(int Argc, const char **Argv);
    void (__cdecl *addConsolemessage)(const char *String, unsigned int Length, unsigned int Colour);
    void (__cdecl *addConsolecommand)(const char *Command, const void *Callback);

    #if defined(__cplusplus)
    Ayriamodule_t()
    {
        #if defined(NDEBUG)
        const auto Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll");
        #else
        const auto Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64d.dll" : "./Ayria/Ayria32d.dll");
        #endif
        assert(Modulehandle);

        #define Import(x) x = (decltype(x))GetProcAddress(Modulehandle, #x)
        Import(addMessagehandler);
        Import(addConsolemessage);
        Import(addConsolecommand);
        Import(JSONAPI);
        #undef Import
    }
    #endif
};
extern Ayriamodule_t Ayria;
