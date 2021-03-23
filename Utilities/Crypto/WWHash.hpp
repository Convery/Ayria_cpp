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
    namespace WWInternal
    {
        constexpr uint64_t _wheatp0 = 0xa0761d6478bd642full, _wheatp1 = 0xe7037ed1a0b428dbull, _wheatp2 = 0x8ebc6af09c88c6e3ull;
        constexpr uint64_t _wheatp3 = 0x589965cc75374cc3ull, _wheatp4 = 0x1d8e4e27c47d124full, _wheatp5 = 0xeb44accab455d165ull;
        constexpr uint64_t _waterp0 = 0xa0761d65ull, _waterp1 = 0xe7037ed1ull, _waterp2 = 0x8ebc6af1ull;
        constexpr uint64_t _waterp3 = 0x589965cdull, _waterp4 = 0x1d8e4e27ull, _waterp5 = 0xeb44accbull;

        template <typename T> concept Iteratable_t = requires (const T &t) { t.cbegin(); t.cend(); };
        template <typename T> concept Bytealigned_t = sizeof(T) == 1;

        template <size_t N, Bytealigned_t T> constexpr uint64_t toINT64(T *p)
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

        // Implementation.
        template <Bytealigned_t T> constexpr uint32_t Waterhash(const T *Input, size_t Length, uint64_t Seed = _waterp0)
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
        template <Bytealigned_t T> constexpr uint64_t Wheathash(const T *Input, size_t Length, uint64_t Seed = _wheatp0)
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

        // Compile-time hashing for simple fixed-length datablocks.
        template <Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint32_t WW32(const std::array<T, N> &Input)
        {
            return Waterhash(Input.data(), N);
        }
        template <Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint64_t WW64(const std::array<T, N> &Input)
        {
            return Wheathash(Input.data(), N);
        }
        template <typename T, size_t N> [[nodiscard]] constexpr uint32_t WW32(const std::array<T, N> &Input)
        {
            return Waterhash<uint8_t>((const uint8_t *)Input.data(), sizeof(T) * N);
        }
        template <typename T, size_t N> [[nodiscard]] constexpr uint64_t WW64(const std::array<T, N> &Input)
        {
            return Wheathash<uint8_t>((const uint8_t *)Input.data(), sizeof(T) * N);
        }

        // Compile-time hashing for string literals.
        template <Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint32_t WW32(T(&Input)[N])
        {
            return Waterhash<T>(Input, N - 1);
        }
        template <Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint64_t WW64(T(&Input)[N])
        {
            return Wheathash<T>(Input, N - 1);
        }
        template <typename T, size_t N> [[nodiscard]] constexpr uint32_t WW32(T(&Input)[N])
        {
            return Waterhash<uint8_t>((const uint8_t *)&Input, sizeof(T) * (N - 1));
        }
        template <typename T, size_t N> [[nodiscard]] constexpr uint64_t WW64(T(&Input)[N])
        {
            return Wheathash<uint8_t>((const uint8_t *)&Input, sizeof(T) * (N - 1));
        }

        // Run-time hashing for dynamic data, constexpr in C++20.
        template <Iteratable_t T> [[nodiscard]] constexpr uint32_t WW32(const T &Vector)
            requires Bytealigned_t<typename T::value_type>
        {
            return Waterhash<T::value_type>(Vector.data(), Vector.size());
        }
        template <Iteratable_t T> [[nodiscard]] constexpr uint64_t WW64(const T &Vector)
            requires Bytealigned_t<typename T::value_type>
        {
            return Wheathash<T::value_type>(Vector.data(), Vector.size());
        }
        template <Iteratable_t T> [[nodiscard]] constexpr uint32_t WW32(const T &Vector)
        {
            return Waterhash<uint8_t>((const uint8_t *)Vector.data(), sizeof(T) * Vector.size());
        }
        template <Iteratable_t T> [[nodiscard]] constexpr uint64_t WW64(const T &Vector)
        {
            return Wheathash<uint8_t>((const uint8_t *)Vector.data(), sizeof(T) * Vector.size());
        }

        // Wrappers for random types, constexpr depending on compiler.
        template <typename T> [[nodiscard]] constexpr uint32_t WW32(const T &Value)
        {
            return Waterhash<uint8_t>((const uint8_t *)&Value, sizeof(Value));
        }
        template <typename T> [[nodiscard]] constexpr uint64_t WW64(const T &Value)
        {
            return Wheathash<uint8_t>((const uint8_t *)&Value, sizeof(Value));
        }
    }

    // Aliases for the different types.
    template <typename ...Args> [[nodiscard]] constexpr auto WW32 (Args&& ...va) { return WWInternal::WW32(std::forward<Args>(va)...); }
    template <typename ...Args> [[nodiscard]] constexpr auto WW64 (Args&& ...va) { return WWInternal::WW64(std::forward<Args>(va)...); }

    // Sanity checking.
    static_assert(WW32("12345") == 0xEE98FD70, "Someone fucked with WW32.");
    static_assert(WW64("12345") == 0x3C570C468027DB01, "Someone fucked with WW64.");
}

// Drop-in generic functions for std:: algorithms, containers, and such.
// e.g. std::unordered_set<SillyType, decltype(WW::Hash), decltype(WW::Equal)>
namespace WW
{
    constexpr auto Hash = [](const auto &v)
    {
        if constexpr (sizeof(size_t) == sizeof(uint32_t)) return Hash::WW32(v);
        else return Hash::WW64(v);
    };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
