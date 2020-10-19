/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once
#include <string_view>
#include <vector>
#include <cstdio>
#include <memory>
#include <string>

using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// Helpers to convert the blobs.
inline Blob S2B(const std::string &Input) { return Blob(Input.begin(), Input.end()); }
inline Blob S2B(const std::string &&Input) { return Blob(Input.begin(), Input.end()); }
inline std::string B2S(const Blob &Input) { return std::string(Input.begin(), Input.end()); }
inline std::string B2S(const Blob &&Input) { return std::string(Input.begin(), Input.end()); }

namespace FS
{
    template <typename T = uint8_t, typename = std::enable_if_t<sizeof(T) == 1, T>>
    [[nodiscard]] inline std::basic_string<T> Readfile(std::string_view Path)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        if (!Filehandle) return {};

        std::fseek(Filehandle, 0, SEEK_END);
        const auto Length = std::ftell(Filehandle);
        std::fseek(Filehandle, 0, SEEK_SET);

        const auto Buffer = std::make_unique<uint8_t[]>(Length);
        std::fread(Buffer.get(), Length, 1, Filehandle);
        std::fclose(Filehandle);

        return std::basic_string<T>(Buffer.get(), Length);
    }
    template <typename T = uint8_t, typename = std::enable_if_t<sizeof(T) == 1, T>>
    [[nodiscard]] inline std::basic_string<T> Readfile(std::wstring_view Path)
    {
        #if defined(_WIN32)
        std::FILE *Filehandle = _wfopen(Path.data(), L"rb");
        #else
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        #endif
        if (!Filehandle) return {};

        std::fseek(Filehandle, 0, SEEK_END);
        const auto Length = std::ftell(Filehandle);
        std::fseek(Filehandle, 0, SEEK_SET);

        const auto Buffer = std::make_unique<uint8_t[]>(Length);
        std::fread(Buffer.get(), Length, 1, Filehandle);
        std::fclose(Filehandle);

        return std::basic_string<T>(Buffer.get(), Length);
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
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        #endif
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size() * sizeof(T), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    template<typename T>
    inline bool Writefile(std::wstring_view Path, const std::basic_string<T> &Buffer)
    { return Writefile(Path, std::basic_string_view<T>(Buffer)); }

    [[nodiscard]] inline bool Fileexists(std::string_view Path)
    {
        return std::filesystem::exists(Path);
    }
    [[nodiscard]] inline bool Fileexists(std::wstring_view Path)
    {
        return std::filesystem::exists(Path);
    }

    [[nodiscard]] inline size_t Filesize(std::string_view Path)
    {
        std::error_code Code;
        const auto Size = std::filesystem::file_size(Path, Code);
        return Code.value() ? 0 : Size;
    }
    [[nodiscard]] inline size_t Filesize(std::wstring_view Path)
    {
        std::error_code Code;
        const auto Size = std::filesystem::file_size(Path, Code);
        return Code.value() ? 0 : Size;
    }

    // Directory iteration, filename if not recursive; else full path.
    [[nodiscard]] inline std::vector<std::string> Findfiles(std::string Directorypath, std::string_view Criteria, bool Recursive = false)
    {
        std::vector<std::string> Result;

        if (!Recursive)
        {
            for (const auto &File : std::filesystem::directory_iterator(Directorypath))
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().string();
                if (std::strstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(Filename);
            }
        }
        else
        {
            for (const auto &File : std::filesystem::recursive_directory_iterator(Directorypath))
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
            for (const auto &File : std::filesystem::directory_iterator(Directorypath))
            {
                if (File.is_directory()) continue;

                const auto Filename = File.path().filename().wstring();
                if (std::wcsstr(Filename.c_str(), Criteria.data()))
                    Result.emplace_back(Filename);
            }
        }
        else
        {
            for (const auto &File : std::filesystem::recursive_directory_iterator(Directorypath))
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
        if (stat(Path.data(), &Buffer) == -1) return {};
        return { (uint32_t)Buffer.st_ctime, (uint32_t)Buffer.st_mtime, (uint32_t)Buffer.st_atime };
        #endif
    }
}
