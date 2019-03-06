/*
    Initial author: Convery (tcn@ayria.se)
    Started: 06-03-2018
    License: MIT
*/

#pragma once
#include <memory>
#include <cassert>
#include <cstdarg>
#include <string_view>

namespace
{
    // Truncate the string if needed.
    #if defined(VA_SIZE)
        constexpr ssize_t Defaultsize = VA_SIZE;
    #else
        constexpr ssize_t Defaultsize = 512;
    #endif

    inline ssize_t va(char *Buffer, ssize_t Size, const std::basic_string_view<char> &Format, std::va_list Varlist)
    {
        return std::vsnprintf(Buffer, Size, Format.data(), Varlist);
    }
}

inline std::basic_string<char> va(const std::basic_string_view<char> Format, ...)
{
    auto Buffer{ std::make_unique<char []>(Defaultsize) };
    std::va_list Varlist;
    ssize_t Size{};

    // Parse the argument-list.
    va_start(Varlist, Format);
    {
        // Try using the default size as it should work 99% of the time.
        Size = va(Buffer.get(), Defaultsize, Format, Varlist);

        // If the size is larger, we need to allocate again =(
        if (Size > Defaultsize)
        {
            Buffer = std::make_unique<char[]>(Size);
            Size = va(Buffer.get(), Size, Format, Varlist);
        }
    }
    va_end(Varlist);

    // Negative result on error.
    if (Size < 0) { assert(false); return {}; }
    assert(Size);

    // Take the memory with us.
    return { std::move(Buffer.get()), static_cast<size_t>(Size) };
}
