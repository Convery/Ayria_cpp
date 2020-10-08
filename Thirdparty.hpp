/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-06-18
    License: MIT

    A header to make dependency-management easier.
    Make sure that you compile with /opt:ref..
*/

#pragma once
#pragma warning(push, 0)

// Nlhohmann modern JSON parsing library.
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#define HAS_NLOHMANN
#endif

// Generic ring-buffer implementation.
#if __has_include(<nonstd/ring_span.hpp>)
#include <nonstd/ring_span.hpp>
#endif

// LZ4 - Fast compression library.
#if __has_include(<lz4.h>)
#include <lz4.h>
#define HAS_LZ4
#if defined(NDEBUG)
#pragma comment (lib, "lz4.lib")
#else
#pragma comment (lib, "lz4d.lib")
#endif
#endif

// OpenSSL - Encryption library.
#if __has_include(<openssl/ssl.h>)
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")
#define HAS_OPENSSL
#endif

// Generic hooking library for Windows.
#if __has_include(<mhook-lib/mhook.h>)
#pragma comment(lib, "mhook.lib")
#include <mhook-lib/mhook.h>
#define HAS_MHOOK
#endif

// Extensive disassembler library.
#if __has_include(<capstone/capstone.h>)
#pragma comment(lib, "capstone.lib")
#include <capstone/capstone.h>
#define HAS_CAPSTONE
#endif

// Advanced hooking library.
#if __has_include(<polyhook2/IHook.hpp>)
#include <polyhook2/Detour/x86Detour.hpp>
#include <polyhook2/Detour/x64Detour.hpp>
#pragma comment(lib, "PolyHook_2.lib")
#define HAS_POLYHOOK
#endif

// Agner Fogs SIMD library.
#if __has_include(<vectorclass/vectorclass.h>)
#define HAS_VECTORCLASS
#define VCL_NAMESPACE VCL
#include <vectorclass/vectorclass.h>
#endif

#pragma warning(pop)
