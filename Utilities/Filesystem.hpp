/*
    Initial author: Convery (tcn@ayria.se)
    Started: 10-04-2019
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
    inline std::basic_string<uint8_t> Readfile(std::string_view Path)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        if (!Filehandle) return {};

        std::fseek(Filehandle, 0, SEEK_END);
        auto Length = std::ftell(Filehandle);
        std::fseek(Filehandle, 0, SEEK_SET);

        auto Buffer = std::make_unique<uint8_t[]>(Length);
        std::fread(Buffer.get(), Length, 1, Filehandle);
        std::fclose(Filehandle);

        return std::basic_string<uint8_t>(std::move(Buffer.get()), Length);
    }
    inline bool Writefile(std::string_view Path, const std::basic_string<uint8_t> &Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(std::string_view Path, std::basic_string_view<uint8_t> Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(std::string_view Path, std::string_view Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.data(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Writefile(std::string_view Path, std::string &&Buffer)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "wb");
        if (!Filehandle) return false;

        std::fwrite(Buffer.c_str(), Buffer.size(), 1, Filehandle);
        std::fclose(Filehandle);
        return true;
    }
    inline bool Fileexists(std::string_view Path)
    {
        std::FILE *Filehandle = std::fopen(Path.data(), "rb");
        if (!Filehandle) return false;
        std::fclose(Filehandle);
        return true;
    }

    // File stat.
    using Stat_t = struct { uint64_t Created, Modified, Accessed; };


    // Windows.
    #if defined(_WIN32)
    #include <Windows.h>
    inline std::vector<std::string> Findfiles(std::string Searchpath, std::string_view Extension)
    {
        std::vector<std::string> Filenames{};
        WIN32_FIND_DATAA Filedata;
        HANDLE Filehandle;

        // Append trailing slash, asterisk and extension.
        if (Searchpath.back() != '/') Searchpath.append("/");
        Searchpath.append("*");
        if (Extension.size()) Searchpath.append(Extension);

        // Find the first plugin.
        Filehandle = FindFirstFileA(Searchpath.c_str(), &Filedata);
        if (Filehandle == (void *)INVALID_HANDLE_VALUE)
        {
            FindClose(Filehandle);
            return std::move(Filenames);
        }

        do
        {
            // Respect hidden files and folders.
            if (Filedata.cFileName[0] == '.')
                continue;

            // Add the file to the list.
            if (!(Filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                Filenames.push_back(Filedata.cFileName);

        } while (FindNextFileA(Filehandle, &Filedata));

        FindClose(Filehandle);
        return std::move(Filenames);
    }
    inline Stat_t Filestats(std::string_view Path)
    {
        Stat_t Result{};
        if (auto Filehandle = CreateFileA(Path.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL))
        {
            GetFileTime(Filehandle, (FILETIME *)&Result.Created, (FILETIME *)&Result.Accessed, (FILETIME *)&Result.Modified);
            CloseHandle(Filehandle);
        }

        return Result;
    }

    #endif

    // *nix.
    #if !defined(_WIN32)
    #include <sys/types.h>
    #include <dirent.h>
    inline std::vector<std::string> Findfiles(std::string Searchpath, std::string_view Extension)
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
    inline Stat_t Filestats(std::string_view Path)
    {
        assert(false);
        return {};
    }
    #endif
}
