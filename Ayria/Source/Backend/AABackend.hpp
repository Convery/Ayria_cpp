/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-21
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Networking
{
    typedef union { uint64_t Raw; struct { uint32_t SessionID, AccountID; }; } Nodeinfo_t;
    typedef void (__cdecl *Callback_t)(Nodeinfo_t Nodeinfo, const char *Message, unsigned int Length);

    // Register handlers for the different packets.
    void Register(std::string_view Identifier, Callback_t Handler);
    void Register(uint32_t Identifier, Callback_t Handler);

    // Broadcast a message to the LAN clients.
    void Publish(std::string_view Identifier, std::string_view Payload, bool isTransient);
    void Publish(uint32_t Identifier, std::string_view Payload, bool isTransient);

    // Drop packets from bad clients on the floor.
    void Blockclient(uint32_t SessionID);

    // Incase a plugin needs to know the address in the future.
    IN_ADDR getPeeraddress(uint32_t SessionID);

    // Initialize the system.
    void Initialize();

    // Send a request and return the async response.
    using Response_t = struct { uint32_t Statuscode; std::string Body; };
    std::future<Response_t> POST(std::string URL, std::string Data);
    std::future<Response_t> GET(std::string URL);
}
