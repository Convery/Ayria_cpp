/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    // Intercept update operations and process them later, might be some minor chance of a data-race.
    std::unordered_set<uint64_t> getDatabasechanges(const std::string &Tablename);

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)());

    // Initialize the system.
    void Initialize();

    // JSON Interface.
    namespace API
    {
        // static std::string __cdecl Callback(JSON::Value_t &&Request);
        using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
        void addEndpoint(std::string_view Functionname, Callback_t Callback);

        // For internal use.
        const char *callEndpoint(std::string_view Functionname, JSON::Value_t &&Request);
        std::vector<JSON::Value_t> listEndpoints();
    }















    // State synchronization.
    namespace Sync
    {
        // Share state changes with the other plugins.
        enum Eventtype_t
        {
            // V1
            GROUPCHAT_NEW, CLIENTCHAT_NEW,
            RELATIONSHIPS_NEW, RELATIONSHIPS_ONLINE,
            CLIENT_JOIN, CLIENT_LEAVE, CLIENT_UPDATE,
            GROUP_CREATE, GROUP_UPDATE, GROUP_TERMINATE,
            MATCHMAKING_CREATE, MATCHMAKING_UPDATE, MATCHMAKING_TERMINATE,
            CLIENTPRESENCE_NEW, CLIENTPRESENCE_UPDATE, CLIENTPRESENCE_DELETE,

            // V2
        };
        using Eventcallback_t = void(__cdecl *)(Eventtype_t Eventtype, std::string_view JSONData);
        void Subscribetoevent(Eventtype_t Eventtype, Eventcallback_t Callback);
        void Unsubscribe(Eventtype_t Eventtype, Eventcallback_t Callback);
        void Postevent(Eventtype_t Eventtype, std::string_view JSONData);

        // Internal state updates.
        void Insert(const std::string &Tablename, JSON::Object_t &&Keyvalues);
        void Erase(const std::string &Tablename, JSON::Object_t &&Keyvalues);
        void Query(const std::string &Tablename, uint32_t AccountID);
        void Dump(const std::string &Tablename, uint32_t AccountID);



        struct Activeclient_t
        {
            uint32_t Sharedkeyhash;
            uint32_t Lastupdated;
        };
        struct Clientinfo_t
        {
            uint32_t Sharedkeyhash;
            uint32_t AccountID;
            uint32_t PublicIP;
            uint32_t GameID;

            std::string Username;
            std::string Sharedkey;
        };
        struct Clientpresence_t
        {
            uint32_t Sharedkeyhash;
            std::string Key, Value;
        };
        struct Relationships_t
        {
            uint32_t SourceID, TargetID;
            bool isFriend, isBlocked;
        };



        #if 0

        enum Capabilities_t
        {
            LZ4Compression = 0x01,
            Relaysupport = 0x02,
        };



        /*
            Clientinfo
                // Identify local clients by hash of the key.
                // Shared key, 512 bit for LAN, 2048 when authed.

                Lastupdate
                ClientID
                PublicIP
                GameID
                Username
                Authticket
                Sharedkey

            Stats



        */

        /*
            Request:

            Auth:
            RSAKEY( SHA512(Email).SHA512(Password) )
            -> Send public key
            <- Get Username, ID, IP-address

            Verify:
            -> Send users PK
            <- Get Username, ID, IP-address

        */

        struct Update
        {
            // Table
            // Totalentries
            // Entriesupdated
            // Entries
        };
        struct Dump
        {
            // Table
            // Totalentries
            // Entries
        };

        // Clientupdate (Key, Value)
        // Presenceupdate (Key, Value)
        // Groupupdate (Key, Value)
        // Chatmessage (RoomID, TargetID, SourceID, Message)

        // Query (ClientID, Type, Key)


        #endif

    }
}

namespace Network
{
    namespace LAN
    {
        // static void __cdecl Callback(unsigned int AccountID, const char *Message, unsigned int Length);
        using Callback_t = void(__cdecl *)(unsigned int AccountID, const char *Message, unsigned int Length);

        // Handlers for the different packets.
        void Register(std::string_view Identifier, Callback_t Handler);
        void Register(uint32_t Identifier, Callback_t Handler);

        // Broadcast a message to the clients.
        void Publish(std::string_view Identifier, std::string_view Payload = std::string_view());
        void Publish(uint32_t Identifier, std::string_view Payload = std::string_view());

        // List all or block clients by their SessionID.
        const std::unordered_map<uint32_t, IN_ADDR> &getPeers();
        IN_ADDR getPeer(uint32_t SessionID);
        void Blockpeer(uint32_t SessionID);

        // Set up the network.
        void Initialize();
    }

    namespace WAN
    {
        // Set up the network.
        void Initialize();
    }

    // Set up the network.
    inline void Initialize()
    {
        LAN::Initialize();
        WAN::Initialize();
    }
}

// TCP connections for upload/download.
namespace Backend::Fileshare
{
    struct Fileshare_t
    {
        uint32_t Filesize, Checksum, OwnerID;
        const wchar_t *Filename;
        uint16_t Port;
    };

    std::future<Blob> Download(std::string_view Address, uint16_t Port, uint32_t Filesize);
    Fileshare_t *Upload(std::wstring_view Filepath);
    std::vector<Fileshare_t *> Listshares();

    // Initialize the system.
    void Initialize();
}
