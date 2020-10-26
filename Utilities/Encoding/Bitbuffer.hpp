/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT
*/

#pragma once
#pragma warning(push)
#include <Stdinclude.hpp>
#include "Bytebuffer.hpp"
#pragma warning(disable: 4702)

struct Bitbuffer
{
    std::unique_ptr<uint8_t[]> Internalbuffer;
    size_t Internalsize{}, Internaliterator{};
    bool Quakecompatibible{ false };

    // Construct the buffer from different containers.
    template <typename Type> Bitbuffer(const std::basic_string<Type> &Data) : Bitbuffer(Data.data(), Data.size()) {}
    template <typename Type> Bitbuffer(const std::vector<Type> &Data) : Bitbuffer(Data.data(), Data.size()) {}
    template <typename Iterator> Bitbuffer(Iterator Begin, Iterator End)
    {
        Internalbuffer = std::make_unique<uint8_t[]>(std::distance(Begin, End) + 1);
        std::copy(Begin, End, Internalbuffer.get());
        Internalsize = std::distance(Begin, End);
    }
    Bitbuffer(const void *Data, size_t Size)
    {
        Internalbuffer = std::make_unique<uint8_t[]>(Size + 1);
        std::memcpy(Internalbuffer.get(), Data, Size);
        Internalsize = Size;
    }
    Bitbuffer(const Bitbuffer &Right)
    {
        Internalsize = Right.Internalsize;
        Internaliterator = Right.Internaliterator;
        Internalbuffer = std::make_unique<uint8_t[]>(Internalsize);
        std::memcpy(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
    }
    Bitbuffer(Bitbuffer &&Right) noexcept
    {
        Internaliterator = Right.Internaliterator;
        Internalbuffer.swap(Right.Internalbuffer);
        Internalsize = Right.Internalsize;
    }
    Bitbuffer() = default;

    // Basic IO.
    void Rawwrite(size_t Writecount, const void *Buffer = nullptr)
    {
        // Expand the buffer if needed.
        if (Internaliterator + Writecount > Internalsize * 8)
        {
            auto Newbuffer = std::make_unique<uint8_t[]>(Internalsize + Writecount / 8 + 1);
            std::memmove(Newbuffer.get(), Internalbuffer.get(), Internalsize);
            Internalsize += Writecount / 8 + 1;
            Internalbuffer.swap(Newbuffer);
        }

        const auto Input = (uint8_t *)Buffer;
        size_t Bitstowrite = Writecount;

        while (Bitstowrite)
        {
            const size_t Readposition = (Writecount - Bitstowrite) / 8;
            const uint8_t Currentbit = (Writecount - Bitstowrite) & 7;
            const uint8_t Writeposition = Internaliterator & 7;
            const uint8_t Currentwrite = uint8_t(std::min(Bitstowrite, size_t(8 - Writeposition)));

            // Can be null when padding/aligning.
            if (Input)
            {
                uint8_t Inputbyte = Input[Readposition];
                const uint8_t Nextbyte = (Writecount - 1) / 8 > Readposition ? Input[Readposition + 1] : 0;
                const uint8_t Mask = (0xFF >> (8 - Writeposition)) | (0xFF << (Writeposition + Currentwrite));

                Inputbyte = (Nextbyte << (8 - Currentbit)) | Inputbyte >> Currentbit;
                const uint8_t Resultbyte = (~Mask & (Inputbyte << Writeposition)) | (Mask & Internalbuffer[Internaliterator / 8]);
                Internalbuffer[Internaliterator / 8] = Resultbyte;
            }

            Internaliterator += Currentwrite;
            Bitstowrite -= Currentwrite;
        }
    }
    bool Rawread(size_t Readcount, void *Buffer = nullptr)
    {
        auto Output = (uint8_t *)Buffer;
        if (Internaliterator + Readcount > Internalsize * 8) return false;

        while (Readcount)
        {
            const uint8_t Thisread = uint8_t(std::min(Readcount, size_t(8)));
            const uint8_t Thisbyte = Internalbuffer[Internaliterator / 8];
            const uint8_t Remaining = Internaliterator & 7;

            // Can be null for discarding data.
            if (Buffer)
            {
                if (Thisread + Remaining <= 8)
                {
                    *Output = (0xFF >> (8 - Thisread)) & (Thisbyte >> Remaining);
                }
                else
                {
                    *Output = ((0xFF >> (8 - Thisread)) & (Internalbuffer[Internaliterator / 8 + 1] << (8 - Remaining))) | (Thisbyte >> Remaining);
                }

                Output++;
            }
            Readcount -= Thisread;
            Internaliterator += Thisread;
        }

        return true;
    }

    // Typed IO.
    template <typename Type> void Write(const Type Value, bool Typechecked = true)
    {
        constexpr auto TypeID = Internal::toID<Type>();

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
        if constexpr (Internal::isDerived<Type, std::basic_string>::value)
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
    template <typename Type> bool Read(Type &Buffer, bool Typechecked = true)
    {
        constexpr auto TypeID = Internal::toID<Type>();
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
    template <> bool Read(std::wstring &Buffer, bool Typechecked)
    {
        constexpr auto TypeID = Internal::toID<std::wstring>();
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
    template <> bool Read(std::string &Buffer, bool Typechecked)
    {
        constexpr auto TypeID = Internal::toID<std::string>();
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
    template <typename Type> Type Read(bool Typechecked = true)
    {
        Type Result{};
        if (Typechecked && Peek() == BB_NAN) Rawread(5);
        else Read(Result, Typechecked);
        return Result;
    }

    // Utility functionality.
    [[nodiscard]] Blob asBlob() const
    {
        return { Internalbuffer.get(), Internalsize };
    }
    [[nodiscard]] void Rewind()
    {
        Internaliterator = 0;
    }
    [[nodiscard]] uint8_t Peek()
    {
        uint8_t Byte{ BB_NONE }; Rawread(5, &Byte);
        if (Byte != BB_NONE) Internaliterator -= 5;
        return Byte;
    }
    [[nodiscard]] size_t Remaininglength() const
    {
        return (Internalsize * 8 - Internaliterator) / 8;
    }
    [[nodiscard]] std::basic_string_view<uint8_t> asView() const
    {
        return { Internalbuffer.get(), Internalsize };
    }

    // Supported operators, acts on the internal state.
    bool operator == (const Bitbuffer &Right) const noexcept
    {
        if (Internalsize != Right.Internalsize) return false;
        return 0 == std::memcmp(Internalbuffer.get(), Right.Internalbuffer.get(), Internalsize);
    }
    Bitbuffer &operator = (Bitbuffer &&Right) noexcept
    {
        if (this != &Right)
        {
            Internaliterator = std::exchange(Right.Internaliterator, NULL);
            Internalbuffer = std::exchange(Right.Internalbuffer, nullptr);
            Internalsize = std::exchange(Right.Internalsize, NULL);
        }

        return *this;
    }
    Bitbuffer &operator = (const Bitbuffer &Right) noexcept
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
    Bitbuffer &operator + (const Bitbuffer &Right) noexcept
    {
        *this += Right;
        return *this;
    }
    Bitbuffer &operator += (const Bitbuffer &Right) noexcept
    {
        Rawwrite(Right.Internalsize, Right.Internalbuffer.get());
        return *this;
    }
};

#pragma warning(pop)
