/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-01-02
    License: MIT

    Work in progress, for internal use only.
    Hyrum's law will not be respected.
*/

#pragma once
#include <Stdinclude.hpp>

namespace IPC
{
    struct Filemap_t
    {
        uint32_t Generation;
        uint32_t Mapsize;
        char Base64data;
    };

    inline Filemap_t *Createmap(std::string_view Name, uint32_t Mapsize)
    {
        assert(Mapsize > 8);

        if (auto Mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, Mapsize, Name.data()))
        {
            if (auto Buffer = (Filemap_t *)MapViewOfFile(Mapping, FILE_MAP_ALL_ACCESS, 0, 0, Mapsize))
            {
                std::memset(Buffer, 0, Mapsize);
                Buffer->Mapsize = Mapsize;
                Buffer->Generation = 1;
                return Buffer;
            }
        }

        return nullptr;
    };
    inline bool Updatemapping(std::string_view Name, std::string_view Data)
    {
        if (auto Mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Name.data()))
        {
            if (auto Buffer = (Filemap_t *)MapViewOfFile(Mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0))
            {
                const std::string B64 = Base64::isValid(Data.data()) ? Data.data() : Base64::Encode(Data);
                if (B64.size() < Buffer->Mapsize)
                {
                    std::memcpy(&Buffer->Base64data, B64.data(), B64.size());
                    Buffer->Generation++;
                }
                UnmapViewOfFile((void *)Buffer);
                return true;
            }
        }

        return false;
    }
}
