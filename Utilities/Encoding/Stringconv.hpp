/*
Initial author: Convery (tcn@ayria.se)
Started: 2020-09-23
License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>
#include "Stringconv.hpp"
#include "Variadicstring.hpp"

namespace Encoding
{
    using Controlcode_t = uint8_t;
    using Codepoint_t = uint32_t;

    namespace UTF8
    {
        constexpr bool isASCII(std::u8string_view Input)
        {
            const auto Count32 = Input.size() / 4;
            const auto Remaining = Input.size() & 3;
            const auto Intptr = (uint32_t *)Input.data();

            for (size_t i = 0; i < Count32; ++i)
                if (Intptr[i] & 0x80808080)
                    return false;

            // Don't care about over-reads.
            if (Remaining == 3) return !(Intptr[Count32] & 0x00800080);
            if (Remaining == 2) return !(Intptr[Count32] & 0x00008080);
            if (Remaining == 1) return !(Intptr[Count32] & 0x00000080);
            return true;
        }
        constexpr size_t Codepointsize(Controlcode_t Code)
        {
            if ((Code & 0x80) == 0x00) return 1;
            if ((Code & 0xE0) == 0xC0) return 2;
            if ((Code & 0xF0) == 0xE0) return 3;
            if ((Code & 0xF8) == 0xF0) return 4;
            if ((Code & 0xFC) == 0xF8) return 5;
            if ((Code & 0xFE) == 0xFC) return 6;
            return 1;   // Fallback.
        }
        inline std::u8string toUTF8Chars(Codepoint_t Codepoint)
        {
            std::u8string Result{};
            Result.reserve(6);  // Worst case.

            do
            {
                // 8-bit.
                if (Codepoint < 0x80)
                {
                    Result.push_back(static_cast<uint8_t>(Codepoint));
                    break;
                }

                // 11-bit.
                if (Codepoint < 0x800)
                {
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x7C0) >> 6) | 0xC0));
                    Result.push_back(static_cast<uint8_t>((Codepoint & 0x3F) | 0x80));
                    break;
                }

                // 16-bit.
                if (Codepoint < 0x10000)
                {
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xF000) >> 12) | 0xE0));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC0) >> 6) | 0x80));
                    Result.push_back(static_cast<uint8_t>((Codepoint & 0x3F) | 0x80));
                    break;
                }

                // 21-bit.
                if (Codepoint < 0x200000)
                {
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x1C0000) >> 18) | 0xF0));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x3F000) >> 12) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC0) >> 6) | 0x80));
                    Result.push_back(static_cast<uint8_t>((Codepoint & 0x3F) | 0x80));
                    break;
                }

                // 26-bit.
                if (Codepoint < 0x4000000)
                {
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x3000000) >> 24) | 0xF8));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC000) >> 18) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x3F000) >> 12) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC0) >> 6) | 0x80));
                    Result.push_back(static_cast<uint8_t>((Codepoint & 0x3F) | 0x80));
                    break;
                }

                // 30-bit.
                if (Codepoint < 0x80000000)
                {
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x40000000) >> 30) | 0xFC));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x3F000000) >> 24) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC000) >> 18) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0x3F000) >> 12) | 0x80));
                    Result.push_back(static_cast<uint8_t>(((Codepoint & 0xFC0) >> 6) | 0x80));
                    Result.push_back(static_cast<uint8_t>((Codepoint & 0x3F) | 0x80));
                    break;
                }

            } while (false);

            return Result;
        }

        inline size_t Strlen(std::u8string_view Input)
        {
            if (isASCII(Input)) return Input.size();

            size_t Size{};
            for (const auto &Byte : Input)
                Size += (Byte & 0xC0) != 0x80;

            return Size;
        }
        inline size_t Offset(std::u8string_view Input, size_t Index)
        {
            for (auto it = Input.begin(); it != Input.end();)
            {
                if (Index-- > 0) it += Codepointsize(*it);
                else return it - Input.begin();
            }

            return Input.size();
        }


        inline std::u8string::iterator at(std::u8string &Input, size_t Index)
        {
            for (auto it = Input.begin(); it != Input.end();)
            {
                if (Index-- > 0) it += Codepointsize(*it);
                else return it;
            }

            return Input.end();
        }
        inline std::u8string_view::iterator at(std::u8string_view Input, size_t Index)
        {
            for (auto it = Input.begin(); it != Input.end();)
            {
                if (Index-- > 0) it += Codepointsize(*it);
                else return it;
            }

            return Input.end();
        }
        inline std::u8string_view Substr(std::u8string_view Input, size_t Start, size_t Stop)
        {
            const auto pStop = Offset(Input, Stop);
            const auto pStart = Offset(Input, Start);
            if (pStart == (size_t)-1) return {};

            return std::u8string_view(&Input[pStart], pStop - pStart);
        }
    }

    [[nodiscard]] inline std::u8string toUTF8(std::string_view Input)
    {
        // Do we even have any code-points to process?
        if (Input.find("\\u") == static_cast<size_t>(-1)) [[likely]]
            return std::u8string((char8_t *)Input.data(), Input.size());

        // Common case is ASCII with the code-points being smaller than text.
        std::u8string Result{}; Result.reserve(Input.size());
        Codepoint_t Extendedpoint{};

        while (true)
        {
            const auto Point = Input.find("\\u");
            if (Point == static_cast<size_t>(-1)) break;

            // ASCII part.
            Result.append((char8_t *)Input.data(), Point);
            Input.remove_prefix(Point + 2);

            // U+0000
            const auto Codepoint = std::strtol((char *)Input.data(), nullptr, 16);
            Input.remove_prefix(4);

            // 32 bit code.
            if (0xD800 <= Codepoint && Codepoint <= 0xDBFF) [[unlikely]]
            {
                Extendedpoint = Codepoint << 10;
            }
            else
            {
                if (!Extendedpoint) Result.append(UTF8::toUTF8Chars(Codepoint));
                else
                {
                    Extendedpoint += Codepoint;
                    Extendedpoint -= 0x35FDC00U;
                    Result.append(UTF8::toUTF8Chars(Extendedpoint));
                    Extendedpoint = 0;
                }
            }
        }

        Result.append((char8_t *)Input.data(), Input.size());
        return Result;
    }
    [[nodiscard]] inline std::u8string toUTF8(std::wstring_view Input)
    {
        // Common case is ASCII, so we pre-allocate for it, next realloc should be enough for 11-bit UTF.
        std::u8string Result{}; Result.reserve(Input.size());

        for (const auto &WChar : Input)
            Result.append(UTF8::toUTF8Chars(WChar));

        return Result;
    }
    [[nodiscard]] inline std::u8string toUTF8(std::u8string_view Input)
    {
        return std::u8string(Input.data(), Input.size());
    }

    [[nodiscard]] inline std::wstring toWide(std::string_view Input)
    {
        const auto Size = std::mbstowcs(NULL, Input.data(), 0);
        std::wstring Result(Size, 0);
        std::mbstowcs(Result.data(), Input.data(), Size);
        return Result;
    }
    [[nodiscard]] inline std::string toNarrow(std::wstring_view Input)
    {
        std::string Result(Input.size(), 0);
        auto Pointer = Result.data();

        for (size_t i = 0; i < Input.size(); ++i)
        {
            Pointer[i] = Input[i] > 0x7F ? '?' : static_cast<char>(Input[i] & 0x7F);
        }

        return Result;
    }

    [[nodiscard]] inline std::wstring toWide(std::u8string_view Input)
    {
        std::wstring Result{}; Result.reserve(Input.size());

        for (size_t i = 0; i < Input.size(); )
        {
            const auto Controlbyte = Input[i++];

            // 8-bit.
            if ((Controlbyte & 0x80) == 0)
            {
                Result.push_back(static_cast<wchar_t>(Controlbyte));
                continue;
            }

            // 11-bit.
            if ((Controlbyte & 0xE0) == 0xC0)
            {
                const auto Databyte = Input[i++];
                Result.push_back(static_cast<wchar_t>(((Controlbyte & 0x1F) << 6) | (Databyte & 0x3F)));
                continue;
            }

            // 16-bit.
            if ((Controlbyte & 0xF0) == 0xE0)
            {
                const auto Databyte = Input[i++];
                const auto Extrabyte = Input[i++];
                Result.push_back(static_cast<wchar_t>(((Controlbyte & 0x0F) << 12) | ((Databyte & 0x3F) << 6) | (Extrabyte & 0x3F)));
                continue;
            }

            // 21-bit.
            if ((Controlbyte & 0xF8) == 0xF0)
            {
                const auto Databyte = Input[i++];
                const auto Extrabyte = Input[i++];
                const auto Triplebyte = Input[i++];

                if constexpr (sizeof(wchar_t) == 4)
                    Result.push_back(static_cast<wchar_t>(((Controlbyte & 0x07) << 18) | ((Databyte & 0x3F) << 12)
                                         | ((Extrabyte & 0x3F) << 6) | (Triplebyte & 0x3F)));
                continue;
            }

            // 26-bit.
            if ((Controlbyte & 0xFC) == 0xF8)
            {
                const auto Databyte = Input[i++];
                const auto Extrabyte = Input[i++];
                const auto Triplebyte = Input[i++];
                const auto Quadbyte = Input[i++];

                if constexpr (sizeof(wchar_t) == 4)
                    Result.push_back(static_cast<wchar_t>(((Controlbyte & 0x03) << 24) | ((Databyte & 0x3F) << 18) |
                                         ((Extrabyte & 0x3F) << 12) | ((Triplebyte & 0x3F) << 6)
                                         | (Quadbyte & 0x3F)));
                continue;
            }

            // 30-bit.
            if ((Controlbyte & 0xFE) == 0xFC)
            {
                const auto Databyte = Input[i++];
                const auto Extrabyte = Input[i++];
                const auto Triplebyte = Input[i++];
                const auto Quadbyte = Input[i++];
                const auto Pentabyte = Input[i++];

                if constexpr (sizeof(wchar_t) == 4)
                    Result.push_back(static_cast<wchar_t>(((Controlbyte & 0x01) << 30) | ((Databyte & 0x3F) << 24) |
                                         ((Extrabyte & 0x3F) << 18) | ((Triplebyte & 0x3F) << 12)
                                         | ((Quadbyte & 0x3F) << 6) | (Pentabyte & 0x3F)));
                continue;
            }

            // No header info, probably a raw codepoint.
            Result.push_back(static_cast<wchar_t>(Controlbyte));
        }

        return Result;
    }
    [[nodiscard]] inline std::string toNarrow(std::u8string_view Input)
    {
        if (UTF8::isASCII(Input)) return std::string(Input.begin(), Input.end());

        std::string Result;
        Result.reserve(Input.size() * sizeof(wchar_t));

        for (const auto &Char : toWide(Input))
        {
            if (Char < 0x80) Result.push_back(Char & 0x7F);
            else
            {
                if constexpr (sizeof(wchar_t) == 2)
                    Result.append(va("\\u%04x", Char));
                else
                    Result.append(va("\\u%04x\\u%04x", 0xD7C0U + (Char >> 10), 0xDC00U + (Char & 0x3FF)));
            }
        }

        return Result;
    }
}
