/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT

    A simple buffer-type that prefixes
    data with an identifying byte.
*/

#pragma once
#pragma warning(push)
#include <Stdinclude.hpp>
#pragma warning(disable: 4702)
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// The types of data that can be handled.
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

namespace BBInternal
{
    // Helpers for type deduction.
    template <class, template <class...> class> inline constexpr bool isDerived = false;
    template< template <class...> class T, class... Args> inline constexpr bool isDerived<T<Args...>, T> = true;

    // Get the ID from the type to simplify future operations.
    template <typename Type> constexpr uint8_t toID()
    {
        // If it's derived from vector, we need to get the internal type.
        if constexpr (isDerived<Type, std::vector>) return BB_ARRAY + toID<typename Type::value_type>();

        // If it's an enumeration, get the base type.
        if constexpr (std::is_enum_v<Type>) return toID<std::underlying_type_t<Type>>();

        // Special containers.
        if constexpr (std::is_same_v<Type, std::basic_string<uint8_t>>)     return BB_BLOB;
        if constexpr (std::is_same_v<Type, std::basic_string<char>>)        return BB_ASCIISTRING;
        if constexpr (std::is_same_v<Type, std::basic_string<wchar_t>>)     return BB_UNICODESTRING;

        // POD.
        if constexpr (std::is_same_v<std::decay_t<Type>, bool>)             return BB_BOOL;
        if constexpr (std::is_same_v<std::decay_t<Type>, wchar_t>)          return BB_WCHAR;
        if constexpr (std::is_same_v<std::decay_t<Type>, int8_t>)           return BB_SINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint8_t>)          return BB_UINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, int16_t>)          return BB_SINT16;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint16_t>)         return BB_UINT16;
        if constexpr (std::is_same_v<std::decay_t<Type>, int32_t>)          return BB_SINT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint32_t>)         return BB_UINT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, int64_t>)          return BB_SINT64;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint64_t>)         return BB_UINT64;
        if constexpr (std::is_same_v<std::decay_t<Type>, float>)            return BB_FLOAT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, double>)           return BB_FLOAT64;

        return BB_NONE;
    }
}

struct Bytebuffer
{
    std::unique_ptr<uint8_t[]> Internalbuffer;
    size_t Internalsize{}, Internaliterator{};

