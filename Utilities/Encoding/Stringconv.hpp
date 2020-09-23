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

        // 16-bit.
        if (WChar < 0x800)
        {
            Result.push_back(uint8_t(((WChar & 0x7C0) >> 6) | 0xC0));
            Result.push_back(uint8_t((WChar & 0x3F) | 0x80));
            continue;
        }

        // 24-bit.
        if (WChar < 0x10000)
        {
            Result.push_back(uint8_t(((WChar & 0xF000) >> 12) | 0xE0));
            Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
            Result.push_back(uint8_t((WChar & 0x3F) | 0x80));
            continue;
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

        // 16-bit.
        if ((Controlbyte & 0xE0) == 0xC0)
        {
            const auto Databyte = Input[i++];
            Result.push_back(wchar_t(((Controlbyte & 0x1F) << 6) | (Databyte & 0x3F)));
            continue;
        }

        // 24-bit.
        if ((Controlbyte & 0xF0) == 0xE0)
        {
            const auto Databyte = Input[i++];
            const auto Extrabyte = Input[i++];
            Result.push_back(wchar_t(((Controlbyte & 0x0F) << 12) | ((Databyte & 0x3F) << 6) | (Extrabyte & 0x3F)));
            continue;
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
    std::string Result{}; Result.reserve(Input.size());

    for (const auto &WChar : Input)
    {
        Result.push_back(WChar > 0xFF ? '_' : char(WChar & 0xFF));
    }

    return Result;
}
[[nodiscard]] inline std::string toNarrow(const std::wstring &Input)
{
    return toNarrow(std::wstring_view(Input));
}

[[nodiscard]] inline std::wstring toWide(std::string_view Input)
{
    std::wstring Result{};

    const auto Size = std::mbstowcs(NULL, Input.data(), 0);
    Result.resize(Size);
    std::mbstowcs(Result.data(), Input.data(), Size);
    return Result;
}
[[nodiscard]] inline std::wstring toWide(const std::string &Input)
{
    return toWide(std::string_view(Input));
}
