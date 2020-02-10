/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// The types of data that can be handled.
using Blob = std::basic_string<uint8_t>;
enum Bytebuffertype : uint8_t
{
    BB_NONE = 0,
    BB_BOOL = 1,
    BB_SINT8 = 2,
    BB_UINT8 = 3,
    BB_WCHAR = 4,
    BB_SINT16 = 5,
    BB_UINT16 = 6,
    BB_SINT32 = 7,
    BB_UINT32 = 8,
    BB_SINT64 = 9,
    BB_UINT64 = 10,
    BB_FLOAT32 = 11,
    BB_FLOAT64 = 12,
    BB_ASCIISTRING = 13,
    BB_UNICODESTRING = 14,
    BB_BLOB = 15,

    BB_NAN = 99,
    BB_ARRAY = 100,
    BB_MAX
};

struct Bytebuffer
{
    std::unique_ptr<uint8_t[]> Internalbuffer;
    size_t Internalsize{}, Internaliterator{};

    // Construct the buffer from different containers.
    template <typename Type> Bytebuffer(const std::basic_string<Type> &Data);
    template <typename Iterator> Bytebuffer(Iterator Begin, Iterator End);
    template <typename Type> Bytebuffer(const std::vector<Type> &Data);
    Bytebuffer(const void *Data, size_t Size);
    Bytebuffer(const Bytebuffer &Right);
    Bytebuffer(Bytebuffer &&Right);
    Bytebuffer() = default;

    // Basic IO.
    bool Rawread(size_t Readcount, void *Buffer = nullptr);
    void Rawwrite(size_t Writecount, const void *Buffer = nullptr);

    // Typed IO.
    template <typename Type> Type Read(bool Typechecked = true);
    template <typename Type> bool Read(Type &Buffer, bool Typechecked = true);
    template <typename Type> void Write(const Type Value, bool Typechecked = true);

    // Special IO
    template <typename Type> Type Readcompressed();
    template <typename Type> bool Readcompressed(Type &Buffer);
    template <typename Type> void Writecompressed(const Type Value);

    // Utility functionality.
    [[nodiscard]] size_t Remaininglength() const;
    [[nodiscard]] std::string to_string();
    [[nodiscard]] Blob asBlob() const;
    [[nodiscard]] uint8_t Peek();
    void Rewind();

    // Supported operators, acts on the internal state.
    Bytebuffer &operator += (const Bytebuffer &Right) noexcept;
    Bytebuffer &operator + (const Bytebuffer &Right) noexcept;
    Bytebuffer &operator = (const Bytebuffer &Right) noexcept;
    Bytebuffer &operator = (Bytebuffer &&Right) noexcept;
    bool operator == (const Bytebuffer &Right) noexcept;
};
