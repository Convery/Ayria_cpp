/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-19
    License: MIT
*/

#include "../Global.hpp"

namespace Social
{
    std::vector<Friend_t> Friendslist;
    std::vector<Friend_t> Playerlist;

    void onNewclient(Client::Client_t &New)
    {
        for (auto &Item : Friendslist)
        {
            if (Item.UserID == New.ClientID || Item.Username == New.Clientname)
            {
                Item.Status = Friend_t::Online;
            }
        }

        Friend_t Newfriend{ New.ClientID, New.Clientname, Friend_t::Online, {} };
        Playerlist.emplace_back(std::move(Newfriend));
    }

    void onStartup()
    {
        if (const auto Filebuffer = FS::Readfile("./Ayria/" + "Friendslist.json"s); !Filebuffer.empty())
        {
            try
            {
                for (const auto &Object : nlohmann::json::parse(Filebuffer.c_str()))
                {
                    Friend_t Newfriend;
                    Newfriend.Status = Friend_t::Offline;
                    Newfriend.Avatar = Object.value("Avatar", Blob());
                    Newfriend.UserID = Object.value("UserID", uint64_t());
                    Newfriend.Username = Object.value("Username", std::string());

                    Friendslist.push_back(Newfriend);
                }

            } catch (...) {};
        }
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl getFriends(const char **Friendslist)
        {
            static std::string JSONString{"{}"};
            auto Parent = nlohmann::json::array();

            for (const auto &Friend : Social::Friendslist)
            {
                auto Object = nlohmann::json::object();
                Object["Username"] = Friend.Username;
                Object["UserID"] = Friend.UserID;
                Object["Avatar"] = Friend.Avatar;
                Object["Status"] = Friend.Status;

                Parent += Object;
            }

            JSONString = Parent.dump();
            *Friendslist = JSONString.c_str();
        }
        EXPORT_ATTR void __cdecl getPlayers(const char **Playerlist)
        {
            static std::string JSONString{"{}"};
            auto Parent = nlohmann::json::array();

            for (const auto &Friend : Social::Playerlist)
            {
                auto Object = nlohmann::json::object();
                Object["Username"] = Friend.Username;
                Object["UserID"] = Friend.UserID;
                Object["Avatar"] = Friend.Avatar;

                Parent += Object;
            }

            JSONString = Parent.dump();
            *Playerlist = JSONString.c_str();
        }
    }
}