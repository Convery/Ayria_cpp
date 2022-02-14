/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-05
    License: MIT

    Runtime implementation provided by OpenSSL for hardware support.
*/

#pragma once
#include <Utilities\Constexprhelpers.hpp>

namespace Hash
{
    namespace SHAInternal
    {
        #pragma region Data
        static constexpr uint32_t kSHA256[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
        static constexpr uint64_t kSHA512[80] = {
            0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
            0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
            0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
            0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
            0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
            0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
            0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
            0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
            0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
            0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
            0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
            0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
            0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
            0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
            0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
            0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
            0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
            0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
            0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
            0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
        };
        #pragma endregion

        template <cmp::Bytealigned_t T> constexpr void Transform256(std::array<uint32_t, 8> &State, std::span<const T> Input)
        {
            std::array<uint32_t, 8> Copy{ State };
            std::array<uint32_t, 64> Scratch{};

            for (uint8_t i = 0; i < 16; ++i)
            {
                Scratch[i] = Endian::toBig(cmp::toINT<uint32_t>(Input.data() + i * sizeof(uint32_t)));
            }

            for (uint8_t i = 16; i < 64; ++i)
            {
                const uint32_t Sigma0 = std::rotr(Scratch[i - 15], 7) ^ std::rotr(Scratch[i - 15], 18) ^ Scratch[i - 15] >> 3;
                const uint32_t Sigma1 = std::rotr(Scratch[i - 2], 17) ^ std::rotr(Scratch[i - 2], 19) ^ Scratch[i - 2] >> 10;
                Scratch[i] = (Scratch[i - 16] + Sigma0 + Scratch[i - 7] + Sigma1);
            }

            for (uint8_t i = 0; i < 64; ++i)
            {
                const uint32_t Sigma0 = std::rotr(Copy[0], 2) ^ std::rotr(Copy[0], 13) ^ std::rotr(Copy[0], 22);
                const uint32_t Sigma1 = std::rotr(Copy[4], 6) ^ std::rotr(Copy[4], 11) ^ std::rotr(Copy[4], 25);
                const uint32_t Maj = (Copy[0] & Copy[1]) ^ (Copy[0] & Copy[2]) ^ (Copy[1] & Copy[2]);
                const uint32_t Ch = (Copy[4] & Copy[5]) ^ ((~Copy[4]) & Copy[6]);
                const uint32_t t1 = Copy[7] + Sigma1 + Ch + kSHA256[i] + Scratch[i];
                const uint32_t t2 = Sigma0 + Maj;

                Copy[7] = Copy[6];
                Copy[6] = Copy[5];
                Copy[5] = Copy[4];
                Copy[4] = (Copy[3] + t1);
                Copy[3] = Copy[2];
                Copy[2] = Copy[1];
                Copy[1] = Copy[0];
                Copy[0] = (t1 + t2);
            }

            for (uint8_t i = 0; i < 8; ++i)
            {
                State[i] += Copy[i];
            }
        }
        template <cmp::Bytealigned_t T> constexpr void Transform512(std::array<uint64_t, 8> &State, std::span<const T> Input)
        {
            std::array<uint64_t, 8> Copy{ State };
            std::array<uint64_t, 80> Scratch{};

            for (uint8_t i = 0; i < 16; ++i)
            {
                Scratch[i] = Endian::toBig(cmp::toINT<uint64_t>(Input.data() + i * sizeof(uint64_t)));
            }

            for (uint8_t i = 16; i < 80; ++i)
            {
                const uint64_t Sigma0 = std::rotr(Scratch[i - 15], 1) ^ std::rotr(Scratch[i - 15], 8) ^ Scratch[i - 15] >> 7;
                const uint64_t Sigma1 = std::rotr(Scratch[i - 2], 19) ^ std::rotr(Scratch[i - 2], 61) ^ Scratch[i - 2] >> 6;
                Scratch[i] = (Scratch[i - 16] + Sigma0 + Scratch[i - 7] + Sigma1);
            }

            for (uint8_t i = 0; i < 80; ++i)
            {
                const uint64_t Sigma0 = std::rotr(Copy[0], 28) ^ std::rotr(Copy[0], 34) ^ std::rotr(Copy[0], 39);
                const uint64_t Sigma1 = std::rotr(Copy[4], 14) ^ std::rotr(Copy[4], 18) ^ std::rotr(Copy[4], 41);

                const uint64_t Maj = (Copy[0] & Copy[1]) ^ (Copy[0] & Copy[2]) ^ (Copy[1] & Copy[2]);
                const uint64_t Ch = (Copy[4] & Copy[5]) ^ ((~Copy[4]) & Copy[6]);
                const uint64_t t1 = Copy[7] + Sigma1 + Ch + kSHA512[i] + Scratch[i];
                const uint64_t t2 = Sigma0 + Maj;

                Copy[7] = Copy[6];
                Copy[6] = Copy[5];
                Copy[5] = Copy[4];
                Copy[4] = (Copy[3] + t1);
                Copy[3] = Copy[2];
                Copy[2] = Copy[1];
                Copy[1] = Copy[0];
                Copy[0] = (t1 + t2);
            }

            for (uint8_t i = 0; i < 8; ++i)
            {
                State[i] += Copy[i];
            }
        }

        template <cmp::Bytealigned_t T> constexpr std::array<uint8_t, 32> SHA256Compiletime(std::span<const T> Input)
        {
            std::array<uint32_t, 8> State{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
            const auto Remaining = Input.size() - int(Input.size() / 64) * 64;
            const size_t Count = Input.size() / 64;

            for (size_t i = 0; i < Count; ++i)
            {
                Transform256(State, Input.subspan(i * 64, 64));
            }

            std::array<uint8_t, 64> Lastblock{};
            std::ranges::copy(Input.subspan(Input.size() - Remaining), Lastblock.data());

            Lastblock[Remaining] = 0x80;
            if (Remaining >= 56)
            {
                Transform256(State, std::span<const uint8_t>(Lastblock));
                Lastblock = {};
            }

            cmp::fromINT(&Lastblock[56], Endian::toBig<uint64_t>(Input.size() << 3));
            Transform256(State, std::span<const uint8_t>(Lastblock));

            std::array<uint8_t, 32> Result{};
            for (uint8_t i = 0; i < 8; ++i)
            {
                cmp::fromINT(&Result[i * sizeof(uint32_t)], Endian::toBig(State[i]));
            }

            return Result;
        }
        template <cmp::Bytealigned_t T> constexpr std::array<uint8_t, 64> SHA512Compiletime(std::span<const T> Input)
        {
            std::array<uint64_t, 8> State{ 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1, 0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179 };
            const auto Remaining = Input.size() - int(Input.size() / 128) * 128;
            const size_t Count = Input.size() / 128;

            for (size_t i = 0; i < Count; ++i)
            {
                Transform512(State, Input.subspan(i * 128, 128));
            }

            std::array<uint8_t, 128> Lastblock{};
            std::ranges::copy(Input.subspan(Input.size() - Remaining), Lastblock.data());

            Lastblock[Remaining] = 0x80;
            if (Remaining >= 112)
            {
                Transform512(State, std::span<const uint8_t>(Lastblock));
                Lastblock = {};
            }

            cmp::fromINT(&Lastblock[120], Endian::toBig<uint64_t>(Input.size() << 3));
            Transform512(State, std::span<const uint8_t>(Lastblock));

            std::array<uint8_t, 64> Result{};
            for (uint8_t i = 0; i < 8; ++i)
            {
                cmp::fromINT(&Result[i * sizeof(uint64_t)], Endian::toBig(State[i]));
            }

            return Result;
        }

        #if defined (HAS_OPENSSL)
        template <cmp::Bytealigned_t T> [[nodiscard]] inline std::array<uint8_t, 32> SHA256OpenSSL(std::span<const T> Input)
        {
            const auto Context = EVP_MD_CTX_create();
            std::array<uint8_t, 32> Result{};

            EVP_DigestInit_ex(Context, EVP_sha256(), nullptr);
            EVP_DigestUpdate(Context, Input.data(), Input.size());
            EVP_DigestFinal_ex(Context, Result.data(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return Result;
        }
        template <cmp::Bytealigned_t T> [[nodiscard]] inline std::array<uint8_t, 64> SHA512OpenSSL(std::span<const T> Input)
        {
            const auto Context = EVP_MD_CTX_create();
            std::array<uint8_t, 64> Result{};

            EVP_DigestInit_ex(Context, EVP_sha512(), nullptr);
            EVP_DigestUpdate(Context, Input.data(), Input.size());
            EVP_DigestFinal_ex(Context, Result.data(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return Result;
        }
        #endif
    }

    // Any range type.
    template <cmp::Range_t U> constexpr cmp::Vector_t<uint8_t, 32> SHA256(const U &Input)
    {
        if (std::is_constant_evaluated())
        {
            return SHAInternal::SHA256Compiletime(std::span{ Input.cbegin(), Input.cend() });
        }
        else
        {
            #if defined (HAS_OPENSSL)
            return SHAInternal::SHA256OpenSSL(std::span(Input.cbegin(), Input.cend()));
            #else
            return SHAInternal::SHA256Compiletime(std::span(Input.cbegin(), Input.cend()));
            #endif
        }
    };
    template <cmp::Range_t U> constexpr cmp::Vector_t<uint8_t, 64> SHA512(const U &Input)
    {
        if (std::is_constant_evaluated())
        {
            return SHAInternal::SHA512Compiletime(std::span(Input.cbegin(), Input.cend()));
        }
        else
        {
            #if defined (HAS_OPENSSL)
            return SHAInternal::SHA512OpenSSL(std::span(Input.cbegin(), Input.cend()));
            #else
            return SHAInternal::SHA512Compiletime(std::span(Input.cbegin(), Input.cend()));
            #endif
        }
    };

    // Wrapper to deal with string-literals.
    template <cmp::Char_t T, size_t N> constexpr auto SHA256(const T(&Input)[N])
    {
        return SHA256(cmp::make_vector(Input));
    }
    template <cmp::Char_t T, size_t N> constexpr auto SHA512(const T(&Input)[N])
    {
        return SHA512(cmp::make_vector(Input));
    }

    // Helper for other types.
    template <typename T> constexpr auto SHA256(const T &Input)
    {
        return SHA256(cmp::make_vector(Input));
    }
    template <typename T> constexpr auto SHA512(const T &Input)
    {
        return SHA512(cmp::make_vector(Input));
    }
}
