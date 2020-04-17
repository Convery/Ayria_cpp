/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-06
    License: MIT

    Keep a single instance of a tool across modules.
*/

#pragma once
#include <Stdinclude.hpp>

namespace Singleinstance
{
    inline void *Create(std::string_view Sharedname, void *Instance)
    {
        const auto Name = va("%u_%*s", GetCurrentProcessId(), Sharedname.size(), Sharedname.data());

        if (const auto Mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(long long), Name.c_str()))
        {
            if (const auto Buffer = (long long *)MapViewOfFile(Mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(long long)))
            {
                const auto Original = (void *)InterlockedCompareExchange64(Buffer, (long long)Instance, {});
                UnmapViewOfFile(Buffer);

                if (Original) return Original;
                else return Instance;
            }
        }

        return nullptr;
    }
}
