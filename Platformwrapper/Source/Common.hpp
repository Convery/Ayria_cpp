/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
using Blob = std::basic_string<uint8_t>;
using Blob_view = std::basic_string_view<uint8_t>;

// Helper to create 'classes' when needed.
using Fakeclass_t = struct { void *VTABLE[70]; };

// Ayria exports.
struct Ayriamodule_t
{
    void *Modulehandle;

    // Console exports.
    void(__cdecl *addConsolestring)(const char *String, int Color);
    void(__cdecl *addFunction)(const char *Name, const void *Callback);

    // Network exports.
    unsigned short(__cdecl *getNetworkport)();
    void(__cdecl *addNetworklistener)(const char *Subject, void *Callback);
    void(__cdecl *addNetworkbroadcast)(const char *Subject, const char *Message);
};

// Keep the global state together.
#pragma pack(push, 1)
struct Globalstate_t
{
    // Ayria helper.
    Ayriamodule_t Ayria;

    // Core info.
    uint64_t UserID;
    std::string Username;
    uint64_t Startuptimestamp;

    // Steam extensions.
    std::string Path;
    std::string Language;
    uint32_t ApplicationID;
};
extern Globalstate_t Global;
#pragma pack(pop)

// Broadcast events over Ayrias communications layer.
namespace Communication
{
    using Eventcallback_t = std::function<void(uint64_t TargetID, uint64_t SenderID, std::string_view Sendername, Bytebuffer &&Data)>;
    void Broadcastmessage(std::string_view Eventtype, uint64_t TargetID, Bytebuffer &&Data);
    void addMessagecallback(std::string_view Eventtype, Eventcallback_t Callback);
    void __cdecl Messagehandler(const char *Request);
}

// Manage friends and such.
namespace Social
{
    struct Friend_t : ISerializable
    {
        Blob Avatar;
        uint64_t UserID;
        std::string Username;
        enum : uint8_t { Online = 1, Offline = 2, Busy = 3, } Status;

        void Serialize(Bytebuffer &Buffer) override
        {
            Buffer.Write(Status);
            Buffer.Write(UserID);
            Buffer.Write(Avatar);
            Buffer.Write(Username);
        }
        void Deserialize(Bytebuffer &Buffer) override
        {
            Buffer.Read(Status);
            Buffer.Read(UserID);
            Buffer.Read(Avatar);
            Buffer.Read(Username);
        }
    };

    std::vector<Friend_t> getFriends();
    void Announceupdate(Friend_t Delta);
}
