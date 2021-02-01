/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

struct Client_t
{
    uint32_t UserID;
    const std::string *Sharedkey;
    const std::u8string *Username;
    const std::u8string *Authticket;
};

// Helper for async tasks.
template<typename T> struct Task_t
{
    std::atomic_flag hasResult, isDone;
    T Result;
};

namespace Clientinfo
{
    // LAN clients are identified by their random ID.
    std::vector<Client_t *> Listclients(bool onlyLAN = true);
    Client_t *getLocalclient(uint32_t NodeID);

    // If another clients crypto-key is needed, we request it.
    void Requestcryptokeys(std::vector<uint32_t> UserIDs);

    // Primarily used for cryptography.
    std::string_view getHardwareID();

    //
    void Initializediscovery();
    void Initializecrypto();
    inline void Initialize()
    {
        Initializediscovery();
        Initializecrypto();
    }
}

namespace Social
{
    // View this as a simple friends-list for now.
    namespace Relations
    {
        using Relationflags_t = union { uint8_t Raw; struct { uint8_t isPending : 1, isFriend : 1, isBlocked : 1; }; };
        using Userrelation_t = struct { uint32_t UserID; std::u8string Username; Relationflags_t Flags; };

        // Modify the relations stored to disk.
        void Insert(uint32_t UserID, std::u8string_view Username, Relationflags_t Flags);
        std::vector<const Userrelation_t *> List();
        void Remove(uint32_t UserID);

        // Load the relations from disk.
        void Initialize();
    }

    // A collection of active users.
    namespace Group
    {
        union GroupID_t
        {
            uint64_t Raw;
            struct
            {
                uint32_t AdminID;   // Creator.
                uint16_t RoomID;    // Random.
                uint8_t HostID;     // Server, 0 == localhost.
                uint8_t Limit;      // Users.
            };
        };
        struct Group_t
        {
            GroupID_t GroupID;
            std::unique_ptr<Hashmap<uint32_t, JSON::Value_t>> Members;
        };
        struct Joinrequest_t
        {
            GroupID_t GroupID;
            uint32_t ClientID;
            JSON::Value_t Extradata;
        };

        // Create a new group as host, optionally request a remote host for it.
        const Task_t<GroupID_t> *Creategroup(bool Local, uint8_t Memberlimit);

        // Manage the group-members for groups this client is admin of, update inserts a new user.
        void updateMember(GroupID_t GroupID, uint32_t ClientID, JSON::Value_t Clientinfo);
        void removeMember(GroupID_t GroupID, uint32_t ClientID);

        // Manage group-membership.
        const Task_t<bool> *Subscribe(GroupID_t GroupID, JSON::Value_t Extradata);
        void Acceptrequest(Joinrequest_t *Request, JSON::Value_t Clientinfo);
        void Rejectrequest(Joinrequest_t *Request);
        void Unsubscribe(GroupID_t GroupID);
        Joinrequest_t *getJoinrequest();

        // List the groups we know of, optionally filtered by Admin or Member.
        std::vector<const Group_t *> List(const uint32_t *byOwner = {}, const uint32_t *byUser = {});
        bool isGroupmember(GroupID_t GroupID, uint32_t UserID);

        // Set up message-handlers.
        void Initialize();
    }

    // Client messages are stored locally and sent when target is online.
    namespace Messages
    {
        using Message_t = struct { uint32_t Timestamp; uint32_t Source, Target; uint64_t GroupID; std::string B64Message; };

        namespace Send
        {
            void toGroup(uint64_t GroupID, std::string_view B64Message);
            void toClient(uint32_t ClientID, std::string_view B64Message);
            void toClientencrypted(uint32_t ClientID, std::string_view B64Message);
        }

        // Read messages optionally filtered by group, sender, and/or timeframe.
        std::vector<const Message_t *> Read(const std::pair<uint32_t, uint32_t> *Timeframe = {},
                                            const uint64_t *GroupID = {},
                                            const uint32_t *Source = {});

        // Add the message-handlers and load pending messages from disk.
        void Initialize();
    }

    //
    inline void Initialize()
    {
        Group::Initialize();
        Messages::Initialize();
        Relations::Initialize();
    }
}
