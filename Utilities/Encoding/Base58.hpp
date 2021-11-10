/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-20
    License: MIT
*/

#pragma once
#include <array>
#include <cstdint>
#include <concepts>
#include <string_view>
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

namespace Base58
{
    constexpr size_t Decodesize(size_t N) { return (N * 733 / 1000.0f); }  // log(58) / log(256), rounded.
    constexpr size_t Encodesize(size_t N) { return (N * 138 / 100.0f); }   // log(256) / log(58), rounded.
    template <typename T> concept Byte_t = sizeof(T) == 1;

    namespace B58Internal
    {
        constexpr uint8_t Table[58] =
        {
            '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J',
            'K', 'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T',
            'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
            'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x','y', 'z'
        };
        constexpr uint8_t Reversetable[128] =
        {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0xFF, 0x11, 0x12, 0x13, 0x14, 0x15, 0xFF,
            0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0xFF, 0x2C, 0x2D, 0x2E,
            0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };

        template <typename T> concept Range_t = requires (const T & t) { std::ranges::begin(t); std::ranges::end(t); };
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

        constexpr bool isSpace(char c) noexcept
        {
            return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
        }
    }

    // Should be constexpr in C++20, questionable compiler support though.
    template <typename C = char, B58Internal::Complexstring_t T> [[nodiscard]] constexpr std::basic_string<C> Encode(const T &Input)
    {
        return Encode<C>(B58Internal::Flatten(Input));
    }
    template <typename C = char, B58Internal::Complexstring_t T> [[nodiscard]] constexpr std::basic_string<C> Decode(const T &Input)
    {
        return Decode<C>(B58Internal::Flatten(Input));
    }
    template <typename C = char, B58Internal::Simplestring_t T> [[nodiscard]] constexpr std::basic_string<C> Encode(const T &Input)
    {
        const auto N = std::ranges::size(Input);
        Blob Buffer(Encodesize(N), 0x00);
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Count leading zeros.
        while (Input[Leadingzeros] == 0) Leadingzeros++;

        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(Input[i]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c] << 8);
                Buffer[c] = static_cast<uint8_t>(Item % 58);
                Item /= 58;
            }

            for (; Item; Item /= 58)
                Buffer[Outputposition++] = static_cast<uint8_t>(Item % 58);
        }

        // Reverse emplace from the map.
        std::basic_string<C> Result(Leadingzeros + Outputposition, '1');
        for (size_t i = 0; i < Outputposition; ++i)
        {
            Result[Leadingzeros + i] = B58Internal::Table[Buffer[Outputposition - 1 - i]];
        }

        return Result;
    }
    template <typename C = char, B58Internal::Simplestring_t T> [[nodiscard]] constexpr std::basic_string<C> Decode(const T &Input)
    {
        const auto N = std::ranges::size(Input);
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Count leading ones and spaces (zero in output).
        while (Input[Leadingzeros] == '1' || B58Internal::isSpace(Input[Leadingzeros])) Leadingzeros++;
        Blob Buffer(Decodesize(N) + Leadingzeros, 0x00);

        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(B58Internal::Reversetable[Input[i] & 0x7F]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c]) * 58;
                Buffer[c] = Item & 0xFF;
                Item >>= 8;
            }

            for (; Item; Item >>= 8)
                Buffer[Outputposition++] = Item & 0xFF;
        }

        return { Buffer.rbegin(), Buffer.rend() };
    }

    // No need for extra allocations, assume the caller knows what they are doing.
    template <typename C = char, B58Internal::Simplestring_t T> constexpr void Encode(const T &Input, C *Result)
    {
        const auto N = std::ranges::size(Input);
        Blob Buffer(Encodesize(N), 0x00);
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Count leading zeros.
        while (Input[Leadingzeros] == 0) Leadingzeros++;

        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(Input[i]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c] << 8);
                Buffer[c] = static_cast<uint8_t>(Item % 58);
                Item /= 58;
            }

            for (; Item; Item /= 58)
                Buffer[Outputposition++] = static_cast<uint8_t>(Item % 58);
        }

        // Reverse emplace from the map.
        for (size_t i = 0; i < Outputposition; ++i)
        {
            Result[Leadingzeros + i] = B58Internal::Table[Buffer[Outputposition - 1 - i]];
        }
    }
    template <typename C = char, B58Internal::Simplestring_t T> constexpr void Decode(const T &Input, C *Result)
    {
        const auto N = std::ranges::size(Input);
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Count leading ones and spaces (zero in output).
        while (Input[Leadingzeros] == '1' || B58Internal::isSpace(Input[Leadingzeros])) Leadingzeros++;
        Blob Buffer(Decodesize(N) + Leadingzeros, 0x00);

        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(B58Internal::Reversetable[Input[i] & 0x7F]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c]) * 58;
                Buffer[c] = Item & 0xFF;
                Item >>= 8;
            }

            for (; Item; Item >>= 8)
                Buffer[Outputposition++] = Item & 0xFF;
        }

        std::ranges::move(Buffer.rbegin(), Buffer.rend(), Result);
    }

    // Verify that the string is valid, MSVC STL does not have String.contains yet.
    [[nodiscard]] inline bool isValid(std::string_view Input)
    {
        constexpr char Valid[] = { "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz" };
        for (const auto &Item : Input)
        {
            if (!std::memchr(Valid, Item, sizeof(Valid))) [[unlikely]]
                return false;
        }
        return true;
    }
}
