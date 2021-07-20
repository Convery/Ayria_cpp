/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once

// Fixup some Visual Studio builds not defining this.
#if !defined(_DEBUG) && !defined(NDEBUG)
    #define NDEBUG
#endif

// Platform identification.
#if defined(_MSC_VER)
#define EXPORT_ATTR __declspec(dllexport)
#define IMPORT_ATTR __declspec(dllimport)
#elif defined(__GNUC__)
#define EXPORT_ATTR __attribute__((visibility("default")))
#define IMPORT_ATTR
#else
#error Compiling for unknown platform.
#endif

// Remove some Windows annoyance.
#if defined(_WIN32)
#define _HAS_DEPRECATED_RESULT_OF 1

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

// Produce a smaller build by not including third-party detours.
// #define NO_HOOKLIB

// Build information helpers to avoid the preprocessor.
namespace Build
{
    constexpr bool is64bit = sizeof(void *) == 8;

    #if defined(_WIN32)
    constexpr bool isWindows = true;
    #else
    constexpr bool isWindows = false;
    #endif

    #if defined(__linux__)
    constexpr bool isLinux = true;
    #else
    constexpr bool isLinux = false;
    #endif


    #if defined(NDEBUG)
    constexpr bool isDebug = false;
    #else
    constexpr bool isDebug = true;
    #endif
}

// Information logging.
namespace Logging { template <typename T> extern void Print(char Prefix, T Message); }
#define Warningprint(string) Logging::Print('W', string)
#define Errorprint(string) Logging::Print('E', string)
#define Infoprint(string) Logging::Print('I', string)
#if !defined(NDEBUG)
    #define Debugprint(string) Logging::Print('D', string)
    #define Traceprint() Logging::Print('>', __FUNCTION__)
#else
    #define Debugprint(string) ((void)0)
    #define Traceprint() ((void)0)
#endif

// Where to keep the log.
#if !defined(LOG_PATH)
    #define LOG_PATH "./Ayria/Logs"
#endif

// Helper to switch between debug and release mutex's.
#if defined(NDEBUG)
    #define Defaultmutex Spinlock
#else
    #define Defaultmutex Debugmutex
#endif

// Ignore ANSI compatibility for structs.
#pragma warning(disable: 4201)

// Ignore warnings about casting float to int.
#pragma warning(disable: 4244)

// Elevate [[nodiscard]] to an error.
#pragma warning(error: 4834)
