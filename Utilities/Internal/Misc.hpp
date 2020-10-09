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

// for(const auto &[Index, Value] : Enumerate(std::vector({ 1, 2, 3, 4 }))) ...
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

        Internaliterator &operator++() { Iterator++; Index++; return *this; };
        auto operator*() const { return std::forward_as_tuple(Index, *Iterator); };
        bool operator!=(const Internaliterator &Right) const { return Iterator != Right.Iterator; };
    };

    // STD accessors.
    auto begin() const { return Internaliterator(std::begin(Iterator), Index); };
    auto end() const { return Internaliterator(std::end(Iterator), Index); };
};

// for (const auto &x : Range(1, 100, 2)) ...
template <typename Valuetype, typename Steptype = int>
struct Range
{
    using value_type = Valuetype;
    Valuetype Current;
    Valuetype Min;
    Valuetype Max;
    Steptype Step;

    constexpr Range(Valuetype Start, Valuetype End, Steptype Step = 1) : Current(Start), Min(Start), Max(End), Step(Step) {};
    constexpr bool operator !=(Range Right) const { return Current != Right.Current; };
    constexpr bool operator !=(Valuetype Right) const { return Current < Right; };
    Range &operator++() { Current += Step; return *this; };
    auto operator*() const { return Current; };

    // STD accessors.
    auto begin() const { return *this; };
    auto end() const { return Max; };
};

#pragma endregion






