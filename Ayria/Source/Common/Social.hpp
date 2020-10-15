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

    // Requests key from Clients, shares own.
    void Syncpublickeys(std::vector<uint32_t> Clients);

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
    inline void API_Initialize()
    {
        API::Registerhandler_Social("addFriend", addFriend);
        API::Registerhandler_Social("Friendslist", Friendslist);
        API::Registerhandler_Social("removeFriend", removeFriend);
    }
}
