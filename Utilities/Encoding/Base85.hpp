/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-20
    License: MIT

    RFC encoding is JSON compatible.
    Z85 encoding is C++ compatible.
    Use accordingly.
*/

#pragma once
#include <span>
#include <array>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>
#include <concepts>
#include <algorithm>
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

    template <typename T> concept Range_t = requires (const T &t) { std::ranges::begin(t); std::ranges::end(t); std::ranges::size(t); };
    template <typename T> concept Complexstring_t = Range_t<T> && !Byte_t<typename T::value_type>;
    template <typename T> concept Simplestring_t = Range_t<T> && Byte_t<typename T::value_type>;

    // Convert the range to a flat type (as you can't cast pointers in constexpr).
    template <Complexstring_t T> [[nodiscard]] constexpr Blob Flatten(const T &Input)
    {
        Blob Bytearray; Bytearray.reserve(Input.size() * sizeof(typename T::value_type));

        for (auto &&Item : Input)
        {
            for (size_t i = 0; i < (sizeof(typename T::value_type) - 1); ++i)
            {
                Bytearray.append(Item & 0xFF);
                Item >>= 8;
            }

            Bytearray.append(Item & 0xFF);
        }

        return Bytearray;
    }

    // Yes, all this to remove the null-byte from a string literal..
    template <size_t N, Byte_t T> consteval std::array<T, N - 1> char_array(const T(&Input)[N])
    {
        return []<size_t ...Index>(const T(&Input)[N], std::index_sequence<Index...>)
        {
            return std::array<T, N - 1>{ {Input[Index]...} };
        }(Input, std::make_index_sequence<N - 1>());
    }

    // Constexpr compatible byteswap (will be added to STL in C++ 23).
    constexpr uint32_t byteswap(uint32_t Value)
    {
        if (std::is_constant_evaluated())
        {
            return (Value & (0xFF << 0) << 24) |
                   (Value & (0xFF << 8) << 8)  |
                   (Value & (0xFF << 16) >> 8) |
                   (Value & (0xFF << 24) >> 24);
        }
        else
        {
            #if defined (_MSC_VER)
            return _byteswap_ulong(Value);
            #else
            return __builtin_bswap32(Value);
            #endif
        }
    }

    // Not allowed to cast a pointer to a value, because reasons..
    template <Byte_t T> constexpr uint32_t toInt32(const T *Ptr)
    {
        return uint8_t(*Ptr++) << 24 | uint8_t(*Ptr++) << 16 | uint8_t(*Ptr++) << 8 | uint8_t(*Ptr++);
    }
    template <Byte_t T> constexpr void fromInt32(T *Ptr, uint32_t Value)
    {
        (*Ptr++) = T((Value >> 24) & 0xFF);
        (*Ptr++) = T((Value >> 16) & 0xFF);
        (*Ptr++) = T((Value >> 8) & 0xFF);
        (*Ptr++) = T((Value >> 0) & 0xFF);
    }

    // TODO(tcn): Benchmark the two implementation-styles and converge on the best one.
    namespace Z85
    {
        constexpr uint8_t Table[85] =
        {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
            'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
            'u', 'v', 'w', 'x', 'y', 'z',
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
            'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
            'U', 'V', 'W', 'X', 'Y', 'Z',
            '.', '-', ':', '+', '=', '^', '!', '/', '*', '?',
            '&', '<', '>', '(', ')', '[', ']', '{', '}', '@',
            '%', '$', '#'
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
            0x21, 0x22, 0x23, 0x4F, 0x00, 0x50,
            /* Null bytes */
        };

        constexpr uint32_t Div85(uint32_t Value) { constexpr uint64_t Magic = 3233857729ULL; return (uint32_t)((Magic * Value) >> 32) >> 6; }
        template <Byte_t T, Byte_t U> constexpr void Encode(std::span<const T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Encodesize_padded(Input.size()));
            if (!std::is_constant_evaluated()) assert((Input.size() & 3) == 0);

            for (size_t i = 0; i < Input.size() / 4; ++i)
            {
                uint32_t Accumulator, Rem;
                Accumulator = byteswap(toInt32(&Input[i * 4]));
                const auto outTuple = Output.subspan(i * 5, 5);

                Rem = Div85(Accumulator); outTuple[4] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[3] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[2] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[1] = U(Table[Accumulator - Rem * 85UL]);
                outTuple[0] = U(Table[Rem]);
            }
        }
        template <Byte_t T, Byte_t U> constexpr void Decode(std::span<const T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Decodesize_padded(Input.size()));
            if (!std::is_constant_evaluated()) assert(Input.size() % 5 == 0);

            for (size_t i = 0; i < Input.size() / 5; ++i)
            {
                const auto inTuple = Input.subspan(i * 5, 5);
                uint32_t Accumulator = 0;

                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[0]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[1]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[2]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[3]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[4]) - 32UL) & 127];

                fromInt32(&Output[i * 4], Accumulator);
            }
        }

        // Overloads because string-types are special and needs another constructor..
        template <Simplestring_t T, Byte_t U> constexpr void Encode(const T &Input, std::span<U> Output)
        {
            return Encode(std::span{ Input }, Output);
        }
        template <Simplestring_t T, Byte_t U> constexpr void Decode(const T &Input, std::span<U> Output)
        {
            return Decode(std::span{ Input }, Output);
        }
        template <Simplestring_t T, Simplestring_t U> constexpr void Encode(const T &Input, U &Output)
        {
            return Encode(std::span{ Input }, std::span{ Output });
        }
        template <Simplestring_t T, Simplestring_t U> constexpr void Decode(const T &Input, U &Output)
        {
            return Decode(std::span{ Input }, std::span{ Output });
        }

        // Covers most containers available, questionable compiler support for constexpr though.
        template <Byte_t T= char, Simplestring_t U> [[nodiscard]] constexpr std::basic_string<T> Encode(const U &Input)
        {
            const auto Size = std::ranges::size(Input);

            // We may need to pad the input.
            if (Size & 3)
            {
                // Prefer a uniform type with the padding being default initialized.
                std::vector<typename U::value_type> Vec(Input.begin(), Input.end());
                Vec.resize(Vec.size() + (4 - (Size & 3)));
                return Encode<T>(Vec);
            }

            std::basic_string<T> Result(Encodesize_padded(Size), '\0');
            Encode<typename U::value_type, T>(Input, Result);
            return Result;
        }
        template <Byte_t T= char, Simplestring_t U> [[nodiscard]] constexpr std::basic_string<T> Decode(const U &Input)
        {
            const auto Size = std::ranges::size(Input);

            // We may need to pad the input.
            if (Size % 5)
            {
                // Prefer a uniform type with the padding being default initialized.
                std::vector<typename U::value_type> Vec(Input.begin(), Input.end());
                Vec.resize(Vec.size() + (5 - (Size % 5)));
                return Decode<T>(Vec);
            }

            std::basic_string<T> Result(Decodesize_padded(Size), '\0');
            Decode<typename U::value_type, T>(Input, Result);
            return Result;
        }
        template <Byte_t T= char, Complexstring_t U> [[nodiscard]] constexpr std::basic_string<T> Encode(const U &Input)
        {
            return Encode<T>(Flatten(Input));
        }
        template <Byte_t T= char, Complexstring_t U> [[nodiscard]] constexpr std::basic_string<T> Decode(const U &Input)
        {
            return Decode<T>(Flatten(Input));
        }

        // For compiletime evaluation, an array is fine too.
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] constexpr std::array<T, Encodesize_padded<N>()> Encode(const std::array<U, N> &Input)
        {
            std::array<T, Encodesize_padded<N>()> Result{};
            Encode<U, T>(Input, Result);
            return Result;
        }
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] constexpr std::array<T, Decodesize_padded<N>()> Decode(const std::array<U, N> &Input)
        {
            std::array<T, Decodesize_padded<N>()> Result{};
            Decode<U, T>(Input, Result);
            return Result;
        }

        // String literals need a bit of extra wrangling.
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] consteval std::array<T, Encodesize_padded<N - 1>()> Encode(const U(&Input)[N])
        {
            std::array<T, Encodesize_padded<N - 1>()> Result{};
            if constexpr (!(std::is_same_v<T, char> || std::is_same_v<T, char8_t>))
                Encode<U, T>(std::to_array(Input), Result);
            else Encode<U, T>(char_array(Input), Result);
            return Result;
        }
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] consteval std::array<T, Decodesize_padded<N - 1>()> Decode(const U(&Input)[N])
        {
            std::array<T, Decodesize_padded<N - 1>()> Result{};
            if constexpr (!(std::is_same_v<T, char> || std::is_same_v<T, char8_t>))
                Decode<U, T>(std::to_array(Input), Result);
            else Decode<U, T>(char_array(Input), Result);
            return Result;
        }

        // Only verifies the chars, padding will be added as needed in other functions.
        template <Simplestring_t T> constexpr bool isValid(const T &Input)
        {
            constexpr auto Charset = Blob_view(Table, 85);
            return std::ranges::all_of(Input, [&](auto Char) { return Charset.find(Char) != Charset.npos; });
        }
    }
    namespace RFC1924
    {
        constexpr uint32_t Pow85[] = { 52200625UL, 614125UL, 7225UL, 85UL, 1UL };
        template <Byte_t T, Byte_t U> constexpr void Encode(std::span<const T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Encodesize_padded(Input.size()));
            if (!std::is_constant_evaluated()) assert((Input.size() & 3) == 0);

            for (size_t i = 0; i < Input.size() / 4; ++i)
            {
                const uint32_t inTuple = byteswap(toInt32(&Input[i * 4]));
                const auto outTuple = Output.subspan(i * 5, 5);

                outTuple[0] = U(((inTuple / Pow85[0]) % 85UL) + 33UL);
                outTuple[1] = U(((inTuple / Pow85[1]) % 85UL) + 33UL);
                outTuple[2] = U(((inTuple / Pow85[2]) % 85UL) + 33UL);
                outTuple[3] = U(((inTuple / Pow85[3]) % 85UL) + 33UL);
                outTuple[4] = U(((inTuple / Pow85[4]) % 85UL) + 33UL);
            }
        }
        template <Byte_t T, Byte_t U> constexpr void Decode(std::span<const T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Decodesize_padded(Input.size()));
            if (!std::is_constant_evaluated()) assert(Input.size() % 5 == 0);

            for (size_t i = 0; i < Input.size() / 5; ++i)
            {
                const auto inTuple = Input.subspan(i * 5, 5);
                uint32_t outTuple = 0;

                outTuple += (uint8_t(inTuple[0]) - 33UL) * Pow85[0];
                outTuple += (uint8_t(inTuple[1]) - 33UL) * Pow85[1];
                outTuple += (uint8_t(inTuple[2]) - 33UL) * Pow85[2];
                outTuple += (uint8_t(inTuple[3]) - 33UL) * Pow85[3];
                outTuple += (uint8_t(inTuple[4]) - 33UL) * Pow85[4];

                fromInt32(&Output[i * 4], byteswap(outTuple));
            }
        }

        // Overloads because string-types are special and needs another constructor..
        template <Simplestring_t T, Byte_t U> constexpr void Encode(const T &Input, std::span<U> Output)
        {
            return Encode(std::span{ Input }, Output);
        }
        template <Simplestring_t T, Byte_t U> constexpr void Decode(const T &Input, std::span<U> Output)
        {
            return Decode(std::span{ Input }, Output);
        }
        template <Simplestring_t T, Simplestring_t U> constexpr void Encode(const T &Input, U &Output)
        {
            return Encode(std::span{ Input }, std::span{ Output });
        }
        template <Simplestring_t T, Simplestring_t U> constexpr void Decode(const T &Input, U &Output)
        {
            return Decode(std::span{ Input }, std::span{ Output });
        }

        // Covers most containers available, questionable compiler support for constexpr though.
        template <Byte_t T= char, Simplestring_t U> [[nodiscard]] constexpr std::basic_string<T> Encode(const U &Input)
        {
            const auto Size = std::ranges::size(Input);

            // We may need to pad the input.
            if (Size & 3)
            {
                // Prefer a uniform type with the padding being default initialized.
                std::vector<typename U::value_type> Vec(Input.begin(), Input.end());
                Vec.resize(Vec.size() + (4 - (Size & 3)));
                return Encode<T>(Vec);
            }

            std::basic_string<T> Result(Encodesize_padded(Size), '\0');
            Encode<typename U::value_type, T>(Input, Result);
            return Result;
        }
        template <Byte_t T= char, Simplestring_t U> [[nodiscard]] constexpr std::basic_string<T> Decode(const U &Input)
        {
            const auto Size = std::ranges::size(Input);

            // We may need to pad the input.
            if (Size % 5)
            {
                // Prefer a uniform type with the padding being default initialized.
                std::vector<typename U::value_type> Vec(Input.begin(), Input.end());
                Vec.resize(Vec.size() + (5 - (Size % 5)));
                return Decode<T>(Vec);
            }

            std::basic_string<T> Result(Decodesize_padded(Size), '\0');
            Decode<typename U::value_type, T>(Input, Result);
            return Result;
        }
        template <Byte_t T= char, Complexstring_t U> [[nodiscard]] constexpr std::basic_string<T> Encode(const U &Input)
        {
            return Encode<T>(Flatten(Input));
        }
        template <Byte_t T= char, Complexstring_t U> [[nodiscard]] constexpr std::basic_string<T> Decode(const U &Input)
        {
            return Decode<T>(Flatten(Input));
        }

        // For compiletime evaluation, an array is fine too.
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] constexpr std::array<T, Encodesize_padded<N>()> Encode(const std::array<U, N> &Input)
        {
            std::array<T, Encodesize_padded<N>()> Result{};
            Encode<U, T>(Input, Result);
            return Result;
        }
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] constexpr std::array<T, Decodesize_padded<N>()> Decode(const std::array<U, N> &Input)
        {
            std::array<T, Decodesize_padded<N>()> Result{};
            Decode<U, T>(Input, Result);
            return Result;
        }

        // String literals need a bit of extra wrangling.
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] consteval std::array<T, Encodesize_padded<N - 1>()> Encode(const U(&Input)[N])
        {
            std::array<T, Encodesize_padded<N - 1>()> Result{};
            if constexpr (!(std::is_same_v<T, char> || std::is_same_v<T, char8_t>))
                Encode<U, T>(std::to_array(Input), Result);
            else Encode<U, T>(char_array(Input), Result);
            return Result;
        }
        template <size_t N, Byte_t T= char, Byte_t U> [[nodiscard]] consteval std::array<T, Decodesize_padded<N - 1>()> Decode(const U(&Input)[N])
        {
            std::array<T, Decodesize_padded<N - 1>()> Result{};
            if constexpr (!(std::is_same_v<T, char> || std::is_same_v<T, char8_t>))
                Decode<U, T>(std::to_array(Input), Result);
            else Decode<U, T>(char_array(Input), Result);
            return Result;
        }

        // Only verifies the chars, padding will be added as needed in other functions.
        template <Simplestring_t T> constexpr bool isValid(const T &Input)
        {
            constexpr auto Charset = std::string_view("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_\'{|}~");
            return std::ranges::all_of(Input, [&](auto Char) { return Charset.find(Char) != Charset.npos; });
        }
    }

    // Default to RFC1924.
    using namespace RFC1924;

    // Sanity checking.
    static_assert(Z85::Decode(Z85::Encode("1234")) == RFC1924::Decode(RFC1924::Encode("1234")), "Someone fucked with the Base85 encoding..");
}
