/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-28
    License: MIT

    const auto [Textsegment, Datasegment] = Defaultranges();
    for(const auto &Address : Findpatterns(Datasegment, "25 64 ? 25")) {}
*/

#pragma once
#include <Stdinclude.hpp>

// GCC exported.
#if !defined(_WIN32)
extern char _etext, _end;
#endif

namespace Patternscan
{
    using Range_t = std::pair<std::uintptr_t, std::uintptr_t>;
    using Patternmask_view = std::basic_string_view<uint8_t>;
    using Patternmask_t = std::basic_string<uint8_t>;

    // Find a single pattern in a range.
    [[nodiscard]] inline std::uintptr_t Findpattern(Range_t &Range, Patternmask_view Pattern, Patternmask_view Mask)
    {
        assert(Pattern.size() == Mask.size());
        assert(Pattern.size() != 0);
        assert(Range.second != 0);
        assert(Range.first != 0);
        assert(Mask[0] != 0); // The first byte can't be a wildcard.

        size_t Count = Range.second - Range.first - Pattern.size();
        auto Base = (const uint8_t *)Range.first;
        uint8_t Firstbyte = Pattern[0];

        // Inline compare.
        const auto Compare = [&](const uint8_t *Address) -> bool
        {
            const size_t Patternlength = Pattern.size();
            for(size_t i = 1; i < Patternlength; ++i)
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
    [[nodiscard]] inline std::vector<std::uintptr_t> Findpatterns(const Range_t Range, Patternmask_view Pattern, Patternmask_view Mask)
    {
        std::vector<std::uintptr_t> Results;
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
    [[nodiscard]] inline std::pair<Range_t /* .text */, Range_t /* .data */> Defaultranges(bool Force = false)
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
        if(Textsegment != Datasegment && !Force) return { Textsegment, Datasegment };

        #if defined(_WIN32)
            HMODULE Module = GetModuleHandleA(NULL);
            if(!Module) return { Textsegment, Datasegment };

            SYSTEM_INFO SI;
            GetNativeSystemInfo(&SI);

            PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Module;
            PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((std::uintptr_t)Module + DOSHeader->e_lfanew);

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
    [[nodiscard]] inline Range_t Virtualrange()
    {
        auto Currentpage = Defaultranges().second.second;
        Range_t Range{ Currentpage, 0 };

        #if defined(_WIN32)
        MEMORY_BASIC_INFORMATION Pageinformation{};
        while (VirtualQueryEx(GetCurrentProcess(), (LPCVOID)(Currentpage + 1), &Pageinformation, sizeof(MEMORY_BASIC_INFORMATION)))
        {
            if (Pageinformation.State != MEM_COMMIT) break;
            Currentpage = (size_t)Pageinformation.BaseAddress + Pageinformation.RegionSize;
        }
        #else
        assert(false);
        #endif

        Range.second = Currentpage;
        return Range;
    }

    // Create a pattern or mask from a readable string.
    [[nodiscard]] inline Patternmask_t from_string(const std::string_view Readable)
    {
        uint32_t Count{ 0 };
        Patternmask_t Result; Result.reserve(Readable.size() >> 1);
        constexpr char Hex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
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
    [[nodiscard]] inline std::vector<std::uintptr_t> Findpatterns(const Range_t Range, std::string IDAPattern)
    {
        const auto Pattern = from_string(IDAPattern);

        while(IDAPattern.find(" 0 ") != std::string::npos) IDAPattern.replace(IDAPattern.find(" 0 "), 4, " 00 ");
        while(IDAPattern.find("00") != std::string::npos) IDAPattern.replace(IDAPattern.find("00"), 2, "01" );

        return Findpatterns(Range, Pattern, from_string(IDAPattern));
    }

    // Find strings in memory - helper.
    [[nodiscard]] inline std::vector<std::uintptr_t> Findstrings(const Range_t Range, const std::string_view String)
    {
        const Patternmask_t Hexstring{ String.begin(), String.end() };
        return Findpatterns(Range, Hexstring, Hexstring);
    };
}
