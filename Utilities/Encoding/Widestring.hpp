/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-08
    License: MIT
*/

#pragma once
#include <string>

// NOTE(tcn): Slow Windows implementation, update in the future.
#if defined (_WIN32)
#include <Windows.h>
[[nodiscard]] inline std::string toNarrow(const std::wstring_view Input)
{
    const auto Size = WideCharToMultiByte(CP_UTF8, 0, Input.data(), Input.size(), NULL, 0, NULL, FALSE);
    const auto Buffer = (char *)alloca(Size * sizeof(char) + sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, Input.data(), Input.size(), Buffer, Size, NULL, FALSE);
    return std::string(Buffer, Size);
}
[[nodiscard]] inline std::wstring toWide(const std::string_view Input)
{
    const auto Size = MultiByteToWideChar(CP_UTF8, 0, Input.data(), Input.size(), NULL, 0);
    const auto Buffer = (wchar_t *)alloca(Size * sizeof(wchar_t) + sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, Input.data(), Input.size(), Buffer, Size);
    return std::wstring(Buffer, Size);
}

[[nodiscard]] inline std::string toNarrow(const wchar_t *Input, size_t Length)
{
    return toNarrow(std::wstring_view(Input, Length));
}
[[nodiscard]] inline std::wstring toWide(const char *Input, size_t Length)
{
    return toWide(std::string_view(Input, Length));
}
#endif
