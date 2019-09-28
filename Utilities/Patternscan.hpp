/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-28
    License: MIT

    const auto [Textsegment, Datasegment] = Patternscan::Defaultranges();
    for(const auto &Address : Patternscan::Findpatterns(Datasegment, "25 64 ? 25")) {}
*/

#pragma once
#include <Stdinclude.hpp>

// GCC exported.
#if !defined(_WIN32)
extern char _etext, _end;
#endif

namespace Patternscan
{
    using Patternmask_t = std::basic_string<uint8_t>;
    using Patternmask_view = std::basic_string_view<uint8_t>;
    using Range_t = std::pair<size_t, size_t>;

    // Find a single pattern in a range.
    inline size_t Findpattern(Range_t &Range, Patternmask_view Pattern, Patternmask_view Mask)
    {
        assert(Pattern.size() == Mask.size());
        assert(Pattern.size() != 0);
        assert(Range.second != 0);
        assert(Range.first != 0);

        size_t Count = Range.second - Range.first - Pattern.size();
        auto Base = (const uint8_t *)Range.first;
        uint8_t Firstbyte = Pattern[0];

        // Inline compare.
        auto Compare = [&](const uint8_t *Address) -> bool
        {
            const size_t Patternlength = Pattern.size();
            for(size_t i = 0; i < Patternlength; ++i)
            {
                if(Mask[i] && Address[i] != Pattern[i])
                    return false;
            }
            return true;
        };

        // Iterate over the range.
        for(size_t Index = 0; Index < Count; ++Index)
        {
            if(Base[Index] != Firstbyte)
                continue;

            if(Compare(Base + Index))
                return Range.first + Index;
        }

        return 0;
    }

    // Scan until the end of the range and return all results.
    inline std::vector<size_t> Findpatterns(Range_t &Range, Patternmask_view Pattern, Patternmask_view Mask)
    {
        std::vector<std::size_t> Results;
        Range_t Localrange = Range;
        size_t Lastresult = 0;

        while(true)
        {
            Lastresult = Findpattern(Localrange, Pattern, Mask);

            if(Lastresult == 0)
                break;

            Localrange.first = Lastresult + 1;
            Results.push_back(Lastresult);
        }

        return Results;
    }

    // Calculate the hosts default ranges, once per compilation-module on some compilers.
    inline std::pair<Range_t /* .text */, Range_t /* .data */> Defaultranges()
    {
        /*
            NOTE(tcn):
            While the GCC exports defined above should be available on
            OSX, I can not confirm that they actually do exist.
            This code may be preferable:

            #include <mach-o/getsect.h>

            Textsegment.second = size_t(get_etext());
            Datasegment.first = size_t(get_etext());
            Datasegment.second = size_t(get_end());
        */

        static Range_t Textsegment{}, Datasegment{};
        if(Textsegment != Datasegment) return { Textsegment, Datasegment };

        #if defined(_WIN32)
            HMODULE Module = GetModuleHandleA(NULL);
            if(!Module) return { Textsegment, Datasegment };

            SYSTEM_INFO SI;
            GetNativeSystemInfo(&SI);

            PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Module;
            PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Module + DOSHeader->e_lfanew);

            Textsegment.first = size_t(Module) + NTHeader->OptionalHeader.BaseOfCode;
            Textsegment.second = Textsegment.first + NTHeader->OptionalHeader.SizeOfCode;
            Textsegment.second -= Textsegment.second % SI.dwPageSize;

            Datasegment.first = Textsegment.second;
            Datasegment.second = Datasegment.first + NTHeader->OptionalHeader.SizeOfInitializedData;
            Datasegment.second -= Datasegment.second % SI.dwPageSize;
        #else
            Textsegment.first = *(size_t *)dlopen(NULL, RTLD_LAZY);
            Textsegment.second = size_t(&_etext);
            Datasegment.first = size_t(&_etext);
            Datasegment.second = size_t(&_end);
        #endif

        return { Textsegment, Datasegment };
    }

    // Create a pattern or mask from a readable string.
    inline Patternmask_t from_string(std::string_view Readable)
    {
        uint32_t Count{ 0 };
        Patternmask_t Result;
        Result.reserve(Readable.size() >> 1);
        const char Hex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };

        for(const auto &Item : Readable)
        {
            if(Item == ' ') { Count = 0; continue; }
            if(Item == '?') Result.push_back('\x00');
            else
            {
                if(Count++ & 1) Result.back() = Result.back() << 4 | Hex[Item - 0x30];
                else Result.push_back(Hex[Item - 0x30]);
            }
        }
        return Result;
    }

    // IDA style pattern scanning.
    inline std::vector<size_t> Findpatterns(Range_t &Range, std::string_view Pattern)
    {
        const auto Patternmask = from_string(Pattern);
        return Findpatterns(Range, Patternmask, Patternmask);
    }
}
