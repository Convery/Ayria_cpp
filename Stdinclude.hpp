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
#include <string_view>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <queue>
#include <any>

// Platform-specific libraries.
#if defined(_WIN32)
#include <WinSock2.h>
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

// Third-party includes, usually included via VCPKG.
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#endif
#if __has_include(<mhook-lib/mhook.h>)
#include <mhook-lib/mhook.h>
#endif

// Restore warnings.
#pragma warning(pop)

// Global utilities.
#include <Utilities/Variadicstring.hpp>
#include <Utilities/Patternscan.hpp>
#include <Utilities/Bytebuffer.hpp>
#include <Utilities/Memprotect.hpp>
#include <Utilities/Filesystem.hpp>
#include <Utilities/FNV1Hash.hpp>
#include <Utilities/Logging.hpp>
#include <Utilities/Base64.hpp>
#include <Utilities/IPC.hpp>

// Extensions to the language.
using namespace std::string_literals;
