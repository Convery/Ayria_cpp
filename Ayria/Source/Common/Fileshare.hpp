/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Fileshare
{
    // Notify the network about public files.
    void Sharefileinfo();

    // Request a filelist from other clients.
    void Requestfileinfo(std::vector<uint32_t> Clients, std::string_view Filter);

    // Add and remove files from the shared list, path is stripped to ./Ayria/*
    void addSharedfile(std::wstring_view Filepath, std::string_view JSONConfig);
    void removeSharedfile(std::wstring_view Filepath);

    // List the files we currently share.
    const std::vector<std::wstring> *getSharedfiles();

    namespace API
    {
        extern "C" EXPORT_ATTR const char *getLocalfiles();
        extern "C" EXPORT_ATTR const char *removeSharedfile(const wchar_t *Filepath);
        extern "C" EXPORT_ATTR const char *addSharedfile(const wchar_t *Filepath, const char *JSONConfig);
        extern "C" EXPORT_ATTR const char *getNetworkfiles(uint32_t Count, uint32_t *ClientIDs, const char *Filter);
    }
}

/*
    TODO(tcn):
    
    Payload_t { uint32_t FileID; uint16_t Fragmentcount; uint16_t FragmentID; **data** };
*/
