/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT

    // 32A = 0x04C11DB7
    // 32B = 0xEDB88320
    // 32T = Tencents alternative
*/

#pragma once
#include <Stdinclude.hpp>

namespace Hash
{
    namespace Internal
    {
        // std::array operator [] is not proper constexpr.
        template <typename T, size_t N> struct Array
        {
            T m_data[N];
            using value_type = T;
            using size_type = std::size_t;
            using reference = value_type &;
            using const_reference = const value_type &;

            constexpr reference operator[](size_type i) noexcept { return m_data[i]; }
            constexpr const_reference operator[](size_type i) const noexcept { return m_data[i]; }
            constexpr size_type size() const noexcept { return N; }
        };
        constexpr Array<uint32_t, 256> CRC32B_Table()
        {
            const uint32_t Polynomial = 0xEDB88320;
            Array<uint32_t, 256> Table{};

            for (uint16_t i = 0; i <= 0xFF; ++i)
            {
                uint32_t Remainder{ i };

                for (uint8_t b = 8; !!b; --b)
                {
                    if (!(Remainder & 1)) Remainder >>= 1;
                    else Remainder = (Remainder >> 1) ^ Polynomial;
                }

                Table[i] = Remainder;
            }

            return Table;
        }
    }

    // Compile-time hashing for fixed-length datablocks.
    [[nodiscard]] constexpr uint32_t CRC32A(const char *Input, const uint32_t Length)
    {
        const uint32_t Polynomial = 0x04C11DB7;
        uint32_t Block = 0xFFFFFFFF;

        for (uint32_t i = 0; i < Length; ++i)
        {
            Block ^= Input[i] << 24;

            for (uint8_t b = 0; b < 8; ++b)
            {
                if (!(Block & (1L << 31))) Block <<= 1;
                else Block = (Block << 1) ^ Polynomial;
            }
        }

        return ~Block;
    }
    [[nodiscard]] constexpr uint32_t CRC32B(const char *Input, const uint32_t Length)
    {
        uint32_t Block = 0xFFFFFFFF;
        for (uint32_t i = 0; i < Length; ++i)
        {
            Block = Internal::CRC32B_Table()[(Block ^ Input[i]) & 0xFF] ^ (Block >> 8);
        }

        return ~Block;
    }
    [[nodiscard]] constexpr uint32_t CRC32T(const char *Input, const uint32_t Length)
    {
        uint32_t Block = ~Length;
        for (uint32_t i = 0; i < Length; ++i)
        {
            Block = Internal::CRC32B_Table()[(Block ^ Input[i]) & 0xFF] ^ (Block >> 8);
        }

        return ~Block;
    }

    static_assert(CRC32A("123456789", 9) == 0xFC891918, "CRC32-A is broken =(");
    static_assert(CRC32B("123456789", 9) == 0xCBF43926, "CRC32-B is broken =(");
    static_assert(CRC32T("123456789", 9) == 0x67578F7D, "CRC32-T is broken =(");
}
