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
    Hashset<int64_t> getModified(const std::string &Tablename);
    sqlite::database Database();

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
    // General N<->N networking, big endian.
    void Connectuser(uint32_t IPv4, uint16_t Port);
    void Publish(std::string_view Identifier, std::string_view Payload);

    // Set up the networking and connect to others.
    void Initialize(bool doLANDiscovery = true);
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
    using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
    // static std::string __cdecl Callback(JSON::Value_t &&Request);

    // Listen for requests to this functionname.
    void addEndpoint(std::string_view Functionname, Callback_t Callback);
}
namespace Layer3 = Backend::JSONAPI;

// Layer 4 - Check database changes and notify the user.
namespace Backend::Notifications
{
    // static void __cdecl Callback(const char *JSONString);
    using Callback_t = void(__cdecl *)(const char *JSONString);
    void Subscribe(std::string_view Identifier, Callback_t Handler);
    void Publish(std::string_view Identifier, const char *JSONString);
    void Unsubscribe(std::string_view Identifier, Callback_t Handler);

    // Internal.
    // static void __cdecl Callback(int64_t RowID);
    using Processor_t = void(__cdecl *)(int64_t RowID);
    void addProcessor(std::string_view Tablename, Processor_t Callback);

    // Set up the system.
    void Initialize();
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

namespace Networking34
{

    /*
        Layer1: Sync streamed table between users.
        Layer2: Parse and insert into the DB.
        Layer3: Parse updates and do notifications.

        APILayer: Insert

    */

    /*

        Basic idea:
        LAN clients connect to eachother over TCP to minimize data-loss.

        Server layer connects to eachother via DHT discovery.
        Server layer share all events between them.

        Clients can do DHT discovery to find a close server.
        onStartup => Check last message time -> Divide missing time by servercount -> request messages for timeslot.

        Server layer periodically prunes. e.g.
        MSG1: Client A sets username to X
        MSG2: Client A sets username to Y

        MSG1 can be pruned after a set time ( 48H? ).
        Should keep the dataset from growing too large.
        for (MSG in Processed) if ( Process(MSG) == DB.getEntry(MSG) ) Erase(MSG);

        Some tables can not be pruned, like messaging.
        So we can let servers delete them after some time, but Ayrias servers should save all..

        Servers can also prune banned accounts.

        Client_packet = { Sig, Payload }
        Server_packet = { PK, Sig, Payload }

    */

}



