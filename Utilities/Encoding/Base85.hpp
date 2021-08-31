/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-20
    License: MIT

    Z85 flavour of Base85 so it can be used for embedded files.
*/

#pragma once
#include <array>
#include <cstdint>
#include <concepts>
#include <string_view>
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

namespace Base85
{
    template <typename T> concept Byte_t = sizeof(T) == 1;
    constexpr size_t Encodesize(size_t N)  { return N * 5 / 4; }
    constexpr size_t Decodesize(size_t N)  { return N * 4 / 5; }
    template <size_t N> constexpr size_t Encodesize() { return N * 5 / 4; }
    template <size_t N> constexpr size_t Decodesize() { return N * 4 / 5; }
    constexpr size_t Encodesize_padded(size_t N) { return Encodesize(N) + ((Encodesize(N) % 5) ? (5 - (Encodesize(N) % 5)) : 0); }
    constexpr size_t Decodesize_padded(size_t N) { return Decodesize(N) + ((Decodesize(N) & 3) ? (4 - (Decodesize(N) & 3)) : 0); }
    template <size_t N> constexpr size_t Encodesize_padded() { return Encodesize<N>() + ((Encodesize<N>() % 5) ? (5 - (Encodesize<N>() % 5)) : 0); }
    template <size_t N> constexpr size_t Decodesize_padded() { return Decodesize<N>() + ((Decodesize<N>() & 3) ? (4 - (Decodesize<N>() & 3)) : 0); }

