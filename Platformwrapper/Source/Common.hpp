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

    // Client information.
    void(__cdecl *getClientID)(unsigned int *ClientID);
    void(__cdecl *getClientname)(const char **Clientname);
    void(__cdecl *getClientticket)(const char **Clientticket);

    // Console interaction, callback = void f(int argc, const char **argv);
    void(__cdecl *addConsolemessage)(const char *String, unsigned int Colour);
    void(__cdecl *addConsolefunction)(const char *Name, void *Callback);

    // Network interaction, callback = void f(const char *Content);
    void(__cdecl *addNetworklistener)(const char *Subject, void *Callback);
    void(__cdecl *addNetworkbroadcast)(const char *Subject, const char *Message);

    // Social interactions, returns JSON.
    void(__cdecl *getLocalplayers)(const char **Playerlist);
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
        uint32_t Lastmodified{};

        std::string Avatar;
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
    void Announcequit();
}

// Basic matchmaking.
namespace Matchmaking
{
    struct Server_t : ISerializable
    {
        uint32_t Lastmodified{};

        uint16_t Port;
        uint32_t Address;
        uint64_t ServerID;
        uint32_t Maxplayers;
        std::string Gametags;
        std::vector<uint64_t> Players;

        nlohmann::json Session{ nlohmann::json::object() };
        template<typename T> void Set(std::string &&Property, T Value) { Session[Property] = Value; }
        template<typename T> T Get(std::string &&Property, T Defaultvalue) { return Session.value(Property, Defaultvalue); }

        void Serialize(Bytebuffer &Buffer) override
        {
            Buffer.Write(Port);
            Buffer.Write(Address);
            Buffer.Write(ServerID);
            Buffer.Write(Maxplayers);
            Buffer.Write(Players);
            Buffer.Write(Gametags);
            Buffer.Write(Base64::Encode(Session.dump()));
        }
        void Deserialize(Bytebuffer &Buffer) override
        {
            Buffer.Read(Port);
            Buffer.Read(Address);
            Buffer.Read(ServerID);
            Buffer.Read(Maxplayers);
            Buffer.Read(Players);
            Buffer.Read(Gametags);

            try
            {
                const auto Sessioninfo = Base64::Decode(Buffer.Read<std::string>());
                Session = nlohmann::json::parse(Sessioninfo.c_str());
            } catch(...){}
        }
    };

    void Announceupdate(Server_t Delta = {});
    std::vector<Server_t> getServers();
    Server_t *Localserver();
    void Announcequit();
}
