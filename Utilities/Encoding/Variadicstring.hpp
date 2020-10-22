/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT

    An alternative until <format> is supported.
*/

#pragma once
#include <memory>
#include <string_view>

template<typename ... Args> [[nodiscard]] std::string va(std::string_view Format, Args ...args)
{
    const auto Size = 1 + std::snprintf(nullptr, 0, Format.data(), args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    std::snprintf(Buffer.get(), Size, Format.data(), args ...);
    return Buffer.get();
}
template<typename ... Args> [[nodiscard]] std::string va(const char *Format, Args ...args)
{
    const auto Size = 1 + std::snprintf(nullptr, 0, Format, args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    std::snprintf(Buffer.get(), Size, Format, args ...);
    return Buffer.get();
}

#if defined (_WIN32)

template<typename ... Args> [[nodiscard]] std::wstring va(std::wstring_view Format, Args ...args)
{
    const auto Size = 1 + _snwprintf(nullptr, 0, Format.data(), args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    _snwprintf(Buffer.get(), Size, Format.data(), args ...);
    return Buffer.get();
}
template<typename ... Args> [[nodiscard]] std::wstring va(const wchar_t *Format, Args ...args)
{
    const auto Size = 1 + _snwprintf(nullptr, 0, Format, args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    _snwprintf(Buffer.get(), Size, Format, args ...);
    return Buffer.get();
}

#endif
