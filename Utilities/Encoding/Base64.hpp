/*
Initial author: Convery (tcn@ayria.se)
Started: 2019-03-14
License: MIT
*/

#pragma once
#include <array>
#include <cstdint>
#include <concepts>
#include <string_view>
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

namespace Base64
{
    template <typename T> concept Byte_t = sizeof(T) == 1;
    constexpr size_t Decodesize(size_t N)  { return (N * 3 / 4) - 1; }
    constexpr size_t Encodesize(size_t N)  { return ((N + 2) / 3 * 4); }
    template <size_t N> constexpr size_t Decodesize()  { return (N * 3 / 4) - 1; }
    template <size_t N> constexpr size_t Encodesize()  { return ((N + 2) / 3 * 4); }

    namespace B64Internal
    {
        constexpr uint8_t Table[64] =
        {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
            'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
            'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
            'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
            's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
            '3', '4', '5', '6', '7', '8', '9', '+', '/'
        };
        constexpr uint8_t Reversetable[128] =
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

        template <typename T> concept Range_t = requires (const T &t) { std::ranges::begin(t); std::ranges::end(t); };
        template <typename T> concept Complexstring_t = Range_t<T> && !Byte_t<typename T::value_type>;
        template <typename T> concept Simplestring_t = Range_t<T> && Byte_t<typename T::value_type>;
        template <Complexstring_t T> [[nodiscard]] constexpr Blob Flatten(const T &Input)
        {
            Blob Bytearray; Bytearray.reserve(Input.size() * sizeof(typename T::value_type));
            static_assert(sizeof(typename T::value_type) <= 8, "Dude..");

            for (auto &&Item : Input)
            {
                if constexpr (sizeof(typename T::value_type) > 7) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 6) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 5) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 4) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 3) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 2) { Bytearray.append(Item & 0xFF); Item >>= 8; }
                if constexpr (sizeof(typename T::value_type) > 1) { Bytearray.append(Item & 0xFF); Item >>= 8; }

                Bytearray.append(Item & 0xFF);
            }

            return Bytearray;
        }
    }

    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<char, Encodesize<N>()> Encode(const std::array<T, N> &Input)
    {
        std::array<char, Encodesize<N>()> Result{};
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
                Result[Outputposition++] = B64Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition++] = B64Internal::Table[Accumulator & 0x3F];
        }

        while (Outputposition < Encodesize<N>())
            Result[Outputposition++] = '=';

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<T, Decodesize<N>()> Decode(const std::array<T, N> &Input)
    {
        std::array<T, Decodesize<N>()> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') [[unlikely]] continue;

            Accumulator = (Accumulator << 6) | B64Internal::Reversetable[static_cast<uint8_t>(Item & 0x7F)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<char, Encodesize<N>()> Encode(const T (&Input)[N])
    {
        std::array<char, Encodesize<N>()> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < (N - 1); ++i)
        {
            const auto Item = Input[i];
            Accumulator = (Accumulator << 8) | (Item & 0xFF);

            Bits += 8;
            while (Bits >= 6)
            {
                Bits -= 6;
                Result[Outputposition++] = B64Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition++] = B64Internal::Table[Accumulator & 0x3F];
        }

        while (Outputposition < Encodesize<N>())
            Result[Outputposition++] = '=';

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<T, Decodesize<N>()> Decode(const T(&Input)[N])
    {
        std::array<T, Decodesize<N>()> Result{};
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < (N - 1); ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') [[unlikely]] continue;

            Accumulator = (Accumulator << 6) | B64Internal::Reversetable[static_cast<uint8_t>(Item & 0x7F)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        return Result;
    }

    // Should be constexpr in C++20, questionable compiler support though.
    template <B64Internal::Complexstring_t T> [[nodiscard]] constexpr std::string Encode(const T &Input)
    {
        return Encode(B64Internal::Flatten(Input));
    }
    template <B64Internal::Complexstring_t T> [[nodiscard]] constexpr std::string Decode(const T &Input)
    {
        return Decode(B64Internal::Flatten(Input));
    }
    template <B64Internal::Simplestring_t T> [[nodiscard]] constexpr std::string Encode(const T &Input)
    {
        const auto N = std::ranges::size(Input);
        std::string Result(Encodesize(N), '\0');
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
                Result[Outputposition++] = B64Internal::Table[Accumulator >> Bits & 0x3F];
            }
        }

        if (Bits)
        {
            Accumulator <<= 6 - Bits;
            Result[Outputposition++] = B64Internal::Table[Accumulator & 0x3F];
        }

        while (Outputposition < Encodesize(N))
            Result[Outputposition++] = '=';

        return Result;
    }
    template <B64Internal::Simplestring_t T> [[nodiscard]] constexpr std::string Decode(const T &Input)
    {
        const auto N = std::ranges::size(Input);
        std::string Result(Decodesize(N), '\0');
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint8_t Bits{};

        for (size_t i = 0; i < N; ++i)
        {
            const auto Item = Input[i];
            if (Item == '=') [[unlikely]] continue;

            Accumulator = (Accumulator << 6) | B64Internal::Reversetable[static_cast<uint8_t>(Item & 0x7F)];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Result[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        return Result;
    }

    // No need for extra allocations.
    template <Byte_t T> constexpr std::basic_string_view<T> Decode_inplace(T *Input, size_t Length)
    {
        size_t Outputposition{};
        uint32_t Accumulator{};
        uint32_t Bits{};

        for (size_t i = 0; i < Length; ++i)
        {
            if (Input[i] == '=') continue;

            Accumulator = (Accumulator << 6) | B64Internal::Reversetable[Input[i] & 0x7F];
            Bits += 6;

            if (Bits >= 8)
            {
                Bits -= 8;
                Input[Outputposition++] = static_cast<char>(Accumulator >> (Bits & 0xFF));
            }
        }

        Input[Outputposition] = '\0';
        return { Input, Outputposition };
    }

    // Verify that the string is valid.
    [[nodiscard]] constexpr bool isValid(std::string_view Input)
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
    [[nodiscard]] inline std::string_view toURL(std::string &Input)
    {
        while (Input.back() == '=')
            Input.pop_back();

        for (auto &Item : Input)
        {
            if (Item == '+') Item = '-';
            if (Item == '/') Item = '_';
        }

        return Input;
    }
    [[nodiscard]] inline std::string_view fromURL(std::string &Input)
    {
        for (auto &Item : Input)
        {
            if (Item == '-') Item = '+';
            if (Item == '_') Item = '/';
        }

        switch (Input.size() & 3)
        {
            case 3: Input += "==="; break;
            case 2: Input += "=="; break;
            case 1: Input += "="; break;
            default: break;
        }

        return Input;
    }

    // Sanity checking.
    static_assert(Decode("MTIzNDU=") == Decode(Encode("12345")), "Someone fucked with the Base64 encoding..");
}
