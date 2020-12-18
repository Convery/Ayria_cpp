/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t Period, void(__cdecl *Callback)());

    // Helper for debug-builds.
    inline void setThreadname(std::string_view Name)
    {
        if constexpr (Build::isDebug)
        {
            #pragma pack(push, 8)
            using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
            #pragma pack(pop)

            __try
            {
                THREADNAME_INFO Info{ 0x1000, Name.data(), 0xFFFFFFFF };
                RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    // Initialize the system.
    void Initialize();
}

// Callbacks for the syncing multi-casts.
namespace Backend::Network
{
    // static void __cdecl Callback(unsigned int NodeID, unsigned int Length, const char *Message);
    using Callback_t = void(__cdecl *)(unsigned int NodeID, unsigned int Length, const char *Message);
    void Sendmessage(const char *Identifier, std::string_view JSONString);
    void addHandler(const char *Identifier, Callback_t Callback);

    // Prevent packets from being processed.
    void Blockclient(uint32_t NodeID);

    // Initialize the system.
    void Initialize();
}

// Callbacks for the APIs, JSON in, JSON out.
namespace Backend::API
{
    // static std::string __cdecl Callback(const char *JSONString);
    const char *callHandler(const char *Function, const char *JSONString);
    using Callback_t = std::string (__cdecl *)(const char *JSONString);
    void addHandler(const char *Function, Callback_t Callback);
}

// TCP connections for upload/download.
namespace Backend::Fileshare
{
    struct Fileshare_t
    {
        uint32_t Filesize, Checksum, OwnerID;
        const wchar_t *Filename;
        uint16_t Port;
    };

    std::future<Blob> Download(std::string_view Address, uint16_t Port, uint32_t Filesize);
    Fileshare_t *Upload(std::wstring_view Filepath);
    std::vector<Fileshare_t *> Listshares();

    // Initialize the system.
    void Initialize();
}
