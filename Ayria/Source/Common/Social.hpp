/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Social
{
    using Relationflags_t = union
    {
        uint8_t Raw;
        struct
        {
            uint8_t
                isFriend : 1,
                isBlocked : 1,
                isFollower : 1,
                isGroupmember : 1,
                AYA_RESERVED1 : 1,
                AYA_RESERVED2 : 1,
                AYA_RESERVED3 : 1,
                AYA_RESERVED4 : 1;
        };
    };
    using Message_t = struct { uint32_t Timestamp; AyriaID_t Source, Target; std::u8string Message; };
    using Userrelation_t = struct { uint32_t UserID; std::u8string Username; Relationflags_t Flags; };

    // View it as a simple friends-list.
    namespace Relationships
    {
        void Remove(uint32_t UserID, std::u8string_view Username);
        std::vector<const Userrelation_t *> All();
        void Add(Userrelation_t &Addition);

        // Load the relations from disk.
        void Initialize();
    }

    // Uses the matchmake port for communication.
    namespace Groups
    {
        // Local groups just reuse the users ID as group ID, global gets a unique ID.
        AyriaID_t Createglobal(uint8_t Memberlimit = 0xFF);
        void Createlocal(uint8_t Memberlimit = 0xFF);
        void Destroygroup();

        // Async, check getSubscriptions later.
        bool Subscribe(AyriaID_t GroupID);
        bool Unsubscribe(AyriaID_t GroupID);
        std::vector<AyriaID_t> getSubscriptions();
        std::vector<AyriaID_t> getMembers(AyriaID_t GroupID);

        // Add the message-handlers.
        void Initialize();
    }

    // Messages to offline clients are stored locally (for now) and sent later.
    namespace Messages
    {
        namespace Read
        {
            std::vector<const Message_t *> All();
            std::vector<const Message_t *> New(uint32_t Lasttime);
            std::vector<const Message_t *> Filtered(std::optional<std::pair<uint32_t, uint32_t>> Timeframe,
                                                    std::optional<std::vector<uint32_t>> Source,
                                                    std::optional<AyriaID_t> GroupID);
        }
        namespace Send
        {
            void toGroup(uint32_t GroupID, std::u8string_view Message);
            void toClient(uint32_t ClientID, std::u8string_view Message);
            void toClientencrypted(uint32_t ClientID, std::u8string_view Message);
        }

        // Add the message-handlers and load messages from disk.
        void Initialize();
    }

    inline void API_Initialize()
    {
        //API::Registerhandler_Social("blockUser", blockUser);
        //API::Registerhandler_Social("addRelation", addRelation);
        //API::Registerhandler_Social("Friendslist", Friendslist);
        //API::Registerhandler_Social("removeRelation", removeRelation);

        //API::Registerhandler_Social("Sendmessage", Sendmessage);
        //API::Registerhandler_Social("Readmessages", Readmessages);
        //API::Registerhandler_Social("Sendmessage_enc", Sendmessage_enc);

        //Messaging::Initialize();
        //Relations::Load();
    }
}
