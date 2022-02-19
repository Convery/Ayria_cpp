/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once
#include <io.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <Utilities/Encoding/UTF8.hpp>
#include <Utilities/Constexprhelpers.hpp>

using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// Helpers to convert the blobs.
inline std::string B2S(const Blob &Input) { return std::string(Input.begin(), Input.end()); }
inline std::string B2S(Blob &&Input) { return std::string(Input.begin(), Input.end()); }
inline Blob S2B(const std::string &Input) { return Blob(Input.begin(), Input.end()); }
inline Blob S2B(std::string &&Input) { return Blob(Input.begin(), Input.end()); }

namespace FS
{
    // Make use of the new STL rather than fopen.
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
        return Code ? 0 : Size;
    }
    [[nodiscard]] inline uintmax_t Filesize(std::wstring_view Path)
    {
        std::error_code Code;
        const auto Size = std::filesystem::file_size(Path, Code);
        return Code ? 0 : Size;
    }

    // Holds a memory view of a file.
    class MMap_t
    {
        #if defined (_WIN32)
        HANDLE Nativehandle{};
        #endif

    public:
        std::span<uint8_t> Data{};
        void *Ptr{};
        int FD;

        // STD accessors.
        auto begin() const { return std::cbegin(Data); };
        auto end() const { return std::cend(Data); };
        auto begin() { return std::begin(Data); };
        auto end() { return std::end(Data); };

        #if defined (_WIN32)
        static size_t getSize(HANDLE Handle)
        {
            using NTQuery = long (NTAPI *)(HANDLE, uint32_t, void *, ULONG, ULONG *);
            static const auto Func = (NTQuery)GetProcAddress(LoadLibraryW(L"ntdll.dll"), "NtQuerySection");

            if (Func)
            {
                struct { void *A; ULONG B; LARGE_INTEGER Size; } Info{};
                if (0 <= Func(Handle, 0, &Info, sizeof(Info), NULL))
                {
                    return Info.Size.QuadPart;
                }
            }

            return 0;
        }

        explicit MMap_t(const std::string &Path) : FD(_open(Path.c_str(), 0x800, 0))
        {
            if (FD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Ptr = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Ptr) { CloseHandle(Nativehandle); _close(FD); return; }

            auto Size = getSize(Nativehandle);
            if (Size == 0) Size = FS::Filesize(Path);
            Data = { (uint8_t *)Ptr, Size };
        }
        explicit MMap_t(const std::wstring &Path) : FD(_wopen(Path.c_str(), 0x800, 0))
        {
            if (FD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Ptr = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Ptr) { CloseHandle(Nativehandle); _close(FD); return; }

            auto Size = getSize(Nativehandle);
            if (Size == 0) Size = FS::Filesize(Path);
            Data = { (uint8_t *)Ptr, Size };
        }
        ~MMap_t()
        {
            if (Ptr) UnmapViewOfFile(Ptr);
            if (Nativehandle) CloseHandle(Nativehandle);
            if (FD > 0)_close(FD);
        }

        #else
        explicit MMap_t(const std::string &Path) : FD(open(Path.c_str(), 0, 0))
        {
            if (FD == -1) return;

            const auto Filesize = lseek(FD, 0, SEEK_END);
            lseek(FD, 0, SEEK_SET);

            Ptr = (uint8_t *)mmap(NULL, Filesize, PROT_READ, MAP_PRIVATE, FD, 0);
            Data = { (uint8_t *)Ptr, Info.RegionSize };
        }
        explicit MMap_t(const std::wstring &Path) : MMap_t(Encoding::toASCII(Path)) {}
        ~MMap_t()
        {
            if (Ptr) munmap((void *)Ptr, Data.size());
            if (FD > 0) close(FD);
        }
        #endif
    };

    // Read a full file into memory.
    template <cmp::Byte_t T = uint8_t> [[nodiscard]] inline std::basic_string<T> Readfile(std::string_view Path)
    {
        const auto Filemapping = MMap_t(std::string(Path));
        return { Filemapping.begin(), Filemapping.end() };
    }
    template <cmp::Byte_t T = uint8_t> [[nodiscard]] inline std::basic_string<T> Readfile(std::wstring_view Path)
    {
        const auto Filemapping = MMap_t(std::wstring(Path));
        return { Filemapping.begin(), Filemapping.end() };
    }

    // Overwrite a file.
    template <cmp::Simple_t T> inline bool Writefile(const std::string &Path, const T &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.c_str(), "wb");
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <cmp::Simple_t T> inline bool Writefile(const std::wstring &Path, const T &Buffer)
    {
        #if defined(_WIN32)
        std::FILE *Filehandle = _wfopen(Path.c_str(), L"wb");
        #else
        std::FILE *Filehandle = std::fopen(Encoding::toASCII(Path).c_str(), "wb");
        #endif
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <cmp::Complex_t T> inline bool Writefile(const std::string &Path, const T &Buffer)
    {
        return Writefile(Path, std::as_bytes(std::span(Buffer)));
    }
    template <cmp::Complex_t T> inline bool Writefile(const std::wstring &Path, const T &Buffer)
    {
        return Writefile(Path, std::as_bytes(std::span(Buffer)));
    }

    // Append to a file.
    template <cmp::Simple_t T> inline bool Appendfile(const std::string &Path, const T &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.c_str(), "ab");
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <cmp::Simple_t T> inline bool Appendfile(const std::wstring &Path, const T &Buffer)
    {
        #if defined(_WIN32)
        std::FILE *Filehandle = _wfopen(Path.c_str(), L"ab");
        #else
        std::FILE *Filehandle = std::fopen(Encoding::toASCII(Path).c_str(), "ab");
        #endif
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <cmp::Complex_t T> inline bool Appendfile(const std::string &Path, const T &Buffer)
    {
        return Appendfile(Path, std::as_bytes(std::span(Buffer)));
    }
    template <cmp::Complex_t T> inline bool Appendfile(const std::wstring &Path, const T &Buffer)
    {
        return Appendfile(Path, std::as_bytes(std::span(Buffer)));
    }

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
    [[nodiscard]] inline Stat_t Filestats(const std::string &Path)
    {
        struct stat Buffer;
        if (stat(Path.c_str(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
    }
    [[nodiscard]] inline Stat_t Filestats(const std::wstring &Path)
    {
        #if defined(_WIN32)
        struct _stat Buffer;
        if (_wstat(Path.c_str(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
        #else
        struct stat Buffer;
        if (stat(Encoding::toASCII(Path).c_str(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
        #endif
    }
}
