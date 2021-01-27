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

        void Insert(uint32_t UserID, std::u8string_view Username, Relationflags_t Flags);
        void Remove(uint32_t UserID, std::u8string_view Username);
        std::vector<const Userrelation_t *> List();

        // Load the relations from disk.
        void Initialize();
    }

    // A collection of active users.
    namespace Group
    {
        struct Group_t // 48 bytes.
        {
            //                       UserID                      User-data
            Inlinedvector<std::pair<uint32_t, std::unique_ptr<JSON::Object_t>>, 2> Members;

            union
            {
                uint64_t GroupID;
                struct
                {
                    uint32_t AdminID;   // Creator.
                    uint16_t RoomID;    // Random.
                    uint8_t HostID;     // Server, 0 == localhost.
                    uint8_t Limit;      // Users.
                };
            };
        };

        // Create a new group as host, request a remote host for it.
        Group_t *Createlocal(bool Public, uint8_t Memberlimit = 2);
        std::future<bool> Makeremote(Group_t *Localgroup);

        // Manage the group-members for groups this client is admin of.
        void insertClient(uint64_t GroupID, uint32_t ClientID, std::string_view JSONData = {});
        void modifyClient(uint64_t GroupID, uint32_t ClientID, std::string_view DeltaJSON);
        void removeClient(uint64_t GroupID, uint32_t ClientID);

        // Get/send a request in the form of <GroupID, ClientID>.
        std::future<bool> Requestjoin(uint64_t GroupID);
        std::pair<uint64_t, uint32_t> getJoinrequest();
        void Leavegroup(uint64_t GroupID);

        // List the groups we know of, optionally filtered by Admin or Member.
        std::vector<const Group_t *> List(const uint32_t *byOwner = {}, const uint32_t *byUser = {});

        // Set up message-handlers.
        void Initialize();
    }

    // Client messages are stored locally and sent when target is online.
    namespace Messages
    {
        using Message_t = struct { uint32_t Timestamp; uint32_t Source, Target; std::string B64Message; };

        // TODO(TCN): Move to the .cpp when implemented.
        Message_t Deserialize(const JSON::Object_t &Object);
        JSON::Object_t Serialize(const Message_t &Message);

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
}