    // Construct the buffer from different containers.
    template <typename Type> Bytebuffer(const std::basic_string<Type> &Data) : Bytebuffer(Data.data(), Data.size()) {}
    template <typename Type> Bytebuffer(const std::vector<Type> &Data) : Bytebuffer(Data.data(), Data.size()) {}
    template <typename Iterator> Bytebuffer(Iterator Begin, Iterator End)
    {
        Internalbuffer = std::make_unique<uint8_t[]>(std::distance(Begin, End));
        std::copy(Begin, End, Internalbuffer.get());
        Internalsize = std::distance(Begin, End);
    }
    Bytebuffer(const void *Data, size_t Size)
    {
        Internalbuffer = std::make_unique<uint8_t[]>(Size + 1);
        std::memcpy(Internalbuffer.get(), Data, Size);
        Internalsize = Size;
    }
    Bytebuffer(const Bytebuffer &Right)
    {
        Internalsize = Right.Internalsize;
        Internaliterator = Right.Internaliterator;
        Internalbuffer = std::make_unique<uint8_t[]>(Internalsize);
        std::memcpy(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
    }
    Bytebuffer(Bytebuffer &&Right) noexcept
    {
        Internaliterator = Right.Internaliterator;
        Internalbuffer.swap(Right.Internalbuffer);
        Internalsize = Right.Internalsize;
    }
    Bytebuffer(size_t Size)
    {
        Internalbuffer = std::make_unique<uint8_t[]>(Size);
        Internalsize = Size;
    }
    Bytebuffer() = default;

    // Basic IO.
    void Rawwrite(size_t Writecount, const void *Buffer = nullptr)
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
    bool Rawread(size_t Readcount, void *Buffer = nullptr)
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
    template <typename Type> void Write(const Type Value, bool Typechecked = true)
    {
        constexpr auto TypeID = BBInternal::toID<Type>();

        // Special case of using a bytebuffer as blob.
        if constexpr (std::is_same_v<Type, Bytebuffer>)
        {
            Rawwrite(Value.Internalsize, Value.Internalbuffer.get());
            return;
        }

        // A byte prefix for the type.
        if (Typechecked || TypeID > BB_ARRAY)
            Rawwrite(sizeof(TypeID), &TypeID);

        // Serialize an array of values.
        if constexpr (BBInternal::isDerived<Type, std::vector>)
        {
            Write(uint32_t(sizeof(typename Type::value_type) * Value.size()));
            Write(uint32_t(Value.size()), false);

            for (const auto &Item : Value) Write(Item, false);
            return;
        }

        // Serialize as a blob of data.
        if constexpr (BBInternal::isDerived<Type, std::basic_string>)
        {
            if constexpr (std::is_same_v<Type, Blob>)
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

        // POD.
        Rawwrite(sizeof(Type), &Value);
    }
    template <typename Type> bool Read(Type &Buffer, bool Typechecked = true)
    {
        constexpr auto TypeID = BBInternal::toID<Type>();
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
        if constexpr (BBInternal::isDerived<Type, std::vector>)
        {
            // Total data-size.
            Buffer.reserve(Read<uint32_t>(true));
            auto Count = Read<uint32_t>(false);

            while (Count--) Buffer.emplace_back(Read<typename Type::value_type>(false));
            return !Buffer.empty();
        }

        // Deserialize as a blob of data.
        if constexpr (std::is_same_v<Type, Blob>)
        {
            auto Bloblength = Read<uint32_t>(Typechecked);
            Buffer.resize(Bloblength);

            return Rawread(Bloblength, Buffer.data());
        }

        // POD.
        return Rawread(sizeof(Type), &Buffer);
    }
    template <> bool Read(std::wstring &Buffer, bool Typechecked)
    {
        constexpr auto TypeID = BBInternal::toID<std::wstring>();
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
    template <> bool Read(std::string &Buffer, bool Typechecked)
    {
        constexpr auto TypeID = BBInternal::toID<std::string>();
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
    template <typename Type> Type Read(bool Typechecked = true)
    {
        Type Result{};
        if (Typechecked && Peek() == BB_NAN) Rawread(1);
        else Read(Result, Typechecked);
        return Result;
    }

    // Utility functionality.
    void Rewind()
    {
        Internaliterator = 0;
    }
    [[nodiscard]] uint8_t Peek()
    {
        const auto Byte = Read<uint8_t>(false);
        if (Byte != BB_NONE) Internaliterator--;
        return Byte;
    }
    [[nodiscard]] Blob asBlob() const
    {
        return { Internalbuffer.get(), Internalsize };
    }
    [[nodiscard]] Blob_view asView() const
    {
        return { Internalbuffer.get(), Internalsize };
    }
    [[nodiscard]] size_t Remaininglength() const
    {
        return Internalsize - Internaliterator;
    }

    // Supported operators, acts on the internal state.
    bool operator == (const Bytebuffer &Right) const noexcept
    {
        if (Internalsize != Right.Internalsize) return false;
        return 0 == std::memcmp(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
    }
    Bytebuffer &operator = (Bytebuffer &&Right) noexcept
    {
        if (this != &Right)
        {
            Internaliterator = std::exchange(Right.Internaliterator, NULL);
            Internalbuffer = std::exchange(Right.Internalbuffer, nullptr);
            Internalsize = std::exchange(Right.Internalsize, NULL);
        }

        return *this;
    }
    Bytebuffer &operator = (const Bytebuffer &Right) noexcept
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
    Bytebuffer &operator + (const Bytebuffer &Right) noexcept
    {
        *this += Right;
        return *this;
    }
    Bytebuffer &operator += (const Bytebuffer &Right) noexcept
    {
        Rawwrite(Right.Internalsize, Right.Internalbuffer.get());
        return *this;
    }
};

#pragma region Extensions
// Interface for serializable objects.
struct ISerializable
{
    virtual ~ISerializable() = default;
    virtual void Serialize(Bytebuffer &) { assert(false); }
    virtual void Deserialize(Bytebuffer &) { assert(false); }
};

// This object needs to own all values (use new).
struct bbObject : ISerializable
{
    std::vector<ISerializable *> Values;

    ~bbObject() override { for (const auto Item : Values) delete Item; }
    explicit bbObject(std::vector<ISerializable *> Input) : Values(std::move(Input)) {}

    void Serialize(Bytebuffer &Buffer) override { for (const auto &Item : Values) Item->Serialize(Buffer); }
    void Deserialize(Bytebuffer &Buffer) override { for (const auto &Item : Values) Item->Deserialize(Buffer); }
};

// Simple wrapper around bytebuffer types.
template<typename Type> struct bbValue : ISerializable
{
    Type Value;
    bool Checked;

    bbValue &operator=(Type &Right) noexcept { Value = Right; return *this; }
    bbValue &operator=(Type &&Right) noexcept { Value = Right; return *this; }
    void Deserialize(Bytebuffer &Buffer) override { Buffer.Read(Value, Checked); }

    bbValue() = default;
    bbValue(Type &&Input, bool Typechecked = true) : Value(Input), Checked(Typechecked) {}
    bbValue(const Type &Input, bool Typechecked = true) : Value(Input), Checked(Typechecked) {}
};

// Helper to bypass formatting.
inline Blob bbSerialize(ISerializable &Object)
{
    Bytebuffer Tempbuffer;
    Object.Serialize(Tempbuffer);
    return Tempbuffer.asBlob();
}
inline Blob bbSerialize(ISerializable &&Object)
{
    Bytebuffer Tempbuffer;
    Object.Serialize(Tempbuffer);
    return Tempbuffer.asBlob();
}

#pragma endregion
#pragma warning(pop)
