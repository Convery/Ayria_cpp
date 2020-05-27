/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-08
    License: MIT
*/

#pragma once
#include <string_view>
#include <codecvt>
#include <locale>

/*
    NOTE(tcn):
    Windows's MultiByteToWideChar occasionally breaks their compiler in MSVC 16.6.
    Reported the issue but it's not like they care about non-enterprise customers
    feedback; so unlikely to be solved anytime soon.
    Fallback to deprecated codecvt.
*/

[[nodiscard]] inline std::string toNarrow(const std::wstring &Input)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(Input);
}
[[nodiscard]] inline std::wstring toWide(const std::string &Input)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Input);
}

[[nodiscard]] inline std::string toNarrow(const std::wstring_view Input)
{
    return toNarrow(std::wstring(Input));
}
[[nodiscard]] inline std::wstring toWide(const std::string_view Input)
{
    return toWide(std::string(Input));
}

[[nodiscard]] inline std::string toNarrow(const wchar_t *Input, size_t Length)
{
    return toNarrow(std::wstring_view(Input, Length));
}
[[nodiscard]] inline std::wstring toWide(const char *Input, size_t Length)
{
    return toWide(std::string_view(Input, Length));
}
