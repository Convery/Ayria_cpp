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

        // Worlds worst strlen, but compilers get upset.
        [[nodiscard]] constexpr size_t WWStrlen(const char *String)
        {
            return *String ? 1 + WWStrlen(String + 1) : 0;
        }

        template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint64_t toINT64(T *p)
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

        constexpr uint64_t Process(uint64_t A, uint64_t B)
        {
            const uint64_t Tmp{ A * B };
            return Tmp - (Tmp >> 32);
        }

        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint64_t Wheathash(T *Input, size_t Length, uint64_t Seed)
        {
            for (size_t i = 0; i + 16 <= Length; i += 16, Input += 16)
            {
                Seed = Process(
                    Process(toINT64<32>(Input) ^ _wheatp1, toINT64<32>(Input + 4) ^ _wheatp2) + Seed,
                    Process(toINT64<32>(Input + 8) ^ _wheatp3, toINT64<32>(Input + 12) ^ _wheatp4));
            }

            Seed += _wheatp5;
            switch (Length & 15)
            {
                case 1:  Seed = Process(_wheatp2 ^ Seed, toINT64<8>(Input) ^ _wheatp1); break;
                case 2:  Seed = Process(_wheatp3 ^ Seed, toINT64<16>(Input) ^ _wheatp4); break;
                case 3:  Seed = Process(toINT64<16>(Input) ^ Seed, toINT64<8>(Input + 2) ^ _wheatp2); break;
                case 4:  Seed = Process(toINT64<16>(Input) ^ Seed, toINT64<16>(Input + 2) ^ _wheatp3); break;
                case 5:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<8>(Input + 4) ^ _wheatp1); break;
                case 6:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<16>(Input + 4) ^ _wheatp1); break;
                case 7:  Seed = Process(toINT64<32>(Input) ^ Seed, (toINT64<16>(Input + 4) << 8 | toINT64<8>(Input + 6)) ^ _wheatp1); break;
                case 8:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp0); break;
                case 9:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed ^ _wheatp4, toINT64<8>(Input + 8) ^ _wheatp3); break;
                case 10: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed, toINT64<16>(Input + 8) ^ _wheatp3); break;
                case 11: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed, ((toINT64<16>(Input + 8) << 8) | toINT64<8>(Input + 10)) ^ _wheatp3); break;
                case 12: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed ^ toINT64<32>(Input + 8), _wheatp4); break;
                case 13: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<8>(Input + 12)) ^ _wheatp4); break;
                case 14: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12)) ^ _wheatp4); break;
                case 15: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _wheatp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12) << 8 | toINT64<8>(Input + 14)) ^ _wheatp4); break;
            }
            Seed = (Seed ^ Seed << 16) * (Length ^ _wheatp0);
            return Seed - (Seed >> 31) + (Seed << 33);
        }

        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint32_t Waterhash(T *Input, size_t Length, uint64_t Seed)
        {
            for (size_t i = 0; i + 16 <= Length; i += 16, Input += 16)
            {
                Seed = Process(
                    Process(toINT64<32>(Input) ^ _waterp1, toINT64<32>(Input + 4) ^ _waterp2) + Seed,
                    Process(toINT64<32>(Input + 8) ^ _waterp3, toINT64<32>(Input + 12) ^ _waterp4));
            }

            Seed += _waterp5;
            switch (Length & 15)
            {
                case 1:  Seed = Process(_waterp2 ^ Seed, toINT64<8>(Input) ^ _waterp1); break;
                case 2:  Seed = Process(_waterp3 ^ Seed, toINT64<16>(Input) ^ _waterp4); break;
                case 3:  Seed = Process(toINT64<16>(Input) ^ Seed, toINT64<8>(Input + 2) ^ _waterp2); break;
                case 4:  Seed = Process(toINT64<16>(Input) ^ Seed, toINT64<16>(Input + 2) ^ _waterp3); break;
                case 5:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<8>(Input + 4) ^ _waterp1); break;
                case 6:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<16>(Input + 4) ^ _waterp1); break;
                case 7:  Seed = Process(toINT64<32>(Input) ^ Seed, ((toINT64<16>(Input + 4) << 8) | toINT64<8>(Input + 6)) ^ _waterp1); break;
                case 8:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp0); break;
                case 9:  Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed ^ _waterp4, toINT64<8>(Input + 8) ^ _waterp3); break;
                case 10: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed, toINT64<16>(Input + 8) ^ _waterp3); break;
                case 11: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed, ((toINT64<16>(Input + 8) << 8) | toINT64<8>(Input + 10)) ^ _waterp3); break;
                case 12: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed ^ toINT64<32>(Input + 8), _waterp4); break;
                case 13: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<8>(Input + 12)) ^ _waterp4); break;
                case 14: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12)) ^ _waterp4); break;
                case 15: Seed = Process(toINT64<32>(Input) ^ Seed, toINT64<32>(Input + 4) ^ _waterp2) ^ Process(Seed ^ toINT64<32>(Input + 8), (toINT64<16>(Input + 12) << 8 | toINT64<8>(Input + 14)) ^ _waterp4); break;
            }
            Seed = (Seed ^ (Seed << 16)) * (Length ^ _waterp0);
            return (uint32_t)(Seed - (Seed >> 32));
        }
    }

    // Compile-time hashing for literals.
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint32_t WW32(T(&Input)[N])
    {
        return Internal::Waterhash(Input, N - 1, Internal::_waterp0);
    }
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint64_t WW64(T(&Input)[N])
    {
        return Internal::Wheathash(Input, N - 1, Internal::_wheatp0);
    }

    // Compile-time hashing for fixed-length datablocks.
    [[nodiscard]] constexpr uint32_t WW32(const uint8_t *Input, size_t Length)
    {
        return Internal::Waterhash(Input, Length, Internal::_waterp0);
    }
    [[nodiscard]] constexpr uint64_t WW64(const uint8_t *Input, size_t Length)
    {
        return Internal::Wheathash(Input, Length, Internal::_wheatp0);
    }

    // Run-time hashing for fixed-length datablocks.
    [[nodiscard]] inline uint32_t WW32(const void *Input, const size_t Length)
    {
        return Internal::Waterhash((const uint8_t *)Input, Length, Internal::_waterp0);
    }
    [[nodiscard]] inline uint64_t WW64(const void *Input, const size_t Length)
    {
        return Internal::Wheathash((const uint8_t *)Input, Length, Internal::_wheatp0);
    }

    // Run-time hashing for dynamic data, constexpr in C++20.
    [[nodiscard]] constexpr uint32_t WW32(const char *Input)
    {
         return Internal::Waterhash(Input, Internal::WWStrlen(Input), Internal::_waterp0);
    }
    [[nodiscard]] constexpr uint64_t WW64(const char *Input)
    {
        return Internal::Wheathash(Input, Internal::WWStrlen(Input), Internal::_wheatp0);
    }
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
