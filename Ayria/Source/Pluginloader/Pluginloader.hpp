/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-17
    License: MIT
*/

#pragma once
#include <cstdint>

namespace Plugins
{
    // Different types of hooking.
    bool InstallTLSHook();
    bool InstallEPHook();

    // Helpers that may be useful elsewhere.
    uintptr_t getEntrypoint();
    uintptr_t getTLSEntry();

    // Simply load all plugins from disk.
    void Initialize();
}
