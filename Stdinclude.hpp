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
#include <memory_resource>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <execution>
#include <concepts>
#include <optional>
#include <cassert>
#include <cstdint>
#include <numbers>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdio>
#include <format>
#include <future>
#include <memory>
#include <ranges>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <queue>
#include <regex>
#include <tuple>
#include <span>
#include <any>
#include <set>

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
#include <Utilities/Containers/Bitbuffer.hpp>
#include <Utilities/Containers/Bytebuffer.hpp>
#include <Utilities/Containers/Ringbuffer.hpp>
#include <Utilities/Containers/SQLitewrapper.hpp>
#include <Utilities/Crypto/Hashes.hpp>
#include <Utilities/Crypto/OpenSSLWrappers.hpp>
#include <Utilities/Crypto/Tiger192Hash.hpp>
#include <Utilities/Crypto/qDSA.hpp>
#include <Utilities/Encoding/Base58.hpp>
#include <Utilities/Encoding/Base64.hpp>
#include <Utilities/Encoding/Base85.hpp>
#include <Utilities/Encoding/JSON.hpp>
#include <Utilities/Encoding/Stringconv.hpp>
#include <Utilities/Encoding/Variadicstring.hpp>
#include <Utilities/Graphics/Overlay.hpp>
#include <Utilities/Hacking/Hooking.hpp>
#include <Utilities/Hacking/Memprotect.hpp>
#include <Utilities/Hacking/Patternscan.hpp>
#include <Utilities/Threads/Spinlock.hpp>
#include <Utilities/Threads/Debugmutex.hpp>
#include <Utilities/Wrappers/Logging.hpp>
#include <Utilities/Wrappers/Filesystem.hpp>


#include <Utilities/Ayria/AyriaAPI.hpp>
#include <Utilities/Ayria/Ayriamodule.h>
#include <Utilities/Ayria/Localnetservers.h>

// Temporary includes.
#include <Utilities/Internal/Misc.hpp>
#include <Utilities/Internal/Asynctaskqueue.hpp>
#include <Utilities/Internal/Compressedstring.hpp>
