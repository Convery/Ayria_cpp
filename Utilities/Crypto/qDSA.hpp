/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-01
    License: MIT

    quotient Digital Signature Algorithm
    arXiv:1709.03358
*/

#pragma once
#include <Stdinclude.hpp>

namespace qDSA
{
    namespace
    {
        // 128-bit point element.
        struct FE128_t final
        {
            union
            {
                __m128 V; // Compiler hint.
                uint8_t Byte[128 / 8]{};
            };

            constexpr FE128_t() : Byte{} {}
            constexpr FE128_t(const FE128_t &Input) : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE128_t(FE128_t &&Input) noexcept : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE128_t(std::array<uint8_t, 128 / 8> &&Input) : Byte{} { std::ranges::copy(Input, Byte); }
            constexpr FE128_t(const std::array<uint8_t, 128 / 8> &Input) : Byte{} { std::ranges::copy(Input, Byte); }

            // Just copy the bytes.
            constexpr FE128_t &operator=(FE128_t &&Right) noexcept
            {
                std::ranges::move(Right.Byte, Byte);
                return *this;
            }
            constexpr FE128_t &operator=(const FE128_t &Right)
            {
                std::ranges::copy(Right.Byte, Byte);
                return *this;
            }

            // Simplify access to Byte.
            constexpr uint8_t &operator[](size_t i) { return Byte[i]; }
            constexpr uint8_t operator[](size_t i) const { return Byte[i]; }
            constexpr operator std::array<uint8_t, 128 / 8>() const { return std::to_array(Byte); }

            // Extracted due to needing expansion and reduction.
            friend constexpr FE128_t operator +(const FE128_t &Left, const FE128_t &Right);
            friend constexpr FE128_t operator -(const FE128_t &Left, const FE128_t &Right);
            friend constexpr FE128_t operator *(const FE128_t &Left, const FE128_t &Right);
            constexpr FE128_t &operator +=(const FE128_t &Right) { *this = (*this + Right); return *this; }
            constexpr FE128_t &operator -=(const FE128_t &Right) { *this = (*this - Right); return *this; }
            constexpr FE128_t &operator *=(const FE128_t &Right) { *this = (*this * Right); return *this; }
        };

        // 256 bit compressed Kummer point.
        struct FE256_t final
        {
            union
            {
                __m256 V; // Compiler hint.
                uint8_t Byte[256 / 8];
                struct { FE128_t X, Y; };
            };

            constexpr FE256_t() : Byte{} {}
            constexpr FE256_t(const FE256_t &Input) : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE256_t(FE256_t &&Input) noexcept : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE256_t(std::array<uint8_t, 256 / 8> &&Input) : Byte{} { std::ranges::copy(Input, Byte); }
            constexpr FE256_t(const std::array<uint8_t, 256 / 8> &Input) : Byte{} { std::ranges::copy(Input, Byte); }

            constexpr FE256_t(const FE128_t &A, const FE128_t &B) { X = A; Y = B; }
            constexpr FE256_t(FE128_t &&A, FE128_t &&B) { X = std::move(A); Y = std::move(B); }

            // Just copy the bytes.
            constexpr FE256_t &operator=(FE256_t &&Right) noexcept
            {
                std::ranges::move(Right.Byte, Byte);
                return *this;
            }
            constexpr FE256_t &operator=(const FE256_t &Right)
            {
                std::ranges::copy(Right.Byte, Byte);
                return *this;
            }

            // Simplify access to Byte.
            constexpr uint8_t &operator[](size_t i) { return Byte[i]; }
            constexpr uint8_t operator[](size_t i) const { return Byte[i]; }
            constexpr operator std::array<uint8_t, 256 / 8>() const { return std::to_array(Byte); }
        };

        // 512 bit uncompressed Kummer point.
        struct FE512_t final
        {
            union
            {
                __m512 V; // Compiler hint.
                uint8_t Byte[512 / 8];
                struct { FE256_t High, Low; };
                struct { FE128_t X, Y, Z, W; };
            };

            constexpr FE512_t() : Byte{} {}
            constexpr FE512_t(const FE512_t &Input) : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE512_t(FE512_t &&Input) noexcept : Byte{} { std::ranges::copy(Input.Byte, Byte); }
            constexpr FE512_t(std::array<uint8_t, 512 / 8> &&Input) : Byte{} { std::ranges::copy(Input, Byte); }
            constexpr FE512_t(const std::array<uint8_t, 512 / 8> &Input) : Byte{} { std::ranges::copy(Input, Byte); }

            // Just copy the bytes.
            constexpr FE512_t &operator=(FE512_t &&Right) noexcept
            {
                std::ranges::move(Right.Byte, Byte);
                return *this;
            }
            constexpr FE512_t &operator=(const FE512_t &Right)
            {
                std::ranges::copy(Right.Byte, Byte);
                return *this;
            }

            // Simplify access to Byte.
            constexpr uint8_t &operator[](size_t i) { return Byte[i]; }
            constexpr uint8_t operator[](size_t i) const { return Byte[i]; }
            constexpr operator std::array<uint8_t, 512 / 8>() const { return std::to_array(Byte); }

            constexpr FE512_t(const FE256_t &A, const FE256_t &B) { High = A; Low = B; }
            constexpr FE512_t(FE256_t &&A, FE256_t &&B) { High = std::move(A); Low = std::move(B); }
            constexpr FE512_t(const FE128_t &A, const FE128_t &B, const FE128_t &C, const FE128_t &D) { X = A; Y = B; Z = C; W = D; }
            constexpr FE512_t(FE128_t &&A, FE128_t &&B, FE128_t &&C, FE128_t &&D) { X = std::move(A); Y = std::move(B); Z = std::move(C); W = std::move(D); }
        };

