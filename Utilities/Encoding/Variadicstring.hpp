/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT
*/

#pragma once
#include <Utilities/AAUtilities.hpp>
#include <Utilities/Constexprhelpers.hpp>

// Helpers to deal with std::basic_string.
template <typename T> concept String_t = requires (const T &Arg) { Arg.c_str(); };
template <String_t T> constexpr decltype(auto) Forward(const T &Arg) { return Arg.c_str(); }
template <typename T> constexpr decltype(auto) Forward(T &&Arg) { static_assert(!cmp::isDerived<T, std::basic_string_view>); return Arg; }
template <typename T> constexpr decltype(auto) Forward(const T &Arg) { static_assert(!cmp::isDerived<T, std::basic_string_view>); return Arg; }

template <typename ... Args> [[nodiscard]] std::string va_impl(const char *Format, const Args& ...args)
{
    const auto Size = std::snprintf(nullptr, 0, Format, args...);
    std::string Buffer(Size, '\0');

    std::snprintf(Buffer.data(), Size + 1, Format, args...);
    return Buffer;
}
template <typename ... Args> [[nodiscard]] std::string va(const std::string &Format, const Args& ...args)
{
    return va_impl(Format.c_str(), Forward<Args>(args)...);
}

// NOTE(tcn): Modern C-API's seem to handle UTF8 without the escapes, may need to update that in the future.
template <typename ... Args> [[nodiscard]] std::u8string va(const std::u8string &Format, const Args& ...args)
{
    const auto Temp = va_impl((const char *)Format.c_str(), Forward<Args>(args)...);
    return { Temp.begin(), Temp.end() };
}

#if defined (_WIN32)
template <typename ... Args> [[nodiscard]] std::wstring va_impl(const wchar_t *Format, const Args& ...args)
{
    const auto Size = _snwprintf(nullptr, 0, Format, args...);
    std::wstring Buffer(Size, L'\0');

    _snwprintf(Buffer.data(), Size, Format, args...);
    return Buffer;
}
template <typename ... Args> [[nodiscard]] std::wstring va(const std::wstring &Format, const Args& ...args)
{
    return va_impl(Format.c_str(), Forward<Args>(args)...);
}
#endif
