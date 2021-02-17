/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-09
    License: MIT

    Misc utilities.
*/

#pragma once
#include <Stdinclude.hpp>

// C++ regression helpers.
#pragma region C++

// Helper to support designated initializers until c++ 20 is mainstream.
#define instantiate(T, ...) ([&]{ T ${}; __VA_ARGS__; return $; }())

#pragma endregion

// Python-esque constructs for loops.
#pragma region Python

// for (const auto &[Index, Value] : Enumerate(std::vector({ 1, 2, 3, 4 }))) ...
#if defined(__cpp_lib_coroutine) && __has_include(<experimental/generator>)
#include <experimental/generator>

template <typename Type, typename Indextype = size_t>
std::experimental::generator<std::pair<Indextype, typename Type::value_type>>
                            Enumerate(const Type &Collection, Indextype Start = {})
{
    for (const auto &Item : Collection)
        co_yield std::make_pair(Start++, Item);
}

template <typename Type, typename Indextype = size_t>
std::experimental::generator<std::pair<Indextype, typename Type::value_type>>
                            Enumerate(Type &&Collection, Indextype Start = {})
{
    for (const auto &Item : Collection)
        co_yield std::make_pair(Start++, Item);
}

#else

template <typename Iteratortype, typename Indextype = size_t>
struct Enumerate
{
    Indextype Index;
    Iteratortype Iterator;
    Enumerate(Iteratortype &&Iterator, Indextype Start = {}) : Iterator(Iterator), Index(Start) {};
    Enumerate(const Iteratortype &Iterator, Indextype Start = {}) : Iterator(Iterator), Index(Start) {};

    template<typename Iteratortype>
    struct Internaliterator
    {
        Indextype Index;
        Iteratortype Iterator;

        Internaliterator(Iteratortype &&Iterator, Indextype Start = {}) : Iterator(Iterator), Index(Start) {};
        bool operator!=(const Internaliterator &Right) const { return Iterator != Right.Iterator; };
        auto operator*() const { return std::forward_as_tuple(Index, *Iterator); };
        Internaliterator &operator++() { Iterator++; Index++; return *this; };
    };

    // STD accessors.
    auto begin() const { return Internaliterator(std::begin(Iterator), Index); };
    auto end() const { return Internaliterator(std::end(Iterator), Index); };
};
#endif

// for (const auto &x : Range(1, 100, 2)) ...
#if defined(__cpp_lib_coroutine) && __has_include(<experimental/generator>)
#include <experimental/generator>

template <typename Valuetype, typename Steptype = int>
std::experimental::generator<Valuetype> Range(Valuetype Start, Valuetype End, Steptype Step = 1)
{
    if constexpr (std::is_arithmetic_v<Valuetype>)
        for (auto i = Start; i < End; i += Step)
            co_yield i;

    else
        for (auto i = Start; i < End; std::advance(i, Step))
            co_yield i;
}

#else
template <typename Valuetype, typename Steptype = int>
struct Range
{
    using value_type = Valuetype;
    Valuetype Current, Max;
    Steptype Step;

    constexpr Range(Valuetype Start, Valuetype End, Steptype Step = 1) : Current(Start), Max(End), Step(Step) {};
    constexpr bool operator !=(const Range &Right) const
    {
        if constexpr (std::is_arithmetic_v<Valuetype>) return Current < Right.Current;
        else return Current != Right.Current;
    };
    constexpr bool operator !=(Valuetype Right) const
    {
        if constexpr (std::is_arithmetic_v<Valuetype>) return Current < Right;
        else return Current != Right;
    };
    Range &operator++()
    {
        if constexpr (std::is_arithmetic_v<Valuetype>) Current += Step;
        else std::advance(Current, Step);
        return *this;
    };
    auto operator*() const { return Current; };

    // STD accessors.
    auto begin() const { return *this; };
    auto end() const { return Max; };
};
#endif

#pragma endregion

// Helper for debug-builds, constexpr if is not valid on MSVC.
#if defined (WIN32)
inline void setThreadname(std::string_view Name)
{
    if constexpr (Build::isDebug)
    {
        #pragma pack(push, 8)
        using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
        #pragma pack(pop)

        __try
        {
            const THREADNAME_INFO Info{ 0x1000, Name.data(), GetCurrentThreadId(), 0 };
            RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR *>(&Info));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}
#elif defined (__linux__)
inline void setThreadname(std::string_view Name)
{
    if constexpr (Build::isDebug)
    {
        prctl(PR_SET_NAME, Name.data(), 0, 0, 0);
    }
}
#else
inline void setThreadname(std::string_view Name)
{
}
#endif
