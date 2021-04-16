/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once
#include "../Encoding/Stringconv.hpp"
#include <vector>
#include <cstdio>
#include <memory>
#include <string>
#include <io.h>

using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// Helpers to convert the blobs.
inline std::string B2S(const Blob &Input) { return std::string(Input.begin(), Input.end()); }
inline std::string B2S(Blob &&Input) { return std::string(Input.begin(), Input.end()); }
inline Blob S2B(const std::string &Input) { return Blob(Input.begin(), Input.end()); }
inline Blob S2B(std::string &&Input) { return Blob(Input.begin(), Input.end()); }

namespace FS
{
    class MMappedfile_t
    {
        // Internal.
        void *Nativehandle;
        int NativeFD;

        public:
        size_t Size;
        void *Data;

        explicit MMappedfile_t(const std::string &Path)
        {
            #if defined(_WIN32)
            NativeFD = _open(Path.c_str(), 0x800, 0);
            if (NativeFD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(NativeFD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Data = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Data) { CloseHandle(Nativehandle); _close(NativeFD); return; }

            MEMORY_BASIC_INFORMATION Info{};
            VirtualQuery(Data, &Info, sizeof(Info));

            Size = Info.RegionSize;

            #else

            NativeFD = open(Path.c_str(), 0, 0);
            if (NativeFD == -1) return {};

            Size = lseek(NativeFD, 0, SEEK_END);
            Data = (uint8_t *)mmap(NULL, Size, PROT_READ, MAP_PRIVATE, NativeFD, 0);

            #endif
        }
        explicit MMappedfile_t(const std::wstring &Path)
        {
            #if defined(_WIN32)
            NativeFD = _wopen(Path.c_str(), 0x800, 0);
            if (NativeFD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(NativeFD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Data = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Data) { CloseHandle(Nativehandle); _close(NativeFD); return; }

            MEMORY_BASIC_INFORMATION Info{};
            VirtualQuery(Data, &Info, sizeof(Info));

            Size = Info.RegionSize;

            #else

            NativeFD = open(Encoding::toNarrow(Path).c_str(), 0, 0);
            if (NativeFD == -1) return {};

            Size = lseek(NativeFD, 0, SEEK_END);
            Data = (uint8_t *)mmap(NULL, Size, PROT_READ, MAP_PRIVATE, NativeFD, 0);

            #endif
        }
        ~MMappedfile_t()
        {
            #if defined(_WIN32)
            UnmapViewOfFile(Data);
            CloseHandle(Nativehandle);
            _close(NativeFD);

            #else

            munmap((void *)Data, Size);
            close(NativeFD);
            #endif
        }
    };

    [[nodiscard]] inline bool Fileexists(std::string_view Path)
    {
        return std::filesystem::exists(Path);
    }
    [[nodiscard]] inline bool Fileexists(std::wstring_view Path)
    {
        return std::filesystem::exists(Path);
    }

    [[nodiscard]] inline uintmax_t Filesize(std::string_view Path)
    {
        std::error_code Code;
        const auto Size = std::filesystem::file_size(Path, Code);
        return Code.value() ? 0 : Size;
    }
    [[nodiscard]] inline uintmax_t Filesize(std::wstring_view Path)
    {
        std::error_code Code;
        const auto Size = std::filesystem::file_size(Path, Code);
        return Code.value() ? 0 : Size;
    }

    namespace Internal
    {
        template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
        [[nodiscard]] inline std::basic_string<T> Readfile_large(std::string_view Path, size_t Size)
        {
            #if defined(_WIN32)
                const auto FD = _open(Path.data(), 0x800, 0);
                if (FD == -1) return {};

                const auto Handle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
                if (!Handle) return {};

                const auto Mapped = MapViewOfFile(Handle, FILE_MAP_READ, 0, 0, Size);
                if (!Mapped) { CloseHandle(Handle); _close(FD); return {}; }

                std::basic_string<T> Filebuffer(Size, 0);
                std::memcpy(Filebuffer.data(), Mapped, Size);

                UnmapViewOfFile(Mapped);
                CloseHandle(Handle);
                _close(FD);
            #else

                const auto FD = open(Path.data(), 0, 0);
                if (FD == -1) return {};

                const auto Mapped = (uint8_t *)mmap(NULL, Size, PROT_READ, MAP_PRIVATE, FD, 0);

                std::basic_string<T> Filebuffer(Size, 0);
                std::memcpy(Filebuffer.data(), Mapped, Size);

                munmap((void *)Mapped, Size);
                close(FD);
            #endif

            return Filebuffer;
        }
        template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
        [[nodiscard]] inline std::basic_string<T> Readfile_large(std::wstring_view Path, size_t Size)
        {
            #if !defined(_WIN32)
                return Readfile_large(Encoding::toNarrow(Path), Size);
            #else
                const auto FD = _wopen(Path.data(), 0x800, 0);
                if (FD == -1) return {};

                const auto Handle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
                if (!Handle) return {};

                const auto Mapped = MapViewOfFile(Handle, FILE_MAP_READ, 0, 0, Size);
                if (!Mapped) { CloseHandle(Handle); _close(FD); return {}; }

                std::basic_string<T> Filebuffer(Size, 0);
                std::memcpy(Filebuffer.data(), Mapped, Size);

                UnmapViewOfFile(Mapped);
                CloseHandle(Handle);
                _close(FD);

                return Filebuffer;
            #endif
        }

        template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
        [[nodiscard]] inline std::basic_string<T> Readfile_small(std::string_view Path, size_t Size)
        {
            std::FILE *Filehandle = std::fopen(Path.data(), "rb");
            if (!Filehandle) return {};

            const auto Buffer = std::make_unique<T[]>(Size);
            std::fread(Buffer.get(), Size, 1, Filehandle);
            std::fclose(Filehandle);

            return std::basic_string<T>(Buffer.get(), Size);
        }
        template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
        [[nodiscard]] inline std::basic_string<T> Readfile_small(std::wstring_view Path, size_t Size)
        {
            #if defined(_WIN32)
            std::FILE *Filehandle = _wfopen(Path.data(), L"rb");
            #else
            std::FILE *Filehandle = std::fopen(Encoding::toNarrow(Path).c_str(), "rb");
            #endif
            if (!Filehandle) return {};

            const auto Buffer = std::make_unique<T[]>(Size);
            std::fread(Buffer.get(), Size, 1, Filehandle);
            std::fclose(Filehandle);

            return std::basic_string<T>(Buffer.get(), Size);
        }
    }

    template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] inline std::basic_string<T> Readfile(std::string_view Path)
    {
        const auto Size = Filesize(Path);

        if (Size > 4096) return Internal::Readfile_large<T>(Path, Size);
        else return Internal::Readfile_small<T>(Path, Size);
    }
    template <typename T = uint8_t, typename = std::enable_if<sizeof(T) == 1>>
    [[nodiscard]] inline std::basic_string<T> Readfile(std::wstring_view Path)
    {
        const auto Size = Filesize(Path);
        if (Size > 4096) return Internal::Readfile_large<T>(Path, Size);
        else return Internal::Readfile_small<T>(Path, Size);
    }

