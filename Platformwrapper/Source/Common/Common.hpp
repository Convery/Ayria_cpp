/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
using Blob = std::basic_string<uint8_t>;

// LAN communication subsystem.
namespace Communication
{
    struct Node_t
    {
        uint32_t Address;
        uint32_t Timestamp;
        uint64_t SessionID;
    };

    // For internal use, get our Node_t.
    std::shared_ptr<Node_t> getLocalhost();

    // Callbacks on events by name.
    using Eventcallback_t = std::function<void(std::shared_ptr<Node_t> Sender, std::string &&Payload)>;
    void addEventhandler(const std::string &Event, Eventcallback_t Callback);

    // Push a message to all LAN clients.
    void Broadcast(const std::string Eventname, std::string &&Payload);

    // Spawn a thread that polls for packets in the background.
    void Initialize(const uint16_t Port);
}

// Extend the communication-core.
namespace Matchmaking
{
    struct Server_t
    {
        nlohmann::json Session{};
        std::shared_ptr<Communication::Node_t> Core;
        template<typename T> void Set(std::string &&Property, T Value) { Session[Property] = Value; }
        template<typename T> T Get(std::string &&Property, T Defaultvalue) { return Session.value(Property, Defaultvalue); }
    };

    // A list of all currently active servers on the local net.
    std::vector<std::shared_ptr<Server_t>> Externalservers();
    std::shared_ptr<Server_t> Localserver();

    // Notify the public about our node being dirty.
    void Broadcast();

    // Callbacks on events by name.
    using Eventcallback_t = std::function<void(std::shared_ptr<Server_t> Sender, std::string &&Payload)>;
    void addEventhandler(const std::string &Event, Eventcallback_t Callback);
}
namespace Social
{
    struct Social_t
    {
        uint64_t UserID;
        nlohmann::json Richpresence{};
        std::shared_ptr<Communication::Node_t> Core;
        template<typename T> void Set(std::string &&Property, T Value) { Richpresence[Property] = Value; }
        template<typename T> T Get(std::string &&Property, T Defaultvalue) { return Richpresence.value(Property, Defaultvalue); }
    };

    // Callbacks on events by name.
    using Eventcallback_t = std::function<void(std::shared_ptr<Social_t> Sender, std::string && Payload)>;
    void addEventhandler(const std::string &Event, Eventcallback_t Callback);
}

namespace Ayria
{
    // Keep the global state together.
    #pragma pack(push, 1)
    struct Globalstate_t
    {
        uint64_t UserID;
        std::string Username;
        uint64_t Startuptimestamp;
    };
    extern Globalstate_t Global;
    #pragma pack(pop)

    // Helper to create 'classes' when needed.
    using Fakeclass_t = struct { void *VTABLE[70]; };
}
