/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-06
    License: MIT
*/

#pragma once
#include <span>
#include <array>
#include <memory>
#include <cstdint>
#include <concepts>
#include <algorithm>
#include "Datatypes.hpp"
#include "Wrappers\Endian.hpp"

namespace cmp
{
    template <typename T> concept Byte_t = sizeof(T) == 1;
    template <typename T> concept Bytealigned_t = sizeof(T) == 1;
    template <typename T> concept Range_t = requires (T t) { t.begin(); t.end(); };
    template <typename T> concept Simple_t = Range_t<T> && Bytealigned_t<typename T::value_type>;
    template <typename T> concept Complex_t = Range_t<T> && !Bytealigned_t<typename T::value_type>;
    template <typename T> concept Sequence_t = Range_t<T> && Bytealigned_t<typename T::value_type>;
    template <typename T> concept Char_t = std::is_same_v<std::remove_const_t<T>, char> || std::is_same_v<std::remove_const_t<T>, char8_t>;

    // Helpers for type deduction.
    template <class, template <class...> class> inline constexpr bool isDerived = false;
    template <template <class...> class T, class... Args> inline constexpr bool isDerived<T<Args...>, T> = true;

    // Helper to avoid nested conditionals in templates.
    template <bool Conditional, typename T> struct Case_t : std::bool_constant<Conditional> { using type = T; };
    using Defaultcase_t = Case_t<false, void>;

    // Get the smallest type that can hold our value. Can use std::bit_width in the future.
    template <int64_t Maxvalue> using Intsize_t = typename std::disjunction<
        Case_t<(Maxvalue >  INT32_MAX), uint64_t>,
        Case_t<(Maxvalue >  INT16_MAX), uint32_t>,
        Case_t<(Maxvalue >  INT8_MAX), uint16_t>,
        Case_t<(Maxvalue <= INT8_MAX), uint8_t>,
        Defaultcase_t>::type;
    template <uint64_t Maxvalue> using UIntsize_t = typename std::disjunction<
        Case_t<(Maxvalue >  UINT32_MAX), uint64_t>,
        Case_t<(Maxvalue >  UINT16_MAX), uint32_t>,
        Case_t<(Maxvalue >  UINT8_MAX), uint16_t>,
        Case_t<(Maxvalue <= UINT8_MAX), uint8_t>,
        Defaultcase_t>::type;

