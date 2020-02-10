/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT
*/

#include "Bitbuffer.hpp"
#include "Bytebuffer.hpp"
#pragma warning(disable: 4702)

// Helpers for type deduction.
template <class T, template <class...> class Template>
struct isDerived : std::false_type {};
template <template <class...> class Template, class... Args>
struct isDerived<Template<Args...>, Template> : std::true_type {};

// Construct the buffer from different containers.
template <typename Type> Bitbuffer::Bitbuffer(const std::basic_string<Type> &Data) : Bitbuffer(Data.data(), Data.size()) {}
template <typename Type> Bitbuffer::Bitbuffer(const std::vector<Type> &Data) : Bitbuffer(Data.data(), Data.size()) {}
template <typename Iterator> Bitbuffer::Bitbuffer(Iterator Begin, Iterator End)
{
    Internalbuffer = std::make_unique<uint8_t[]>(std::distance(Begin, End) + 1);
    std::copy(Begin, End, Internalbuffer.get());
    Internalsize = std::distance(Begin, End);
}
Bitbuffer::Bitbuffer(const void *Data, size_t Size)
{
    Internalbuffer = std::make_unique<uint8_t[]>(Size + 1);
    std::memcpy(Internalbuffer.get(), Data, Size);
    Internalsize = Size;
}
Bitbuffer::Bitbuffer(const Bitbuffer &Right)
{
    Internalsize = Right.Internalsize;
    Internaliterator = Right.Internaliterator;
    Internalbuffer = std::make_unique<uint8_t[]>(Internalsize);
    std::memcpy(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
}
Bitbuffer::Bitbuffer(Bitbuffer &&Right)
{
    Internaliterator = Right.Internaliterator;
    Internalbuffer.swap(Right.Internalbuffer);
    Internalsize = Right.Internalsize;
}

// Get the ID from the type to simplify future operations.
template <typename Type> constexpr uint8_t toID()
{
    // Special containers.
    if constexpr (std::is_same<Type, Blob>::value)                                   return BB_BLOB;
    if constexpr (std::is_same<Type, std::basic_string<char>>::value)                return BB_ASCIISTRING;
    if constexpr (std::is_same<Type, std::basic_string<wchar_t>>::value)             return BB_UNICODESTRING;

    // POD.
    if constexpr (std::is_same<typename std::decay<Type>::type, bool>::value)        return BB_BOOL;
    if constexpr (std::is_same<typename std::decay<Type>::type, wchar_t>::value)     return BB_WCHAR;
    if constexpr (std::is_same<typename std::decay<Type>::type, int8_t>::value)      return BB_SINT8;
    if constexpr (std::is_same<typename std::decay<Type>::type, uint8_t>::value)     return BB_UINT8;
    if constexpr (std::is_same<typename std::decay<Type>::type, int16_t>::value)     return BB_SINT16;
    if constexpr (std::is_same<typename std::decay<Type>::type, uint16_t>::value)    return BB_UINT16;
    if constexpr (std::is_same<typename std::decay<Type>::type, int32_t>::value)     return BB_SINT32;
    if constexpr (std::is_same<typename std::decay<Type>::type, uint32_t>::value)    return BB_UINT32;
    if constexpr (std::is_same<typename std::decay<Type>::type, int64_t>::value)     return BB_SINT64;
    if constexpr (std::is_same<typename std::decay<Type>::type, uint64_t>::value)    return BB_UINT64;
    if constexpr (std::is_same<typename std::decay<Type>::type, float>::value)       return BB_FLOAT32;
    if constexpr (std::is_same<typename std::decay<Type>::type, double>::value)      return BB_FLOAT64;

    return BB_NONE;
}

// Basic IO.
void Bitbuffer::Rawwrite(size_t Writecount, const void *Buffer)
{
    // Expand the buffer if needed.
    if (Internaliterator + Writecount > Internalsize * 8)
    {
        auto Newbuffer = std::make_unique<uint8_t[]>(Internalsize + Writecount / 8 + 1);
        std::memmove(Newbuffer.get(), Internalbuffer.get(), Internalsize);
        Internalsize += Writecount / 8 + 1;
        Internalbuffer.swap(Newbuffer);
    }

    size_t Bitstowrite = Writecount;
    auto Input = (uint8_t *)Buffer;

    while (Bitstowrite)
    {
        size_t Readposition = (Writecount - Bitstowrite) / 8;
        uint8_t Currentbit = (Writecount - Bitstowrite) & 7;
        uint8_t Writeposition = Internaliterator & 7;
        uint8_t Currentwrite = uint8_t(std::min(Bitstowrite, size_t(8 - Writeposition)));

        // Can be null when padding/aligning.
        if (Input)
        {
            uint8_t Inputbyte = Input[Readposition];
            uint8_t Nextbyte = (Writecount - 1) / 8 > Readposition ? Input[Readposition + 1] : 0;
            uint8_t Mask = (0xFF >> (8 - Writeposition)) | (0xFF << (Writeposition + Currentwrite));

            Inputbyte = (Nextbyte << (8 - Currentbit)) | Inputbyte >> Currentbit;
            uint8_t Resultbyte = ~Mask & (Inputbyte << Writeposition) | Mask & Internalbuffer[Internaliterator / 8];
            Internalbuffer[Internaliterator / 8] = Resultbyte;
        }

        Internaliterator += Currentwrite;
        Bitstowrite -= Currentwrite;
    }
}
bool Bitbuffer::Rawread(size_t Readcount, void *Buffer)
{
    auto Output = (uint8_t *)Buffer;
    if (Internaliterator + Readcount > Internalsize * 8) return false;

    while (Readcount)
    {
        uint8_t Thisread = uint8_t(std::min(Readcount, size_t(8)));
        uint8_t Thisbyte = Internalbuffer[Internaliterator / 8];
        uint8_t Remaining = Internaliterator & 7;

        // Can be null for discarding data.
        if (Buffer)
        {
            if (Thisread + Remaining <= 8)
            {
                *Output = (0xFF >> (8 - Thisread)) & (Thisbyte >> Remaining);
            }
            else
            {
                *Output = (0xFF >> (8 - Thisread)) & (Internalbuffer[Internaliterator / 8 + 1] << (8 - Remaining)) | (Thisbyte >> Remaining);
            }

            Output++;
        }
        Readcount -= Thisread;
        Internaliterator += Thisread;
    }

    return true;
}

// Typed IO.
template <typename Type> Type Bitbuffer::Read(bool Typechecked)
{
    Type Result{};
    if (Typechecked && Peek() == BB_NAN) Rawread(5);
    else Read(Result, Typechecked);
    return Result;
}
template <typename Type> bool Bitbuffer::Read(Type &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<Type>();
    uint8_t Storedtype;

    // Verify the data-type.
    if (Typechecked)
    {
        if (!Rawread(5, &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator -= 5;
            return false;
        }
    }

    // Special case of bool being 1 bit.
    if constexpr (std::is_same<Type, bool>::value)
    {
        return Rawread(1, &Buffer);
    }

    // Deserialize as a blob of data.
    if constexpr (std::is_same<Type, Blob>::value)
    {
        auto Bloblength = Read<uint32_t>(Typechecked);
        Buffer.resize(Bloblength);

        return Rawread(Bloblength * 8, Buffer.data());
    }

    // POD.
    return Rawread(sizeof(Type) * 8, &Buffer);
}
template <> bool Bitbuffer::Read(std::string &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<std::string>();
    uint8_t Storedtype;

    // Verify the datatype.
    if (Typechecked)
    {
        if (!Rawread(5, &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator -= 5;
            return false;
        }
    }

    // Quake expects strings to be byte-aligned.
    if (Quakecompatibible) Rawread(Internaliterator % 8);

    const auto Offset = Internalbuffer.get() + Internaliterator / 8;
    Buffer = (char *)Offset;
    return Rawread((Buffer.size() + 1) * sizeof(char) * 8);
}
template <> bool Bitbuffer::Read(std::wstring &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<std::wstring>();
    uint8_t Storedtype;

    // Verify the datatype.
    if (Typechecked)
    {
        if (!Rawread(5, &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator -= 5;
            return false;
        }
    }

    const auto Offset = Internalbuffer.get() + Internaliterator / 8;
    Buffer = (wchar_t *)Offset;
    return Rawread((Buffer.size() + 1) * sizeof(wchar_t) * 8);
}
template <typename Type> void Bitbuffer::Write(const Type Value, bool Typechecked)
{
    constexpr auto TypeID = toID<Type>();

    // A byte prefix for the type.
    if (Typechecked)
        Rawwrite(5, &TypeID);

    // Special case of bool being 1 bit.
    if constexpr (std::is_same<Type, bool>::value)
    {
        Rawwrite(1, &Value);
        return;
    }

    // Serialize as a blob of data.
    if constexpr (isDerived<Type, std::basic_string>::value)
    {
        if constexpr (std::is_same<Type, Blob>::value)
        {
            Write(uint32_t(Value.size()), Typechecked);
            Rawwrite(Value.size() * 8, Value.data());
        }
        else
        {
            // Quake expects strings to be byte-aligned.
            if (Quakecompatibible && Internaliterator % 8) Rawwrite(8 - Internaliterator % 8);

            Rawwrite((Value.size() + 1) * sizeof(typename Type::value_type) * 8, Value.c_str());
        }

        return;
    }

    // POD.
    Rawwrite(sizeof(Type) * 8, &Value);
}

// Utility functionality.
size_t Bitbuffer::Remaininglength() const
{
    return Internalsize * 8 - Internaliterator;
}
Blob Bitbuffer::asBlob() const
{
    return { Internalbuffer.get(), Internalsize };
}
uint8_t Bitbuffer::Peek()
{
    uint8_t Byte{ BB_NONE }; Rawread(5, &Byte);
    if (Byte != BB_NONE) Internaliterator -= 5;
    return Byte;
}
void Bitbuffer::Rewind()
{
    Internaliterator = 0;
}

// Supported operators, acts on the internal state.
Bitbuffer &Bitbuffer::operator + (const Bitbuffer &Right) noexcept
{
    *this += Right;
    return *this;
}
Bitbuffer &Bitbuffer::operator += (const Bitbuffer &Right) noexcept
{
    Rawwrite(Right.Internalsize, Right.Internalbuffer.get());
    return *this;
}
Bitbuffer &Bitbuffer::operator = (const Bitbuffer &Right) noexcept
{
    if (this != &Right)
    {
        if (Internalsize != Right.Internalsize) Internalbuffer = std::make_unique<uint8_t[]>(Right.Internalsize);
        std::memcpy(Internalbuffer.get(), Right.Internalbuffer.get(), Right.Internalsize);

        Internaliterator = Right.Internaliterator;
        Internalsize = Right.Internalsize;
    }

    return *this;
}
Bitbuffer &Bitbuffer::operator = (Bitbuffer &&Right) noexcept
{
    if (this != &Right)
    {
        Internaliterator = std::exchange(Right.Internaliterator, NULL);
        Internalbuffer = std::exchange(Right.Internalbuffer, nullptr);
        Internalsize = std::exchange(Right.Internalsize, NULL);
    }

    return *this;
}
bool Bitbuffer::operator == (const Bitbuffer &Right) noexcept
{
    if (Internalsize != Right.Internalsize) return false;
    return 0 == std::memcmp(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
}

// MSVC does not like it if templates are first used in other modules.
namespace MSVC_HACKERY
{
    void BitFoobar()
    {
        Bitbuffer B;
        B = Bitbuffer(Blob());
        Bitbuffer K{ Blob() };
        Bitbuffer L{ std::string() };

        #define Hackery(x) { B.Read<x>(); B.Write<x>({}); }
        Hackery(bool);
        Hackery(char);
        Hackery(wchar_t);
        Hackery(int8_t);
        Hackery(uint8_t);
        Hackery(int16_t);
        Hackery(uint16_t);
        Hackery(int32_t);
        Hackery(uint32_t);
        Hackery(int64_t);
        Hackery(uint64_t);
        Hackery(float);
        Hackery(double);
        Hackery(Blob);
        Hackery(std::string);
        Hackery(std::wstring);
    }
}
