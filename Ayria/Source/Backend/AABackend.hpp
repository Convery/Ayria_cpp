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

    // Interface with the client database.
    sqlite::Database_t Database();

    // Set the global cryptokey from various sources.
    void setCryptokey_CRED(std::string_view Cred1, std::string_view Cred2);
    void setCryptokey_TEMP();
    void setCryptokey_HWID();

    // Initialize the system.
    void Initialize();
}

// Layer 1 - Networking between clients.
namespace Backend::Messagebus
{
    // Share a message with the network.
    void Publish(std::string_view Identifier, std::string_view Payload);
    void Publish(std::string_view Identifier, JSON::Object_t &&Payload);
    void Publish(std::string_view Identifier, const JSON::Object_t &Payload);

    // Client-selected (trust, but optionally verify) routing-servers.
    void removeRouter(const sockaddr_in &Address);
    void addRouter(const sockaddr_in &Address);

    // Set up winsock.
    void Initialize();
}
namespace Layer1 = Backend::Messagebus;

// Layer 2 - Processing the messages into the DB.
namespace Backend::Messageprocessing
{
    typedef bool (__cdecl *Callback_t)(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length);
    // static bool __cdecl Handler(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length);

    // Listen for packets of a certain type.
    void addMessagehandler(std::string_view Identifier, Callback_t Handler);

    // Set up the system.
    void Initialize();
}
namespace Layer2 = Backend::Messageprocessing;

// Layer 3 - User-triggered calls to Layer 1 & 2.
namespace Backend::JSONAPI
{
    // static std::string __cdecl Callback(JSON::Value_t &&Request);
    using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);

    // Listen for requests to this functionname.
    void addEndpoint(std::string_view Functionname, Callback_t Callback);
}
namespace Layer3 = Backend::JSONAPI;

// Layer 4 - Check database changes and notify the user.
namespace Backend::Notifications
{
    // static void __cdecl Callback(const char *Message, unsigned int Length);
    using Callback_t = void(__cdecl *)(const char *Message, unsigned int Length);

    void Subscribe(std::string_view Identifier, Callback_t Handler);
    void Unsubscribe(std::string_view Identifier, Callback_t Handler);

    void Publish(std::string_view Identifier, std::string_view Payload);
    void Publish(std::string_view Identifier, JSON::Object_t &&Payload);
    void Publish(std::string_view Identifier, const JSON::Object_t &Payload);
}
namespace Layer4 = Backend::Notifications;

// User and plugin interaction with the backend.
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
    // UTF8 escaped ASCII strings are passed to argv for compatibility.
    using Functioncallback_t = void(__cdecl *)(int argc, const char **argv);
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

// Helper for HTTP(s) requests.
namespace Backend::HTTP
{
    // Send a request and return the async response.
    using Response_t = struct { uint32_t Statuscode; std::string Body; };
    std::future<Response_t> POST(std::string URL, std::string Data);
    std::future<Response_t> GET(std::string URL);
}
