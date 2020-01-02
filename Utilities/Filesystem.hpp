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

namespace FS
{
    [[nodiscard]] inline std::basic_string<uint8_t> Readfile(const std::string_view Path)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        if (!Filehandle) return {};

        std::fseek(Filehandle, 0, SEEK_END);
        const auto Length = std::ftell(Filehandle);
        std::fseek(Filehandle, 0, SEEK_SET);

        const auto Buffer = std::make_unique<uint8_t[]>(Length);
        std::fread(Buffer.get(), Length, 1, Filehandle);
        std::fclose(Filehandle);

        return std::basic_string<uint8_t>(Buffer.get(), Length);
    }
    inline bool Writefile(const std::string_view Path, const std::basic_string<uint8_t> &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(const std::string_view Path, const std::basic_string_view<uint8_t> Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(const std::string_view Path, const std::string_view Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(const std::string_view Path, const std::string &&Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.c_str(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    [[nodiscard]] inline bool Fileexists(const std::string_view Path)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        if (!Filehandle) return false;
        std::fclose(Filehandle);
        return true;
    }
    [[nodiscard]] inline size_t Filesize(const std::string_view Path)
    {
        size_t Filesize{};

        if (const auto Filehandle = std::fopen(Path.data(), "rb"))
        {
            std::fseek(Filehandle, 0, SEEK_END);
            Filesize = std::ftell(Filehandle);
            std::fclose(Filehandle);
        }

        return Filesize;
    }

    // File stat.
    using Stat_t = struct { uint32_t Created, Modified, Accessed; };

    // Windows.
    #if defined(_WIN32)
    #include <Windows.h>
    [[nodiscard]] inline Stat_t Filestats(const std::string_view Path)
    {
        Stat_t Result{};
        if (const auto Filehandle = CreateFileA(Path.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr))
        {
            uint64_t Created, Accessed, Modified;
            GetFileTime(Filehandle, (FILETIME *)&Created, (FILETIME *)&Accessed, (FILETIME *)&Modified);
            Result.Modified = uint32_t(Modified / 10000000 - 11644473600);
            Result.Accessed = uint32_t(Accessed / 10000000 - 11644473600);
            Result.Created = uint32_t(Created / 10000000 - 11644473600);
            CloseHandle(Filehandle);
        }

        return Result;
    }
    [[nodiscard]] inline std::vector<std::string> Findfiles(std::string Searchpath, const std::string_view Criteria)
    {
        std::vector<std::string> Results{};
        WIN32_FIND_DATAA Filedata;

        // Ensure that we have the backslash if the user forgot.
        if (Searchpath.back() != '/') Searchpath.append("/");

        // Initial query, fails if the search-path is broken.
        HANDLE Filehandle = FindFirstFileA((Searchpath + "*").c_str(), &Filedata);
        if (Filehandle == INVALID_HANDLE_VALUE) { FindClose(Filehandle); return Results; }

        do
        {
            // Respect hidden files and folders.
            if (Filedata.cFileName[0] == '.') continue;

            // Skip sub-directories.
            if (Filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

            // Only add the file to the list if matching the criteria.
            if (std::strstr(Filedata.cFileName, Criteria.data()))
                Results.push_back(Filedata.cFileName);

        } while (FindNextFileA(Filehandle, &Filedata));

        FindClose(Filehandle);
        return Results;
    }
    [[nodiscard]] inline std::vector<std::pair<std::string, std::string>> Findfilesrecursive(std::string Searchpath, const std::string_view Criteria)
    {
        std::vector<std::pair<std::string, std::string>> Results{};
        WIN32_FIND_DATAA Filedata;

        // Ensure that we have the backslash if the user forgot.
        if (Searchpath.back() != '/') Searchpath.append("/");

        // Initial query, fails if the search-path is broken.
        HANDLE Filehandle = FindFirstFileA((Searchpath + "*").c_str(), &Filedata);
        if (Filehandle == INVALID_HANDLE_VALUE) { FindClose(Filehandle); return Results; }

        do
        {
            // Respect hidden files and folders.
            if (Filedata.cFileName[0] == '.') continue;

            // Recurse into the directories.
            if (Filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                const auto Local = Findfilesrecursive(Searchpath + Filedata.cFileName, Criteria);
                Results.insert(Results.end(), Local.begin(), Local.end());
                continue;
            }

            // Only add the file to the list if matching the criteria.
            if (std::strstr(Filedata.cFileName, Criteria.data()))
                Results.push_back({ Searchpath, Filedata.cFileName });

        } while (FindNextFileA(Filehandle, &Filedata));

        FindClose(Filehandle);
        return Results;
    }
    #endif

    // *nix.
    #if !defined(_WIN32)
    #include <sys/types.h>
    #include <dirent.h>
    [[nodiscard]] inline std::vector<std::pair<std::string, std::string>> Findfilesrecursive(std::string Searchpath, std::string_view Criteria)
    {
        // TODO(tcn): Just port the NT version.
        assert(false);
        return {};
    }
    [[nodiscard]] inline std::vector<std::string> Findfiles(std::string Searchpath, std::string_view Extension)
    {
        std::vector<std::string> Filenames{};
        struct stat Fileinfo;
        dirent *Filedata;
        DIR *Filehandle;

        // Iterate through the directory.
        Filehandle = opendir(Searchpath.c_str());
        while ((Filedata = readdir(Filehandle)))
        {
            // Respect hidden files and folders.
            if (Filedata->d_name[0] == '.')
                continue;

            // Get extended fileinfo.
            std::string Filepath = Searchpath + "/" + Filedata->d_name;
            if (stat(Filepath.c_str(), &Fileinfo) == -1) continue;

            // Add the file to the list.
            if (!(Fileinfo.st_mode & S_IFDIR))
                if (!Extension.size())
                    Filenames.push_back(Filedata->d_name);
                else
                    if (std::strstr(Filedata->d_name, Extension.data()))
                        Filenames.push_back(Filedata->d_name);
        }
        closedir(Filehandle);

        return std::move(Filenames);
    }
    [[nodiscard]] inline Stat_t Filestats(std::string_view Path)
    {
        assert(false);
        return {};
    }
    #endif
}
