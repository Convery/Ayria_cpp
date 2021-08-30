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

namespace Communication
{
    #pragma pack(push, 1)
    enum class Source_t : uint8_t  { LAN, ZMQ_CLIENT, ZMQ_SERVER };
    struct Payload_t { std::array<uint8_t, 32> toPublic; uint32_t Type; /* B85 Data here */ };
    struct Packet_t { std::array<uint8_t, 32> fromPublic; std::array<uint8_t, 64> Signature;  Payload_t Payload; };
    typedef void (__cdecl *Callback_t)(uint32_t AccountID /* WW32(Pubkey) */, const char *Message, unsigned int Length);
    #pragma pack(pop)

    // Register handlers for the different packets message WW32(ID).
    void registerHandler(std::string_view Identifier, Callback_t Handler, bool General);
    void registerHandler(uint32_t Identifier, Callback_t Handler, bool General);

    // Save a packet for later processing or forward to the handlers, internal.
    void forwardPacket(uint32_t Identifier, uint32_t AccountID, std::string_view Payload, bool General);
    void savePacket(const Packet_t *Header, std::string_view Payload, Source_t Source);

    // Access for services to list clients by source, internal.
    std::unordered_set<uint32_t> enumerateSource(Source_t Source);

    // Publish a message to a client, or broadcast if ID == 0.
    bool Publish(std::string_view Identifier, std::string_view Payload, uint32_t TargetID = 0);
    bool Publish(uint32_t Identifier, std::string_view Payload, uint32_t TargetID = 0);

    // Initialize the system.
    void Initialize();
}

namespace Networking
{
    // Source_t == LAN
    namespace LAN
    {
        // Publish a message to the local multicast group.
        bool Publish(std::unique_ptr<char []> &&Buffer, size_t Buffersize, size_t Overhead);

        // Initialize the system.
        void Initialize();
    }

    // Helper for HTTP(s) requests.
    namespace HTTP
    {
        // Send a request and return the async response.
        using Response_t = struct { uint32_t Statuscode; std::string Body; };
        std::future<Response_t> POST(std::string URL, std::string Data);
        std::future<Response_t> GET(std::string URL);
    }

    // Initialize the system.
    inline void Initialize()
    {
        LAN::Initialize();
    }
}


namespace Plugins
{
    // Different types of hooking.
    bool InstallTLSHook();
    bool InstallEPHook();

    // Simply load all plugins from disk.
    void Initialize();
}

namespace Console
{
    // UTF8 escaped ACII strings are passed to Argv for compatibility.
    using Functioncallback_t = void(__cdecl *)(int Argc, const char **Argv);
    using Logline_t = std::pair<std::wstring, Color_t>;

    // Useful for checking for new messages.
    extern uint32_t LastmessageID;

    // Threadsafe injection into and fetching from the global log.
    template <typename T> void addMessage(const std::basic_string<T> &Message, Color_t RGBColor);
    template <typename T> void addMessage(std::basic_string_view<T> Message, Color_t RGBColor);
    std::vector<Logline_t> getMessages(size_t Maxcount, std::wstring_view Filter);

    // Manage and execute the commandline, with optional logging.
    template <typename T> void addCommand(const std::basic_string<T> &Name, Functioncallback_t Callback);
    template <typename T> void addCommand(std::basic_string_view<T> Name, Functioncallback_t Callback);
    template <typename T> void execCommand(const std::basic_string<T> &Commandline, bool Log = true);
    template <typename T> void execCommand(std::basic_string_view<T> Commandline, bool Log = true);

    // Add common commands to the backend.
    void Initialize();
}

namespace Notifications
{
    using Callback_t = void(__cdecl *)(const char *JSONString);
    void Register(std::string_view Identifier, Callback_t Handler);
    void Publish(std::string_view Identifier, const char *JSONString);
}
