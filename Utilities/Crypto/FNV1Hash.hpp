/*
Initial author: Convery (tcn@ayria.se)
Started: 2019-03-14
License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>

namespace Hash
{
    namespace FNVInternal
    {
        constexpr uint32_t FNV1_Prime_32 = 16777619u;
        constexpr uint32_t FNV1_Offset_32 = 2166136261u;
        constexpr uint64_t FNV1_Prime_64 = 1099511628211u;
        constexpr uint64_t FNV1_Offset_64 = 14695981039346656037u;

        template <typename T> concept Iteratable_t = requires (const T &t) { t.cbegin(); t.cend(); };
        template <typename T> concept Bytealigned_t = sizeof(T) == 1;

        // Implementation.
        template <bool TypeA, Bytealigned_t T> [[nodiscard]] constexpr uint32_t FNV1_32_t(const T *Input, size_t Length)
        {
            uint32_t Hash = FNV1_Offset_32;

            if constexpr (TypeA)
            {
                for (size_t i = 0; i < Length; ++i)
                {
                    Hash ^= Input[i];
                    Hash *= FNV1_Prime_32;
                }
            }
            else
            {
                for (size_t i = 0; i < Length; ++i)
                {
                    Hash *= FNV1_Prime_32;
                    Hash ^= Input[i];
                }
            }

            return Hash;
        }
        template <bool TypeA, Bytealigned_t T> [[nodiscard]] constexpr uint64_t FNV1_64_t(const T *Input, size_t Length)
        {
            uint64_t Hash = FNV1_Offset_64;

            if constexpr (TypeA)
            {
                for (size_t i = 0; i < Length; ++i)
                {
                    Hash ^= Input[i];
                    Hash *= FNV1_Prime_64;
                }
            }
            else
            {
                for (size_t i = 0; i < Length; ++i)
                {
                    Hash *= FNV1_Prime_64;
                    Hash ^= Input[i];
                }
            }

            return Hash;
        }

        // Compile-time hashing for simple fixed-length datablocks.
        template <bool TypeA, Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint32_t FNV1_32_t(const std::array<T, N> &Input)
        {
            return FNV1_32_t<TypeA, T>(Input.data(), N);
        }
        template <bool TypeA, Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint64_t FNV1_64_t(const std::array<T, N> &Input)
        {
            return FNV1_64_t<TypeA, T>(Input.data(), N);
        }
        template <bool TypeA, typename T, size_t N> [[nodiscard]] constexpr uint32_t FNV1_32_t(const std::array<T, N> &Input)
        {
            return FNV1_32_t<TypeA, uint8_t>((const uint8_t *)Input.data(), sizeof(T) * N);
        }
        template <bool TypeA, typename T, size_t N> [[nodiscard]] constexpr uint64_t FNV1_64_t(const std::array<T, N> &Input)
        {
            return FNV1_64_t<TypeA, uint8_t>((const uint8_t *)Input.data(), sizeof(T) * N);
        }

        // Compile-time hashing for string literals.
        template <bool TypeA, Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint32_t FNV1_32_t(T(&Input)[N])
        {
            return FNV1_32_t<TypeA, T>(Input, N - 1);
        }
        template <bool TypeA, Bytealigned_t T, size_t N> [[nodiscard]] constexpr uint64_t FNV1_64_t(T(&Input)[N])
        {
            return FNV1_64_t<TypeA, T>(Input, N - 1);
        }
        template <bool TypeA, typename T, size_t N> [[nodiscard]] constexpr uint32_t FNV1_32_t(T(&Input)[N])
        {
            return FNV1_32_t<TypeA, uint8_t>((const uint8_t *)&Input, sizeof(T) * (N - 1));
        }
        template <bool TypeA, typename T, size_t N> [[nodiscard]] constexpr uint64_t FNV1_64_t(T(&Input)[N])
        {
            return FNV1_64_t<TypeA, uint8_t>((const uint8_t *)&Input, sizeof(T) * (N - 1));
        }

        // Run-time hashing for dynamic data, constexpr in C++20.
        template <bool TypeA, Iteratable_t T> [[nodiscard]] constexpr uint32_t FNV1_32_t(const T &Vector)
            requires Bytealigned_t<typename T::value_type>
        {
            return FNV1_32_t<TypeA, T::value_type>(Vector.data(), Vector.size());
        }
        template <bool TypeA, Iteratable_t T> [[nodiscard]] constexpr uint64_t FNV1_64_t(const T &Vector)
            requires Bytealigned_t<typename T::value_type>
        {
            return FNV1_64_t<TypeA, T::value_type>(Vector.data(), Vector.size());
        }
        template <bool TypeA, Iteratable_t T> [[nodiscard]] constexpr uint32_t FNV1_32_t(const T &Vector)
        {
            return FNV1_32_t<TypeA, uint8_t>((const uint8_t *)Vector.data(), sizeof(T) * Vector.size());
        }
        template <bool TypeA, Iteratable_t T> [[nodiscard]] constexpr uint64_t FNV1_64_t(const T &Vector)
        {
            return FNV1_64_t<TypeA, uint8_t>((const uint8_t *)Vector.data(), sizeof(T) * Vector.size());
        }

        // Wrappers for random types, constexpr depending on compiler.
        template <bool TypeA, typename T> [[nodiscard]] constexpr uint32_t FNV1_32_t(const T &Value)
        {
            return FNV1_32_t<TypeA, uint8_t>((const uint8_t *)&Value, sizeof(Value));
        }
        template <bool TypeA, typename T> [[nodiscard]] constexpr uint64_t FNV1_64_t(const T &Value)
        {
            return FNV1_64_t<TypeA, uint8_t>((const uint8_t *)&Value, sizeof(Value));
        }
    }

    // Aliases for the different types.
    template <typename ...Args> [[nodiscard]] constexpr auto FNV1a_32 (Args&& ...va) { return FNVInternal::FNV1_32_t<true>(std::forward<Args>(va)...); }
    template <typename ...Args> [[nodiscard]] constexpr auto FNV1a_64 (Args&& ...va) { return FNVInternal::FNV1_64_t<true>(std::forward<Args>(va)...); }
    template <typename ...Args> [[nodiscard]] constexpr auto FNV1_32 (Args&& ...va) { return FNVInternal::FNV1_32_t<false>(std::forward<Args>(va)...); }
    template <typename ...Args> [[nodiscard]] constexpr auto FNV1_64 (Args&& ...va) { return FNVInternal::FNV1_64_t<false>(std::forward<Args>(va)...); }

    // Sanity checking.
    static_assert(FNV1_32("12345") == 0xDEEE36FA, "Someone fucked with FNV32.");
    static_assert(FNV1a_32("12345") == 0x43C2C0D8, "Someone fucked with FNV32a.");
    static_assert(FNV1_64("12345") == 0xA92F4455DA95A77A, "Someone fucked with FNV64.");
    static_assert(FNV1a_64("12345") == 0xE575E8883C0F89F8, "Someone fucked with FNV64a.");
}

// Drop-in generic functions for std:: algorithms, containers, and such.
// e.g. std::unordered_map<SillyType, int, decltype(FNV::Hash), decltype(FNV::Equal)>
namespace FNV
{
    constexpr auto Hash = [](const auto &v)
    {
        if constexpr(sizeof(size_t) == sizeof(uint32_t)) return Hash::FNV1a_32(v);
        else return Hash::FNV1a_64(v);
    };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
