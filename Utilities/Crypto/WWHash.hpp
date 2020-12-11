/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-18

    Algorithms Wheathash (64) and Waterhash (32) by Tommy Ettinger <tommy.ettinger@gmail.com>
    Base algorithm created by Wang Yi <godspeed_china@yeah.net>
*/

#pragma once
#include <cstdint>
#include <string_view>

namespace Hash
{
    namespace Internal
    {
        constexpr uint64_t _wheatp0 = 0xa0761d6478bd642full, _wheatp1 = 0xe7037ed1a0b428dbull, _wheatp2 = 0x8ebc6af09c88c6e3ull;
        constexpr uint64_t _wheatp3 = 0x589965cc75374cc3ull, _wheatp4 = 0x1d8e4e27c47d124full, _wheatp5 = 0xeb44accab455d165ull;
        constexpr uint64_t _waterp0 = 0xa0761d65ull, _waterp1 = 0xe7037ed1ull, _waterp2 = 0x8ebc6af1ull;
        constexpr uint64_t _waterp3 = 0x589965cdull, _waterp4 = 0x1d8e4e27ull, _waterp5 = 0xeb44accbull;

        constexpr size_t Strlen(const char *Input)
        {
            size_t Length = 0;
            while (*Input++) Length++;
            return Length;
        }

        template<size_t N> constexpr uint64_t ROT(const char *p)
        {
            uint64_t Result{};
            if constexpr (N >= 8) Result |= (uint64_t(*p++) << (64 - 8));
            if constexpr (N >= 16) Result |= (uint64_t(*p++) << (64 - 16));
            if constexpr (N >= 32) Result |= (uint64_t(*p++) << (64 - 32));
            return Result;
        }
        constexpr uint64_t Process(uint64_t A, uint64_t B)
        {
            const uint64_t Tmp{ A * B };
            return Tmp - (Tmp >> 32);
        }