    // Memcpy of integer types, for *(int *)&Type = 1;
    template <std::integral T, Byte_t U> constexpr T toINT(const U *Ptr)
    {
        if (!std::is_constant_evaluated())
        {
            return *(T *)Ptr;
        }
        else
        {
            constexpr size_t N = sizeof(T) * 8;
            T Result{};

            if constexpr (N >= 64) Result |= (T(*Ptr++) << (N - 64));
            if constexpr (N >= 56) Result |= (T(*Ptr++) << (N - 56));
            if constexpr (N >= 48) Result |= (T(*Ptr++) << (N - 48));
            if constexpr (N >= 40) Result |= (T(*Ptr++) << (N - 40));
            if constexpr (N >= 32) Result |= (T(*Ptr++) << (N - 32));
            if constexpr (N >= 24) Result |= (T(*Ptr++) << (N - 24));
            if constexpr (N >= 16) Result |= (T(*Ptr++) << (N - 16));
            if constexpr (N >= 8)  Result |= (T(*Ptr  ) << (N - 8));

            return Result;
        }
    }
    template <Byte_t T, std::integral U> constexpr void fromINT(T *Ptr, U Value)
    {
        if (!std::is_constant_evaluated())
        {
            *(U *)Ptr = Value;
        }
        else
        {
            constexpr size_t N = sizeof(U) * 8;

            if constexpr (N >= 64) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 56) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 48) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 40) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 32) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 24) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 16) { *Ptr++ = Value & 0xFF; Value >>= 8; }
            if constexpr (N >= 8)  { *Ptr   = Value & 0xFF; Value >>= 8; }
        }
    }

    // Sometimes we just need a bit more room..
    template <typename T, size_t Old, size_t New>
    constexpr std::array<T, New> resize_array(const std::array<T, Old> &Input)
    {
        constexpr auto Min = std::min(Old, New);

        return[]<size_t ...Index>(const std::array<T, Old> &Input, std::index_sequence<Index...>)
        {
            return std::array<T, New>{ { Input[Index]... }};
        }(Input, std::make_index_sequence<Min>());
    }

    // Since we can't cast (char *) <-> (void *) in constexpr.
    template <Byte_t T, Byte_t U> constexpr void memcpy(T *Dst, const U *Src, size_t Size)
    {
        if (!std::is_constant_evaluated())
        {
            std::memcpy(Dst, Src, Size);
        }
        else
        {
            while (Size--) *Dst++ = T(*Src++);
        }
    }
    template <Byte_t T, Byte_t U> constexpr bool memcmp(const T *A, const T *B, size_t Size)
    {
        if (!std::is_constant_evaluated())
        {
            return 0 == std::memcmp(A, B, Size);
        }
        else
        {
            while (Size--)
            {
                if ((U(*A++) ^ T(*B++)) != 0) [[unlikely]]
                    return false;
            }

            return true;
        }
    }
    template <Byte_t T, typename U> requires (sizeof(U) != 1) constexpr void memcpy(T *Dst, const U *Src, size_t Size)
    {
        if (!std::is_constant_evaluated())
        {
            std::memcpy(Dst, Src, Size);
        }
        else
        {
            const auto Temp = std::bit_cast<std::array<uint8_t, sizeof(U)>>(*Src);
            for (size_t i = 0; i < sizeof(U); ++i) Dst[i] = Temp[i];
            if (--Size) memcpy(Dst + sizeof(U), ++Src, Size);
        }
    }

    // Can't do user-defined conversions because C++, so need to derive from STL.
    template <Byte_t T, size_t N>
    struct Basevector_t : std::array<T, N>
    {
        using Basetype = std::array<T, N>;

        using const_reverse_iterator = typename Basetype::const_reverse_iterator;
        using reverse_iterator = typename Basetype::reverse_iterator;
        using difference_type = typename Basetype::difference_type;
        using const_reference = typename Basetype::const_reference;
        using const_iterator = typename Basetype::const_iterator;
        using const_pointer = typename Basetype::const_pointer;
        using value_type = typename Basetype::value_type;
        using size_type = typename Basetype::size_type;
        using reference = typename Basetype::reference;
        using iterator = typename Basetype::iterator;
        using pointer = typename Basetype::pointer;

        using Basetype::operator[];
        using Basetype::cbegin;
        using Basetype::begin;
        using Basetype::cend;
        using Basetype::data;
        using Basetype::size;
        using Basetype::end;

        constexpr operator std::span<T, N>() { return { data(), size() }; }

        using Basetype::array;
        constexpr Basevector_t() = default;
        constexpr Basevector_t(const std::array<T, N> &Input) : Basetype::array(Input) {}
        constexpr Basevector_t(std::array<T, N> &&Input) : Basetype::array(std::move(Input)) {}
        template <Simple_t U> constexpr Basevector_t(U &&Input) : Basetype::array()
        {
            std::ranges::move(Input.begin(), Input.end(), data());
        }
    };

    template <Byte_t T>
    struct Basevector_t<T, 0> : std::span<T>
    {
        using Basetype = std::span<T>;

        using reverse_iterator = typename Basetype::reverse_iterator;
        using difference_type = typename Basetype::difference_type;
        using const_reference = typename Basetype::const_reference;
        using const_pointer = typename Basetype::const_pointer;
        using value_type = typename Basetype::value_type;
        using size_type = typename Basetype::size_type;
        using reference = typename Basetype::reference;
        using iterator = typename Basetype::iterator;
        using pointer = typename Basetype::pointer;

        using Basetype::operator[];
        using Basetype::begin;
        using Basetype::data;
        using Basetype::size;
        using Basetype::end;

        // Just some space to keep data if initialized via move.
        std::shared_ptr<T[]> Storage{};

        using Basetype::span;
        constexpr Basevector_t() = default;
        template <Simple_t U> constexpr Basevector_t(U &&Input) : Storage(std::make_shared<T[]>(Input.size()))
        {
            std::ranges::copy(Input.begin(), Input.end(), Storage.get());
            Basetype::span(Storage.get(), Input.size());
        }
    };

    template <Byte_t T = uint8_t, size_t N = 0>
    struct Vector_t : Basevector_t<T, N>
    {
        using reverse_iterator = typename Basevector_t<T, N>::reverse_iterator;
        using difference_type = typename Basevector_t<T, N>::difference_type;
        using const_reference = typename Basevector_t<T, N>::const_reference;
        using const_pointer = typename Basevector_t<T, N>::const_pointer;
        using value_type = typename Basevector_t<T, N>::value_type;
        using size_type = typename Basevector_t<T, N>::size_type;
        using reference = typename Basevector_t<T, N>::reference;
        using pointer = typename Basevector_t<T, N>::pointer;
        using Basevector_t<T, N>::Basevector_t;
        using Basevector_t<T, N>::operator[];
        using Basevector_t<T, N>::iterator;
        using Basevector_t<T, N>::begin;
        using Basevector_t<T, N>::size;
        using Basevector_t<T, N>::data;
        using Basevector_t<T, N>::end;

        // Convert to pretty much any range.
        template <Range_t U> constexpr operator U() const
        {
            return U ((typename U::value_type *)&*data(), size());
        }

        // Override the empty operator for use with arrays.
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            if constexpr (N == 0) return Basevector_t<T, N>::empty();
            else
            {
                return std::ranges::all_of(*this, [](const T &Item) { return Item == T{}; });
            }
        }

        constexpr Vector_t() = default;
        template <Simple_t U> constexpr Vector_t(const U &Range) : Basevector_t<T, N>::Basevector_t(Range.begin(), Range.end()) {}
        template <Complex_t U> constexpr Vector_t(const U &Range)
        {
            Basevector_t<T, N>::Basevector_t(std::as_bytes(std::span(Range)));
        }
    };

    // Helper to strip the null char from string literals.
    template <Char_t T, size_t N> constexpr auto make_vector(const T(&Input)[N])
    {
        return[]<size_t ...Index>(const T(&Input)[N], std::index_sequence<Index...>)
        {
            return Vector_t<T, N - 1>{ { Input[Index]... }};
        }(Input, std::make_index_sequence<N - 1>());
    }

    // Helper to convert integers..
    template <std::integral T> constexpr auto make_vector(const T &Value)
    {
        constexpr auto N = sizeof(Value);
        Vector_t<uint8_t, N> Buffer{};
        fromINT(Buffer.data(), Value);
        return Buffer;
    }

    // And pretty much everything else.
    template <Byte_t T, size_t N> requires (N != static_cast<size_t>(-1)) constexpr auto make_vector(const std::span<T, N> &Input)
    {
        return[]<size_t ...Index>(const std::span<T, N> &Input, std::index_sequence<Index...>)
        {
            return Vector_t<T, N>{ { Input[Index]... }};
        }(Input, std::make_index_sequence<N>());
    }
    template <typename T> requires(!Range_t<T>) constexpr Vector_t<> make_vector(const T &Value) requires (!std::is_pointer_v<T>)
    {
        constexpr auto N = sizeof(Value);
        std::array<uint8_t, N> Buffer{};

        memcpy(Buffer.data(), &Value, N);
        return Vector_t<uint8_t, N>(Buffer);
    }
    template <Byte_t T, size_t N> constexpr Vector_t<T, N> make_vector(const std::array<T, N> &Input)
    {
        return Vector_t<T, N>(Input);
    }
    template <Range_t T> constexpr Vector_t<> make_vector(const T &Value)
    {
        return Vector_t<>(Value);
    }
}

// Combine arrays, no idea why the STL doesn't provide this..
template <typename T, size_t N, size_t M>
constexpr std::array<T, N + M> operator+(const std::array<T, N> &Left, const std::array<T, M> &Right)
{
    return[]<size_t ...LIndex, size_t ...RIndex>(const std::array<T, N> &Left, const std::array<T, M> &Right,
                                                 std::index_sequence<LIndex...>, std::index_sequence<RIndex...>)
    {
        return std::array<T, N + M>{ { Left[LIndex]..., Right[RIndex]... } };
    }(Left, Right, std::make_index_sequence<N>(), std::make_index_sequence<M>());
}
