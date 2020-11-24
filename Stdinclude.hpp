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
#include <Ws2tcpip.h>
#include <WinSock2.h>
#include <Windowsx.h>
#include <Windows.h>
#include <intrin.h>
#include <direct.h>
#undef min
#undef max
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
#include <Utilities/Hacking/Branchless.hpp>
#include <Utilities/Hacking/Hooking.hpp>
#include <Utilities/Hacking/Memprotect.hpp>
#include <Utilities/Hacking/Patternscan.hpp>
#include <Utilities/Wrappers/Logging.hpp>
#include <Utilities/Wrappers/Filesystem.hpp>

// Temporary includes.
#include <Utilities/Internal/Misc.hpp>
#include <Utilities/Internal/Spinlock.hpp>
#include <Utilities/Internal/Debugmutex.hpp>

// Ayria module used throughout the projects.
// Exports as struct for easier plugin initialization.
struct Ayriamodule_t
{
    HMODULE Modulehandle{};

    // For use with the APIs.
    typedef union
    {
        uint64_t Raw;
        struct
        {
            uint32_t AccountID;
            uint16_t Creationdate;
            uint8_t Accountflags;
            uint8_t Stateflags;
        };
    } AyriaID_t;
    typedef union
    {
        uint8_t Raw;
        struct
        {
            uint8_t
                isClan : 1,
                isAdmin : 1,
                isGroup : 1,
                isServer : 1,
                isPrivate : 1,
                isModerator : 1,
                AYA_RESERVED1 : 1,
                AYA_RESERVED2 : 1;
        };
    } Accountflags_t;
    typedef union
    {
        uint8_t Raw;
        struct
        {
            uint8_t
                isIngame : 1,
                isHosting : 1,

                MAX : 1;
        };
    } Stateflags_t;

    // Create a functionID from the name of the service.
    static uint32_t toFunctionID(const char *Name) { return Hash::WW32(Name); };

    // UTF8 escaped ASCII strings.
    // using Callback_t = void(__cdecl *)(int Argc, const char **Argv);
    void(__cdecl *addConsolemessage)(const char *String, unsigned int Length, unsigned int Colour);
    void(__cdecl *addConsolecommand)(const char *Name, unsigned int Length, const void *Callback);
    void(__cdecl *execCommandline)(const char *String, unsigned int Length);

    // using Messagecallback_t = void(__cdecl *)(const char *JSONString);
    void(__cdecl *addNetworklistener)(uint32_t MessageID, void *Callback);

    // FunctionID = WW32("Service name"); ID 0 / Invalid = List all available.
    const char *(__cdecl *API_Client)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Social)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Network)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Matchmake)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Fileshare)(uint32_t FunctionID, const char *JSONString);

    Ayriamodule_t()
    {
        #if defined(NDEBUG)
        Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll");
        #else
        Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64d.dll" : "./Ayria/Ayria32d.dll");
        #endif
        assert(Modulehandle);

        #define Import(x) x = (decltype(x))GetProcAddress(Modulehandle, #x);
        Import(addConsolemessage);
        Import(addConsolecommand);
        Import(execCommandline);
        Import(addNetworklistener);
        Import(API_Client);
        Import(API_Social);
        Import(API_Network);
        Import(API_Matchmake);
        Import(API_Fileshare);
        #undef Import
    }
};
extern Ayriamodule_t Ayria;
