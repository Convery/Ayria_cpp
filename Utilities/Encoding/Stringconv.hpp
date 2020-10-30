/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-23
    License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>

namespace Encoding
{
    namespace UTF8
    {
        constexpr bool isASCII(std::u8string_view Input)
        {
            const auto Count32 = Input.size() / 4;
            const auto Remaining = Input.size() & 3;
            const auto Intptr = (uint32_t *)Input.data();

            for (size_t i = 0; i < Count32; ++i)
                if (Intptr[i] & 0xAAAA)
                    return false;

            // Don't care about over-reads.
            if (Remaining == 3) return Intptr[Count32] & 0x0AAA;
            if (Remaining == 2) return Intptr[Count32] & 0x00AA;
            if (Remaining == 1) return Intptr[Count32] & 0x000A;
            return true;
        }
        constexpr size_t Codepointsize(char8_t Controlcode)
        {
            if ((Controlcode & 0x80) == 0) return 1;
            if ((Controlcode & 0xE0) == 0xC0) return 2;
            if ((Controlcode & 0xF8) == 0xF0) return 3;
            if ((Controlcode & 0xFC) == 0xF8) return 4;
            if ((Controlcode & 0xFE) == 0xFC) return 5;
        }

        inline size_t Strlen(std::u8string_view Input)
        {
            if (isASCII(Input)) return Input.size();

            size_t Size{};
            for (const auto &Byte : Input)
                Size += (Byte & 0xC0) != 0x80;

            return Size;
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
            auto pStop = at(Input, Stop);
            auto pStart = at(Input, Start);
            if (pStart == Input.end()) return {};

            return std::u8string_view(&*pStart, std::distance(pStart, pStop));
        }
    }

    [[nodiscard]] inline std::u8string toUTF8(std::string_view Input)
    {
        return std::u8string((char8_t *)Input.data(), Input.size());
    }
    [[nodiscard]] inline std::u8string toUTF8(std::wstring_view Input)
    {
        // Common case is ASCII, so we pre-allocate for it, next realloc should be enough for 11-bit UTF.
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
                if (WChar < 0x4000000)
                {
                    Result.push_back(uint8_t(((WChar & 0x3000000) >> 24) | 0xF8));
                    Result.push_back(uint8_t(((WChar & 0xFC000) >> 18) | 0x80));
                    Result.push_back(uint8_t(((WChar & 0x3F000) >> 12) | 0x80));
                    Result.push_back(uint8_t(((WChar & 0xFC0) >> 6) | 0x80));
                    Result.push_back(uint8_t(((WChar & 0x3F) | 0x80)));
                    continue;
                }

                // 30-bit.
                if (WChar < 0x80000000)
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
            Pointer[i] = Input[i] > 0x7F ? '?' : char(Input[i] & 0x7F);
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
    [[nodiscard]] inline std::string toNarrow(std::u8string_view Input)
    {
        return UTF8::isASCII(Input) ?
            std::string(Input.begin(), Input.end()) : toNarrow(toWide(Input));
    }
}

struct UTF8Char_t
{
    wchar_t Internal{};

    UTF8Char_t() = default;
    UTF8Char_t(char Value) : Internal(Value) {}
    UTF8Char_t(wchar_t Value) : Internal(Value) {}
    UTF8Char_t(std::u8string_view Value) { *this = Value; }

    UTF8Char_t &operator=(std::u8string_view Input)
    {
        // Nix / OSX.
        if constexpr (sizeof(wchar_t) == 4)
        {
            switch (Encoding::UTF8::Codepointsize(Input.front()))
            {
                case 1:
                    Internal = wchar_t(Input[0]); break;
                case 2:
                    Internal = wchar_t(((Input[0] & 0x1F) << 6) | (Input[1] & 0x3F)); break;
                case 3:
                    Internal = wchar_t(((Input[0] & 0x0F) << 12) | ((Input[1] & 0x3F) << 6) | (Input[2] & 0x3F)); break;
                case 4:
                    Internal = wchar_t(((Input[0] & 0x07) << 18) | ((Input[1] & 0x3F) << 12)
                        | ((Input[2] & 0x3F) << 6) | (Input[3] & 0x3F)); break;
                case 5:
                    Internal = wchar_t(((Input[0] & 0x03) << 24) | ((Input[1] & 0x3F) << 18) |
                        ((Input[2] & 0x3F) << 12) | ((Input[3] & 0x3F) << 6)
                        | (Input[4] & 0x3F)); break;
                case 6:
                    Internal = wchar_t(((Input[0] & 0x01) << 30) | ((Input[1] & 0x3F) << 24) |
                        ((Input[2] & 0x3F) << 18) | ((Input[3] & 0x3F) << 12)
                        | ((Input[4] & 0x3F) << 6) | (Input[5] & 0x3F)); break;
            }
        }
        else
        {
            switch (Encoding::UTF8::Codepointsize(Input.front()))
            {
                case 1:
                    Internal = wchar_t(Input[0]); break;
                case 2:
                    Internal = wchar_t(((Input[0] & 0x1F) << 6) | (Input[1] & 0x3F)); break;
                case 3:
                    Internal = wchar_t(((Input[0] & 0x0F) << 12) | ((Input[1] & 0x3F) << 6) | (Input[2] & 0x3F)); break;
            }
        }

        return *this;
    };
    UTF8Char_t &operator=(wchar_t Input)
    {
        Internal = Input;
        return *this;
    }
    UTF8Char_t &operator=(char Input)
    {
        Internal = Input;
        return *this;
    }
    wchar_t operator*() { return Internal; }

