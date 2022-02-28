/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-13
    License: MIT
*/

#pragma once
#include <Utilities/Constexprhelpers.hpp>

namespace Base58
{
    constexpr size_t Decodesize(size_t N) { return size_t(N * 733 / 1000.0f + 0.5f); }  // log(58) / log(256), rounded.
    constexpr size_t Encodesize(size_t N) { return size_t(N * 137 / 100.0f + 0.5f); }   // log(256) / log(58), rounded.

    static constexpr uint8_t Table[58]
    {
        '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J',
        'K', 'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x','y', 'z'
    };
    static constexpr uint8_t Reversetable[128]
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

    // Compiletime encoding, Vector_t is compatible with std::array.
    template <cmp::Byte_t T, size_t N> constexpr cmp::Vector_t<T, Encodesize(N)> Encode(const std::array<T, N> &Input)
    {
        cmp::Vector_t<T, Encodesize(N)> Buffer{}, Result{};
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Process the rest of the input.
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

        // Count leading zeros.
        while (Input[Leadingzeros] == 0 && Leadingzeros < N)
        { Buffer[Leadingzeros] = '1'; Leadingzeros++; }

        // Reverse emplace from the map.
        for (size_t i = 0; i < Outputposition; ++i)
        {
            Result[Leadingzeros + i] = Table[Buffer[Outputposition - 1 - i]];
        }

        return Result;
    }
    template <cmp::Byte_t T, size_t N> constexpr cmp::Vector_t<T, Decodesize(N)> Decode(const std::array<T, N> &Input)
    {
        std::array<T, Encodesize(N)> Buffer{};
        size_t Outputposition{ 1 };

        // Process the rest of the input.
        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(Reversetable[Input[i] & 0x7F]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c]) * 58;
                Buffer[c] = Item & 0xFF;
                Item >>= 8;
            }

            for (; Item; Item >>= 8)
                Buffer[Outputposition++] = Item & 0xFF;
        }

        for (size_t i = 0; i < N && Input[i] == '1'; i++)
            Buffer[Outputposition++] = 0;

        std::ranges::reverse(Buffer);
        return cmp::resize_array<T, Encodesize(N), Decodesize(N)>(Buffer);
    }

    // Runtime encoding.
    template <cmp::Byte_t T, cmp::Bytealigned_t U> constexpr cmp::Vector_t<U> Encode(std::span<T> Input, std::span<U> Output)
    {
        const auto N = std::ranges::size(Input);
        cmp::Vector_t<T, Encodesize(N)> Buffer{};
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Verify that there's enough space to decode.
        if (!std::is_constant_evaluated()) assert(Output.size() >= Encodesize(N));

        // Process the rest of the input.
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

        // Count leading zeros.
        while (Input[Leadingzeros] == 0 && Leadingzeros < N)
        {
            Buffer[Leadingzeros] = '1'; Output[Leadingzeros] = 0; Leadingzeros++;
        }

        // Reverse emplace from the map.
        for (size_t i = 0; i < Outputposition; ++i)
        {
            Output[Leadingzeros + i] = Table[Buffer[Outputposition - 1 - i]];
        }

        return Output.subspan(0, Encodesize(Input.size()));
    }
    template <cmp::Byte_t T, cmp::Bytealigned_t U> constexpr cmp::Vector_t<U> Decode(std::span<T> Input, std::span<U> Output)
    {
        const auto N = std::ranges::size(Input);
        Blob_t Buffer(Encodesize(N), 0);
        size_t Outputposition{ 1 };
        size_t Leadingzeros{};

        // Verify that there's enough space to decode.
        if (!std::is_constant_evaluated()) assert(Output.size() >= Decodesize(N));

        // Process the rest of the input.
        for (size_t i = 0; i < N; ++i)
        {
            auto Item = static_cast<uint32_t>(Reversetable[Input[i] & 0x7F]);

            for (size_t c = 0; c < Outputposition; c++)
            {
                Item += static_cast<uint32_t>(Buffer[c]) * 58;
                Buffer[c] = Item & 0xFF;
                Item >>= 8;
            }

            for (; Item; Item >>= 8)
                Buffer[Outputposition++] = Item & 0xFF;
        }

        for (size_t i = 0; i < N && Input[i] == '1'; i++)
            Buffer[Outputposition++] = 0;

        std::ranges::reverse(Buffer);
        std::ranges::copy_n(Buffer, Decodesize(Input.size()), Output);
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
        constexpr auto Charset = std::string_view("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
        return std::ranges::all_of(Input, [&](char Char)
        {
            #if defined(__cpp_lib_string_contains)
            return Charset.contains(Char);
            #else
            return Charset.find(Char) != Charset.end();
            #endif
        });
    }
}
