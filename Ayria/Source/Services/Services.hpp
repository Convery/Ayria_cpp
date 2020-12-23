/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

struct Client_t
{
    union
    {
        uint64_t AccountID;
        struct
        {
            uint32_t UserID;
            uint8_t Stateflags;
            uint8_t Accountflags;
            uint16_t Creationdate;
        };
    };

    uint32_t NetworkID;
    const std::string *Sharedkey;
    const std::wstring *Username;
};

namespace Clientinfo
{
    // Primarily used for cryptography.
    std::string_view getHardwareID();

    // Clients are split into LAN and WAN, networkIDs are for LAN.
    Client_t *getClientbyID(uint32_t NetworkID);
    std::vector<Client_t *> getRemoteclients();
    std::vector<Client_t *> getLocalclients();

    // If another clients crypto-key is needed, we request it.
    void Requestcryptokeys(std::vector<uint32_t> UserIDs);

    // Load client-info from disk.
    void Initialize();
}

namespace Social
{
    // View this as a simple friends-list for now.
    namespace Relations
    {
        using Relationflags_t = union { uint8_t Raw; struct { uint8_t Pending : 1, isFriend : 1, isBlocked : 1; }; };
        using Userrelation_t = struct { uint32_t UserID; std::wstring Username; Relationflags_t Flags; };

        void Add(uint32_t UserID, std::wstring_view Username, Relationflags_t Flags);
        void Remove(uint32_t UserID, std::wstring_view Username);
        std::vector<const Userrelation_t *> List();

        // Load the relations from disk.
        void Initialize();
    }

    // A collection of active users.
    namespace Group
    {
        struct Group_t
        {
            uint64_t GroupID;
            uint32_t AdminID, Memberlimit;
            Inlinedvector<Client_t, 2> Members;
            Hashmap<uint32_t, JSON::Object_t> Memberdata;
        };

        // Create a new group as host.
        Group_t *Create(uint32_t Memberlimit);
        void Destroy(uint64_t GroupID);

        // Manage groups you host, pending requests as <GroupID, AccountID>.
        void addClient(uint64_t GroupID, Client_t &&Clientinfo);
        void removeClient(uint64_t GroupID, uint32_t ClientID);
        std::pair<uint64_t, uint64_t> getJoinrequest();

        // Manage membership, async so call list to get result.
        std::vector<const Group_t *> List(bool Subscribedonly);
        void Leave(uint64_t GroupID);
        void Join(uint64_t GroupID);

        // Set up message-handlers.
        void Initialize();
    }

    // Messages are stored locally (for now) and sent when target is online.
    namespace Messages
    {
        using Message_t = struct { uint32_t Timestamp; uint64_t Source, Target; std::string B64Message; };

        // TODO(tcn): Move to the message module.
        // Helpers for serialisation.
        JSON::Object_t toObject(const Message_t &Message);
        Message_t toMessage(const JSON::Value_t &Object);

        namespace Send
        {
            void toGroup(uint64_t GroupID, std::u8string_view Message);
            void toClient(uint32_t ClientID, std::u8string_view Message);
            void toClientencrypted(uint32_t ClientID, std::u8string_view Message);
        }

        // std::optional was being a pain on MSVC, so pointers it is.
        std::vector<const Message_t *> Read(const std::pair<uint32_t, uint32_t> *Timeframe,
                                            const std::vector<uint64_t> *Source,
                                            const uint64_t *GroupID);


        // Add the message-handlers and load pending messages from disk.
        void Initialize();
    }
}
