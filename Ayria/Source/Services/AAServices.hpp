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
        // The strings are cached internally.
        struct Client_t
        {
            std::u8string_view Username;
            std::string_view ClientID;
            uint32_t GameID, ModID;
            bool isIngame, isAway;
            int64_t Lastseen;

            // Internal.
            int64_t Lastupdated;
            uint64_t getShortID() const { return Hash::WW64(ClientID); }
            std::string getLongID() const { return std::string(ClientID); }
        };

        // Format between C++ and JSON representations.
        std::optional<Client_t> fromJSON(const JSON::Value_t &Object);
        std::optional<Client_t> fromJSON(std::string_view JSON);
        JSON::Object_t toJSON(const Client_t &Client);

        // Fetch the client by ID, for use with services.
        std::shared_ptr<Client_t> getClient(const std::string &LongID);

        // Internal helper to trigger an update when the global state changes.
        void triggerUpdate();

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Clientrelations
    {
        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Groups
    {
        struct Group_t
        {
            std::u8string_view Groupname;
            std::string_view GroupID;
            uint32_t Membercount;
            uint32_t Maxmembers;
            bool isPublic;

            // Internal.
            int64_t Lastupdated;

            inline bool isFull() const
            {
                return Membercount == Maxmembers;
            }
        };

        // Format between C++ and JSON representations.
        std::optional<Group_t> fromJSON(const JSON::Value_t &Object);
        std::optional<Group_t> fromJSON(std::string_view JSON);
        JSON::Object_t toJSON(const Group_t &Group);

        // Fetch the group by ID, for use with services.
        std::shared_ptr<Group_t> getGroup(const std::string &LongID);

        // Internal.
        void addMember(const std::string &GroupID, const std::string &MemberID);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Keyvalues
    {
        // Helper for other services to set presence.
        void setPresence(const std::string &Key, const std::optional<std::string> &Value);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Messaging
    {
        // Internal access for the services.
        void sendGroupmessage(const std::string &GroupID, uint32_t Messagetype, std::string_view Message);
        void sendClientmessage(const std::string &ClientID, uint32_t Messagetype, std::string_view Message);
        void sendMulticlientmessage(const std::unordered_set<std::string> &ClientIDs, uint32_t Messagetype, std::string_view Message);

        // Add the handlers and tasks.
        void Initialize();
    }

    namespace Matchmaking
    {
        // Add the handlers and tasks.
        void Initialize();
    }

    // Set up all our services.
    inline void Initialize()
    {
        Groups::Initialize();
        Messaging::Initialize();
        Keyvalues::Initialize();
        Clientinfo::Initialize();
        Matchmaking::Initialize();
        Clientrelations::Initialize();
    }
}
