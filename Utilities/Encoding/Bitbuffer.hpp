/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// The types of data that can be handled.
using Blob = std::basic_string<uint8_t>;
enum Bytebuffertype : uint8_t;

struct Bitbuffer
{
    std::unique_ptr<uint8_t[]> Internalbuffer;
    size_t Internalsize{}, Internaliterator{};
    bool Quakecompatibible{ false };

    // Construct the buffer from different containers.
    template <typename Type> Bitbuffer(const std::basic_string<Type> &Data);
    template <typename Iterator> Bitbuffer(Iterator Begin, Iterator End);
    template <typename Type> Bitbuffer(const std::vector<Type> &Data);
    Bitbuffer(const void *Data, size_t Size);
    Bitbuffer(const Bitbuffer &Right);
    Bitbuffer(Bitbuffer &&Right);
    Bitbuffer() = default;

    // Basic IO.
    bool Rawread(size_t Readcount, void *Buffer = nullptr);
    void Rawwrite(size_t Writecount, const void *Buffer = nullptr);

    // Typed IO.
    template <typename Type> Type Read(bool Typechecked = true);
    template <typename Type> bool Read(Type &Buffer, bool Typechecked = true);
    template <typename Type> void Write(const Type Value, bool Typechecked = true);

    // Utility functionality.
    [[nodiscard]] size_t Remaininglength() const;
    [[nodiscard]] Blob asBlob() const;
    [[nodiscard]] uint8_t Peek();
    void Rewind();

    // Supported operators, acts on the internal state.
    Bitbuffer &operator += (const Bitbuffer &Right) noexcept;
    Bitbuffer &operator + (const Bitbuffer &Right) noexcept;
    Bitbuffer &operator = (const Bitbuffer &Right) noexcept;
    Bitbuffer &operator = (Bitbuffer &&Right) noexcept;
    bool operator == (const Bitbuffer &Right) noexcept;
};
