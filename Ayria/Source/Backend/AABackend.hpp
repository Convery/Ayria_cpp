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
    // Incase the struct is extended in the future.
    #pragma pack(push, 1)
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
    #pragma pack(pop)

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
        std::string toJSON(const Client_t *Client);
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
