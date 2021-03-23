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

        template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
        constexpr uint32_t FNV1_32(T *Input, size_t Length)
        {
            uint32_t Hash = FNVInternal::FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= FNVInternal::FNV1_Prime_32;
                Hash ^= Input[i];
            }

            return Hash;
        }
        template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
        constexpr uint32_t FNV1a_32(T *Input, size_t Length)
        {
            uint32_t Hash = FNVInternal::FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= FNVInternal::FNV1_Prime_32;
            }

            return Hash;
        }

        template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
        constexpr uint64_t FNV1_64(T *Input, size_t Length)
        {
            uint64_t Hash = FNVInternal::FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= FNVInternal::FNV1_Prime_64;
                Hash ^= Input[i];
            }

            return Hash;
        }
        template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
        constexpr uint64_t FNV1a_64(T *Input, size_t Length)
        {
            uint64_t Hash = FNVInternal::FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= FNVInternal::FNV1_Prime_64;
            }

            return Hash;
        }
    }

    // Compile-time hashing for literals.
    template<size_t N, typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1_32(T(&Input)[N])
    {
        return FNVInternal::FNV1_32(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1_64(T(&Input)[N])
    {
        return FNVInternal::FNV1_64(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1a_32(T(&Input)[N])
    {
        return FNVInternal::FNV1a_32(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1a_64(T(&Input)[N])
    {
        return FNVInternal::FNV1a_64(Input, N - 1);
    }

    // Compile-time hashing for fixed-length datablocks.
    template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1_32(const T *Input, size_t Length)
    {
        return FNVInternal::FNV1_32(Input, Length);
    }
    template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1_64(const T *Input, size_t Length)
    {
        return FNVInternal::FNV1_64(Input, Length);
    }
    template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1a_32(const T *Input, size_t Length)
    {
        return FNVInternal::FNV1a_32(Input, Length);
    }
    template<typename T, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1a_64(const T *Input, size_t Length)
    {
        return FNVInternal::FNV1a_64(Input, Length);
    }
    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1_32(const std::array<T, N> &Input)
    {
        return FNVInternal::FNV1_32(Input, N);
    }
    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1_64(const std::array<T, N> &Input)
    {
        return FNVInternal::FNV1_64(Input, N);
    }
    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint32_t FNV1a_32(const std::array<T, N> &Input)
    {
        return FNVInternal::FNV1a_32(Input, N);
    }
    template<typename T, size_t N, typename = std::enable_if<(sizeof(T) == 1)>::type>
    [[nodiscard]] constexpr uint64_t FNV1a_64(const std::array<T, N> &Input)
    {
        return FNVInternal::FNV1a_64(Input, N);
    }

    // Run-time hashing for dynamic data, constexpr in C++20.
    template<FNVInternal::Iteratable_t T> [[nodiscard]] constexpr uint32_t FNV1_32(const T &String)
    {
        return FNV1_32(String.data(), String.size());
    }
    template<FNVInternal::Iteratable_t T> [[nodiscard]] constexpr uint64_t FNV1_64(const T &String)
    {
        return FNV1_64(String.data(), String.size());
    }
    template<FNVInternal::Iteratable_t T> [[nodiscard]] constexpr uint32_t FNV1a_32(const T &String)
    {
        return FNV1a_32(String.data(), String.size());
    }
    template<FNVInternal::Iteratable_t T> [[nodiscard]] constexpr uint64_t FNV1a_64(const T &String)
    {
        return FNV1a_64(String.data(), String.size());
    }

    // Wrappers for random types.
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1_32(const T &Value) { return FNV1_32((uint8_t *)&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1_64(const T &Value) { return FNV1_64((uint8_t *)&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1a_32(const T &Value) { return FNV1a_32((uint8_t *)&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1a_64(const T &Value) { return FNV1a_64((uint8_t *)&Value, sizeof(Value)); }

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
        if constexpr(sizeof(size_t) == sizeof(uint32_t))
            return Hash::FNV1a_32((uint8_t *)&v, sizeof(v));
        else
            return Hash::FNV1a_64((uint8_t *)&v, sizeof(v));
    };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
