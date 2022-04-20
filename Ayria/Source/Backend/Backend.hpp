/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Core
{
    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)());

    // Terminate the background thread and prepare for exit.
    void Terminate();

    // Interface with the client database.
    sqlite::Database_t QueryDB();

    // Create a worker-thread in the background.
    void Initialize();
}

namespace Config
{
    // Load (or reload) the configuration file.
    void Initialize();

    // Re-configure the client's public key (ID).
    void setKey_RANDOM();
    void setKey_HWID();
}

namespace JSONAPI
{
    // static std::string __cdecl Callback(JSON::Value_t &&Request);
    using Callback_t = std::string(__cdecl *)(JSON::Value_t &&Request);

    // Listen for requests to this functionname.
    void addEndpoint(std::string_view Functionname, Callback_t Callback);
}

namespace Notifications
{
    // static void __cdecl CPPCallback(const JSON::Object_t &Object);
    using CPPCallback_t = void(__cdecl *)(const JSON::Object_t &Object);
    // static void __cdecl CCallback(const char *Message, unsigned int Length);
    using CCallback_t = void(__cdecl *)(const char *Message, unsigned int Length);

    // Duplicate subscriptions get ignored.
    void Subscribe(std::string_view Identifier, CCallback_t Handler);
    void Subscribe(std::string_view Identifier, CPPCallback_t Handler);
    void Unsubscribe(std::string_view Identifier, CCallback_t Handler);
    void Unsubscribe(std::string_view Identifier, CPPCallback_t Handler);

    // RowID is the row in Syncpackets that triggered the notification.
    void Publish(std::string_view Identifier, int64_t RowID, JSON::Value_t &&Payload);
    void Publish(std::string_view Identifier, int64_t RowID, std::string_view Payload);
}

namespace Synchronization
{
    // Servers provide (filtered by client ID) access to past packets.
    void addPublickeys(const std::vector<std::string_view> &ClientIDs);
    void addServer(const std::string &Hostname, uint16_t Port);
    void addPublickey(std::string_view ClientID);

    // static void __cdecl Callback(int64_t Timestamp, int64_t RowID, const std::u8string &SenderID, const JSON::Value_t &Payload)
    using Callback_t = void(__cdecl *)(int64_t Timestamp, int64_t RowID, const std::u8string &SenderID, const JSON::Value_t &Payload);

    // Listen for incoming messages.
    void addRoutinglistener(std::string_view Messagetype, Callback_t Callback);
    void addPluginlistener(std::string_view Messagetype, Callback_t Callback);
    void addSynclistener(std::string_view Tablename, Callback_t Callback);

    // Helpers for publishing certain packets..
    void sendPluginpacket(std::string_view Messagetype, std::string_view Pluginname, JSON::Object_t &&Payload);
    void sendSyncpacket(int Operation, std::string_view Tablename, const JSON::Array_t &Values);
    void sendRoutingpacket(std::string_view Messagetype, JSON::Object_t &&Payload);

    // Initialize networking and LAN discovery.
    void Initialize();
}

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
    using Functioncallback_t = void(__cdecl *)(int argc, const wchar_t **argv);
    using Logline_t = std::pair<std::wstring, Color_t>;

    // Useful for checking for new messages.
    extern uint32_t LastmessageID;

    // Threadsafe injection and fetching from the global log.
    void addMessage(Logline_t &&Message);
    std::vector<Logline_t> getMessages(size_t Maxcount = 256, std::wstring_view Filter = L"");
    template <typename T> inline void addMessage(std::basic_string_view<T> Message, Color_t RGBColor)
    {
        // Passthrough for T = wchar_t
        return addMessage({ Encoding::toUNICODE(Message), RGBColor });
    }

    // Manage and execute the commandline, with optional logging.
    void execCommand(std::wstring_view Commandline, bool Log = true);
    void addCommand(std::wstring_view Name, Functioncallback_t Callback);
    template <typename T> inline void execCommand(std::basic_string_view<T> Commandline, bool Log = true)
    {
        // Passthrough for T = wchar_t
        return execCommand(Encoding::toUNICODE(Commandline), Log);
    }
    template <typename T> inline void addCommand(std::basic_string_view<T> Name, Functioncallback_t Callback)
    {
        // Passthrough for T = wchar_t
        return addCommand(Encoding::toUNICODE(Name), Callback);
    }

    // Add common commands to the backend.
    void Initialize();
}

namespace Backend
{
    inline void Initialize()
    {
        // As of Windows 10 update 20H2 (v2004) we need to set the interrupt resolution for each process.
        #if defined (_WIN32)
        timeBeginPeriod(1);
        #endif

        // Initialize the core subsystems.
        Core::Initialize();
        Config::Initialize();

        // Initialize the independant systems.
        const auto A = std::async(std::launch::async, []() { Synchronization::Initialize(); });
        const auto B = std::async(std::launch::async, []() { Console::Initialize(); });

        // Wait for the initialization to finis.
        A.wait(); B.wait();
    }
}

