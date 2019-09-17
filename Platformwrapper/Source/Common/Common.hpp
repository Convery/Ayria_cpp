/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Ayria
{
    // Keep the global state together.
    #pragma pack(push, 1)
    struct Globalstate_t
    {
        uint64_t UserID;
        std::string Username;
        uint64_t Startuptimestamp;
    };
    extern Globalstate_t Global;
    #pragma pack(pop)

    // Helper to create 'classes' when needed.
    using Fakeclass_t = struct { void *VTABLE[70]; };
}
