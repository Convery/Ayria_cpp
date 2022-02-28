/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-13
    License: MIT
*/

#pragma once
#include <Utilities/Wrappers/Endian.hpp>
#include <Utilities/Constexprhelpers.hpp>

// TODO(tcn): Benchmark the two implementation-styles and converge on the best one.
namespace Base85
{
    constexpr size_t Encodesize_min(size_t N)  { return N * 5 / 4; }
    constexpr size_t Decodesize_min(size_t N)  { return N * 4 / 5; }
    constexpr size_t Encodesize(size_t N) { return Encodesize_min(N) + ((Encodesize_min(N) % 5) ? (5 - (Encodesize_min(N) % 5)) : 0); }
    constexpr size_t Decodesize(size_t N) { return Decodesize_min(N) + ((Decodesize_min(N) & 3) ? (4 - (Decodesize_min(N) & 3)) : 0); }

    // Zero-MQ version, for source-code embedding.
    namespace Z85
    {
        constexpr uint8_t Table[85]
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
        constexpr uint8_t Reversetable[128]
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

        template <cmp::Byte_t T, size_t N> requires((N & 3) == 0) constexpr cmp::Vector_t<T, Encodesize(N)> Encode(const std::array<T, N> &Input)
        {
            cmp::Vector_t<T, Encodesize(N)> Output{};
            const auto sOutput = std::span(Output);

            for (size_t i = 0; i < N / 4; ++i)
            {
                uint32_t Accumulator, Rem;
                Accumulator = std::byteswap(cmp::toINT<uint32_t>(&Input[i * 4]));
                const auto outTuple = sOutput.subspan(i * 5, 5);

                Rem = Div85(Accumulator); outTuple[4] = T(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[3] = T(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[2] = T(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[1] = T(Table[Accumulator - Rem * 85UL]);
                outTuple[0] = T(Table[Rem]);
            }

            return Output;
        }
        template <cmp::Byte_t T, size_t N> requires((N % 5) == 0) constexpr cmp::Vector_t<T, Decodesize(N)> Decode(const std::array<T, N> &Input)
        {
            cmp::Vector_t<T, Decodesize(N)> Output{};
            const auto sInput = std::span(Input);

            for (size_t i = 0; i < N / 5; ++i)
            {
                const auto inTuple = sInput.subspan(i * 5, 5);
                uint32_t Accumulator = 0;

                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[0]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[1]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[2]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[3]) - 32UL) & 127];
                Accumulator = Accumulator * 85 + Reversetable[(uint8_t(inTuple[4]) - 32UL) & 127];

                cmp::fromINT(&Output[i * 4], Accumulator);
            }

            return Output;
        }

        // Runtime encoding.
        template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Encode(std::span<T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Encodesize(Input.size()));
            if (!std::is_constant_evaluated()) assert((Input.size() & 3) == 0);

            for (size_t i = 0; i < Input.size() / 4; ++i)
            {
                uint32_t Accumulator, Rem;
                Accumulator = std::byteswap(cmp::toINT<uint32_t>(&Input[i * 4]));
                const auto outTuple = Output.subspan(i * 5, 5);

                Rem = Div85(Accumulator); outTuple[4] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[3] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[2] = U(Table[Accumulator - Rem * 85UL]); Accumulator = Rem;
                Rem = Div85(Accumulator); outTuple[1] = U(Table[Accumulator - Rem * 85UL]);
                outTuple[0] = U(Table[Rem]);
            }

            return Output.subspan(0, Encodesize(Input.size()));
        }
        template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Decode(std::span<T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Decodesize(Input.size()));
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

