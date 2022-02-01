/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-21
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Will be added in MSVC 17.1, copied from MS-STL.
#if !defined (__cpp_lib_byteswap)
#define __cpp_lib_byteswap
namespace std
{
    [[nodiscard]] constexpr uint32_t _Byteswap_ulong(const uint32_t _Val) noexcept
    {
        if (std::is_constant_evaluated())
        {
            return (_Val << 24) | ((_Val << 8) & 0x00FF'0000) | ((_Val >> 8) & 0x0000'FF00) | (_Val >> 24);
        }
        else
        {
            #if defined (_MSC_VER)
            return _byteswap_ulong(_Val);
            #else
            return __builtin_bswap32(_Val);
            #endif
        }
    }
    [[nodiscard]] constexpr uint16_t _Byteswap_ushort(const uint16_t _Val) noexcept
    {
        if (std::is_constant_evaluated())
        {
            return static_cast<uint16_t>((_Val << 8) | (_Val >> 8));
        }
        else
        {
            #if defined (_MSC_VER)
            return _byteswap_ushort(_Val);
            #else
            return __builtin_bswap16(_Val);
            #endif
        }
    }
    [[nodiscard]] constexpr uint64_t _Byteswap_uint64(const uint64_t _Val) noexcept
    {
        if (std::is_constant_evaluated())
        {
            return (_Val << 56) | ((_Val << 40) & 0x00FF'0000'0000'0000) | ((_Val << 24) & 0x0000'FF00'0000'0000)
                | ((_Val << 8) & 0x0000'00FF'0000'0000) | ((_Val >> 8) & 0x0000'0000'FF00'0000)
                | ((_Val >> 24) & 0x0000'0000'00FF'0000) | ((_Val >> 40) & 0x0000'0000'0000'FF00) | (_Val >> 56);
        }
        else
        {
            #if defined (_MSC_VER)
            return _byteswap_uint64(_Val);
            #else
            return __builtin_bswap64(_Val);
            #endif
        }
    }
    template <std::integral _Ty> [[nodiscard]] constexpr _Ty byteswap(const _Ty _Val) noexcept
    {
        if constexpr (sizeof(_Ty) == 1) return _Val;
        else if constexpr (sizeof(_Ty) == 2) return static_cast<_Ty>(_Byteswap_ushort(static_cast<uint16_t>(_Val)));
        else if constexpr (sizeof(_Ty) == 4) return static_cast<_Ty>(_Byteswap_ulong(static_cast<uint32_t>(_Val)));
        else if constexpr (sizeof(_Ty) == 8) return static_cast<_Ty>(_Byteswap_uint64(static_cast<uint64_t>(_Val)));

        // Should never happen.
        else static_assert(false, "Unexpected integer size");
    }
}
#endif

namespace Endian
{
    // constexpr implementations of htonX / ntohX utilities.
    template <std::integral T> constexpr T toLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <std::integral T> constexpr T toBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }

    // Convert to native.
    template <std::integral T> constexpr T fromLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <std::integral T> constexpr T fromBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }
}
