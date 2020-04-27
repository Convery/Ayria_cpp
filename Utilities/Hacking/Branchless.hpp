/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-28
    License: Public Domain

    Some branchless alternatives to common functions by Sean Eron Anderson (seander@cs.stanford.edu )
*/

#pragma once
#if __has_include(<concepts>)
#include <type_traits>

namespace Branchless
{
    template<typename T> concept Integral = std::is_integral<T>::value;
    template<typename T> requires Integral<T> constexpr T min(const T A, const T B)
    {
        return B ^ ((A ^ B) & -(A < B));
    }
    template<typename T> requires Integral<T> constexpr T max(const T A, const T B)
    {
        return A ^ ((A ^ B) & -(A < B));
    }
    template<typename T> requires Integral<T> constexpr bool even(const T Value)
    {
        return Value && !(Value & (Value - 1));
    }
    template<typename T> requires Integral<T> constexpr bool odd(const T Value)
    {
        return !even(Value);
    }
    template<typename T> requires Integral<T> constexpr T abs(const T Value)
    {
        const auto Mask = Value >> (sizeof(Value) * 8 - 1);
        return (Value + Mask) ^ Mask;
    }

    template<typename T> requires Integral<T> constexpr T clamp(const T Value, const T Min, const T Max)
    {
        return max(Min, min(Value, Max));
    }

    // Do not modify the functions =(
    static_assert(abs(-23) == 23, "Branchless::abs");
    static_assert(min(2, 3) == 2, "Branchless::min");
    static_assert(max(2, 3) == 3, "Branchless::min");
    static_assert(abs(23) == 23, "Branchless::abs");
    static_assert(even(2), "Branchless::even");
    static_assert(odd(3), "Branchless::odd");

    static_assert(clamp(42, 3, 40) == 40, "Branchless::clamp");
    static_assert(clamp(23, 3, 40) == 23, "Branchless::clamp");
    static_assert(clamp(2, 3, 40) == 3, "Branchless::clamp");
}
#endif
