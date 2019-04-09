/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#pragma once

// Fixup for Visual Studio 2015 no longer defining this.
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
#define EXPORT_ATTR
#define IMPORT_ATTR
#error Compiling for unknown platform.
#endif

// Remove some Windows annoyance.
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#endif

// This should be set in CMake.
#if !defined(MODULENAME)
#error "MODULENAME is not set!"
#endif

// Where to keep the log.
#define LOGPATH "./Ayria/Logs"

// Build information.
namespace Build
{
    constexpr bool is64bit = sizeof(void *) == 8;
    constexpr const char *Modulename = MODULENAME;
}

// Information logging.
#if !defined(NDEBUG)
#define Warningprint(string) Logging::Print('W', string)
#define Errorprint(string) Logging::Print('E', string)
#define Traceprint() Logging::Print('>', __FUNCTION__)
#define Infoprint(string) Logging::Print('I', string)
#else
#define Warningprint(string)
#define Errorprint(string)
#define Infoprint(string)
#define Traceprint()
#endif

// Helper to support designated initializers until we get C++20
#define instantiate(T, ...) ([&]{ T ${}; __VA_ARGS__; return $; }())

// Ignore ANSI compatibility for stucts.
#pragma warning(disable: 4201)
