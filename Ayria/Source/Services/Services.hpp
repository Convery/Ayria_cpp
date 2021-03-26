/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Services
{
    union GroupID_t
    {
        uint64_t Raw;
        struct
        {
            uint32_t AdminID;   // Creator.
            uint16_t RoomID;    // Random.
            uint8_t Limit;      // Users.
            uint8_t Type;       // Server, localhost, or clan.
        };
    };

    namespace Clientinfo
    {
        // Helper functionallity.
        std::string_view getHardwareID();
        uint32_t getClientID(uint32_t NodeID);
        std::string getSharedkey(uint32_t ClientID);

        // Set up the service.
        void Initialize();
    }

    namespace Matchmakingsessions
    {
        // Set up the service.
        void Initialize();
    }

    namespace Relationships
    {
        // Helper functionallity.
        bool isBlocked(uint32_t SourceID, uint32_t TargetID);
        bool isFriend(uint32_t SourceID, uint32_t TargetID);

        // Set up the service.
        void Initialize();
    }

    namespace Usergroups
    {
        // Helper functionallity.
        bool isMember(uint64_t GroupID, uint32_t ClientID);

        // Set up the service.
        void Initialize();
    }

    namespace Messaging
    {
        // Set up the service.
        void Initialize();
    }

    // Set up the services.
    inline void Initialize()
    {
        Messaging::Initialize();
        Clientinfo::Initialize();
        Usergroups::Initialize();
        Relationships::Initialize();
        Matchmakingsessions::Initialize();
    }
}
