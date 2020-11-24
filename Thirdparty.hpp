/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-06-18
    License: MIT

    A header to make dependency-management easier.
    Make sure that you compile with /opt:ref..
*/

#pragma once
#pragma warning(push, 0)

// NLhohmann modern JSON parsing library.
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
#define VCL_NAMESPACE vcl
#include <vectorclass/vectorclass.h>
#endif

// Alternative JSON parser.
#if __has_include(<simdjson.h>)
#define SIMDJSON_NO_PORTABILITY_WARNING
#define HAS_SIMDJSON
#include <simdjson.h>
#pragma comment(lib, "simdjson.lib")
#endif

// NOTE(tcn): Removed due to libtorrent adding 12MB in release mode.
//#if __has_include(<libtorrent/session.hpp>)
//#define HAS_LIBTORRENT
//#define TORRENT_NO_DEPRECATE
//#define TORRENT_USE_OPENSSL
//
//#pragma comment(lib, "torrent-rasterbar.lib")
//#pragma comment(lib, "Iphlpapi.lib")
//#pragma comment(lib, "Dbghelp.lib")
//#endif

// Googles extensions/alternatives to the STL.
#if __has_include(<absl/strings/str_join.h>)
#define HAS_ABSEIL
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/node_hash_map.h>
#include <absl/container/node_hash_set.h>
#include <absl/random/random.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_split.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include <absl/strings/strip.h>
#include <absl/strings/escaping.h>
#include <absl/synchronization/mutex.h>
#include <absl/container/inlined_vector.h>

#pragma comment(lib, "absl_base.lib")
#pragma comment(lib, "absl_city.lib")
#pragma comment(lib, "absl_hash.lib")
#pragma comment(lib, "absl_hashtablez_sampler.lib")
#pragma comment(lib, "absl_raw_hash_set.lib")
#pragma comment(lib, "absl_str_format_internal.lib")
#pragma comment(lib, "absl_strings.lib")
#pragma comment(lib, "absl_synchronization.lib")
#pragma comment(lib, "absl_raw_logging_internal.lib")
#else
#pragma message ("Ayria 0.9 relies on Abseil, remember to update VCPKG.")
#endif
#pragma warning(pop)
