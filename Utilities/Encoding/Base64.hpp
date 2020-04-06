/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

namespace Base64
{
    namespace
    {
        constexpr char Table[64] =
        {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
            'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
            'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
            'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
            's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
            '3', '4', '5', '6', '7', '8', '9', '+', '/' };
        constexpr char Reversetable[128] =
        {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
        };
    }

    [[nodiscard]] inline std::string Encode(const std::string_view Input)
    {
        std::string Result((((Input.size() + 2) / 3) * 4), '=');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (const auto &Item : Input)
        {
            Accumulator = (Accumulator << 8) | (Item & 0xFF);
            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = Table[(Accumulator >> Bits) & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition] = Table[Accumulator & 0x3F];
        }

        return Result;
    }
    [[nodiscard]] inline std::string Decode(const std::string_view Input)
    {
        std::string Result(((Input.size() / 4) * 3), '\0');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (const auto &Item : Input)
        {
            if (Item == '=') continue;

            Accumulator = (Accumulator << 6) | Reversetable[uint8_t(Item)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = char((Accumulator >> Bits) & 0xFF);
            }
        }

        return Result;
    }
    [[nodiscard]] constexpr inline bool isValid(const std::string_view Input)
    {
        for (const auto &Item : Input)
        {
            if (Item >= 'A' && Item <= 'Z') continue;
            if (Item >= 'a' && Item <= 'z') continue;
            if (Item >= '/' && Item <= '9') continue;
            if (Item == '=' || Item == '+') continue;
            return false;
        }

        return !Input.empty();
    }

    [[nodiscard]] inline Blob Encode(const Blob_view Input)
    {
        Blob Result((((Input.size() + 2) / 3) * 4), '=');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (const auto &Item : Input)
        {
            Accumulator = (Accumulator << 8) | (Item & 0xFF);
            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = Table[(Accumulator >> Bits) & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition] = Table[Accumulator & 0x3F];
        }

        return Result;
    }
    [[nodiscard]] inline Blob Decode(const Blob_view Input)
    {
        Blob Result(((Input.size() / 4) * 3), '\0');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (const auto &Item : Input)
        {
            if (Item == '=') continue;

            Accumulator = (Accumulator << 6) | Reversetable[uint8_t(Item)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = char((Accumulator >> Bits) & 0xFF);
            }
        }

        return Result;
    }
    [[nodiscard]] constexpr inline bool isValid(const Blob_view Input)
    {
        for (const auto &Item : Input)
        {
            if (Item >= 'A' && Item <= 'Z') continue;
            if (Item >= 'a' && Item <= 'z') continue;
            if (Item >= '/' && Item <= '9') continue;
            if (Item == '=' || Item == '+') continue;
            return false;
        }

        return !Input.empty();
    }

    // RFC7515 compatibility.
    [[nodiscard]] inline std::string toURL(std::string &&Input)
    {
        while (Input.back() == '=')
            Input.pop_back();

        for (auto &Item : Input)
        {
            if (Item == '+') Item = '-';
            if (Item == '/') Item = '_';
        }

        return std::move(Input);
    }
    [[nodiscard]] inline std::string fromURL(std::string &&Input)
    {
        for (auto &Item : Input)
        {
            if (Item == '-') Item = '+';
            if (Item == '_') Item = '/';
        }

        switch (Input.size() % 4)
        {
            case 2: Input += "=="; break;
            case 1: Input += "="; break;
            default: ;
        }

        return std::move(Input);
    }
}
