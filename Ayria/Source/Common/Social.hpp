/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Social
{
    // Returns false if clients are offline or missing public key.
    bool Sendinstantmessage(std::vector<uint32_t> Clients, uint32_t Messagetype, std::string_view Message);
    bool Sendinstantmessage_enc(uint32_t Client, uint32_t Messagetype, std::string_view Message);

    // Returns the chat messages seen this session.
    using Message_t = struct { uint32_t Sender; uint32_t Type; std::string Message; };
    std::vector<Message_t> Readinstantmessages(uint32_t Offset, uint32_t Count, uint32_t Messagetype, uint32_t SenderID = 0);

    // Requests key from Clients, shares own.
    void Syncpublickeys(std::vector<uint32_t> Clients);

    // Add the message-handlers.
    void Initialize();

    // Just saves and modifies the friendslist. Use Clientinfo::getNetworkclients to check online status.
    using Userinfo_t = struct { uint32_t UserID; std::string Username; };
    void addFriend(uint32_t UserID, std::string_view Username);
    const std::vector<Userinfo_t> *getFriendslist();
    void removeFriend(uint32_t UserID);

    // Add API handlers.
    inline std::string __cdecl addFriend(const char *JSONString)
    {
        do
        {
            if (!JSONString) break;
            const auto Object = ParseJSON(JSONString);
            if (!Object.contains("UserID")) break;

            addFriend(Object["UserID"], Object.value("Username", "Unknown"));
        } while (false);

        return "{}";
    }
    inline std::string __cdecl removeFriend(const char *JSONString)
    {
        do
        {
            if (!JSONString) break;
            const auto Object = ParseJSON(JSONString);
            if (!Object.contains("UserID")) break;

            removeFriend(Object["UserID"]);
        } while (false);

        return "{}";
    }
    inline std::string __cdecl Friendslist(const char *)
    {
        const auto Friends = getFriendslist();
        auto Array = nlohmann::json::array();

        for (const auto &Item : *Friends)
            Array += { { "Username", Item.Username }, { "UserID", Item.UserID } };

        return Array.dump(4);
    }

    inline std::string __cdecl ReadIM(const char *JSONString)
    {
        const auto Request = ParseJSON(JSONString);
        const auto Type = Request.value("Type", uint32_t());
        const auto Count = Request.value("Count", uint32_t(1));
        const auto Sender = Request.value("Sender", uint32_t());
        const auto Offset = Request.value("Offset", uint32_t());

        auto Array = nlohmann::json::array();
        for (const auto &Message : Readinstantmessages(Offset, Count, Type, Sender))
        {
            auto Object = nlohmann::json::object();
            Object["Message"] = Message.Message;
            Object["Sender"] = Message.Sender;
            Object["Type"] = Message.Type;
            Array += Object;
        }

        return Array.dump();
    }
    inline std::string __cdecl SendIM(const char *JSONString)
    {
        const auto Request = ParseJSON(JSONString);
        const auto Type = Request.value("Type", uint32_t());
        const auto Message = Request.value("Message", std::string());
        const auto Clients = Request.value("Clients", std::vector<uint32_t>());

        if (Sendinstantmessage(Clients, Type, Message))
            return "{}";
        else
            return "{ \"Error\": \"All clients are offline\" }";
    }
    inline std::string __cdecl SendIM_enc(const char *JSONString)
    {
        const auto Request = ParseJSON(JSONString);
        const auto Type = Request.value("Type", uint32_t());
        const auto Client = Request.value("Client", uint32_t());
        const auto Message = Request.value("Message", std::string());

        if (Sendinstantmessage_enc(Client, Type, Message))
            return "{}";
        else
            return "{ \"Error\": \"Client is offline or missing pub-key, try again later.\" }";
    }

    inline void API_Initialize()
    {
        API::Registerhandler_Social("addFriend", addFriend);
        API::Registerhandler_Social("Friendslist", Friendslist);
        API::Registerhandler_Social("removeFriend", removeFriend);

        API::Registerhandler_Social("ReadIM", ReadIM);
        API::Registerhandler_Social("SendIM", SendIM);
        API::Registerhandler_Social("SendIM_enc", SendIM_enc);

        Initialize();
    }
}
