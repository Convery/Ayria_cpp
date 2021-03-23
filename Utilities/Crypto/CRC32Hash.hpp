/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT

    // 32T = Tencents alternative
*/

#pragma once
#include <Stdinclude.hpp>

namespace Hash
{
    namespace CRCInternal
    {
        template <typename T> concept Bytealigned_t = sizeof(T) == 1;

        template <Bytealigned_t T, uint32_t Polynomial, bool Shiftright>
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
                        else  CRC = (CRC >> 1) ^ Polynomial;
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
    }

    // Compile-time hashing for fixed-length datablocks.
    template <CRCInternal::Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32A(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<T, 0x04C11DB7, false>(Input, Length, 0xFFFFFFFF);
    }
    template <CRCInternal::Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32B(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<T, 0xEDB88320, true>(Input, Length, 0xFFFFFFFF);
    }
    template <CRCInternal::Bytealigned_t T> [[nodiscard]] constexpr uint32_t CRC32T(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<T, 0xEDB88320, true>(Input, Length, ~uint32_t(Length));
    }

    // Run-time hashing for fixed-length datablocks, constexpr depending on compiler.
    template <typename T> [[nodiscard]] constexpr uint32_t CRC32A(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<uint8_t, 0x04C11DB7, false>((const uint8_t *)Input, sizeof(T) * Length, 0xFFFFFFFF);
    }
    template <typename T> [[nodiscard]] constexpr uint32_t CRC32B(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<uint8_t, 0xEDB88320, true>((const uint8_t *)Input, sizeof(T) * Length, 0xFFFFFFFF);
    }
    template <typename T> [[nodiscard]] constexpr uint32_t CRC32T(const T *Input, size_t Length)
    {
        return CRCInternal::CRC32<uint8_t, 0xEDB88320, true>((const uint8_t *)Input, sizeof(T) * Length, ~uint32_t(Length));
    }

    static_assert(CRC32A("123456789", 9U) == 0xFC891918, "CRC32-A is broken =(");
    static_assert(CRC32B("123456789", 9U) == 0xCBF43926, "CRC32-B is broken =(");
    static_assert(CRC32T("123456789", 9U) == 0x67578F7D, "CRC32-T is broken =(");
}