    template<typename T>
    inline bool Writefile(std::string_view Path, std::basic_string_view<T> Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size() * sizeof(T), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    template<typename T>
    inline bool Writefile(std::string_view Path, const std::basic_string<T> &Buffer)
    { return Writefile(Path, std::basic_string_view<T>(Buffer)); }

    template<typename T>
    inline bool Writefile(std::wstring_view Path, std::basic_string_view<T> Buffer)
    {
        #if defined(_WIN32)
        std::FILE *Filehandle = _wfopen(Path.data(), L"wb");
        #else
        std::FILE *Filehandle = std::fopen(Encoding::toNarrow(Path).c_str(), "wb");
        #endif
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size() * sizeof(T), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    template<typename T>
    inline bool Writefile(std::wstring_view Path, const std::basic_string<T> &Buffer)
    { return Writefile(Path, std::basic_string_view<T>(Buffer)); }

    // Directory iteration, filename if not recursive; else full path.
    [[nodiscard]] inline std::vector<std::string> Findfiles(std::string Directorypath, std::string_view Criteria, bool Recursive = false)
    {
        std::vector<std::string> Result;

        if (!Recursive)
        {
            for (const auto Items = std::filesystem::directory_iterator(Directorypath); const auto &File : Items)
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().string();
                if (std::strstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(Filename);
            }
        }
        else
        {
            for (const auto Items = std::filesystem::recursive_directory_iterator(Directorypath); const auto &File : Items)
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().string();
                if (std::strstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(File.path().string());
            }
        }

        return Result;
    }
    [[nodiscard]] inline std::vector<std::wstring> Findfiles(std::wstring Directorypath, std::wstring_view Criteria, bool Recursive = false)
    {
        std::vector<std::wstring> Result;

        if (!Recursive)
        {
            for (const auto Items = std::filesystem::directory_iterator(Directorypath); const auto &File : Items)
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().wstring();
                if (std::wcsstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(Filename);
            }
        }
        else
        {
            for (const auto Items = std::filesystem::recursive_directory_iterator(Directorypath); const auto &File : Items)
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().wstring();
                if (std::wcsstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(File.path().wstring());
            }
        }

        return Result;
    }

    // Simplified status.
    #include <sys/types.h>
    #include <sys/stat.h>
    using Stat_t = struct { uint32_t Created, Modified, Accessed; };
    [[nodiscard]] inline Stat_t Filestats(std::string_view Path)
    {
        struct stat Buffer;
        if (stat(Path.data(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
    }
    [[nodiscard]] inline Stat_t Filestats(std::wstring_view Path)
    {
        #if defined(_WIN32)
        struct _stat Buffer;
        if (_wstat(Path.data(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
        #else
        struct stat Buffer;
        if (stat(Encoding::toNarrow(Path).c_str(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
        #endif
    }
}
