/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-13
    License: MIT
*/

#pragma once
#include <Utilities/Constexprhelpers.hpp>

namespace Base64
{
    // As we can't really calculate padding when decoding, we allocate for the worst case.
    constexpr size_t Encodesize(size_t N)  { return ((N + 2) / 3) << 2; }
    constexpr size_t Decodesize(size_t N)  { return (N * 3) >> 2; }

    static constexpr uint8_t Table[64]
    {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
        'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
        's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
        '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };
    static constexpr uint8_t Reversetable[128]
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

    // Compiletime encoding, Vector_t is compatible with std::array.
    template <cmp::Byte_t T, size_t N> constexpr cmp::Vector_t<T, Encodesize(N)> Encode(const std::array<T, N> &Input)
    {
        cmp::Vector_t<T, Encodesize(N)> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            Accumulator = (Accumulator << 8) | (Item & 0xFF);

            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition++] = Table[Accumulator & 0x3F];
        }

        while (Outputposition < Encodesize(N))
            Result[Outputposition++] = '=';

        return Result;
    }
    template <cmp::Byte_t T, size_t N> constexpr cmp::Vector_t<T, Decodesize(N)> Decode(const std::array<T, N> &Input)
    {
        cmp::Vector_t<T, Decodesize(N)> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') [[unlikely]] continue;

            Accumulator = (Accumulator << 6) | Reversetable[static_cast<uint8_t>(Item & 0x7F)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        return Result;
    }

    // Runtime encoding.
    template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Encode(std::span<T> Input, std::span<U> Output)
    {
        const auto N = std::ranges::size(Input);
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            Accumulator = (Accumulator << 8) | (Item & 0xFF);

            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Output[Outputposition++] = Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Output[Outputposition++] = Table[Accumulator & 0x3F];
        }

        while (Outputposition < Encodesize(N))
            Output[Outputposition++] = '=';

        return Output.subspan(0, Encodesize(Input.size()));

    }
    template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Decode(std::span<T> Input, std::span<U> Output)
    {
        const auto N = std::ranges::size(Input);
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') [[unlikely]] continue;

            Accumulator = (Accumulator << 6) | Reversetable[static_cast<uint8_t>(Item & 0x7F)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Output[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        return Output.subspan(0, Decodesize(Input.size()));
    }

    // Wrapper to return a owning container, std::array compatible if size is provided..
    template <cmp::Byte_t T, size_t N> constexpr auto Encode(std::span<T, N> Input)
    {
        // Size provided, compiletime possible.
        if constexpr (N != 0 && N != static_cast<size_t>(-1))
        {
            return Encode(cmp::make_vector(Input));
        }

        // Dynamic span, runtime only.
        else
        {
            Blob_t Temp(Encodesize(Input.size()), 0);
            Encode(std::span(Input), std::span(Temp));
            return Temp;
        }
    }
    template <cmp::Byte_t T, size_t N> constexpr auto Decode(std::span<T, N> Input)
    {
        // Size provided, compiletime possible.
        if constexpr (N != 0 && N != static_cast<size_t>(-1))
        {
            return Decode(cmp::make_vector(Input));
        }

        // Dynamic span, runtime only.
        else
        {
            Blob_t Temp(Decodesize(Input.size()), 0);
            Decode(std::span(Input), std::span(Temp));
            return Temp;
        }
    }

    // Wrapper to deal with string-literals.
    template <cmp::Char_t T, size_t N> constexpr auto Encode(const T(&Input)[N])
    {
        return Encode(cmp::make_vector(Input));
    }
    template <cmp::Char_t T, size_t N> constexpr auto Decode(const T(&Input)[N])
    {
        return Decode(cmp::make_vector(Input));
    }

    // Just verify the characters, not going to bother with length.
    template <cmp::Simple_t T> [[nodiscard]] constexpr bool isValid(const T &Input)
    {
        constexpr auto Charset = std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
        return std::ranges::all_of(Input, [&](auto Char) { return Charset.contains(char(Char)); });
    }

    // RFC7515, URL compatible charset.
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
}
