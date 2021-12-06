/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-15
    License: MIT
*/

#pragma once
#include <string>
#include <cstdint>
#include <concepts>

#if defined (HAS_OPENSSL)
#include <openssl/hmac.h>
#include <openssl/evp.h>
#endif

namespace Hash
{
    namespace Internal
    {
        template <typename T> concept Iteratable_t = requires (const T &t) { t.cbegin(); t.cend(); };
        template <typename T> concept Bytealigned_t = sizeof(T) == 1;

        #pragma region Data
        constexpr uint64_t FNV1_Offset_64 = 14695981039346656037ULL;
        constexpr uint64_t FNV1_Prime_64 = 1099511628211ULL;
        constexpr uint32_t FNV1_Offset_32 = 2166136261UL;
        constexpr uint32_t FNV1_Prime_32 = 16777619UL;

        constexpr uint64_t _wheatp0 = 0xa0761d6478bd642full, _wheatp1 = 0xe7037ed1a0b428dbull, _wheatp2 = 0x8ebc6af09c88c6e3ull;
        constexpr uint64_t _wheatp3 = 0x589965cc75374cc3ull, _wheatp4 = 0x1d8e4e27c47d124full, _wheatp5 = 0xeb44accab455d165ull;
        constexpr uint64_t _waterp0 = 0xa0761d65ull, _waterp1 = 0xe7037ed1ull, _waterp2 = 0x8ebc6af1ull;
        constexpr uint64_t _waterp3 = 0x589965cdull, _waterp4 = 0x1d8e4e27ull, _waterp5 = 0xeb44accbull;

