/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-23
    License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>

[[nodiscard]] inline std::u8string toUTF8(std::wstring_view Input)
{
    std::u8string Result{}; Result.reserve(Input.size());

    for (const auto &WChar : Input)
    {
        // 8-bit.
        if (WChar < 0x80)
        {
            Result.push_back(uint8_t(WChar));
            continue;
        }

        // 11-bit.
        if (WChar < 0x800)
        {
            Result.push_back(uint8_t(((WChar & 0x7C0) >> 6) | 0xC0));
            Result.push_back(uint8_t((WChar & 0x3F) | 0x80));
            continue;
        }

        // 16-bit.
        if (WChar < 0x10000)
        {
            Result.push_back(uint8_t(((WChar & 0xF000) >> 12) | 0xE0));
            Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
            Result.push_back(uint8_t((WChar & 0x3F) | 0x80));
            continue;
        }

        // Nix / OSX.
        if constexpr (sizeof(wchar_t) == 4)
        {
            // 21-bit.
            if (WChar < 0x200000)
            {
                Result.push_back(uint8_t(((WChar & 0x1C0000) >> 18) | 0xF0));
                Result.push_back(uint8_t(((WChar & 0x3F000) >> 12) | 0x80));
                Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
                Result.push_back(uint8_t(((WChar & 0x3F) | 0x80)));
                continue;
            }

            // 26-bit.
            if(WChar < 0x4000000)
            {
                Result.push_back(uint8_t(((WChar & 0x3000000) >> 24) | 0xF8));
                Result.push_back(uint8_t(((WChar & 0xFC000) >> 18) | 0x80));
                Result.push_back(uint8_t(((WChar & 0x3F000) >> 12) | 0x80));
                Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
                Result.push_back(uint8_t(((WChar & 0x3F) | 0x80)));
                continue;
            }

            // 30-bit.
            if(WChar < 0x80000000)
            {
                Result.push_back(uint8_t(((WChar & 0x40000000) >> 30) | 0xFC));
                Result.push_back(uint8_t(((WChar & 0x3F000000) >> 24) | 0x80));
                Result.push_back(uint8_t(((WChar & 0xFC000) >> 18) | 0x80));
                Result.push_back(uint8_t(((WChar & 0x3F000) >> 12) | 0x80));
                Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
                Result.push_back(uint8_t(((WChar & 0x3F) | 0x80)));
                continue;
            }
        }
    }

    return Result;
}
[[nodiscard]] inline std::u8string toUTF8(const std::wstring &Input)
{
    return toUTF8(std::wstring_view(Input));
}

[[nodiscard]] inline std::wstring fromUTF8(std::u8string_view Input)
{
    std::wstring Result{}; Result.reserve(Input.size());

    for (size_t i = 0; i < Input.size(); )
    {
        const auto Controlbyte = Input[i++];

        // 8-bit.
        if ((Controlbyte & 0x80) == 0)
        {
            Result.push_back(wchar_t(Controlbyte));
            continue;
        }

        // 11-bit.
        if ((Controlbyte & 0xE0) == 0xC0)
        {
            const auto Databyte = Input[i++];
            Result.push_back(wchar_t(((Controlbyte & 0x1F) << 6) | (Databyte & 0x3F)));
            continue;
        }

        // 16-bit.
        if ((Controlbyte & 0xF0) == 0xE0)
        {
            const auto Databyte = Input[i++];
            const auto Extrabyte = Input[i++];
            Result.push_back(wchar_t(((Controlbyte & 0x0F) << 12) | ((Databyte & 0x3F) << 6) | (Extrabyte & 0x3F)));
            continue;
        }

        // Nix / OSX.
        if constexpr (sizeof(wchar_t) == 4)
        {
            // 21-bit.
            if ((Controlbyte & 0xF8) == 0xF0)
            {
                const auto Databyte = Input[i++];
                const auto Extrabyte = Input[i++];
                const auto Triplebyte = Input[i++];
                Result.push_back(wchar_t(((Controlbyte & 0x07) << 18) | ((Databyte & 0x3F) << 12)
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
                Result.push_back(wchar_t(((Controlbyte & 0x03) << 24) | ((Databyte & 0x3F) << 18) |
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
                Result.push_back(wchar_t(((Controlbyte & 0x01) << 30) | ((Databyte & 0x3F) << 24) |
                    ((Extrabyte & 0x3F) << 18) | ((Triplebyte & 0x3F) << 12)
                    | ((Quadbyte & 0x3F) << 6) | (Pentabyte & 0x3F)));
                continue;
            }
        }
    }

    return Result;
}
[[nodiscard]] inline std::wstring fromUTF8(const std::u8string &Input)
{
    return fromUTF8(std::u8string_view(Input));
}

[[nodiscard]] inline std::string toNarrow(std::wstring_view Input)
{
    std::string Result(Input.size(), 0);
    auto Pointer = Result.data();

    for (size_t i = 0; i < Input.size(); ++i)
    {
        Pointer[i] = Input[i] > 0xFF ? '_' : char(Input[i] & 0xFF);
    }

    return Result;
}
[[nodiscard]] inline std::string toNarrow(const std::wstring &Input)
{
    return toNarrow(std::wstring_view(Input));
}

[[nodiscard]] inline std::wstring toWide(std::string_view Input)
{
    const auto Size = std::mbstowcs(NULL, Input.data(), 0);
    std::wstring Result(Size, 0);
    std::mbstowcs(Result.data(), Input.data(), Size);
    return Result;
}
[[nodiscard]] inline std::wstring toWide(const std::string &Input)
{
    return toWide(std::string_view(Input));
}