    namespace B85Internal
    {
        constexpr uint8_t Table[85] =
        {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
            'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
            'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
            'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
            'Y', 'Z', '.', '-', ':', '+', '=', '^', '!', '/',
            '*', '?', '&', '<', '>', '(', ')', '[', ']', '{',
            '}', '@', '%', '$', '#'
        };
        constexpr uint8_t Reversetable[128] =
        {
            0x00, 0x44, 0x00, 0x54, 0x53, 0x52, 0x48, 0x00,
            0x4B, 0x4C, 0x46, 0x41, 0x00, 0x3F, 0x3E, 0x45,
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x40, 0x00, 0x49, 0x42, 0x4A, 0x47,
            0x51, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
            0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
            0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
            0x3B, 0x3C, 0x3D, 0x4D, 0x00, 0x4E, 0x43, 0x00,
            0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
            0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
            0x21, 0x22, 0x23, 0x4F, 0x00, 0x50, 0x00, 0x00
            /* Null bytes */
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

        constexpr uint32_t Div(uint32_t Value)
        {
            constexpr uint64_t Magic = 3233857729ULL;
            return (uint32_t)((Magic * Value) >> 32) >> 6;
        }
    }

    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<char, Encodesize_padded<N>()> Encode(const std::array<T, N> &Input)
    {
        if constexpr (N & 3)
        {
            std::array<T, (N + 4 - (N & 3))> A(Input.begin(), Input.end());
            return Encode(A);
        }

        std::array<char, Encodesize_padded<N>()> Result{};
        size_t Outputposition{};

        for (size_t i = 0; i < N; i += 4)
        {
            uint32_t A, B;

            // Big-endian to native.
            A = (Input[i + 0] << 24) | (Input[i + 1] << 16) | (Input[i + 2] << 8) | Input[i + 3];

            B = B85Internal::Div(A); Result[Outputposition + 4] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 3] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 2] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 1] = B85Internal::Table[A - B * 85];
            Result[Outputposition + 0] = B85Internal::Table[B];
            Outputposition += 5;
        }

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<T, Decodesize_padded<N>()> Decode(const std::array<T, N> &Input)
    {
        if constexpr (N % 5)
        {
            std::array<T, (N + 5 - (N % 5))> A(Input.begin(), Input.end());
            return Decode(A);
        }

        std::array<T, Decodesize_padded<N>()> Result{};
        size_t Outputposition{};

        for (size_t i = 0; i < N; i += 5)
        {
            uint32_t A = B85Internal::Reversetable[((uint8_t)Input[i + 0] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 1] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 2] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 3] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 4] - 0x20) & 0x7F];

            Result[Outputposition + 0] = (A >> 24) & 0xFF;
            Result[Outputposition + 1] = (A >> 16) & 0xFF;
            Result[Outputposition + 2] = (A >> 8) & 0xFF;
            Result[Outputposition + 3] = (A >> 0) & 0xFF;

            Outputposition += 4;
        }

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<char, Encodesize_padded<(N - 1)>()> Encode(const T(&Input)[N])
    {
        static_assert(!((N - 1) & 3), "Base85 with string litterals need to be % 4.");
        std::array<char, Encodesize_padded<(N - 1)>()> Result{};
        size_t Outputposition{};

        for (size_t i = 0; i < (N - 1); i += 4)
        {
            uint32_t A, B;

            // Big-endian to native.
            A = (Input[i + 0] << 24) | (Input[i + 1] << 16) | (Input[i + 2] << 8) | Input[i + 3];

            B = B85Internal::Div(A); Result[Outputposition + 4] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 3] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 2] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 1] = B85Internal::Table[A - B * 85];
            Result[Outputposition + 0] = B85Internal::Table[B];
            Outputposition += 5;
        }

        return Result;
    }
    template <size_t N, Byte_t T> [[nodiscard]] constexpr std::array<T, Decodesize_padded<(N - 1)>()> Decode(const T(&Input)[N])
    {
        static_assert(!((N - 1) % 5), "Base85 with string litterals need to be % 5.");
        std::array<T, Decodesize_padded<(N - 1)>()> Result{};
        size_t Outputposition{};

        for (size_t i = 0; i < (N - 1); i += 5)
        {
            uint32_t A = B85Internal::Reversetable[((uint8_t)Input[i + 0] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 1] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 2] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 3] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 4] - 0x20) & 0x7F];

            Result[Outputposition + 0] = (A >> 24) & 0xFF;
            Result[Outputposition + 1] = (A >> 16) & 0xFF;
            Result[Outputposition + 2] = (A >> 8) & 0xFF;
            Result[Outputposition + 3] = (A >> 0) & 0xFF;

            Outputposition += 4;
        }

        return Result;
    }

    // Should be constexpr in C++20, questionable compiler support though.
    template <typename C = char, B85Internal::Complexstring_t T> [[nodiscard]] constexpr std::basic_string<C> Encode(const T &Input)
    {
        return Encode<C>(B85Internal::Flatten(Input));
    }
    template <typename C = char, B85Internal::Complexstring_t T> [[nodiscard]] constexpr std::basic_string<C> Decode(const T &Input)
    {
        return Decode<C>(B85Internal::Flatten(Input));
    }
    template <typename C = char, B85Internal::Simplestring_t T> [[nodiscard]] constexpr std::basic_string<C>Encode(const T &Input)
    {
        size_t Outputposition{};
        const auto N = std::ranges::size(Input);
        std::basic_string<C> Result(Encodesize_padded(N), '\0');

        for (size_t i = 0; i < N; i += 4)
        {
            uint32_t A, B;

            // Big-endian to native.
            A = (Input[i + 0] << 24) | (Input[i + 1] << 16) | (Input[i + 2] << 8) | Input[i + 3];

            B = B85Internal::Div(A); Result[Outputposition + 4] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 3] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 2] = B85Internal::Table[A - B * 85]; A = B;
            B = B85Internal::Div(A); Result[Outputposition + 1] = B85Internal::Table[A - B * 85];
            Result[Outputposition + 0] = B85Internal::Table[B];
            Outputposition += 5;
        }

        return Result;
    }
    template <typename C = char, B85Internal::Simplestring_t T> [[nodiscard]] constexpr std::basic_string<C> Decode(const T &Input)
    {
        size_t Outputposition{};
        const auto N = std::ranges::size(Input);
        std::basic_string<C> Result(Decodesize_padded(N), '\0');

        for (size_t i = 0; i < N; i += 5)
        {
            uint32_t A = B85Internal::Reversetable[((uint8_t)Input[i + 0] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 1] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 2] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 3] - 0x20) & 0x7F];
            A = A * 85 + B85Internal::Reversetable[((uint8_t)Input[i + 4] - 0x20) & 0x7F];

            Result[Outputposition + 0] = (A >> 24) & 0xFF;
            Result[Outputposition + 1] = (A >> 16) & 0xFF;
            Result[Outputposition + 2] = (A >> 8) & 0xFF;
            Result[Outputposition + 3] = (A >> 0) & 0xFF;

            Outputposition += 4;
        }

        return Result;
    }

    // Verify that the string is valid, MSVC STL does not have String.contains yet.
    [[nodiscard]] inline bool isValid(std::string_view Input)
    {
        constexpr char Valid[] = { "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#" };
        for (const auto &Item : Input)
        {
            if (!std::memchr(Valid, Item, sizeof(Valid))) [[unlikely]]
                return false;
        }
        return !Input.empty();
    }

    // Sanity checking.
    static_assert(Decode("f!$Kw") == Decode(Encode("1234")), "Someone fucked with the Base85 encoding..");
}
