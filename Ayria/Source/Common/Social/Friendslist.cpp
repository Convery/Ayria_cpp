/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-27
    License: MIT
*/

#include <Global.hpp>

namespace Social::Friendslist
{
    std::vector<Friend_t> Friendslist{};

    static void fromString(std::string_view JSONString)
    {
        try
        {
            const auto Object = nlohmann::json::parse(JSONString);
            for (const auto &Item : Object)
            {
                Friend_t Newfriend{};
                const std::string Username = Item.value("Username", std::string());
                Newfriend.UserID = Item.value("UserID", decltype(Newfriend.UserID)());
                Newfriend.Presence = Item.value("Presence", decltype(Newfriend.Presence)());
                Newfriend.b64Avatar = Item.value("b64Avatar", decltype(Newfriend.b64Avatar)());
                std::memcpy(Newfriend.Username, Username.c_str(), std::min(Username.size(), size_t(19)));

                Friendslist.push_back(Newfriend);
            }

        } catch (const std::exception &e)
        {
            Infoprint(va("Could not load Friendscache: %s", e.what()));
        }
    }
    static std::string toString()
    {
        auto Array = nlohmann::json::array();
        for(const auto &Item : Friendslist)
        {
            auto Object = nlohmann::json::object();
            Object["b64Avatar"] = Item.b64Avatar;
            Object["Presence"] = Item.Presence;
            Object["Username"] = Item.Username;
            Object["UserID"] = Item.UserID;

            Array += Object;
        }

        return Array.dump(4);
    }

    std::vector<Friend_t> *getFriends()
    {
        // TODO(tcn): Load from other sources.
        if (Friendslist.empty()) { fromString(B2S(FS::Readfile("./Ayria/Friendscache.json"))); }

        // Limit updates to once every 5 sec.
        static uint64_t Lastcheck = 0;
        if(time(NULL) - Lastcheck > 5)
        {
            Lastcheck = time(NULL);

            // Set the online-status if they are on LAN.
            for(const auto &Client : *Clients::getLocalclients())
            {
                auto Friend = std::find_if(Friendslist.begin(), Friendslist.end(),
                    [&](const auto &Item) {return Item.UserID == Client.ClientID; });

                Friend->State = (Userstate_t)std::max(uint8_t(Friend->State), uint8_t(Friend != Friendslist.end()));
            }

            // Cache to disk.
            FS::Writefile("./Ayria/Friendscache.json", toString());
        }

        return &Friendslist;
    }
}

extern "C"
{
    EXPORT_ATTR void __cdecl getFriends(const char **Friendslist)
    {
        static std::string JSON;
        auto List = Social::Friendslist::getFriends();
        if (List->empty()) JSON = "{}";
        else
        {
            auto Array = nlohmann::json::array();
            for (const auto &Item : *List)
            {
                auto Object = nlohmann::json::object();
                Object["b64Avatar"] = Item.b64Avatar;
                Object["Presence"] = Item.Presence;
                Object["Username"] = Item.Username;
                Object["UserID"] = Item.UserID;

                Object["State"] = uint32_t(Item.State);
                Array += Object;
            }
            JSON = Array.dump();
        }

        *Friendslist = JSON.c_str();
    }

    EXPORT_ATTR void __cdecl setFriends(const char *Friendslist)
    {
        Social::Friendslist::fromString(Friendslist);
    }
}
