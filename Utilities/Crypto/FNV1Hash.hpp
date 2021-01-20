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
    namespace Internal
    {
        constexpr uint32_t FNV1_Prime_32 = 16777619u;
        constexpr uint32_t FNV1_Offset_32 = 2166136261u;
        constexpr uint64_t FNV1_Prime_64 = 1099511628211u;
        constexpr uint64_t FNV1_Offset_64 = 14695981039346656037u;

        // Worlds worst strlen, but compilers get upset.
        [[nodiscard]] constexpr size_t FNVStrlen(const char *String)
        {
            return *String ? 1 + FNVStrlen(String + 1) : 0;
        }

        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint32_t FNV1_32(T *Input, size_t Length)
        {
            uint32_t Hash = Internal::FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= Internal::FNV1_Prime_32;
                Hash ^= Input[i];
            }

            return Hash;
        }
        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint32_t FNV1a_32(T *Input, size_t Length)
        {
            uint32_t Hash = Internal::FNV1_Offset_32;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= Internal::FNV1_Prime_32;
            }

            return Hash;
        }

        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint64_t FNV1_64(T *Input, size_t Length)
        {
            uint64_t Hash = Internal::FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash *= Internal::FNV1_Prime_64;
                Hash ^= Input[i];
            }

            return Hash;
        }
        template<typename T, typename = std::enable_if<sizeof(T) == 1>>
        constexpr uint64_t FNV1a_64(T *Input, size_t Length)
        {
            uint64_t Hash = Internal::FNV1_Offset_64;
            for (size_t i = 0; i < Length; ++i)
            {
                Hash ^= Input[i];
                Hash *= Internal::FNV1_Prime_64;
            }

            return Hash;
        }
    }

    // Compile-time hashing for literals.
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint32_t FNV1_32(T(&Input)[N])
    {
        return Internal::FNV1_32(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint64_t FNV1_64(T(&Input)[N])
    {
        return Internal::FNV1_64(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint32_t FNV1a_32(T(&Input)[N])
    {
        return Internal::FNV1a_32(Input, N - 1);
    }
    template<size_t N, typename T, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] constexpr uint64_t FNV1a_64(T(&Input)[N])
    {
        return Internal::FNV1a_64(Input, N - 1);
    }

    // Compile-time hashing for fixed-length datablocks.
    [[nodiscard]] constexpr uint32_t FNV1_32(const uint8_t *Input, size_t Length)
    {
        return Internal::FNV1_32(Input, Length);
    }
    [[nodiscard]] constexpr uint64_t FNV1_64(const uint8_t *Input, size_t Length)
    {
        return Internal::FNV1_64(Input, Length);
    }
    [[nodiscard]] constexpr uint32_t FNV1a_32(const uint8_t *Input, size_t Length)
    {
        return Internal::FNV1a_32(Input, Length);
    }
    [[nodiscard]] constexpr uint64_t FNV1a_64(const uint8_t *Input, size_t Length)
    {
        return Internal::FNV1a_64(Input, Length);
    }

    // Run-time hashing for fixed-length datablocks.
    [[nodiscard]] inline uint32_t FNV1_32(const void *Input, const size_t Length)
    {
        return Internal::FNV1_32((const uint8_t *)Input, Length);
    }
    [[nodiscard]] inline uint64_t FNV1_64(const void *Input, const size_t Length)
    {
        return Internal::FNV1_64((const uint8_t *)Input, Length);
    }
    [[nodiscard]] inline uint32_t FNV1a_32(const void *Input, const size_t Length)
    {
        return Internal::FNV1a_32((const uint8_t *)Input, Length);
    }
    [[nodiscard]] inline uint64_t FNV1a_64(const void *Input, const size_t Length)
    {
        return Internal::FNV1a_64((const uint8_t *)Input, Length);
    }

    // Run-time hashing for dynamic data, constexpr in C++20.
    [[nodiscard]] constexpr uint32_t FNV1_32(const char *Input)
    {
        return Internal::FNV1_32(Input, Internal::FNVStrlen(Input));
    }
    [[nodiscard]] constexpr uint64_t FNV1_64(const char *Input)
    {
        return Internal::FNV1_64(Input, Internal::FNVStrlen(Input));
    }
    [[nodiscard]] constexpr uint32_t FNV1a_32(const char *Input)
    {
        return Internal::FNV1a_32(Input, Internal::FNVStrlen(Input));
    }
    [[nodiscard]] constexpr uint64_t FNV1a_64(const char *Input)
    {
        return Internal::FNV1a_64(Input, Internal::FNVStrlen(Input));
    }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1_32(std::basic_string_view<T> String)
    {
        return FNV1_32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1_64(std::basic_string_view<T> String)
    {
        return FNV1_64(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1a_32(std::basic_string_view<T> String)
    {
        return FNV1a_32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1a_64(std::basic_string_view<T> String)
    {
        return FNV1a_64(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1_32(const std::basic_string<T> &String)
    {
        return FNV1_32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1_64(const std::basic_string<T> &String)
    {
        return FNV1_64(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1a_32(const std::basic_string<T> &String)
    {
        return FNV1a_32(String.data(), String.size());
    }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1a_64(const std::basic_string<T> &String)
    {
        return FNV1a_64(String.data(), String.size());
    }

    // Wrappers for random types.
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1_32(T Value) { return FNV1_3232(&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1_64(T Value) { return FNV1_6464(&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint32_t FNV1a_32(T Value) { return FNV1a_3232(&Value, sizeof(Value)); }
    template<typename T> [[nodiscard]] constexpr uint64_t FNV1a_64(T Value) { return FNV1a_6464(&Value, sizeof(Value)); }
}

// Drop-in generic functions for std:: algorithms, containers, and such.
// e.g. std::unordered_map<SillyType, int, decltype(FNV::Hash), decltype(FNV::Equal)>
namespace FNV
{
    constexpr auto Hash = [](const auto &v) { return Hash::FNV1a_64(&v, sizeof(v)); };
    constexpr auto Equal = [](const auto &l, const auto &r) { return Hash(l) == Hash(r); };
}