    operator wchar_t() const { return Internal; };
    operator bool() const { return !!Internal; }
    operator std::u8string()
    {
        std::u8string Result{};
        Result.reserve(sizeof(wchar_t) * 1.5);

        do
        {
            // 8-bit.
            if (Internal < 0x80)
            {
                Result.push_back(uint8_t(Internal));
                break;
            }

            // 11-bit.
            if (Internal < 0x800)
            {
                Result.push_back(uint8_t(((Internal & 0x7C0) >> 6) | 0xC0));
                Result.push_back(uint8_t((Internal & 0x3F) | 0x80));
                break;
            }

            // 16-bit.
            if (Internal < 0x10000)
            {
                Result.push_back(uint8_t(((Internal & 0xF000) >> 12) | 0xE0));
                Result.push_back(uint8_t(((Internal & 0xFC0) >> 6) | 0x80));
                Result.push_back(uint8_t((Internal & 0x3F) | 0x80));
                break;
            }

            // Nix / OSX.
            if constexpr (sizeof(wchar_t) == 4)
            {
                // 21-bit.
                if (Internal < 0x200000)
                {
                    Result.push_back(uint8_t(((Internal & 0x1C0000) >> 18) | 0xF0));
                    Result.push_back(uint8_t(((Internal & 0x3F000) >> 12) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0xFC0) >> 6) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0x3F) | 0x80)));
                    break;
                }

                // 26-bit.
                if (Internal < 0x4000000)
                {
                    Result.push_back(uint8_t(((Internal & 0x3000000) >> 24) | 0xF8));
                    Result.push_back(uint8_t(((Internal & 0xFC000) >> 18) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0x3F000) >> 12) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0xFC0) >> 6) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0x3F) | 0x80)));
                    break;
                }

                // 30-bit.
                if (Internal < 0x80000000)
                {
                    Result.push_back(uint8_t(((Internal & 0x40000000) >> 30) | 0xFC));
                    Result.push_back(uint8_t(((Internal & 0x3F000000) >> 24) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0xFC000) >> 18) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0x3F000) >> 12) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0xFC0) >> 6) | 0x80));
                    Result.push_back(uint8_t(((Internal & 0x3F) | 0x80)));
                    break;
                }
            }
        } while (false);

        return Result;
    }
    operator char() const
    {
        return Internal > 0x7F ? '?' : char(Internal & 0x7F);
    };
};
struct String_t
{
    std::basic_string<UTF8Char_t> Storage{};

    String_t() = default;
    String_t(std::string_view Input) : Storage(Input.begin(), Input.end()) {}
    String_t(std::wstring_view Input) : Storage(Input.begin(), Input.end()) {}
    String_t(const std::string &Input) : Storage(Input.begin(), Input.end()) {}
    String_t(const std::wstring &Input) : Storage(Input.begin(), Input.end()) {}
    String_t(std::u8string_view Input)
    {
        const auto Wide = Encoding::toWide(Input);
        Storage.resize(Wide.size());
        std::move(Wide.begin(), Wide.end(), Storage.begin());
    }
    String_t(const std::u8string &Input)
    {
        const auto Wide = Encoding::toWide(Input);
        Storage.resize(Wide.size());
        std::move(Wide.begin(), Wide.end(), Storage.begin());
    }

    std::string asASCII() const { return std::string(Storage.begin(), Storage.end()); }
    std::wstring asWCHAR() const { return std::wstring(Storage.begin(), Storage.end()); }
    std::u8string asUTF8() const { return Encoding::toUTF8({ (wchar_t *)Storage.data(), Storage.size() }); }
    std::wstring_view asView() const { return std::wstring_view((wchar_t *)Storage.data(), Storage.size()); }

    String_t &operator+=(std::string_view Input) { Storage.append(Input.begin(), Input.end()); return *this; }
    String_t &operator+=(std::wstring_view Input) { Storage.append(Input.begin(), Input.end()); return *this; }
    String_t &operator+=(std::u8string_view Input)
    {
        const auto Wide = Encoding::toWide(Input);
        Storage.append((UTF8Char_t *)Wide.data(), Wide.size());
        return *this;
    }

    size_t size() const { return Storage.size(); }
    auto begin() const { return Storage.begin(); }
    auto end() const { return Storage.end(); }

    UTF8Char_t &operator [](size_t Index)
    {
        return Storage[Index];
    }
};