        // Partial addition with an offset for sets.
        constexpr FE512_t Addpartial(const FE512_t &Left, const FE256_t &Right, uint8_t Offset)
        {
            uint8_t Carry = 0;
            FE512_t Result{ Left };

            for (uint8_t i = 0; i < 32; ++i)
            {
                const uint16_t Temp = uint16_t(Left[i + Offset]) + uint16_t(Right[i]) + Carry;
                Carry = (Temp >> 8) & 1;
                Result[i + Offset] = uint8_t(Temp);
            }

            for (uint8_t i = 32 + Offset; i < 64; ++i)
            {
                const uint16_t Temp = uint16_t(Left[i]) + Carry;
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            return Result;
        }

        // Helpers for N-bit <-> N-bit conversion.
        constexpr FE256_t Expand(const FE128_t &X, const FE128_t &Y)
        {
            std::array<uint16_t, 32> Buffer{};
            std::array<uint8_t, 32> Result{};

            for (uint8_t i = 0; i < 16; ++i)
            {
                for (uint8_t c = 0; c < 16; ++c)
                {
                    const uint16_t Temp = uint16_t(X[i]) * uint16_t(Y[c]);
                    Buffer[i + c + 1] += (Temp >> 8) & 0xFF;
                    Buffer[i + c] += Temp & 0xFF;
                }
            }

            for (uint8_t i = 0; i < 31; ++i)
            {
                Buffer[i + 1] += Buffer[i] >> 8;
                Result[i] = uint8_t(Buffer[i]);
            }

            Result[31] = uint8_t(Buffer[31]);
            return Result;
        }
        constexpr FE512_t Expand(const FE256_t &X, const FE256_t &Y)
        {
            FE512_t Result{};
            FE256_t Temp;

            Result.High = Expand(X.X, Y.X);

            Temp = Expand(X.X, Y.Y);
            Result = Addpartial(Result, Temp, 16);

            Temp = Expand(X.Y, Y.X);
            Result = Addpartial(Result, Temp, 16);

            Temp = Expand(X.Y, Y.Y);
            Result = Addpartial(Result, Temp, 32);

            return Result;
        }
        constexpr FE128_t Reduce(const FE256_t &Input)
        {
            std::array<uint16_t, 16> Buffer{};
            std::array<uint8_t, 16> Result{};

            for (uint8_t i = 0; i < 16; ++i)
            {
                Buffer[i] = uint16_t(Input[i]);
                Buffer[i] += 2 * Input[i + 16];
            }

            // NOTE(tcn): After 4 million iterations, it looks good enough..
            // TODO(tcn): Verify if we need to do two iterations.
            for (uint8_t i = 0; i < 15; ++i)
            {
                Buffer[i + 1] += (Buffer[i] >> 8);
                Buffer[i] &= 0x00FF;
            }

            Buffer[0] += 2 * (Buffer[15] >> 8);
            Buffer[15] &= 0x00FF;

            for (uint8_t i = 0; i < 15; ++i)
            {
                Buffer[i + 1] += (Buffer[i] >> 8);
                Result[i] = uint8_t(Buffer[i]);
            }

            Result[15] = uint8_t(Buffer[15]);
            return Result;
        }
        constexpr FE256_t Reduce(const FE512_t &Input)
        {
            constexpr FE256_t L1
            { {
                0xbd, 0x05, 0x0c, 0x84, 0x4b, 0x0b, 0x73, 0x47,
                0xff, 0x54, 0xa1, 0xf9, 0xc9, 0x7f, 0xc2, 0xd2,
                0x94, 0x52, 0xc7, 0x20, 0x98, 0xd6, 0x34, 0x03
            } };
            constexpr FE256_t L6
            { {
                0x40, 0x6f, 0x01, 0x03, 0xe1, 0xd2, 0xc2, 0xdc,
                0xd1, 0x3f, 0x55, 0x68, 0x7e, 0xf2, 0x9f, 0xb0,
                0x34, 0xa5, 0xd4, 0x31, 0x08, 0xa6, 0x35, 0xcd
            } };

            FE512_t Temp, Buffer = Input;
            FE256_t Result;

            for (uint8_t i = 0; i < 4; ++i)
            {
                Temp = Expand(Buffer.Low, L6);
                for (uint8_t c = 32; c < 64; ++c) Buffer[c] = Temp[c];
                Buffer = Addpartial(Buffer, Temp.High, 0);
            }

            Buffer[33] = (Buffer[32] & 0x1c) >> 2;
            Buffer[32] = Buffer[32] << 6;
            Buffer[32] |= (Buffer[31] & 0xfc) >> 2;
            Buffer[31] &= 0x03;

            Temp = Expand(Buffer.Low, L1);
            for (uint8_t c = 32; c < 64; ++c) Buffer[c] = Temp[c];
            Buffer = Addpartial(Buffer, Temp.High, 0);

            Buffer[33] = 0;
            Buffer[32] = (Buffer[31] & 0x04) >> 2;
            Buffer[31] &= 0x03;

            Temp = Expand(Buffer.Low, L1);
            Buffer[32] = 0;
            Buffer = Addpartial(Buffer, Temp.High, 0);

            for (uint8_t i = 0; i < 32; i++) { Result[i] = Buffer[i]; }
            return Result;
        }

        // Extracted due to needing expansion and reduction.
        constexpr FE128_t operator +(const FE128_t &Left, const FE128_t &Right)
        {
            FE128_t Result{};

            uint8_t Carry = 0;
            for (uint8_t i = 0; i < 16; ++i)
            {
                const uint16_t Temp = uint16_t(Left[i]) + uint16_t(Right[i]) + Carry;
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            Carry *= 2;
            for (uint8_t i = 0; i < 16; ++i)
            {
                const uint16_t Temp = uint16_t(Result[i]) + Carry;
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            return Result;
        }
        constexpr FE128_t operator -(const FE128_t &Left, const FE128_t &Right)
        {
            FE128_t Result{};

            uint8_t Carry = 0;
            for (uint8_t i = 0; i < 16; ++i)
            {
                const uint16_t Temp = uint16_t(Left[i]) - (uint16_t(Right[i]) + Carry);
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            Carry *= 2;
            for (uint8_t i = 0; i < 16; ++i)
            {
                const uint16_t Temp = uint16_t(Result[i]) - Carry;
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            return Result;
        }
        constexpr FE128_t operator *(const FE128_t &Left, const FE128_t &Right)
        {
            return Reduce(Expand(Left, Right));
        }

        // Point helpers.
        constexpr bool isZero(const FE128_t &Input)
        {

            return std::to_array(Input.Byte) == std::to_array(FE128_t{}.Byte);
        }
        constexpr bool isZero(const FE256_t &Input)
        {
            return std::to_array(Input.Byte) == std::to_array(FE256_t{}.Byte);
        }
        constexpr bool isZero(const FE512_t &Input)
        {
            return std::to_array(Input.Byte) == std::to_array(FE512_t{}.Byte);
        }
        constexpr FE128_t Freeze(const FE128_t &Input)
        {
            FE128_t Result;
            auto Carry = Input[15] >> 7;
            Result[15] = Input[15] & 0x7F;

            for (uint8_t i = 0; i < 15; ++i)
            {
                const uint16_t Temp = uint16_t(Input[i]) + Carry;
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            Result[15] += Carry;
            Result[0] += (Result[15] >> 7);
            Result[15] &= 0x7F;

            return Result;
        }
        constexpr FE256_t Negate(const FE256_t &Input)
        {
            constexpr FE256_t N
            { {
                0x43, 0xFA, 0xF3, 0x7B, 0xB4, 0xF4, 0x8C, 0xB8,
                0x0,  0xAB, 0x5E, 0x6,  0x36, 0x80, 0x3D, 0x2D,
                0x6B, 0xAD, 0x38, 0xDF, 0x67, 0x29, 0xCB, 0xFC,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03
            } };

            uint8_t Carry{};
            FE256_t Result;

            for (uint8_t i = 0; i < 32; ++i)
            {
                const uint16_t Temp = uint16_t(N[i]) - (uint16_t(Input[i]) + Carry);
                Carry = (Temp >> 8) & 1;
                Result[i] = uint8_t(Temp);
            }

            return Result;
        }
        constexpr FE128_t Negate(const FE128_t &Input)
        {
            return FE128_t{} - Input;
        }
        constexpr FE128_t InvSQRT(const FE128_t &Input)
        {
            // NOTE(tcn): Avert thine eyes..
            auto A = Input * (Input * Input);
            auto B = A * A;
            FE128_t C;

            B *= B;
            A *= B;
            B = A * A;
            B *= Input;

            C = B * B;
            for (uint8_t i = 0; i < 4; ++i) C *= C;
            B *= C;

            C = B * B;
            for (uint8_t i = 0; i < 9; ++i) C *= C;
            B *= C;

            C = B * B;
            for (uint8_t i = 0; i < 19; ++i) C *= C;
            B *= C;

            C = B * B;
            for (uint8_t i = 0; i < 39; ++i) C *= C;
            C *= B;

            for (uint8_t i = 0; i < 40; ++i) C *= C;
            C *= B;

            for (uint8_t i = 0; i < 4; ++i) C *= C;

            C *= A;
            C *= C;
            B = C * (Input * Input);
            B *= B;
            C *= B;

            return C;
        }
        constexpr FE128_t SQRT(const FE128_t &Delta, bool Sigma)
        {
            FE128_t Result = InvSQRT(Delta);
            Result *= Delta;

            FE128_t Remainder = Result * Result;
            Remainder -= Delta;

            // Invalid.
            if (!isZero(Remainder)) return {};

            Result = Freeze(Result);
            if ((Result[0] & 1) ^ Sigma)
                Result = Negate(Result);

            return Result;
        }
        constexpr FE128_t Invert(const FE128_t &Input)
        {
            auto Result = InvSQRT(Input * Input);
            const auto Temp = Result * Input;
            Result *= Temp;
            return Result;
        }

        // Kummer surface helpers.
        constexpr FE512_t Square4(FE512_t Input)
        {
            Input.X *= Input.X;
            Input.Y *= Input.Y;
            Input.Z *= Input.Z;
            Input.W *= Input.W;
            return Input;
        }
        constexpr FE512_t Hadamard(const FE512_t &Input)
        {
            const auto A = Input.Y - Input.X;
            const auto B = Input.Z + Input.W;
            const auto C = Input.X + Input.Y;
            const auto D = Input.Z - Input.W;

            return { A + B, A - B, D - C, C + D };
        }
        constexpr std::array<FE128_t, 4> Square4(FE128_t Input[4])
        {
            Input[0] *= Input[0];
            Input[1] *= Input[1];
            Input[2] *= Input[2];
            Input[3] *= Input[3];
            return { Input[0], Input[1], Input[2], Input[3] };
        }
        constexpr FE512_t Multiply4(FE512_t Left, const FE512_t &Right)
        {
            Left.X *= Right.X;
            Left.Y *= Right.Y;
            Left.Z *= Right.Z;
            Left.W *= Right.W;
            return Left;
        }
        constexpr FE128_t Dotproduct(const FE512_t &X, const FE512_t &Y)
        {
            return (X.X * Y.X) + (X.Y * Y.Y) + (X.Z * Y.Z) + (X.W * Y.W);
        }
        constexpr FE128_t negDotproduct(const FE512_t &X, const FE512_t &Y)
        {
            return (X.X * Y.X) - (X.Y * Y.Y) - (X.Z * Y.Z) + (X.W * Y.W);
        }
        constexpr std::array<FE128_t, 4> Hadamard(const FE128_t Input[4])
        {
            const auto A = Input[1] - Input[0];
            const auto B = Input[2] + Input[3];
            const auto C = Input[0] + Input[1];
            const auto D = Input[2] - Input[3];

            return { A + B, A - B, D - C, C + D };
        }
        constexpr std::array<FE128_t, 4> Multiply4(FE128_t Left[4], const FE128_t Right[4])
        {
            Left[0] *= Right[0];
            Left[1] *= Right[1];
            Left[2] *= Right[2];
            Left[3] *= Right[3];
            return { Left[0], Left[1], Left[2], Left[3] };
        }

        // Scalar groups.
        constexpr FE256_t getScalar32(const uint8_t *Input)
        {
            FE512_t Temp{};
            for (uint8_t i = 0; i < 32; ++i)
                Temp[i] = Input[i];
            return Reduce(Temp);
        }
        constexpr FE256_t getScalar64(const uint8_t *Input)
        {
            FE512_t Temp;
            for (uint8_t i = 0; i < 64; ++i)
                Temp[i] = Input[i];
            return Reduce(Temp);
        }
        constexpr FE256_t getScalar32(const std::array<uint8_t, 32> &Input)
        {
            FE512_t Temp{};
            for (uint8_t i = 0; i < 32; ++i)
                Temp[i] = Input[i];
            return Reduce(Temp);
        }
        constexpr FE256_t getScalar64(const std::array<uint8_t, 64> &Input)
        {
            FE512_t Temp;
            for (uint8_t i = 0; i < 64; ++i)
                Temp[i] = Input[i];
            return Reduce(Temp);
        }
        constexpr FE256_t getPositive(const FE256_t &Input)
        {
            if (Input[0] & 1) return Negate(Input);
            else return Input;
        }
        constexpr FE256_t opsScalar(const FE256_t &A, const FE256_t &B, const FE256_t &C)
        {
            auto Temp = Expand(B, C);

            Temp.High = Negate(Reduce(Temp));
            Temp.Low = {};

            Temp = Addpartial(Temp, A, 0);
            return Reduce(Temp);
        }

        // Pre-computing inverted Kummer point coordinates.
        constexpr FE512_t Wrap(const FE512_t &Input)
        {
            const auto A = Invert((Input.Y * Input.Z) * Input.W) * Input.X;
            const auto B = A * Input.W;

            return { {}, { B * Input.Z }, { B * Input.Y }, { (Input.Y * Input.Z) * A} };
        }
        constexpr FE512_t Unwrap(const FE512_t &Input)
        {
            return
            {
                (Input.Y * Input.Z) * Input.W,
                Input.Z * Input.W,
                Input.Y * Input.W,
                Input.Y * Input.Z
            };
        }

        // Scalar pseudo-multiplication using a Montgomery ladder.
        constexpr std::pair<FE512_t, FE512_t> xDBLADD(FE512_t Xp, FE512_t Xq, const FE512_t &Xd)
        {
            constexpr FE512_t eHat{ {{ 0x41, 0x03 }}, {{ 0xC3, 0x09 }}, {{ 0x51, 0x06 }}, {{ 0x31, 0x02 }} };
            constexpr FE512_t e{ {{ 0x72, 0x00 }}, {{ 0x39, 0x00 }}, {{ 0x42, 0x00 }}, {{ 0xA2, 0x01 }} };

            Xq = Hadamard(Xq);
            Xp = Hadamard(Xp);

            Xq = Multiply4(Xq, Xp);
            Xp = Square4(Xp);

            Xq = Multiply4(Xq, eHat);
            Xp = Multiply4(Xp, eHat);

            Xq = Hadamard(Xq);
            Xp = Hadamard(Xp);

            Xq = Square4(Xq);
            Xp = Square4(Xp);

            Xq.Y *= Xd.Y;
            Xq.Z *= Xd.Z;
            Xq.W *= Xd.W;

            Xp = Multiply4(Xp, e);
            return { Xp, Xq };
        }
        constexpr FE512_t Ladder(FE512_t *Xq, const FE512_t &Xd, const FE256_t &Scalars, int Bits)
        {
            constexpr FE512_t mu{ FE128_t{{ 0x0B, 0x00 }}, FE128_t{{ 0x16, 0x00 }}, FE128_t{{ 0x13, 0x00 }}, FE128_t{{ 0x03, 0x00 }} };
            int Previous{};
            auto Xp{ mu };

            for(int i = Bits; i >= 0; i--)
            {
                const auto Bit = (Scalars[i >> 3] >> (i & 0x07)) & 1;
                const auto Swap = Bit ^ Previous;
                Previous = Bit;

                Xq->X = Negate(Xq->X);
                if (Swap) std::swap(Xp, *Xq);
                std::tie(Xp, *Xq) = xDBLADD(Xp, *Xq, Xd);
            }

            Xp.X = Negate(Xp.X);
            if (Previous) std::swap(Xp, *Xq);
            return Xp;
        }
        constexpr FE512_t Ladder(const FE256_t &Scalars, int Bits)
        {
            // Wrapped Kummer base-point.
            const FE512_t WBP
            {
                {},
                {{ 0x48, 0x1A, 0x93, 0x4E, 0xA6, 0x51, 0xB3, 0xAE, 0xE7, 0xC2, 0x49, 0x20, 0xDC, 0xC3, 0xE0, 0x1B }},
                {{ 0xDF, 0x36, 0x7E, 0xE0, 0x18, 0x98, 0x65, 0x64, 0x30, 0xA6, 0xAB, 0x8E, 0xCD, 0x16, 0xB4, 0x23 }},
                {{ 0x1E, 0x44, 0x15, 0x72, 0x05, 0x3D, 0xAE, 0xC7, 0x4D, 0xA2, 0x47, 0x44, 0x38, 0x5C, 0xB3, 0x5D }}
            };

            auto Xq = Unwrap(WBP);
            return Ladder(&Xq, WBP, Scalars, Bits);
        }

        // Evaluate the polynomials at (L1, L2, Tau).
        constexpr std::tuple<FE128_t, FE128_t> getK2(const FE128_t &L1, const FE128_t &L2, bool Tau)
        {
            FE128_t A, B;

            A = L1 * L2 * FE128_t{{ 0x11, 0x12 }};

            if (Tau)
            {
                B = L1 * FE128_t{{ 0xF7, 0x0D }};
                A += B;

                B = L2 * FE128_t{{ 0x99, 0x25 }};
                A -= B;
            }

            A *= {{ 0xE3, 0x2F }};
            A += A;

            B = L1 * FE128_t{{ 0x33, 0x1D }};
            B *= B;
            A = B - A;

            B = L2 * FE128_t{{ 0xE3, 0x2F }};
            B *= B;
            A += B;;

            if (Tau)
            {
                B = {{ 0x0B, 0x2C }};
                B *= B;
                A += B;
            }

            return { A, B };
        }
        constexpr std::tuple<FE128_t, FE128_t, FE128_t> getK3(const FE128_t &L1, const FE128_t &L2, bool Tau)
        {
            FE128_t A, B, C;

            A = L1 * L1;
            B = L2 * L2;

            if (Tau)
            {
                C = {{ 0x01, 0x00 }};
                A += C;
                B += C;
                C = A + B;
            }

            A *= L2 * FE128_t{{ 0xF7, 0x0D }};
            B *= L1 * FE128_t{{ 0x99, 0x25 }};
            A -= B;

            if (Tau)
            {
                B = {{ 0x01, 0x00 }};
                C -= B;
                C -= B;
                C *= {{ 0x11, 0x12 }};
                A += C;
            }

            A *= {{ 0xE3, 0x2F }};

            if (Tau)
            {
                B = L1 * L2 * FE128_t{{ 0x79, 0x17 }} * FE128_t{{ 0xD7, 0xAB }};
                A -= B;
            }

            return { A, B, C };
        }
        constexpr std::tuple<FE128_t, FE128_t> getK4(const FE128_t &L1, const FE128_t &L2, bool Tau)
        {
            FE256_t Result{};
            FE128_t A, B;

            if (Tau)
            {
                A = L1 * FE128_t{{ 0x99, 0x25 }};
                B = L2 * FE128_t{{ 0xF7, 0x0D }};
                B -= A;

                B += {{ 0x11, 0x12 }};
                B *= L1 * L2 * FE128_t{{ 0xE3, 0x2F }};
                B += B;

                A = L1 * FE128_t{{ 0xE3, 0x2F }};
                A *= A;
                B = A - B;

                A = L2 * FE128_t{{ 0x33, 0x1D }};
                A *= A;
                B += A;
            }

            A = L1 * L2 * FE128_t{{ 0x0B, 0x2C }};
            A *= A;

            if (Tau) A += B;

            return { A, B };
        }

        // Compression.
        constexpr FE128_t kRow(const FE128_t &X1, const FE128_t &X2, const FE128_t &X3, const FE128_t &X4)
        {
            constexpr FE512_t kHat( {{ 0xC1, 0x03 }}, {{ 0x80, 0x00 }}, {{ 0x39, 0x02 }}, {{ 0x49, 0x04 }});

            auto Result = X2 * kHat.Y;
            Result += X3 * kHat.Z;
            Result += X4 * kHat.W;
            Result -= X1 * kHat.X;
            return Result;
        }
        constexpr std::tuple<FE128_t, FE128_t>  Compress(const FE512_t &Input)
        {
            FE128_t L1, L2;

            // Matrix multiplication by kHat.
            FE512_t Temp
            {
                kRow(Input.W, Input.Z, Input.Y, Input.X),
                kRow(Input.Z, Input.W, Input.X, Input.Y),
                kRow(Input.Y, Input.X, Input.W, Input.Z),
                kRow(Input.X, Input.Y, Input.Z, Input.W)
            };

            L2 = [&]()
            {
                if (!isZero(Temp.Z)) return Invert(Temp.Z);
                if (!isZero(Temp.Y)) return Invert(Temp.Y);
                if (!isZero(Temp.X)) return Invert(Temp.X);
                return Invert(Temp.W);
            }();


            // Normalize.
            Temp.W *= L2;
            L1 = Temp.X * L2;
            L2 *= Temp.Y;

            // K_2 * L4
            const auto Tau = !isZero(Temp.Z);
            std::tie(Temp.Z, Temp.X) = getK2(L1, L2, Tau);
            Temp.Z *= Temp.W;

            // - K_3
            std::tie(Temp.W, Temp.X, Temp.Y) = getK3(L1, L2, Tau);
            Temp.Z -= Temp.W;

            L1 = Freeze(L1);
            L2 = Freeze(L2);
            Temp.Z = Freeze(Temp.Z);

            L1[15] |= (Tau << 7);
            L2[15] |= ((Temp.Z[0] & 1) << 7);

            return { L1, L2 };
        }

        // Decompression.
        constexpr FE128_t mRow(const FE128_t &X1, const FE128_t &X2, const FE128_t &X3, const FE128_t &X4)
        {
            constexpr FE512_t mu{ {{ 0x0B, 0x00 }}, {{ 0x16, 0x00 }}, {{ 0x13, 0x00 }}, {{ 0x03, 0x00 }} };
            auto Result = X2 + X2;

            Result -= X1;
            Result *= mu.X;
            Result += X3 * mu.Z;
            Result += X4 * mu.W;

            return Result;
        }
        constexpr FE512_t Decompress(const FE256_t &Input)
        {
            FE512_t Result, Temp;

            Result.X = Input.X;
            Result.Y = Input.Y;

            const auto Tau = (Result.X[15] & 0x80) >> 7;
            const auto Sigma = (Result.Y[15] & 0x80) >> 7;
            Result.X[15] &= 0x7F;
            Result.Y[15] &= 0x7F;

            std::tie(Temp.Y, Result.Z) = getK2(Result.X, Result.Y, Tau);
            std::tie(Temp.Z, Result.Z, Result.W) = getK3(Result.X, Result.Y, Tau);
            std::tie(Temp.W, Result.Z) = getK4(Result.X, Result.Y, Tau);

            if (!isZero(Temp.Y)) // Y = K_2
            {
                Result.Z = Temp.Z * Temp.Z;
                Result.W = Temp.Y * Temp.W;
                Result.Z -= Result.W;

                const auto Root = SQRT(Result.Z, Sigma);
                if (isZero(Root)) return {};
                Result.W = Root;

                Temp.W = Temp.Z + Result.W;
                if (Tau) Temp.Z = Temp.Y;
                else Temp.Z = {};

                Temp.X = Temp.Y * Result.X;
                Temp.Y *= Result.Y;
            }
            else
            {
                Temp.Z = Freeze(Temp.Z);
                if (isZero(Temp.Z)) // Z = K_3
                {
                    if (!isZero(Result.X) || !isZero(Result.Y) || Tau || Sigma)
                    {
                        return {};
                    }

                    Temp = {};
                    Temp.W[0] = 1;
                }
                else if (Sigma ^ Temp.Z[0])
                {
                    Temp.X = Temp.Z * Result.X;
                    Temp.X += Temp.X;

                    Temp.Y = Temp.Z * Result.Y;
                    Temp.Y += Temp.Y;

                    if (Tau) Temp.Z += Temp.Z;
                    else Temp.Z = {};
                }
                else
                {
                    return {};
                }
            }

            Result.X = mRow(Temp.W, Temp.Z, Temp.Y, Temp.X);
            Result.Y = mRow(Temp.Z, Temp.W, Temp.X, Temp.Y);
            Result.Z = mRow(Temp.Y, Temp.X, Temp.W, Temp.Z);
            Result.W = mRow(Temp.X, Temp.Y, Temp.Z, Temp.W);
            return Result;
        }

        // (Bjj * R1^2) - (2 * C * Bij * R1 * R2) + (Bii * R2^2) == 0
        constexpr bool isQuad(const FE128_t &Bij, const FE128_t &Bjj, const FE128_t &Bii, const FE128_t &R1, const FE128_t &R2)
        {
            constexpr FE128_t C
            {{
                0x43, 0xA8, 0xDD, 0xCD,
                0xD8, 0xE3, 0xF7, 0x46,
                0xDD, 0xA2, 0x20, 0xA3,
                0xEF, 0x0E, 0xF5, 0x40
            }};

            const auto X = (R1 * R1 * Bjj);
            const auto Z = (R2 * R2 * Bii);
            const auto Y = (C * Bij * R1 * R2) + (C * Bij * R1 * R2);

            const auto Result = Freeze(FE128_t{ {1} } + (X - Y + Z));
            return std::to_array(Result.Byte) == std::to_array(FE128_t{ {1} }.Byte);
        }

        // Bi-quadratic form.
        constexpr FE512_t biiValues(const FE512_t &P, const FE512_t &Q)
        {
            constexpr FE512_t muHat{ {{ 0x21, 0x00 }}, {{ 0x0B, 0x00 }}, {{ 0x11, 0x00 }}, {{ 0x31, 0x00 }} };
            constexpr FE512_t eHat{ {{ 0x41, 0x03 }}, {{ 0xC3, 0x09 }}, {{ 0x51, 0x06 }}, {{ 0x31, 0x02 }} };
            constexpr FE512_t k{ {{ 0x59, 0x12 }}, {{ 0x3F, 0x17 }}, {{ 0x79, 0x16 }}, {{ 0xC7, 0x07 }} };
            auto Result = Square4(Q);
            auto T0 = Square4(P);
            FE512_t T1;

            Result = Multiply4(Result, eHat);
            T0 = Multiply4(T0, eHat);

            Result.X = Negate(Result.X);
            T0.X = Negate(T0.X);

            T1.X = Dotproduct({ T0.X, T0.Y, T0.Z, T0.W }, { Result.X, Result.Y, Result.Z, Result.W });
            T1.Y = Dotproduct({ T0.X, T0.Y, T0.Z, T0.W }, { Result.Y, Result.X, Result.W, Result.Z });
            T1.Z = Dotproduct({ T0.X, T0.Z, T0.Y, T0.W }, { Result.Z, Result.X, Result.W, Result.Y });
            T1.W = Dotproduct({ T0.X, T0.W, T0.Y, T0.Z }, { Result.W, Result.X, Result.Z, Result.Y });

            Result.X = negDotproduct({ T1.X, T1.Y, T1.Z, T1.W }, k);
            Result.Y = negDotproduct({ T1.Y, T1.X, T1.W, T1.Z }, k);
            Result.Z = negDotproduct({ T1.Z, T1.W, T1.X, T1.Y }, k);
            Result.W = negDotproduct({ T1.W, T1.Z, T1.Y, T1.X }, k);

            Result = Multiply4(Result, muHat);
            Result.X = Negate(Result.X);
            return Result;
        }
        constexpr FE128_t bijValues(const FE512_t &P, const FE512_t &Q, const FE512_t &C)
        {
            auto Result = P.X * P.Y;
            FE512_t Temp;

            Temp.X = Q.X * Q.Y;
            Temp.Y = P.Z * P.W;
            Result -= Temp.Y;

            Temp.Z = Q.Z * Q.W;
            Temp.X -= Temp.Z;

            Result *= Temp.X;
            Temp.X = Temp.Y * Temp.Z;

            Result *= C.Z;
            Result *= C.W;

            Temp.Y = (C.Z * C.W) + (C.X * C.Y);
            Temp.X *= Temp.Y;

            Result = Temp.X - Result;
            Result *= C.X;
            Result *= C.Y;

            Temp.Y = (C.Y * C.W) + (C.X * C.Z);
            Result *= Temp.Y;

            Temp.Y = (C.Y * C.Z) + (C.X * C.W);
            Result *= Temp.Y;

            return Result;
        }

        // Verification.
        constexpr bool Check(FE512_t &P, FE512_t &Q, const FE256_t &Rcomp)
        {
            constexpr FE512_t muHat{ {{ 0x21, 0x00 }}, {{ 0x0B, 0x00 }}, {{ 0x11, 0x00 }}, {{ 0x31, 0x00 }} };

            P.X = Negate(P.X); P = Hadamard(P); P.W = Negate(P.W);
            Q.X = Negate(Q.X); Q = Hadamard(Q); Q.W = Negate(Q.W);
            const auto Bii = biiValues(P, Q);

            auto R = Decompress(Rcomp);
            if (isZero(R)) return false;
            R.X = Negate(R.X); R = Hadamard(R); R.W = Negate(R.W);

            //

            // B_1,2
            auto Bij = bijValues(P, Q, muHat);
            if (!isQuad(Bij, Bii.Y, Bii.X, R.X, R.Y)) return false;

            // B_1,3
            Bij = bijValues({ P.X, P.Z, P.Y, P.W }, { Q.X, Q.Z, Q.Y, Q.W }, { muHat.X, muHat.Z, muHat.Y, muHat.W });
            if (!isQuad(Bij, Bii.Z, Bii.X, R.X, R.Z)) return false;

            // B_1,4
            Bij = bijValues({ P.X, P.W, P.Y, P.Z }, { Q.X, Q.W, Q.Y, Q.Z }, { muHat.X, muHat.W, muHat.Y, muHat.Z });
            if (!isQuad(Bij, Bii.W, Bii.X, R.X, R.W)) return false;

            // B_2,3
            Bij = bijValues({ P.Y, P.Z, P.X, P.W }, { Q.Y, Q.Z, Q.X, Q.W }, { muHat.Y, muHat.Z, muHat.X, muHat.W });
            Bij = Negate(Bij);
            if (!isQuad(Bij, Bii.Z, Bii.Y, R.Y, R.Z)) return false;

            // B_2,4
            Bij = bijValues({ P.Y, P.W, P.X, P.Z }, { Q.Y, Q.W, Q.X, Q.Z }, { muHat.Y, muHat.W, muHat.X, muHat.Z });
            Bij = Negate(Bij);
            if (!isQuad(Bij, Bii.W, Bii.Y, R.Y, R.W)) return false;

            // B_3,4
            Bij = bijValues({ P.Z, P.W, P.X, P.Y }, { Q.Z, Q.W, Q.X, Q.Y }, { muHat.Z, muHat.W, muHat.X, muHat.Y });
            Bij = Negate(Bij);
            if (!isQuad(Bij, Bii.W, Bii.Z, R.Z, R.W)) return false;

            return true;
        }
    }

    template <typename T> requires (sizeof(T) == 1)
    inline std::array<uint8_t, 64> Sign(const std::array<uint8_t, 32> &Publickey,
                                        const std::array<uint8_t, 32> &Privatekey,
                                        const T *Message, size_t Length)
    {
        // First point.
        const auto Seed1 = [&]()
        {
            // Add a bit of 'randomness' to prevent attacks on multiple PKs at once.
            auto Buffer = std::make_unique<uint8_t[]>(64);
            const auto PRND = Hash::SHA256(Privatekey);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit(Context, EVP_sha512());
            EVP_DigestUpdate(Context, PRND.data(), 32);
            EVP_DigestUpdate(Context, Message, Length);
            EVP_DigestFinal(Context, Buffer.get(), nullptr);

            return std::move(Buffer);
        }();
        auto P = Ladder(getScalar64(Seed1.get()), 250);
        std::tie(P.Z, P.W) = Compress(P);

        // Second point.
        const auto Seed2 = [&]()
        {
            auto Buffer = std::make_unique<uint8_t[]>(64);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit(Context, EVP_sha512());
            EVP_DigestUpdate(Context, P.Z.Byte, 16);
            EVP_DigestUpdate(Context, P.W.Byte, 16);
            EVP_DigestUpdate(Context, Publickey.data(), 32);
            EVP_DigestUpdate(Context, Message, Length);
            EVP_DigestFinal(Context, Buffer.get(), {});

            return std::move(Buffer);
        }();
        const auto Q = getScalar64(Seed2.get());

        // Third point.
        const auto W = opsScalar(getScalar64(Seed1.get()), getPositive(Q), getScalar32(Privatekey.data()));

        // Share with the world..
        std::array<uint8_t, 64> Signature;
        std::memcpy(Signature.data() + 32, W.Byte, 32);
        std::memcpy(Signature.data() + 0, P.Z.Byte, 16);
        std::memcpy(Signature.data() + 16, P.W.Byte, 16);
        return Signature;
    }

    template <typename T> requires (sizeof(T) == 1)
    inline bool Verify(const std::array<uint8_t, 32> &Publickey,
                       const std::array<uint8_t, 64> &Signature,
                       const T *Message, size_t Length)
    {
        // Validate compression.
        auto P = Decompress(FE256_t{ Publickey });
        if (isZero(P)) return false;

        // Second point.
        const auto Seed2 = [&]()
        {
            auto Buffer = std::make_unique<uint8_t[]>(64);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit(Context, EVP_sha512());
            EVP_DigestUpdate(Context, Signature.data(), 32);
            EVP_DigestUpdate(Context, Publickey.data(), 32);
            EVP_DigestUpdate(Context, Message, Length);
            EVP_DigestFinal(Context, Buffer.get(), {});

            return std::move(Buffer);
        }();
        const auto Q = getScalar64(Seed2.get());

        // Third point.
        auto W = Ladder(&P, Wrap(P), Q, 250);
        P = Ladder(getScalar32(FE512_t{ Signature }.Low.Byte), 250);

        return Check(P, W, FE512_t{ Signature }.High);
    }

    inline std::array<uint8_t, 32> Generatesecret(const uint8_t *Publickey, const std::array<uint8_t, 32> &Privatekey)
    {
        std::array<uint8_t, 32> Tmp{};
        std::memcpy(Tmp.data(), Publickey, 32);

        const auto Scalar = getScalar32(Privatekey.data());
        auto PK = Decompress(Tmp);
        const auto PKW = Wrap(PK);

        auto Secret = Ladder(&PK, PKW, Scalar, 250);
        std::tie(Secret.Z, Secret.W) = Compress(Secret);

        std::array<uint8_t, 32> Result;
        std::memcpy(Result.data() + 0, Secret.Z.Byte, 16);
        std::memcpy(Result.data() + 16, Secret.W.Byte, 16);

        return Result;
    }
    inline std::array<uint8_t, 32> Generatesecret(const std::array<uint8_t, 32> &Publickey, const std::array<uint8_t, 32> &Privatekey)
    {
        const auto Scalar = getScalar32(Privatekey.data());
        auto PK = Decompress({ Publickey });
        const auto PKW = Wrap(PK);

        auto Secret = Ladder(&PK, PKW, Scalar, 250);
        std::tie(Secret.Z, Secret.W) = Compress(Secret);

        std::array<uint8_t, 32> Result;
        std::memcpy(Result.data() + 0, Secret.Z.Byte, 16);
        std::memcpy(Result.data() + 16, Secret.W.Byte, 16);

        return Result;
    }
    inline std::tuple<std::array<uint8_t, 32>, std::array<uint8_t, 32>> Createkeypair(std::array<uint8_t, 32> Seed)
    {
        auto PK = Ladder(getScalar32(Seed), 250);
        std::tie(PK.X, PK.Y) = Compress(PK);

        std::array<uint8_t, 32> Pub{}, Private{};
        std::memcpy(Pub.data() + 0, PK.X.Byte, 16);
        std::memcpy(Pub.data() + 16, PK.Y.Byte, 16);
        std::memcpy(Private.data(), Seed.data(), 32);

        return { Pub, Private };
    }
    inline std::tuple<std::array<uint8_t, 32>, std::array<uint8_t, 32>> Createkeypair(uint8_t *Seed)
    {
        auto PK = Ladder(getScalar32(Seed), 250);
        std::tie(PK.X, PK.Y) = Compress(PK);

        std::array<uint8_t, 32> Pub{}, Private{};
        std::memcpy(Pub.data() + 0, PK.X.Byte, 16);
        std::memcpy(Pub.data() + 16, PK.Y.Byte, 16);
        std::memcpy(Private.data(), Seed, 32);

        return { Pub, Private };
    }
}
