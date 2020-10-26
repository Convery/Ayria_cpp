/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Social
{
    using Message_t = struct { uint32_t Timestamp; AccountID_t Source; std::u8string Message; };
    using Relation_t = struct { uint32_t AccountID; std::u8string Username; uint32_t Flags; };
    using Relationflags_t = union
    {
        uint32_t Raw;
        struct
        {
            uint32_t
                isFriend : 1,
                isBlocked : 1,
                isFollower : 1,
                isGroupmember : 1,
                AYA_RESERVED1 : 1,
                AYA_RESERVED2 : 1,
                AYA_RESERVED3 : 1;
        };
    };

    namespace Messaging
    {
        namespace Read
        {
            std::vector<Message_t *> All();
            std::vector<Message_t *> bySenders(const std::vector<uint32_t> &SenderIDs);
            std::vector<Message_t *> byTime(uint32_t First, uint32_t Last, uint32_t SenderID = 0);
        }
        namespace Send
        {
            // Returns false if clients are offline or missing public key.
            bool toClient(uint32_t ClientID, std::u8string_view Message);
            bool toClientencrypted(uint32_t ClientID, std::u8string_view Message);
            bool Broadcast(std::vector<uint32_t> ClientIDs, std::u8string_view Message);
        }

        // Current size of the storage.
        size_t getMessagecount();

        // Add the message-handlers.
        void Initialize();
    }

    namespace Relations
    {
        void Add(uint32_t UserID, std::u8string_view Username, uint32_t Relationflags);
        void Remove(uint32_t UserID, std::u8string_view Username);
        const std::vector<Relation_t> *Get();

        // Disk management.
        void Load(std::wstring_view Path = L"./Ayria/Relations.json"s);
        void Save(std::wstring_view Path = L"./Ayria/Relations.json"s);
    }

    // Add API handlers.
    inline std::string __cdecl removeFriend(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto UserID = Object.value("UserID", uint32_t());
        const auto Username = Object.value("Username", std::u8string());

        // We need an ID or username.
        if (UserID == 0 && Username.empty()) [[unlikely]] return "{ \"Status\": \"Error\" }";

        Relations::Remove(UserID, Username);
        return "{ \"Status\": \"OK\" }";
    }
    inline std::string __cdecl addFriend(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto Flags = Object.value("Flags", uint32_t());
        const auto UserID = Object.value("UserID", uint32_t());
        const auto Username = Object.value("Username", std::u8string());

        // We need an ID, the username is optional.
        if (UserID == 0) [[unlikely]] return "{ \"Status\": \"Error\" }";

        Relationflags_t Internal{ Flags }; Internal.isFriend = true;
        Relations::Add(UserID, Username, Internal.Raw);
        return "{ \"Status\": \"OK\" }";
    }
    inline std::string __cdecl blockUser(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto Flags = Object.value("Flags", uint32_t());
        const auto UserID = Object.value("UserID", uint32_t());
        const auto Username = Object.value("Username", std::u8string());

        // We need an ID, the username is optional.
        if (UserID == 0) [[unlikely]] return "{ \"Status\": \"Error\" }";

        Relationflags_t Internal{ Flags }; Internal.isBlocked = true;
        Relations::Add(UserID, Username, Internal.Raw);
        return "{ \"Status\": \"OK\" }";
    }
    inline std::string __cdecl Friendslist(const char *)
    {
        auto Array = nlohmann::json::array();
        for (const auto &[ID, Username, Flags] : *Relations::Get())
        {
            const Relationflags_t Internal{ Flags };
            if (Internal.isFriend)
            {
                Array += { { "Username", Username }, { "UserID", ID }, { "Flags", Flags }};
            }
        }

        return DumpJSON(Array);
    }

    inline std::string __cdecl Readmessages(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto Endtime = Object.value("Endtime", uint32_t());
        const auto SenderID = Object.value("SenderID", uint32_t());
        const auto Starttime = Object.value("Starttime", uint32_t());
        auto SenderIDs = Object.value("SenderIDs", std::vector<uint32_t>());

        std::vector<Message_t *> Messages;
        if (Starttime + Endtime == 0)
        {
            SenderIDs.push_back(SenderID);
            Messages = Messaging::Read::bySenders(SenderIDs);
        }
        else
        {
            Messages = Messaging::Read::byTime(Starttime, Endtime, SenderID);
        }

        auto Array = nlohmann::json::array();
        for (const auto Pointer : Messages)
        {
            const auto &[Timestamp, Source, Message] = *Pointer;
            Array += { { "Timestamp", Timestamp }, { "Source", Source.Raw }, { "Message", Message } };
        }

        return DumpJSON(Array);
    }
    inline std::string __cdecl Sendmessage(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto Target = Object.value("Target", uint32_t());
        const auto Message = Object.value("Message", std::u8string());
        auto Targets = Object.value("Targets", std::vector<uint32_t>());

        if (!Targets.empty())
        {
            Targets.push_back(Target);
            if(Messaging::Send::Broadcast(Targets, Message))
                return "{ \"Status\": \"OK\" }";
        }
        else
        {
            if (Messaging::Send::toClient(Target, Message))
                return "{ \"Status\": \"OK\" }";
        }

        return "{ \"Status\": \"Error\" }";
    }
    inline std::string __cdecl Sendmessage_enc(const char *JSONString)
    {
        const auto Object = ParseJSON(JSONString);
        const auto Target = Object.value("Target", uint32_t());
        const auto Message = Object.value("Message", std::u8string());

        if (Messaging::Send::toClientencrypted(Target, Message))
            return "{ \"Status\": \"OK\" }";
        else
            return "{ \"Status\": \"Error\" }";
    }

    inline void API_Initialize()
    {
        API::Registerhandler_Social("addFriend", addFriend);
        API::Registerhandler_Social("blockUser", blockUser);
        API::Registerhandler_Social("Friendslist", Friendslist);
        API::Registerhandler_Social("removeFriend", removeFriend);

        API::Registerhandler_Social("Sendmessage", Sendmessage);
        API::Registerhandler_Social("Readmessages", Readmessages);
        API::Registerhandler_Social("Sendmessage_enc", Sendmessage_enc);

        Messaging::Initialize();
        Relations::Load();
    }
}
