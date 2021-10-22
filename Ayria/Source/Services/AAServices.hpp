/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Services
{
    namespace Clientinfo
    {
        struct Client_t
        {
            uint32_t GameID, ModID, Flags;
            std::string Publickey;
            std::string Username;

            // Internal.
            uint64_t Timestamp;
            std::string getLongID() const { return Publickey; }
            uint64_t getShortID() const { return Hash::WW64(getLongID()); }
        };

        // Format as JSON so that other tools can read it.
        inline std::optional<Client_t> fromJSON(const JSON::Value_t &Object)
        {
            // The required fields, we wont do partial parsing.
            if (!Object.contains_all("Publickey", "Username", "GameID", "ModID")) [[unlikely]] return {};

            Client_t Client{};
            Client.Flags = Object.value<uint32_t>("Flags");
            Client.ModID = Object.value<uint32_t>("ModID");
            Client.GameID = Object.value<uint32_t>("GameID");
            Client.Username = Object.value<std::string>("Username");
            Client.Publickey = Object.value<std::string>("Publickey");

            return Client;
        }
        inline std::optional<Client_t> fromJSON(std::string_view JSON)
        {
            return fromJSON(JSON::Parse(JSON));
        }
        inline JSON::Object_t toJSON(const Client_t &Client)
        {
            return JSON::Object_t({
                { "Flags", Client.Flags },
                { "ModID", Client.ModID },
                { "GameID", Client.GameID },
                { "Username", Client.Username },
                { "Timestamp", Client.Timestamp },
                { "Publickey", Client.Publickey }
            });
        }

        // Fetch the client by ID, for use with services.
        std::shared_ptr<Client_t> getClient(const std::string &LongID);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Relations
    {
        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Presence
    {
        // Helper for other services to set presence.
        void setPresence(const std::string &Key, const std::string &Category, const std::optional<std::string> &Value);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Groups
    {
        struct Group_t
        {
            int32_t Membercount;
            std::string GroupID;
            std::string Groupname;
            bool isPublic, isFull;

            // Internal.
            uint64_t Timestamp;
        };

        // Format as JSON so that other tools can read it.
        inline std::optional<Group_t> fromJSON(const JSON::Value_t &Object)
        {
            // The required fields, we wont do partial parsing.
            if (!Object.contains_all("GroupID", "Groupname", "isPublic", "isFull")) [[unlikely]] return {};

            Group_t Group{};
            Group.isFull = Object.value<bool>("isFull");
            Group.isPublic = Object.value<bool>("isPublic");
            Group.GroupID = Object.value<std::string>("GroupID");
            Group.Groupname = Object.value<std::string>("Groupname");
            Group.Membercount = Object.value<uint32_t>("Membercount");

            return Group;
        }
        inline std::optional<Group_t> fromJSON(std::string_view JSON)
        {
            return fromJSON(JSON::Parse(JSON));
        }
        inline JSON::Object_t toJSON(const Group_t &Group)
        {
            return JSON::Object_t({
                { "isFull", Group.isFull },
                { "GroupID", Group.GroupID },
                { "isPublic", Group.isPublic },
                { "Groupname", Group.Groupname },
                { "Membercount", Group.Membercount }
            });
        }

        // Fetch the group by ID, for use with services.
        std::shared_ptr<Group_t> getGroup(const std::string &LongID);

        // Internal.
        void addMember(const std::string &GroupID, const std::string &MemberID);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Messaging
    {
        // Internal access for the services.
        void sendUsermessage(const std::string &UserID, std::string_view Messagetype, std::string_view Payload);
        void sendGroupmessage(const std::string &GroupID, std::string_view Messagetype, std::string_view Payload);
        void sendMultiusermessage(const std::unordered_set<std::string> &UserIDs, std::string_view Messagetype, std::string_view Payload);

        // Add the handlers and tasks.
        void Initialize();
    }

    // Set up all our services.
    inline void Initialize()
    {
        Clientinfo::Initialize();
        Messaging::Initialize();
        Relations::Initialize();
        Presence::Initialize();
        Groups::Initialize();
    }
}
