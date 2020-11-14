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
                isClanmember : 1,
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
        AyriaID_t Createlocal(uint8_t Memberlimit = 0xFF);
        void Destroygroup();

        // Async, check getSubscriptions later.
        bool Subscribe(AyriaID_t GroupID);
        bool Unsubscribe(AyriaID_t GroupID);
        std::vector<AyriaID_t> getSubscriptions();
        std::vector<AyriaID_t> getMembers(AyriaID_t GroupID);

        // Manage the local group.
        void addUser(AyriaID_t UserID);
        void removeUser(AyriaID_t UserID);
        const std::vector<AyriaID_t> *getRequests();

        // Add the message-handlers.
        void Initialize();
    }

    // Messages to offline clients are stored locally (for now) and sent later.
    namespace Messages
    {
        // Helpers.
        JSON::Object_t toObject(const Message_t &Message);
        Message_t toMessage(const JSON::Value_t &Object);

        // std::optional was being a pain on MSVC, so pointers it is.
        std::vector<const Message_t *> Read(const std::pair<uint32_t, uint32_t> *Timeframe,
            const std::vector<uint64_t> *Source,
            const uint64_t *GroupID);

        namespace Send
        {
            void toGroup(AyriaID_t GroupID, std::u8string_view Message);
            void toClient(AyriaID_t ClientID, std::u8string_view Message);
            void toClientencrypted(AyriaID_t ClientID, std::u8string_view Message);
        }

        // Add the message-handlers and load messages from disk.
        void Initialize();
    }

    // Exported JSON API endpoints.
    namespace API
    {
        inline std::string __cdecl getRelations(const char *)
        {
            const auto Relations = Relationships::All();
            JSON::Array_t Array; Array.reserve(Relations.size());

            for (const auto &Relation : Relations)
            {
                Array.push_back(JSON::Object_t({
                    { "UserID", Relation->UserID },
                    { "Username", Relation->Username},
                    { "isFriend", Relation->Flags.isFriend},
                    { "isBlocked", Relation->Flags.isBlocked },
                    { "isFollower", Relation->Flags.isFollower },
                    { "isClanmember", Relation->Flags.isClanmember }
                }));
            }

            return JSON::Dump(Array);
        }
        inline std::string __cdecl setRelation(const char *JSONString)
        {
            Userrelation_t Relation{};

            const auto Object = JSON::Parse(JSONString);
            Relation.UserID = Object.value("UserID", uint32_t());
            Relation.Username = Object.value("Username", std::u8string());
            Relation.Flags.isFriend = Object.value("isFriend", false);
            Relation.Flags.isBlocked = Object.value("isBlocked", false);
            Relation.Flags.isFollower = Object.value("isFollower", false);
            Relation.Flags.isClanmember = Object.value("isClanmember", false);

            if (Relation.UserID != 0)
            {
                if (!Object.value("Remove", false)) Relationships::Add(Relation);
                else Relationships::Remove(Relation.UserID, Relation.Username);
            }

            return "{}";
        }

        inline std::string __cdecl Sendmessage(const char *JSONString)
        {
            const auto Object = JSON::Parse(JSONString);
            const auto Encrypt = Object.value("Encrypt", false);
            const auto Target = Object.value("Target", uint64_t());
            const auto Message = Object.value("Message", std::u8string());

            if (Target != 0)
            {
                const auto AyriaID = AyriaID_t{ .Raw = Target };

                if (Accountflags_t{ .Raw = AyriaID.Accountflags }.isGroup)
                {
                    Messages::Send::toGroup(AyriaID, Message);
                }
                else
                {
                    if (!Encrypt) Messages::Send::toClient(AyriaID, Message);
                    else Messages::Send::toClientencrypted(AyriaID, Message);
                }
            }

            return "{}";
        }
        inline std::string __cdecl Readmessages(const char *JSONString)
        {
            const auto Object = JSON::Parse(JSONString);
            auto Sources = Object.value("Sources", std::vector<uint64_t>());
            const auto Timefirst = Object.value("Timefirst", uint32_t());
            const auto Timelast = Object.value("Timelast", uint32_t());
            const auto GroupID = Object.value("GroupID", uint64_t());

            const auto Timeframe = std::make_pair(Timefirst, Timelast);
            const auto lMessages = Messages::Read(
                (Timefirst && Timelast) ? &Timeframe : nullptr,
                Sources.empty() ? nullptr : &Sources,
                GroupID ? &GroupID : nullptr
            );

            JSON::Array_t Array;
            Array.reserve(lMessages.size());
            for (const auto &Message : lMessages)
            {
                Array.push_back(Messages::toObject(*Message));
            }
            return JSON::Dump(Array);
        }

        inline std::string __cdecl getJoinrequests(const char *)
        {
            const auto Requests = Groups::getRequests();
            JSON::Array_t Array; Array.reserve(Requests->size());

            for (const auto &Item : *Requests)
                Array.push_back(Item.Raw);

            return JSON::Dump(Array);
        }
        inline std::string __cdecl getSubscriptions(const char *)
        {
            const auto Subscriptions = Groups::getSubscriptions();
            JSON::Array_t Array; Array.reserve(Subscriptions.size());

            for (const auto &Group : Subscriptions)
            {
                const auto Members = Groups::getMembers(Group);
                JSON::Array_t mArray; mArray.reserve(Members.size());

                for (const auto &Member : Groups::getMembers(Group))
                {
                    mArray.push_back(Member.Raw);
                }

                Array.push_back(JSON::Object_t({ { "GroupID", Group.Raw }, { "Members", mArray } }));
            }

            return JSON::Dump(Array);
        }
        inline std::string __cdecl manageLocalgroup(const char *JSONString)
        {
            const auto Object = JSON::Parse(JSONString);
            const auto UserID = Object.value("UserID", uint64_t());
            const auto Memberlimit = Object.value("Memberlimit", uint8_t(0xFF));

            if (Object.value("Createlocal", false))
            {
                return JSON::Dump(Groups::Createlocal(Memberlimit));
            }
            if (Object.value("Createglobal", false))
            {
                return JSON::Dump(Groups::Createglobal(Memberlimit));
            }
            if (Object.value("Destroygroup", false))
            {
                Groups::Destroygroup();
            }

            if (Object.value("addUser", false))
            {
                Groups::addUser(AyriaID_t{ .Raw = UserID });
            }
            if (Object.value("removeUser", false))
            {
                Groups::removeUser(AyriaID_t{ .Raw = UserID });
            }

            return "{}";
        }
        inline std::string __cdecl manageSubscriptions(const char *JSONString)
        {
            const auto Object = JSON::Parse(JSONString);
            const auto GroupID = Object.value("GroupID", uint64_t());
            if (GroupID)
            {
                if (Object.value("Join", false)) Groups::Subscribe(AyriaID_t{ .Raw = GroupID });
                else Groups::Unsubscribe(AyriaID_t{ .Raw = GroupID });
            }

            return "{}";
        }
    }

    inline void API_Initialize()
    {
        ::API::Registerhandler_Social("setRelation", API::setRelation);
        ::API::Registerhandler_Social("getRelations", API::getRelations);

        ::API::Registerhandler_Social("Sendmessage", API::Sendmessage);
        ::API::Registerhandler_Social("Readmessages", API::Readmessages);

        ::API::Registerhandler_Social("getJoinrequests", API::getJoinrequests);
        ::API::Registerhandler_Social("getSubscriptions", API::getSubscriptions);
        ::API::Registerhandler_Social("manageLocalgroup", API::manageLocalgroup);
        ::API::Registerhandler_Social("manageSubscriptions", API::manageSubscriptions);

        Relationships::Initialize();
        Messages::Initialize();
        Groups::Initialize();
    }
}