        constexpr inline uint64_t Wheathash(const char *key, uint32_t len, uint64_t seed)
        {
            for (uint32_t i = 0; i + 16 <= len; i += 16, key += 16)
            {
                seed = Process(
                    Process(ROT<32>(key) ^ _wheatp1, ROT<32>(key + 4) ^ _wheatp2) + seed,
                    Process(ROT<32>(key + 8) ^ _wheatp3, ROT<32>(key + 12) ^ _wheatp4));
            }

            seed += _wheatp5;
            switch (len & 15)
            {
                case 1:  seed = Process(_wheatp2 ^ seed, ROT<8>(key) ^ _wheatp1); break;
                case 2:  seed = Process(_wheatp3 ^ seed, ROT<16>(key) ^ _wheatp4); break;
                case 3:  seed = Process(ROT<16>(key) ^ seed, ROT<8>(key + 2) ^ _wheatp2); break;
                case 4:  seed = Process(ROT<16>(key) ^ seed, ROT<16>(key + 2) ^ _wheatp3); break;
                case 5:  seed = Process(ROT<32>(key) ^ seed, ROT<8>(key + 4) ^ _wheatp1); break;
                case 6:  seed = Process(ROT<32>(key) ^ seed, ROT<16>(key + 4) ^ _wheatp1); break;
                case 7:  seed = Process(ROT<32>(key) ^ seed, (ROT<16>(key + 4) << 8 | ROT<8>(key + 6)) ^ _wheatp1); break;
                case 8:  seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp0); break;
                case 9:  seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed ^ _wheatp4, ROT<8>(key + 8) ^ _wheatp3); break;
                case 10: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed, ROT<16>(key + 8) ^ _wheatp3); break;
                case 11: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed, ((ROT<16>(key + 8) << 8) | ROT<8>(key + 10)) ^ _wheatp3); break;
                case 12: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed ^ ROT<32>(key + 8), _wheatp4); break;
                case 13: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<8>(key + 12)) ^ _wheatp4); break;
                case 14: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<16>(key + 12)) ^ _wheatp4); break;
                case 15: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _wheatp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<16>(key + 12) << 8 | ROT<8>(key + 14)) ^ _wheatp4); break;
            }
            seed = (seed ^ seed << 16) * (len ^ _wheatp0);
            return seed - (seed >> 31) + (seed << 33);
        }
        constexpr inline uint32_t Waterhash(const char *key, uint32_t len, uint64_t seed)
        {
            for (uint32_t i = 0; i + 16 <= len; i += 16, key += 16)
            {
                seed = Process(
                    Process(ROT<32>(key) ^ _waterp1, ROT<32>(key + 4) ^ _waterp2) + seed,
                    Process(ROT<32>(key + 8) ^ _waterp3, ROT<32>(key + 12) ^ _waterp4));
            }

            seed += _waterp5;
            switch (len & 15)
            {
                case 1:  seed = Process(_waterp2 ^ seed, ROT<8>(key) ^ _waterp1); break;
                case 2:  seed = Process(_waterp3 ^ seed, ROT<16>(key) ^ _waterp4); break;
                case 3:  seed = Process(ROT<16>(key) ^ seed, ROT<8>(key + 2) ^ _waterp2); break;
                case 4:  seed = Process(ROT<16>(key) ^ seed, ROT<16>(key + 2) ^ _waterp3); break;
                case 5:  seed = Process(ROT<32>(key) ^ seed, ROT<8>(key + 4) ^ _waterp1); break;
                case 6:  seed = Process(ROT<32>(key) ^ seed, ROT<16>(key + 4) ^ _waterp1); break;
                case 7:  seed = Process(ROT<32>(key) ^ seed, (ROT<16>(key + 4) << 8 | ROT<8>(key + 6)) ^ _waterp1); break;
                case 8:  seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp0); break;
                case 9:  seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed ^ _waterp4, ROT<8>(key + 8) ^ _waterp3); break;
                case 10: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed, ROT<16>(key + 8) ^ _waterp3); break;
                case 11: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed, ((ROT<16>(key + 8) << 8) | ROT<8>(key + 10)) ^ _waterp3); break;
                case 12: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed ^ ROT<32>(key + 8), _waterp4); break;
                case 13: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<8>(key + 12)) ^ _waterp4); break;
                case 14: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<16>(key + 12)) ^ _waterp4); break;
                case 15: seed = Process(ROT<32>(key) ^ seed, ROT<32>(key + 4) ^ _waterp2) ^ Process(seed ^ ROT<32>(key + 8), (ROT<16>(key + 12) << 8 | ROT<8>(key + 14)) ^ _waterp4); break;
            }
            seed = (seed ^ seed << 16) * (len ^ _waterp0);
            return (uint32_t)(seed - (seed >> 32));
        }
    }

    // Compile-time hashing for fixed-length datablocks.
    [[nodiscard]] constexpr uint32_t WW32(const char *Input, const size_t Length)
    {
        return Internal::Waterhash(Input, Length & 0xFFFFFFFF, Internal::_waterp0);
    }
    [[nodiscard]] constexpr uint64_t WW64(const char *Input, const size_t Length)
    {
        return Internal::Wheathash(Input, Length & 0xFFFFFFFF, Internal::_wheatp0);
    }
    [[nodiscard]] constexpr uint32_t WW32(const void *Input, const size_t Length)
    {
        return Internal::Waterhash((char *)Input, Length & 0xFFFFFFFF, Internal::_waterp0);
    }
    [[nodiscard]] constexpr uint64_t WW64(const void *Input, const size_t Length)
    {
        return Internal::Wheathash((char *)Input, Length & 0xFFFFFFFF, Internal::_wheatp0);
    }

    // Compile-time hashing for null-terminated strings.
    template<size_t N> [[nodiscard]] constexpr uint32_t WW32(char const (&String)[N])
    {
        return WW32(String, N - 1);
    }
    template<size_t N> [[nodiscard]] constexpr uint64_t WW64(char const (&String)[N])
    {
        return WW64(String, N - 1);
    }
    [[nodiscard]] constexpr uint32_t WW32(const char *Input)
    {
        return WW32(Input, Internal::Strlen(Input));
    }
    [[nodiscard]] constexpr uint32_t WW64(const char *Input)
    {
        return WW64(Input, Internal::Strlen(Input));
    }

    // Wrappers for runtime hashing of strings, and C++ 20 compile-time std::strings.
    template<typename T> [[nodiscard]] constexpr uint32_t WW32(std::basic_string_view<T> String)
    {
        return WW32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t WW64(std::basic_string_view<T> String)
    {
        return WW64(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint32_t WW32(const std::basic_string<T> &String)
    {
        return WW32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t WW64(const std::basic_string<T> &String)
    {
        return WW64(String.data(), String.size());
    }

    // Wrappers for random types.
    template<typename T> [[nodiscard]] constexpr uint32_t WW32(T Value) { return WW32(&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint64_t WW64(T Value) { return WW64(&Value, sizeof(Value)); }
}

// Drop-in generic functions for std:: algorithms, containers, and such.
// e.g. std::unordered_set<SillyType, decltype(WW::Hash), decltype(WW::Equal)>
namespace WW
{
    constexpr auto Hash = [](const auto &v) { return Hash::WW64(&v, sizeof(v)); };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
