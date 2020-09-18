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
#include <variant>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <queue>
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

// Global utilities.
#include <Utilities/Crypto/FNV1Hash.hpp>
#include <Utilities/Crypto/CRC32Hash.hpp>
#include <Utilities/Crypto/OpenSSLWrappers.hpp>
#include <Utilities/Crypto/Tiger192Hash.hpp>
#include <Utilities/Encoding/Base64.hpp>
#include <Utilities/Encoding/Bitbuffer.hpp>
#include <Utilities/Encoding/Bytebuffer.hpp>
#include <Utilities/Encoding/Variadicstring.hpp>
#include <Utilities/Encoding/Widestring.hpp>
#include <Utilities/Hacking/Branchless.hpp>
#include <Utilities/Hacking/Hooking.hpp>
#include <Utilities/Hacking/Memprotect.hpp>
#include <Utilities/Hacking/Patternscan.hpp>
#include <Utilities/Wrappers/Logging.hpp>
#include <Utilities/Wrappers/Filesystem.hpp>

// Temporary includes.
#include <Utilities/Internal/Spinlock.hpp>
#include <Utilities/Internal/Debugmutex.hpp>
#include <Utilities/Internal/Singleinstance.hpp>

// Extensions to the language.
using namespace std::string_literals;
