/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT
*/

#include "Bytebuffer.hpp"
#pragma warning(disable: 4702)

// Helpers for type deduction.
template <class T, template <class...> class Template>
struct isDerived : std::false_type {};
template <template <class...> class Template, class... Args>
struct isDerived<Template<Args...>, Template> : std::true_type {};

// Construct the buffer from different containers.
template <typename Type> Bytebuffer::Bytebuffer(const std::basic_string<Type> &Data) : Bytebuffer(Data.data(), Data.size()) {}
template <typename Type> Bytebuffer::Bytebuffer(const std::vector<Type> &Data) : Bytebuffer(Data.data(), Data.size()) {}
template <typename Iterator> Bytebuffer::Bytebuffer(Iterator Begin, Iterator End)
{
    Internalbuffer = std::make_unique<uint8_t[]>(std::distance(Begin, End));
    std::copy(Begin, End, Internalbuffer.get());
    Internalsize = std::distance(Begin, End);
}
Bytebuffer::Bytebuffer(const void *Data, size_t Size)
{
    Internalbuffer = std::make_unique<uint8_t[]>(Size + 1);
    std::memcpy(Internalbuffer.get(), Data, Size);
    Internalsize = Size;
}
Bytebuffer::Bytebuffer(const Bytebuffer &Right)
{
    Internalsize = Right.Internalsize;
    Internaliterator = Right.Internaliterator;
    Internalbuffer = std::make_unique<uint8_t[]>(Internalsize);
    std::memcpy(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
}
Bytebuffer::Bytebuffer(Bytebuffer &&Right)
{
    Internaliterator = Right.Internaliterator;
    Internalbuffer.swap(Right.Internalbuffer);
    Internalsize = Right.Internalsize;
}

// Get the ID from the type to simplify future operations.
template <typename Type> constexpr uint8_t toID()
{
    // If it's derived from vector, we need to get the internal type.
    if constexpr (isDerived<Type, std::vector>{}) return BB_ARRAY + toID<typename Type::value_type>();

    // If it's an enumeration, get the base type.
    if constexpr (std::is_enum<Type>::value) return toID<typename std::underlying_type<Type>::type>();

    // Special containers.
    if constexpr (std::is_same<Type, Blob>::value)                          return BB_BLOB;
    if constexpr (std::is_same<Type, std::basic_string<char>>::value)       return BB_ASCIISTRING;
    if constexpr (std::is_same<Type, std::basic_string<wchar_t>>::value)    return BB_UNICODESTRING;

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
void Bytebuffer::Rawwrite(size_t Writecount, const void *Buffer)
{
    // If we are at the end of the buffer, increase it.
    if (Internaliterator == Internalsize)
    {
        auto Newbuffer = std::make_unique<uint8_t[]>(Internalsize + Writecount);
        std::memmove(Newbuffer.get(), Internalbuffer.get(), Internalsize);
        Internalbuffer.swap(Newbuffer);
        Internalsize += Writecount;
    }

    // If the buffer is already large enough, overwrite.
    if ((Internaliterator + Writecount) <= Internalsize)
    {
        if (Buffer) std::memcpy(Internalbuffer.get() + Internaliterator, Buffer, Writecount);
        Internaliterator += Writecount;
        return;
    }

    // Else we need to make an overwrite and an append call.
    const size_t Appendcount = Writecount - Remaininglength();
    const size_t Overwritecount = Remaininglength();
    Rawwrite(Overwritecount, Buffer);
    Rawwrite(Appendcount, Buffer ? (uint8_t *)Buffer + Overwritecount : Buffer);
}
bool Bytebuffer::Rawread(size_t Readcount, void *Buffer)
{
    // Range-check, we do not do truncated reads as they are a pain to debug.
    if ((Internaliterator + Readcount) > Internalsize) return false;

    // Copy the data into the new buffer if provided.
    if (Buffer) std::memcpy(Buffer, Internalbuffer.get() + Internaliterator, Readcount);

    // Advance the internal iterator.
    Internaliterator += Readcount;
    return true;
}

// Typed IO.
template <typename Type> Type Bytebuffer::Read(bool Typechecked)
{
    Type Result{};
    if (Typechecked && Peek() == BB_NAN) Rawread(1);
    else Read(Result, Typechecked);
    return Result;
}
template <typename Type> bool Bytebuffer::Read(Type &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<Type>();
    uint8_t Storedtype;

    // Verify the datatype.
    if (Typechecked || TypeID > BB_ARRAY)
    {
        if (!Rawread(sizeof(uint8_t), &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator--;
            return false;
        }
    }

    // Deserialize as an array.
    if constexpr (isDerived<Type, std::vector>::value)
    {
        // Total data-size.
        Buffer.reserve(Read<uint32_t>(true));
        auto Count = Read<uint32_t>(false);

        while (Count--)
            Buffer.push_back(Read<typename Type::value_type>(false));
        return !Buffer.empty();
    }

    // Deserialize as a blob of data.
    if constexpr (std::is_same<Type, Blob>::value)
    {
        auto Bloblength = Read<uint32_t>(Typechecked);
        Buffer.resize(Bloblength);

        return Rawread(Bloblength, Buffer.data());
    }

    // POD.
    return Rawread(sizeof(Type), &Buffer);
}
template <> bool Bytebuffer::Read(std::string &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<std::string>();
    uint8_t Storedtype;

    // Verify the datatype.
    if (Typechecked)
    {
        if (!Rawread(sizeof(uint8_t), &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator--;
            return false;
        }
    }

    const auto Offset = Internalbuffer.get() + Internaliterator;
    Buffer = (char *)Offset;
    return Rawread((Buffer.size() + 1) * sizeof(char));
}
template <> bool Bytebuffer::Read(std::wstring &Buffer, bool Typechecked)
{
    constexpr auto TypeID = toID<std::wstring>();
    uint8_t Storedtype;

    // Verify the datatype.
    if (Typechecked)
    {
        if (!Rawread(sizeof(uint8_t), &Storedtype))
            return false;

        if (Storedtype != TypeID)
        {
            Internaliterator--;
            return false;
        }
    }

    const auto Offset = Internalbuffer.get() + Internaliterator;
    Buffer = (wchar_t *)Offset;
    return Rawread((Buffer.size() + 1) * sizeof(wchar_t));
}
template <typename Type> void Bytebuffer::Write(const Type Value, bool Typechecked)
{
    constexpr auto TypeID = toID<Type>();

    // A byte prefix for the type.
    if (Typechecked || TypeID > BB_ARRAY)
        Rawwrite(sizeof(TypeID), &TypeID);

    // Serialize an array of values.
    if constexpr (isDerived<Type, std::vector>::value)
    {
        Write(uint32_t(sizeof(typename Type::value_type) * Value.size()));
        Write(uint32_t(Value.size()), false);

        for (const auto &Item : Value) Write(Item, false);
        return;
    }

    // Serialize as a blob of data.
    if constexpr (isDerived<Type, std::basic_string>::value)
    {
        if constexpr (std::is_same<Type, Blob>::value)
        {
            Write(uint32_t(Value.size()), Typechecked);
            Rawwrite(Value.size(), Value.data());
        }
        else
        {
            Rawwrite((Value.size() + 1) * sizeof(typename Type::value_type), Value.c_str());
        }

        return;
    }

    // Special case of using a bytebuffer as blob.
    if constexpr (std::is_same<Type, Bytebuffer>::value)
    {
        Rawwrite(Value.Internalsize, Value.Internalbuffer.get());
        return;
    }

    // POD.
    Rawwrite(sizeof(Type), &Value);
}

// Utility functionality.
size_t Bytebuffer::Remaininglength() const
{
    return Internalsize - Internaliterator;
}
uint8_t Bytebuffer::Peek()
{
    const auto Byte = Read<uint8_t>(false);
    if (Byte != BB_NONE) Internaliterator--;
    return Byte;
}
void Bytebuffer::Rewind()
{
    Internaliterator = 0;
}
Blob Bytebuffer::asBlob() const
{
    return { Internalbuffer.get(), Internalsize };
}

// Supported operators, acts on the internal state.
Bytebuffer &Bytebuffer::operator + (const Bytebuffer &Right) noexcept
{
    *this += Right;
    return *this;
}
Bytebuffer &Bytebuffer::operator += (const Bytebuffer &Right) noexcept
{
    Rawwrite(Right.Internalsize, Right.Internalbuffer.get());
    return *this;
}
Bytebuffer &Bytebuffer::operator = (const Bytebuffer &Right) noexcept
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
Bytebuffer &Bytebuffer::operator = (Bytebuffer &&Right) noexcept
{
    if (this != &Right)
    {
        Internaliterator = std::exchange(Right.Internaliterator, NULL);
        Internalbuffer = std::exchange(Right.Internalbuffer, nullptr);
        Internalsize = std::exchange(Right.Internalsize, NULL);
    }

    return *this;
}
bool Bytebuffer::operator == (const Bytebuffer &Right) noexcept
{
    if (Internalsize != Right.Internalsize) return false;
    return 0 == std::memcmp(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
}

// MSVC does not like it if templates are first used in other modules.
namespace MSVC_HACKERY
{
    void ByteFoobar()
    {
        Bytebuffer B;
        B = Bytebuffer(Blob());
        Bytebuffer K{ Blob() };
        Bytebuffer L{ std::string() };

        #define Hackery(x) { std::vector<x> y; B.Read<x>(); B.Read(y); B.Write(y); B.Write<x>({}); }
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
        #undef Hackery
    }
}
