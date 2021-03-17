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
    namespace Internal
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

        template <typename T> concept Iteratable_t = requires (const T &t) { t.cbegin(); t.cend(); };
    }

    template<size_t N, typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr std::array<char, ((N + 2) / 3 * 4)> Encode(const T (&Input)[N])
    {
        std::array<char, ((N + 2) / 3 * 4)> Result;
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            Accumulator = (Accumulator << 8) | (Item & 0xFF);
            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition] = Internal::Table[Accumulator & 0x3F];
        }

        while (Outputposition < ((N + 2) / 3 * 4))
            Result[Outputposition++] = '=';

        return Result;
    }

    template<size_t N, typename T = char, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr std::array<T, (3 * N / 4)> Decode(const char (&Input)[N])
    {
        std::array<T, (3 * N / 4)> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') continue;

            Accumulator = Accumulator << 6 | Internal::Reversetable[static_cast<uint8_t>(Item)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> Bits & 0xFF);
            }
        }

        return Result;
    }

    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr std::array<char, ((N + 2) / 3 * 4)> Encode(const std::array<T, N> &Input)
    {
        std::array<char, ((N + 2) / 3 * 4)> Result;
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            Accumulator = (Accumulator << 8) | (Item & 0xFF);
            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition] = Internal::Table[Accumulator & 0x3F];
        }

        while (Outputposition < ((N + 2) / 3 * 4))
            Result[Outputposition++] = '=';

        return Result;
    }

    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr std::array<T, (3 * N / 4)> Decode(const std::array<T, N> &Input)
    {
        std::array<T, (3 * N / 4)> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') continue;

            Accumulator = Accumulator << 6 | Internal::Reversetable[static_cast<uint8_t>(Item)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> Bits & 0xFF);
            }
        }

        return Result;
    }

    template <Internal::Iteratable_t T, typename = std::enable_if<(sizeof(T::value_type) == 1)>::type>
    [[nodiscard]] inline std::string Encode(const T &Input)
    {
        std::string Result(((Input.size() + 2) / 3 * 4), '=');
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
                Result[Outputposition++] = Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition] = Internal::Table[Accumulator & 0x3F];
        }

        return Result;
    }

    template <Internal::Iteratable_t T, typename = std::enable_if<(sizeof(T::value_type) == 1)>::type>
    [[nodiscard]] inline std::basic_string<typename T::value_type> Decode(const T &Input)
    {
        std::basic_string<T::value_type> Result(( 3 * Input.size() / 4), '\0');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (const auto &Item : Input)
        {
            if (Item == '=') continue;

            Accumulator = Accumulator << 6 | Internal::Reversetable[static_cast<uint8_t>(Item)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> Bits & 0xFF);
            }
        }

        return Result;
    }

    // No need for extra allocations.
    template <typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    constexpr std::basic_string_view<T> Decode_inplace(T *Input, size_t Length)
    {
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < Length; ++i)
        {
            if (Input[i] == '=') continue;

            Accumulator = Accumulator << 6 | Internal::Reversetable[Input[i] & 0x7F];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Input[Outputposition++] = static_cast<char>(Accumulator >> Bits & 0xFF);
            }
        }

        Input[Outputposition] = '\0';
        return { Input, Outputposition };
    }

    // Verify that the string is valid.
    [[nodiscard]] inline bool isValid(const std::string &Input)
    {
        if ((Input.size() & 3) != 0) return false;

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
    [[nodiscard]] inline std::string toURL(std::string_view Input)
    {
        while (Input.back() == '=')
            Input.remove_suffix(1);

        std::string Result(Input);
        for (auto &Item : Result)
        {
            if (Item == '+') Item = '-';
            if (Item == '/') Item = '_';
        }

        return Result;
    }
    [[nodiscard]] inline std::string fromURL(std::string_view Input)
    {
        std::string Result(Input);

        for (auto &Item : Result)
        {
            if (Item == '-') Item = '+';
            if (Item == '_') Item = '/';
        }

        switch (Result.size() & 3)
        {
            case 3: Result += "==="; break;
            case 2: Result += "=="; break;
            case 1: Result += "="; break;
            default: break;
        }

        return Result;
    }

    // Sanity checking.
    static_assert(Encode("12345") == Encode(Decode(Encode("12345"))), "Someone fucked with the Base64 encoding");
}
