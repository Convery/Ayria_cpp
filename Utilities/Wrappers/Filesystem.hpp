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

using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// Helpers to convert the blobs.
inline std::string B2S(const Blob &Input) { return std::string(Input.begin(), Input.end()); }
inline std::string B2S(Blob &&Input) { return std::string(Input.begin(), Input.end()); }
inline Blob S2B(const std::string &Input) { return Blob(Input.begin(), Input.end()); }
inline Blob S2B(std::string &&Input) { return Blob(Input.begin(), Input.end()); }

namespace FS
{
    template <typename T> concept Byte_t = sizeof(T) == 1;
    template <typename T> concept Range_t = requires (const T &t) { std::ranges::begin(t); std::ranges::end(t); std::ranges::size(t); };
    template <typename T> concept Complexstring_t = Range_t<T> && !Byte_t<typename T::value_type>;
    template <typename T> concept Simplestring_t = Range_t<T> && Byte_t<typename T::value_type>;

    // Convert the range to a flat type (as you can't cast pointers in constexpr).
    template <Complexstring_t T> [[nodiscard]] constexpr Blob Flatten(const T &Input)
    {
        Blob Bytearray; Bytearray.reserve(Input.size() * sizeof(typename T::value_type));

        for (auto &&Item : Input)
        {
            for (size_t i = 0; i < (sizeof(typename T::value_type) - 1); ++i)
            {
                Bytearray.append(Item & 0xFF);
                Item >>= 8;
            }

            Bytearray.append(Item & 0xFF);
        }

        return Bytearray;
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
        int FD{};

        // STD accessors.
        auto begin() const { return std::cbegin(Data); };
        auto end() const { return std::cend(Data); };
        auto begin() { return std::begin(Data); };
        auto end() { return std::end(Data); };

        #if defined (_WIN32)
        explicit MMap_t(const std::string &Path)
        {
            FD = _open(Path.c_str(), 0x800, 0);
            if (FD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Ptr = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Ptr) { CloseHandle(Nativehandle); _close(FD); return; }

            MEMORY_BASIC_INFORMATION Info{};
            VirtualQuery(Ptr, &Info, sizeof(Info));

            Data = { (uint8_t *)Ptr, Info.RegionSize };
        }
        explicit MMap_t(const std::wstring &Path)
        {
            FD = _wopen(Path.c_str(), 0x800, 0);
            if (FD == -1) return;

            Nativehandle = CreateFileMappingA(HANDLE(_get_osfhandle(FD)), NULL, PAGE_READONLY, 0, 0, NULL);
            if (!Nativehandle) return;

            Ptr = MapViewOfFile(Nativehandle, FILE_MAP_READ, 0, 0, 0);
            if (!Ptr) { CloseHandle(Nativehandle); _close(FD); return; }

            MEMORY_BASIC_INFORMATION Info{};
            VirtualQuery(Ptr, &Info, sizeof(Info));

            Data = { (uint8_t *)Ptr, Info.RegionSize };
        }
        ~MMap_t()
        {
            if (Ptr) UnmapViewOfFile(Ptr);
            if (Nativehandle) CloseHandle(Nativehandle);
            if (FD > 0)_close(FD);
        }

        #else
        explicit MMap_t(const std::string &Path) : Data{}
        {
            FD = open(Path.c_str(), 0, 0);
            if (FD == -1) return {};

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

    // Read a full file into memory.
    template <Byte_t T = uint8_t> [[nodiscard]] inline std::basic_string<T> Readfile(std::string_view Path)
    {
        const auto Filemapping = MMap_t(std::string(Path));
        return { Filemapping.begin(), Filemapping.end() };
    }
    template <Byte_t T = uint8_t> [[nodiscard]] inline std::basic_string<T> Readfile(std::wstring_view Path)
    {
        const auto Filemapping = MMap_t(std::wstring(Path));
        return { Filemapping.begin(), Filemapping.end() };
    }

    // Overwrite a file.
    template <Simplestring_t T> inline bool Writefile(const std::string &Path, const T &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.c_str(), "wb");
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <Simplestring_t T> inline bool Writefile(const std::wstring &Path, const T &Buffer)
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
    template <Complexstring_t T> inline bool Writefile(const std::string &Path, const T &Buffer)
    {
        return Writefile(Path, Flatten(Buffer));
    }
    template <Complexstring_t T> inline bool Writefile(const std::wstring &Path, const T &Buffer)
    {
        return Writefile(Path, Flatten(Buffer));
    }

    // Append to a file.
    template <Simplestring_t T> inline bool Appendfile(const std::string &Path, const T &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.c_str(), "ab");
        if (!Filehandle) return false;

        const auto Result = std::fwrite(Buffer.data(), Buffer.size() * sizeof(typename T::value_type), 1, Filehandle);
        std::fclose(Filehandle);
        return Result == 1;
    }
    template <Simplestring_t T> inline bool Appendfile(const std::wstring &Path, const T &Buffer)
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
    template <Complexstring_t T> inline bool Appendfile(const std::string &Path, const T &Buffer)
    {
        return Appendfile(Path, Flatten(Buffer));
    }
    template <Complexstring_t T> inline bool Appendfile(const std::wstring &Path, const T &Buffer)
    {
        return Appendfile(Path, Flatten(Buffer));
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
