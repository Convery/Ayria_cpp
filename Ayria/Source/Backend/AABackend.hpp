/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-21
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Backend
{
    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)());

    // Interface with the client database, remember try-catch.
    sqlite::database Database();

    // Initialize the system.
    void Initialize();
}

namespace API
{
    // static std::string __cdecl Callback(JSON::Value_t &&Request);
    using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
    void addEndpoint(std::string_view Functionname, Callback_t Callback);

    // For internal use.
    const char *callEndpoint(std::string_view Functionname, JSON::Value_t &&Request);
    std::vector<std::string> listEndpoints();
}

namespace Networking
{
    struct Client_t
    {
        AccountID_t AccountID;
        uint32_t GameID, ModID;
        std::array<char8_t, 20> Username;
        std::array<uint8_t, 32> SigningkeyPublic;
        std::array<uint8_t, 32> EncryptionkeyPublic;

        // Internal properties.
        uint32_t InternalIP;
        uint32_t Lastmessage;
    };
    typedef void (__cdecl *Callback_t)(Client_t *Clientinfo, const char *Message, unsigned int Length);

    // Register handlers for the different packets.
    void Register(std::string_view Identifier, Callback_t Handler);
    void Register(uint32_t Identifier, Callback_t Handler);

    // Broadcast a message to the LAN clients.
    void Publish(std::string_view Identifier, std::string_view Payload, uint64_t TargetID = 0, bool Encrypt = false, bool Sign = false);
    void Publish(uint32_t Identifier, std::string_view Payload, uint64_t TargetID = 0, bool Encrypt = false, bool Sign = false);

    // Drop packets from bad clients on the floor.
    void Blockclient(uint32_t SessionID);

    // Service access to the LAN clients.
    namespace Clientinfo
    {
        const Client_t *byAccount(AccountID_t AccountID);
        const Client_t *bySession(uint32_t SessionID);
    }

    // Initialize the system.
    void Initialize();

    // Send a request and return the async response.
    using Response_t = struct { uint32_t Statuscode; std::string Body; };
    std::future<Response_t> POST(std::string URL, std::string Data);
    std::future<Response_t> GET(std::string URL);
}

namespace Plugins
{
    // Different types of hooking.
    bool InstallTLSHook();
    bool InstallEPHook();

    // Simply load all plugins from disk.
    void Initialize();
}
