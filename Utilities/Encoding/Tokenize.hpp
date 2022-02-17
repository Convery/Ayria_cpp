/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-17
    License: MIT
*/

#pragma once
#include <regex>
#include <vector>
#include <Utilities/AAUtilities.hpp>

#if defined (HAS_ABSEIL)
[[nodiscard]] inline std::vector<std::string> Tokenizestring(const std::string &String, std::string_view Needle)
{
    return absl::StrSplit(String, absl::ByAnyChar(Needle), absl::SkipEmpty());
}
[[nodiscard]] inline std::vector<std::string> Tokenizestring(std::string_view String, char Needle)
{
    return absl::StrSplit(String, absl::ByChar(Needle), absl::SkipEmpty());
}
#else
#if defined (HAS_LZ)
[[nodiscard]] inline std::vector<std::string> Tokenizestring(const std::string &String, std::string_view Needle)
{
    return lz::split<std::string>(String, std::string(Needle)).toVector();
}
[[nodiscard]] inline std::vector<std::string> Tokenizestring(std::string_view String, char Needle)
{
    return lz::split<std::string>(String, Needle).toVector();
}
#else
[[nodiscard]] inline std::vector<std::string> Tokenizestring(const std::string &String, std::string_view Needle)
{
    const std::regex rxFields("([^" + std::string(Needle) + "]*)", std::regex_constants::optimize);
    auto It = std::sregex_iterator(String.cbegin(), String.cend(), rxFields);
    const auto Size = std::distance(It, std::sregex_iterator());
    std::vector<std::string> Results;
    Results.reserve(Size);

    for (ptrdiff_t i = 0; i < Size; ++i)
    {
        const auto Match = (It++)->str();
        if (!Match.empty()) Results.emplace_back(std::move(Match));
    }

    return Results;
}
[[nodiscard]] inline std::vector<std::string> Tokenizestring(std::string_view String, char Needle)
{
    std::vector<std::string> Results;
    Results.reserve(8);

    while (!String.empty())
    {
        const auto Offset = String.find_first_of(Needle);
        Results.emplace_back(String.substr(0, Offset));
        String.remove_prefix(Offset);
    }

    return Results;
}
#endif
#endif

[[nodiscard]] inline std::vector<std::wstring> Tokenizestring(const std::wstring &String, std::wstring_view Needle)
{
    const auto Tokens = Tokenizestring(Encoding::toASCII(String), Encoding::toASCII(Needle));
    std::vector<std::wstring> Result; Result.reserve(Tokens.size());

    for (const auto &Token : Tokens) Result.emplace_back(Encoding::toUNICODE(Token));
    return Result;
}
[[nodiscard]] inline std::vector<std::wstring> Tokenizestring(std::wstring_view String, wchar_t Needle)
{
    const auto Tokens = Tokenizestring(Encoding::toASCII(String), Encoding::toASCII(std::wstring(1, Needle)));
    std::vector<std::wstring> Result; Result.reserve(Tokens.size());

    for (const auto &Token : Tokens) Result.emplace_back(Encoding::toUNICODE(Token));
    return Result;
}
