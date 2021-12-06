/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-06-18
    License: MIT

    A header to make dependency-management easier.
    Make sure that you compile with /opt:ref..
*/

#pragma once
#pragma warning(push, 0)

// Lazy iterators, required.
#define LZ_STANDALONE
#define HAS_LZ
#include <Lz/Lz.hpp>

// NLhohmann modern JSON parsing library.
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#define HAS_NLOHMANN
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
#define HAS_OPENSSL

// BoringSSL version.
#if __has_include(<openssl/curve25519.h>)
    #pragma comment(lib, "crypto.lib")
    #pragma comment(lib, "ssl.lib")
#else
    #pragma comment(lib, "libcrypto.lib")
    #pragma comment(lib, "libssl.lib")
#endif
#endif

// STB image decompression for assets.
#if __has_include(<stb_image.h>)
#define HAS_STB_IMAGE
#include <stb.h>
#include <stb_image.h>
#endif

// Generic hooking library.
#if __has_include(<MinHook.h>)
#define HAS_MINHOOK
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(NDEBUG)
    #pragma comment(lib, "minhook.x64.lib")
    #else
    #pragma comment(lib, "minhook.x64d.lib")
    #endif
#else
    #if defined(NDEBUG)
    #pragma comment(lib, "minhook.x32.lib")
    #else
    #pragma comment(lib, "minhook.x32d.lib")
    #endif
#endif
#endif

// Extensive disassembler library.
#if __has_include(<capstone/capstone.h>)
#pragma comment(lib, "capstone.lib")
#define HAS_CAPSTONE
#endif

// Advanced hooking library.
#if __has_include(<polyhook2/IHook.hpp>)
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
#if __has_include(<absl/base/config.h>)
#define HAS_ABSEIL

#include <absl/container/inlined_vector.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/node_hash_map.h>
#include <absl/container/node_hash_set.h>
#include <absl/synchronization/mutex.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_split.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_cat.h>
#include <absl/random/random.h>

// NOTE(tcn): Can't be arsed to update these, so just ls | grep all and let the linker figure it out if it fails..
#pragma comment(lib, "absl_bad_any_cast_impl.lib")
#pragma comment(lib, "absl_bad_optional_access.lib")
#pragma comment(lib, "absl_bad_variant_access.lib")
#pragma comment(lib, "absl_base.lib")
#pragma comment(lib, "absl_city.lib")
#pragma comment(lib, "absl_civil_time.lib")
#pragma comment(lib, "absl_cord.lib")
#pragma comment(lib, "absl_debugging_internal.lib")
#pragma comment(lib, "absl_demangle_internal.lib")
#pragma comment(lib, "absl_examine_stack.lib")
#pragma comment(lib, "absl_exponential_biased.lib")
#pragma comment(lib, "absl_failure_signal_handler.lib")
#pragma comment(lib, "absl_flags.lib")
#pragma comment(lib, "absl_flags_commandlineflag.lib")
#pragma comment(lib, "absl_flags_commandlineflag_internal.lib")
#pragma comment(lib, "absl_flags_config.lib")
#pragma comment(lib, "absl_flags_internal.lib")
#pragma comment(lib, "absl_flags_marshalling.lib")
#pragma comment(lib, "absl_flags_parse.lib")
#pragma comment(lib, "absl_flags_private_handle_accessor.lib")
#pragma comment(lib, "absl_flags_program_name.lib")
#pragma comment(lib, "absl_flags_reflection.lib")
#pragma comment(lib, "absl_flags_usage.lib")
#pragma comment(lib, "absl_flags_usage_internal.lib")
#pragma comment(lib, "absl_graphcycles_internal.lib")
#pragma comment(lib, "absl_hash.lib")
#pragma comment(lib, "absl_hashtablez_sampler.lib")
#pragma comment(lib, "absl_int128.lib")
#pragma comment(lib, "absl_leak_check.lib")
#pragma comment(lib, "absl_leak_check_disable.lib")
#pragma comment(lib, "absl_log_severity.lib")
#pragma comment(lib, "absl_malloc_internal.lib")
#pragma comment(lib, "absl_periodic_sampler.lib")
#pragma comment(lib, "absl_random_distributions.lib")
#pragma comment(lib, "absl_random_internal_distribution_test_util.lib")
#pragma comment(lib, "absl_random_internal_platform.lib")
#pragma comment(lib, "absl_random_internal_pool_urbg.lib")
#pragma comment(lib, "absl_random_internal_randen.lib")
#pragma comment(lib, "absl_random_internal_randen_hwaes.lib")
#pragma comment(lib, "absl_random_internal_randen_hwaes_impl.lib")
#pragma comment(lib, "absl_random_internal_randen_slow.lib")
#pragma comment(lib, "absl_random_internal_seed_material.lib")
#pragma comment(lib, "absl_random_seed_gen_exception.lib")
#pragma comment(lib, "absl_random_seed_sequences.lib")
#pragma comment(lib, "absl_raw_hash_set.lib")
#pragma comment(lib, "absl_raw_logging_internal.lib")
#pragma comment(lib, "absl_scoped_set_env.lib")
#pragma comment(lib, "absl_spinlock_wait.lib")
#pragma comment(lib, "absl_stacktrace.lib")
#pragma comment(lib, "absl_status.lib")
#pragma comment(lib, "absl_statusor.lib")
#pragma comment(lib, "absl_strerror.lib")
#pragma comment(lib, "absl_strings.lib")
#pragma comment(lib, "absl_strings_internal.lib")
#pragma comment(lib, "absl_str_format_internal.lib")
#pragma comment(lib, "absl_symbolize.lib")
#pragma comment(lib, "absl_synchronization.lib")
#pragma comment(lib, "absl_throw_delegate.lib")
#pragma comment(lib, "absl_time.lib")
#pragma comment(lib, "absl_time_zone.lib")
#pragma comment(lib, "absl_wyhash.lib")

template <class K, class V,
          class Hash = absl::container_internal::hash_default_hash<K>,
          class Eq = absl::container_internal::hash_default_eq<K>>
using Hashmap = absl::flat_hash_map<K, V, Hash, Eq>;

template <class Key, class Value,
          class Hash = absl::container_internal::hash_default_hash<Key>,
          class Eq = absl::container_internal::hash_default_eq<Key>>
using Nodemap = absl::node_hash_map<Key, Value, Hash, Eq>;

template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>>
using Hashset = absl::flat_hash_set<T, Hash, Eq>;

template<typename T, size_t N>
using Inlinedvector = absl::InlinedVector<T, N>;

#else

template <class _Kty, class _Ty, class _Hasher = std::hash<_Kty>, class _Keyeq = std::equal_to<_Kty>>
using Hashmap = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq>;

template <class _Kty, class _Ty, class _Hasher = std::hash<_Kty>, class _Keyeq = std::equal_to<_Kty>>
using Nodemap = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq>;

template <class _Kty, class _Hasher = std::hash<_Kty>, class _Keyeq = std::equal_to<_Kty>>
using Hashset = std::unordered_set<_Kty, _Hasher, _Keyeq>;

template <class _Ty, size_t N>
using Inlinedvector = std::vector<_Ty>;

#endif
#pragma warning(pop)
