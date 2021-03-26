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
    // Get the client-database.
    sqlite::database Database();

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t Period, void(__cdecl *Callback)());

    // Initialize the system.
    void Initialize();

    // JSON Interface.
    namespace API
    {
        // static std::string __cdecl Callback(JSON::Value_t &&Request);
        using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
        void addEndpoint(std::string_view Functionname, Callback_t Callback, std::string_view Usagestring = {});

        // For internal use.
        const char *callEndpoint(std::string_view Functionname, JSON::Value_t &&Request);
        std::vector<JSON::Value_t> listEndpoints();
    }

    // Network interface.
    namespace Network
    {
        // static void __cdecl Callback(unsigned int NodeID, const char *Message, unsigned int Length);
        using Callback_t = void(__cdecl *)(unsigned int NodeID, const char *Message, unsigned int Length);
        void Registerhandler(std::string_view Identifier, Callback_t Handler);

        // Formats and LZ compresses the message.
        void Transmitmessage(std::string_view Identifier, JSON::Value_t &&Message);

        // Resolve a LAN nodes address.
        IN_ADDR Nodeaddress(uint32_t NodeID);

        // Prevent packets from being processed.
        void Blockclient(uint32_t NodeID);

        // Initialize the system.
        void Initialize();
    }
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
