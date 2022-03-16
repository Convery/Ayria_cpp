/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-16
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

template <typename T, size_t N> class Ringbuffer_t
{
    using size_type = cmp::UIntsize_t<N>;

    size_type Head{}, Tail{};
    std::array<T, N> Storage{};

    // For clearer code.
    static inline size_type Clamp(size_type Value) noexcept
    {
        // Apparently MSVC did not know how to do the most basic optimization..
        if constexpr (N && !(N & (N - 1))) return Value & (N - 1);
        else return Value % N;
    }
    static inline size_type Distance(size_type A, size_type B) noexcept
    {
        const auto X = N - A - B;
        const auto Y = B - A;
        const auto C = B < A;

        return Y ^ (-C & (X ^ Y));
    }
    static inline size_type Advance(size_type Index, int Offset) noexcept
    {
        return Clamp(Index + Offset + size_type(N));
    }

    public:
    T pop() noexcept
    {
        const auto Old = Tail;
        Tail = Advance(Tail, 1);
        return Storage[Old];
    }
    void push_back(const T &Value) noexcept
    {
        Storage[Head] = Value;
        Head = Advance(Head, 1);
        Tail = Advance(Tail, Head == Tail);
    }
    template <typename... Args> T &emplace_back(Args&&... args) noexcept
    {
        Storage[Head] = { std::forward<Args>(args)... };
        auto &Ret = Storage[Head];

        Head = Advance(Head, 1);
        Tail = Advance(Tail, Head == Tail);
        return Ret;
    }

    // Really only used for debugging.
    [[nodiscard]] size_type size() const noexcept
    {
        return (Head - Tail + (N + 1)) % (N + 1);
    }
    [[nodiscard]] bool empty() const noexcept
    {
        return Head == Tail;
    }

    // Simple iterator using sentinels rather than math.
    struct Iterator_t
    {
        // Deprecated, but here for C++17 compatibility.
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using reference = const T &;
        using pointer = const T *;
        using value_type = T;

        size_type Index{}, Count{};
        const T *Data{};

        [[nodiscard]] reference operator*() const noexcept { return Data[Index]; }
        [[nodiscard]] pointer operator->() const noexcept { return Data + Index; }

        auto operator++(int) noexcept { auto Copy{ *this }; operator++(); return Copy; }
        auto operator--(int) noexcept { auto Copy{ *this }; operator--(); return Copy; }
        auto operator++() noexcept { Index = Advance(Index, 1); --Count; return *this; }
        auto operator--() noexcept { Index = Advance(Index, -1); --Count; return *this; }
        auto operator==(const Iterator_t &Right) const noexcept { return Count == Right.Count; }
        auto operator!=(const Iterator_t &Right) const noexcept { return Count != Right.Count; }

        Iterator_t() = default;
        Iterator_t(const T *Baseptr, size_type index, size_type count) noexcept : Index(index), Count(count), Data(Baseptr) { assert(Index < N); }
    };

    [[nodiscard]] auto end() const noexcept { return Iterator_t(); }
    [[nodiscard]] auto rend() const noexcept { return std::reverse_iterator(Iterator_t()); }
    [[nodiscard]] auto begin() const noexcept { return Iterator_t(Storage.data(), Tail, size()); }
    [[nodiscard]] auto rbegin() const noexcept { return std::reverse_iterator(Iterator_t(Storage.data(), Head, size())); }
};