        template <size_t N, Bytealigned_t T> constexpr uint64_t toINT64(const T *p)
        {
            uint64_t Result{};

            if constexpr (N >= 64) Result |= (uint64_t(*p++) << (N - 64));
            if constexpr (N >= 56) Result |= (uint64_t(*p++) << (N - 56));
            if constexpr (N >= 48) Result |= (uint64_t(*p++) << (N - 48));
            if constexpr (N >= 40) Result |= (uint64_t(*p++) << (N - 40));
            if constexpr (N >= 32) Result |= (uint64_t(*p++) << (N - 32));
            if constexpr (N >= 24) Result |= (uint64_t(*p++) << (N - 24));
            if constexpr (N >= 16) Result |= (uint64_t(*p++) << (N - 16));
            if constexpr (N >= 8)  Result |= (uint64_t(*p++) << (N - 8));
            return Result;
        }
        constexpr uint64_t WWProcess(uint64_t A, uint64_t B)
        {
            const uint64_t Tmp{ A * B };
            return Tmp - (Tmp >> 32);
        }
        #pragma endregion

        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t FNV1_32(const T *Input, size_t Length)
        {
            uint32_t Hash = FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= FNV1_Prime_32;
                Hash ^= Input[i];
            }
            return Hash;
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t FNV1a_32(const T *Input, size_t Length)
        {
            uint32_t Hash = FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= FNV1_Prime_32;
            }
            return Hash;
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint64_t FNV1_64(const T *Input, size_t Length)
        {
            uint64_t Hash = FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= FNV1_Prime_64;
                Hash ^= Input[i];
            }
            return Hash;
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint64_t FNV1a_64(const T *Input, size_t Length)
        {
            uint64_t Hash = FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= FNV1_Prime_64;
            }
            return Hash;
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t WW32(const T *Input, size_t Length, uint64_t Seed = _waterp0)
        {
            for (size_t i = 0; i + 16 <= Length; i += 16, Input += 16)
            {
                Seed = WWProcess(
                       WWProcess(toINT64<32>(Input) ^ _waterp1, toINT64<32>(Input + 4) ^ _waterp2) + Seed,
                       WWProcess(toINT64<32>(Input + 8) ^ _waterp3, toINT64<32>(Input + 12) ^ _waterp4));
            }

            Seed += _waterp5;
            switch (Length & 15)
            {
                case 1:  Seed = WWProcess(_waterp2 ^ Seed, toINT64<8>(Input) ^ _waterp1); break;
                case 2:  Seed = WWProcess(_waterp3 ^ Seed, toINT64<16>(Input) ^ _waterp4); break;
                case 3:  Seed = WWProcess(toINT64<16>(Input) ^ Seed, toINT64<8>(Input + 2) ^ _waterp2); break;
                case 4:  Seed = WWProcess(toINT64<16>(Input) ^ Seed, toINT64<16>(Input + 2) ^ _waterp3); break;
                case 5:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<8>(Input + 4) ^ _waterp1); break;
                case 6:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<16>(Input + 4) ^ _waterp1); break;
                case 7:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, ((toINT64<16>(Input + 4) << 8) | toINT64<8>(Input + 6)) ^ _waterp1); break;
                case 8:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp0); break;
                case 9:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed ^ _waterp4, toINT64<8>(Input + 8) ^ _waterp3); break;
                case 10: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed, toINT64<16>(Input + 8) ^ _waterp3); break;
                case 11: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed, ((toINT64<16>(Input + 8) << 8) | toINT64<8>(Input + 10)) ^ _waterp3); break;
                case 12: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), _waterp4); break;
                case 13: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<8>(Input + 12)) ^ _waterp4); break;
                case 14: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12)) ^ _waterp4); break;
                case 15: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12) << 8 | toINT64<8>(Input + 14)) ^ _waterp4); break;
            }
            Seed = (Seed ^ (Seed << 16)) * (Length ^ _waterp0);
            return (uint32_t)(Seed - (Seed >> 32));
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint64_t WW64(const T *Input, size_t Length, uint64_t Seed = _wheatp0)
        {
            for (size_t i = 0; i + 16 <= Length; i += 16, Input += 16)
            {
                Seed = WWProcess(
                       WWProcess(toINT64<32>(Input) ^ _wheatp1, toINT64<32>(Input + 4) ^ _wheatp2) + Seed,
                       WWProcess(toINT64<32>(Input + 8) ^ _wheatp3, toINT64<32>(Input + 12) ^ _wheatp4));
            }

            Seed += _wheatp5;
            switch (Length & 15)
            {
                case 1:  Seed = WWProcess(_wheatp2 ^ Seed, toINT64<8>(Input) ^ _wheatp1); break;
                case 2:  Seed = WWProcess(_wheatp3 ^ Seed, toINT64<16>(Input) ^ _wheatp4); break;
                case 3:  Seed = WWProcess(toINT64<16>(Input) ^ Seed, toINT64<8>(Input + 2) ^ _wheatp2); break;
                case 4:  Seed = WWProcess(toINT64<16>(Input) ^ Seed, toINT64<16>(Input + 2) ^ _wheatp3); break;
                case 5:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<8>(Input + 4) ^ _wheatp1); break;
                case 6:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<16>(Input + 4) ^ _wheatp1); break;
                case 7:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, (toINT64<16>(Input + 4) << 8 | toINT64<8>(Input + 6)) ^ _wheatp1); break;
                case 8:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp0); break;
                case 9:  Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed ^ _wheatp4, toINT64<8>(Input + 8) ^ _wheatp3); break;
                case 10: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed, toINT64<16>(Input + 8) ^ _wheatp3); break;
                case 11: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed, ((toINT64<16>(Input + 8) << 8) | toINT64<8>(Input + 10)) ^ _wheatp3); break;
                case 12: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), _wheatp4); break;
                case 13: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<8>(Input + 12)) ^ _wheatp4); break;
                case 14: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12)) ^ _wheatp4); break;
                case 15: Seed = WWProcess(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ WWProcess(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12) << 8 | toINT64<8>(Input + 14)) ^ _wheatp4); break;
            }
            Seed = (Seed ^ Seed << 16) * (Length ^ _wheatp0);
            return Seed - (Seed >> 31) + (Seed << 33);
        }

        template <Bytealigned_t T, uint32_t Polynomial, bool Shiftright> [[nodiscard]]
        constexpr uint32_t CRC32(const T *Input, size_t Length, uint32_t IV)
        {
            uint32_t CRC = IV;

            for (size_t i = 0; i < Length; ++i)
            {
                if constexpr (Shiftright) CRC ^= Input[i];
                else CRC ^= Input[i] << 24;

                for (size_t b = 0; b < 8; ++b)
                {
                    if constexpr (Shiftright)
                    {
                        if (!(CRC & 1)) CRC >>= 1;
                        else CRC = (CRC >> 1) ^ Polynomial;
                    }
                    else
                    {
                        if (!(CRC & (1UL << 31))) CRC <<= 1;
                        else CRC = (CRC << 1) ^ Polynomial;
                    }
                }
            }

            return ~CRC;
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32A(const T *Input, size_t Length)
        {
            return CRC32<T, 0x04C11DB7, false>(Input, Length, 0xFFFFFFFF);
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32B(const T *Input, size_t Length)
        {
            return CRC32<T, 0xEDB88320, true>(Input, Length, 0xFFFFFFFF);
        }
        template <Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32T(const T *Input, size_t Length)
        {
            return CRC32<T, 0xEDB88320, true>(Input, Length, ~uint32_t(Length));
        }

        #if defined (HAS_OPENSSL)
        template <Bytealigned_t T> [[nodiscard]] inline std::string MD5(const T *Input, const size_t Size)
        {
            const auto Buffer = std::make_unique<uint8_t[]>(16);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit_ex(Context, EVP_md5(), nullptr);
            EVP_DigestUpdate(Context, Input, Size);
            EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return std::string((char *)Buffer.get(), 16);
        }
        template <Bytealigned_t T> [[nodiscard]] inline std::string SHA1(const T *Input, const size_t Size)
        {
            const auto Buffer = std::make_unique<uint8_t[]>(20);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit_ex(Context, EVP_sha1(), nullptr);
            EVP_DigestUpdate(Context, Input, Size);
            EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return std::string((char *)Buffer.get(), 20);
        }
        template <Bytealigned_t T> [[nodiscard]] inline std::string SHA256(const T *Input, const size_t Size)
        {
            const auto Buffer = std::make_unique<uint8_t[]>(32);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit_ex(Context, EVP_sha256(), nullptr);
            EVP_DigestUpdate(Context, Input, Size);
            EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return std::string((char *)Buffer.get(), 32);
        }
        template <Bytealigned_t T> [[nodiscard]] inline std::string SHA512(const T *Input, const size_t Size)
        {
            const auto Buffer = std::make_unique<uint8_t[]>(64);
            const auto Context = EVP_MD_CTX_create();

            EVP_DigestInit_ex(Context, EVP_sha512(), nullptr);
            EVP_DigestUpdate(Context, Input, Size);
            EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return std::string((char *)Buffer.get(), 64);
        }
        #endif

        // Compile-time hashing for simple fixed-length datablocks.
        template <typename F, Bytealigned_t T, size_t N> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const std::array<T, N> &Input)
        {
            return Func(Input.data(), N);
        }
        template <typename F, typename T, size_t N> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const std::array<T, N> &Input)
        {
            return Func((const uint8_t *)Input.data(), sizeof(T) * N);
        }
        template <typename F, Bytealigned_t T> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T *Input, size_t Length)
        {
            return Func(Input, Length);
        }
        template <typename F, typename T> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T *Input, size_t Length)
        {
            return Func((const uint8_t *)Input, sizeof(T) * Length);
        }

        // Run-time hashing for dynamic data, constexpr in C++20.
        template <typename F, Iteratable_t T> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T &Vector) requires Bytealigned_t<typename T::value_type>
        {
            return Func(Vector.data(), std::distance(Vector.cbegin(), Vector.cend()));
        }
        template <typename F, Iteratable_t T> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T &Vector)
        {
            return Func((const uint8_t *)Vector.data(), std::distance(Vector.cbegin(), Vector.cend()) * sizeof(T::value_type));
        }

        // Compile-time hashing for string literals.
        template <typename F, Bytealigned_t T, size_t N> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T(&Input)[N])
        {
            return Func(Input, N - 1);
        }
        template <typename F, typename T, size_t N> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T(&Input)[N])
        {
            return Func((const uint8_t *)&Input, sizeof(T) * (N - 1));
        }

        // Wrappers for random types, constexpr depending on compiler.
        template <typename F, typename T> [[nodiscard]] constexpr decltype(auto) doHash(F &&Func, const T &Value)
        {
            static_assert(!std::is_pointer_v<T>, "Trying to hash a pointer, cast to integer if intentional.");
            return Func((const uint8_t *)&Value, sizeof(Value));
        }
    }

    // Aliases for the different types.
    #define Impl(x) template <typename ...Args> [[nodiscard]] constexpr decltype(auto) x (Args&& ...va) \
                    { return Internal::doHash([](auto...X){ return Internal:: x(X...); }, std::forward<Args>(va)...); }
    Impl(FNV1_32); Impl(FNV1_64);
    Impl(FNV1a_32); Impl(FNV1a_64);
    Impl(WW32); Impl(WW64);
    Impl(CRC32A); Impl(CRC32B); Impl(CRC32T);

    #if defined (HAS_OPENSSL)
    Impl(MD5); Impl(SHA1); Impl(SHA256); Impl(SHA512);
    inline std::string HMACSHA1(const void *Input, const size_t Size, const void *Key, const size_t Keysize)
    {
        unsigned int Buffersize = 20;
        unsigned char Buffer[20]{};

        HMAC(EVP_sha1(), Key, uint32_t(Keysize), (uint8_t *)Input, Size, Buffer, &Buffersize);
        return std::string((char *)Buffer, Buffersize);
    }
    inline std::string HMACSHA256(const void *Input, const size_t Size, const void *Key, const size_t Keysize)
    {
        unsigned int Buffersize = 32;
        unsigned char Buffer[32]{};

        HMAC(EVP_sha256(), Key, uint32_t(Keysize), (uint8_t *)Input, Size, Buffer, &Buffersize);
        return std::string((char *)Buffer, Buffersize);
    }
    inline std::string HMACSHA512(const void *Input, const size_t Size, const void *Key, const size_t Keysize)
    {
        unsigned int Buffersize = 64;
        unsigned char Buffer[64]{};

        HMAC(EVP_sha512(), Key, uint32_t(Keysize), (uint8_t *)Input, Size, Buffer, &Buffersize);
        return std::string((char *)Buffer, Buffersize);
    }
    #endif
    #undef Impl

    // Sanity checking.
    static_assert(WW32("12345") == 0xEE98FD70, "Someone fucked with WW32.");
    static_assert(FNV1_32("12345") == 0xDEEE36FA, "Someone fucked with FNV32.");
    static_assert(CRC32A("12345") == 0x426548B8, "Someone fucked with CRC32-A.");
    static_assert(CRC32B("12345") == 0xCBF53A1C, "Someone fucked with CRC32-B.");
    static_assert(CRC32T("12345") == 0x0315B56C, "Someone fucked with CRC32-T.");
    static_assert(FNV1a_32("12345") == 0x43C2C0D8, "Someone fucked with FNV32a.");
    static_assert(WW64("12345") == 0x3C570C468027DB01ULL, "Someone fucked with WW64.");
    static_assert(FNV1_64("12345") == 0xA92F4455DA95A77AULL, "Someone fucked with FNV64.");
    static_assert(FNV1a_64("12345") == 0xE575E8883C0F89F8ULL, "Someone fucked with FNV64a.");
}

// Drop-in generic functions for std:: algorithms, containers, and such.
// e.g. std::unordered_set<SillyType, decltype(X::Hash), decltype(X::Equal)>
namespace FNV
{
    constexpr auto Hash = [](const auto &v)
    {
        if constexpr(sizeof(size_t) == sizeof(uint32_t)) return Hash::FNV1a_32(v);
        else return Hash::FNV1a_64(v);
    };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
namespace WW
{
    constexpr auto Hash = [](const auto &v)
    {
        if constexpr (sizeof(size_t) == sizeof(uint32_t)) return Hash::WW32(v);
        else return Hash::WW64(v);
    };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
