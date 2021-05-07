/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-16
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

template<typename T, size_t N>
class Ringbuffer_t
{
    size_t Head{}, Tail{}, Size{};
    std::array<T, N> Storage{};

    public:
    Ringbuffer_t() = default;
    ~Ringbuffer_t() { clear(); }
    Ringbuffer_t(const Ringbuffer_t &Right) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        Head = Right.Head; Tail = Right.Tail; Size = Right.Size;

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(Storage, Right.Storage, Right.Size * sizeof(T));
        }
        else
        {
            try
            {
                for (auto i = 0; i < Size; ++i)
                    new(Storage + ((Tail + i) % N)) T(Right[Tail + ((Tail + i) % N)]);
            }
            catch (...)
            {
                while (!empty())
                {
                    erase(Tail);
                    Tail = (Tail + 1) % N;
                    --Size;
                }
                throw;
            }
        }
    }
    Ringbuffer_t &operator=(const Ringbuffer_t &Right) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        // Sanity checking.
        if (this == &Right) return *this;

        clear();
        this = Ringbuffer_t(Right);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept { return Size == 0; }
    [[nodiscard]] bool full() const noexcept { return Size == N; }
    [[nodiscard]] size_t capacity() const noexcept { return N; }

    void clear() noexcept
    {
        if constexpr (std::is_trivially_destructible_v<T>)
        {
            while (!empty())
            {
                erase(Tail);
                Tail = (Tail + 1) % N;
                --Size;
            }
        }
    }
    void erase(size_t Index) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            reinterpret_cast<T *>(&Storage[Index])->~T();
    }

    void pop_front() noexcept
    {
        if (empty()) return;
        erase(Tail);

        Tail = (Tail + 1) % N;
        --Size;
    }
    const T *push_back(T &&Value) noexcept
    {
        if (full()) erase(Head);
        Storage[Head] = std::move(Value);
        const auto Result = &Storage[Head];

        if (full()) Tail = (Tail + 1) % N;
        if (!full()) ++Size;
        Head = (Head + 1) % N;

        return Result;
    }
    const T *push_back(const T &Value) noexcept
    {
        if (full()) erase(Head);
        Storage[Head] = Value;
        const auto Result = &Storage[Head];

        if (full()) Tail = (Tail + 1) % N;
        if (!full()) ++Size;
        Head = (Head + 1) % N;

        return Result;
    }

    template <typename... Args>
    T &emplace_back(Args&&... args) noexcept
    {
        if (full()) erase(Head);
        Storage[Head] = { args };
        auto Result = &Storage[Head];

        if (full()) Tail = (Tail + 1) % N;
        if (!full()) ++Size;
        Head = (Head + 1) % N;

        return Result;
    }

    [[nodiscard]] T &back() noexcept { return reinterpret_cast<T &>(Storage[std::clamp(Head, 0UL, N - 1)]); }
    [[nodiscard]] const T &front() const noexcept { return const_cast<Ringbuffer_t<T, N> *>(this)->front(); }
    [[nodiscard]] const T &back() const noexcept { return const_cast<Ringbuffer_t<T, N> *>(back)->back(); }
    [[nodiscard]] T &front() noexcept { return reinterpret_cast<T &>(Storage[Tail]); }

    [[nodiscard]] T &operator[](size_t Index) noexcept { return reinterpret_cast<T &>(Storage[Index]); }
    [[nodiscard]] const T &operator[](size_t Index) const noexcept { return const_cast<Ringbuffer_t<T, N> *>(this)->operator[](Index); }

    template<typename T, size_t N, bool C>
    struct Iterator
    {
        using Buffer_t = std::conditional_t<!C, Ringbuffer_t<T, N> *, Ringbuffer_t<T, N> const *>;
        using iterator_category = std::bidirectional_iterator_tag;
        using self_type = Iterator<T, N, C>;
        using difference_type = ptrdiff_t;
        using const_reference = T const &;
        using const_pointer = T const *;
        using size_type = size_t;
        using value_type = T;
        using reference = T &;
        using pointer = T *;

        Buffer_t Storage{};
        size_t Index{}, Count{};

        Iterator() noexcept = default;
        Iterator(Iterator &&) noexcept = default;
        Iterator(Iterator const &) noexcept = default;
        Iterator &operator=(Iterator const &) noexcept = default;
        Iterator(Buffer_t source, size_t index, size_t count) noexcept : Storage{ source }, Index{ index }, Count{ count } { }

        template<typename = std::enable_if<!C>> [[nodiscard]] T &operator*() noexcept { return (*Storage)[Index]; }
        template<typename = std::enable_if<!C>> [[nodiscard]] T *operator->() noexcept { return &(*Storage)[Index]; }
        template<typename = std::enable_if<C>> [[nodiscard]] const T &operator*() const noexcept { return (*Storage)[Index]; }
        template<typename = std::enable_if<C>> [[nodiscard]] const T *operator->() const noexcept { return &(*Storage)[Index]; }

        bool operator!=(const Iterator &Right) const { return Index != Right.Index; }
        Iterator<T, N, C> &operator++() noexcept
        {
            Index = (Index + 1) % N;
            ++Count;
            return *this;
        }
        Iterator<T, N, C> &operator--() noexcept
        {
            Index = (Index + N - 1) % N;
            --Count;
            return *this;
        }

        [[nodiscard]] size_t index() const noexcept { return Index; }
        [[nodiscard]] size_t count() const noexcept { return Count; }
        ~Iterator() = default;
    };

    using iterator = Iterator<T, N, false>;
    using const_iterator = Iterator<T, N, true>;
    using reverse_iterator = std::reverse_iterator<Iterator<T, N, false>>;
    using const_reverse_iterator = std::reverse_iterator<Iterator<T, N, true>>;
    using const_reference = T const &;
    using const_pointer = T const *;
    using size_type = size_t;
    using value_type = T;
    using reference = T &;
    using pointer = T *;

    [[nodiscard]] iterator begin() noexcept { return iterator{ this, Tail, 0 }; }
    [[nodiscard]] iterator end() noexcept { return iterator{ this, Head, Size }; }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator{ this, Tail, 0 }; }
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator{ this, Head, Size }; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return const_iterator{ this, Tail, 0 }; }
    [[nodiscard]] const_iterator cend() const noexcept { return const_iterator{ this, Head, Size }; }

    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator{ iterator{this, Tail, 0} }; }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator{ iterator{this, Head, Size} }; }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator{ const_iterator{this, Tail, 0} }; }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{ const_iterator{this, Head, Size} }; }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator{ const_iterator{this, Tail, 0} }; }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{ const_iterator{this, Head, Size} }; }
};