                cmp::fromINT(&Output[i * 4], Accumulator);
            }

            return Output.subspan(0, Decodesize(Input.size()));
        }

        // Helper to deal with padding.
        template <cmp::Byte_t T, size_t N> requires((N & 3) != 0) constexpr auto Encode(const std::array<T, N> &Input)
        {
            constexpr auto Newarray = cmp::resize_array<T, N, N + (4 - (N & 3))>(Input);
            return Encode(Newarray);
        }
        template <cmp::Byte_t T, size_t N> requires((N % 5) != 0) constexpr auto Decode(const std::array<T, N> &Input)
        {
            constexpr auto Newarray = cmp::resize_array<T, N, N + (5 - (N % 5))>(Input);
            return Decode(Newarray);
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
            constexpr auto Charset = std::string_view("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#");
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

    // Standard Base85, for everything else.
    namespace RFC1924
    {
        constexpr uint32_t Pow85[] = { 52200625UL, 614125UL, 7225UL, 85UL, 1UL };

        // Compiletime encoding, Vector_t is compatible with std::array.
        template <cmp::Byte_t T, size_t N> requires((N & 3) == 0) constexpr cmp::Vector_t<T, Encodesize(N)> Encode(const std::array<T, N> &Input)
        {
            cmp::Vector_t<T, Encodesize(N)> Output{};

            // Rather think about everything as spans.
            const auto sOutput = std::span(Output);
            const auto sInput = std::span(Input);

            for (size_t i = 0; i < N / 4; ++i)
            {
                const uint32_t inTuple = std::byteswap(cmp::toINT<uint32_t>(&Input[i * 4]));
                const auto outTuple = sOutput.subspan(i * 5, 5);

                outTuple[0] = T(((inTuple / Pow85[0]) % 85UL) + 33UL);
                outTuple[1] = T(((inTuple / Pow85[1]) % 85UL) + 33UL);
                outTuple[2] = T(((inTuple / Pow85[2]) % 85UL) + 33UL);
                outTuple[3] = T(((inTuple / Pow85[3]) % 85UL) + 33UL);
                outTuple[4] = T(((inTuple / Pow85[4]) % 85UL) + 33UL);
            }

            return Output;
        }
        template <cmp::Byte_t T, size_t N> requires((N % 5) == 0) constexpr cmp::Vector_t<T, Decodesize(N)> Decode(const std::array<T, N> &Input)
        {
            cmp::Vector_t<T, Decodesize(N)> Output{};
            const auto sInput = std::span(Input);

            for (size_t i = 0; i < N / 5; ++i)
            {
                const auto inTuple = sInput.subspan(i * 5, 5);
                uint32_t outTuple = 0;

                outTuple += (uint8_t(inTuple[0]) - 33UL) * Pow85[0];
                outTuple += (uint8_t(inTuple[1]) - 33UL) * Pow85[1];
                outTuple += (uint8_t(inTuple[2]) - 33UL) * Pow85[2];
                outTuple += (uint8_t(inTuple[3]) - 33UL) * Pow85[3];
                outTuple += (uint8_t(inTuple[4]) - 33UL) * Pow85[4];

                cmp::fromINT(&Output[i * 4], outTuple);
            }

            return Output;
        }

        // Runtime encoding.
        template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Encode(std::span<T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Encodesize(Input.size()));
            if (!std::is_constant_evaluated()) assert((Input.size() & 3) == 0);

            for (size_t i = 0; i < Input.size() / 4; ++i)
            {
                const uint32_t inTuple = std::byteswap(cmp::toINT<uint32_t>(&Input[i * 4]));
                const auto outTuple = Output.subspan(i * 5, 5);

                outTuple[0] = U(((inTuple / Pow85[0]) % 85UL) + 33UL);
                outTuple[1] = U(((inTuple / Pow85[1]) % 85UL) + 33UL);
                outTuple[2] = U(((inTuple / Pow85[2]) % 85UL) + 33UL);
                outTuple[3] = U(((inTuple / Pow85[3]) % 85UL) + 33UL);
                outTuple[4] = U(((inTuple / Pow85[4]) % 85UL) + 33UL);
            }

            return Output.subspan(0, Encodesize(Input.size()));
        }
        template <cmp::Byte_t T, cmp::Byte_t U> constexpr cmp::Vector_t<U> Decode(std::span<T> Input, std::span<U> Output)
        {
            if (!std::is_constant_evaluated()) assert(Output.size() >= Decodesize(Input.size()));
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

                cmp::fromINT(&Output[i * 4], outTuple);
            }

            return Output.subspan(0, Decodesize(Input.size()));
        }

        // Helper to deal with padding.
        template <cmp::Byte_t T, size_t N> requires((N & 3) != 0) constexpr auto Encode(const std::array<T, N> &Input)
        {
            constexpr auto Newarray = cmp::resize_array<T, N, N + (4 - (N & 3))>(Input);
            return Encode(Newarray);
        }
        template <cmp::Byte_t T, size_t N> requires((N % 5) != 0) constexpr auto Decode(const std::array<T, N> &Input)
        {
            constexpr auto Newarray = cmp::resize_array<T, N, N + (5 - (N % 5))>(Input);
            return Decode(Newarray);
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
            constexpr auto Charset = std::string_view("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_\'{|}~");
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

    // Default to RFC1924.
    using namespace RFC1924;
}
