/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-04
    License: MIT

    Got annoyed by JSON using so much memory.
    This is the result.
*/

#pragma once
#include <Stdinclude.hpp>

#if defined (HAS_LZ4)
struct LZString_t
{
    std::shared_ptr<char8_t []> Internalstorage{};
    size_t Buffersize{};

    template<typename T> LZString_t(const std::basic_string<T> &Input)
    {
        const auto Safestring = Encoding::toUTF8(Input);
        const auto Size = LZ4_compressBound(int(Safestring.size()));

        Buffersize = Size + 1;
        Internalstorage = std::make_shared<char8_t[]>(Buffersize);
        LZ4_compress_default((char *)Safestring.data(), (char *)Internalstorage.get(), (int)Input.size(), (int)Buffersize);
    }
    template<typename T> LZString_t(std::basic_string_view<T> Input)
    {
        const auto Safestring = Encoding::toUTF8(Input);
        const auto Size = LZ4_compressBound(int(Safestring.size()));

        Buffersize = Size + 1;
        Internalstorage = std::make_shared<char8_t[]>(Buffersize);
        LZ4_compress_default((char *)Safestring.data(), (char *)Internalstorage.get(), (int)Input.size(), (int)Buffersize);
    }
    LZString_t() = default;

    [[nodiscard]] operator std::u8string() const
    {
        const auto Decodebuffer = std::make_shared<char8_t []>(Buffersize * 3);
        const auto Decodesize = LZ4_decompress_safe((char *)Internalstorage.get(),
            (char *)Decodebuffer.get(), (int)Buffersize, (int)Buffersize * 3);

        return std::u8string(Decodebuffer.get(), Decodesize);
    }
    [[nodiscard]] operator std::wstring() const
    {
        const auto Decodebuffer = std::make_shared<char8_t []>(Buffersize * 3);
        const auto Decodesize = LZ4_decompress_safe((char *)Internalstorage.get(),
            (char *)Decodebuffer.get(), (int)Buffersize, (int)Buffersize * 3);

        return Encoding::toWide(std::u8string_view(Decodebuffer.get(), Decodesize));
    }
    [[nodiscard]] operator std::string() const
    {
        const auto Decodebuffer = std::make_shared<char8_t []>(Buffersize * 3);
        const auto Decodesize = LZ4_decompress_safe((char *)Internalstorage.get(),
            (char *)Decodebuffer.get(), (int)Buffersize, (int)Buffersize * 3);

        return Encoding::toNarrow(std::u8string_view(Decodebuffer.get(), Decodesize));
    }

    [[nodiscard]] const char8_t *data() const
    {
        return Internalstorage.get();
    }
    [[nodiscard]] size_t size() const
    {
        return Buffersize;
    }
};
#else
using LZString_t = std::u8string;
#endif
