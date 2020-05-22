/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-25
    License: MIT
*/

#include "../../Common.hpp"

namespace Social
{
    std::unordered_map<uint64_t, Friend_t> Friendslist;
    Friend_t Self{};

    std::vector<Friend_t> getFriends()
    {
        // DEV: Ask Ayria for all known clients.
        if (const auto Callback = Global.Ayria.getPlayers)
        {
            const char *JSONString;
            Callback(&JSONString);

            try
            {
                for (const auto &Object : nlohmann::json::parse(JSONString))
                {
                    Friend_t Newfriend;
                    Newfriend.Status = Friend_t::Online;
                    Newfriend.Lastmodified = uint32_t(time(NULL));
                    Newfriend.Avatar = Object.value("Avatar", Blob());
                    Newfriend.UserID = Object.value("UserID", uint64_t());
                    Newfriend.Username = Object.value("Username", std::string());

                    if (!Friendslist.contains(Newfriend.UserID))
                        Friendslist[Newfriend.UserID] = Newfriend;
                }

            } catch (...) {}
        }

        std::vector<Friend_t> Friends;
        Friends.reserve(Friendslist.size());
        for (const auto &[ID, Item] : Friendslist)
            Friends.emplace_back(Item);

        return Friends;
    }
    void Announceupdate(Friend_t Delta)
    {
        if (!Delta.Status) Self.Status = Delta.Status;
        if (!Delta.UserID) Self.UserID = Global.UserID;
        if (!Delta.Avatar.empty()) Self.Avatar = Delta.Avatar;
        if (!Delta.Username.empty()) Self.Username = Global.Username;
        Communication::Broadcastmessage("Socialupdate", 0, Bytebuffer(bbSerialize(Self)));
    }
    void Announcequit()
    {
        Self.Status = Self.Offline;
        Communication::Broadcastmessage("Socialupdate", 0, Bytebuffer(bbSerialize(Self)));
    }

    // Callbacks from the Ayria network-layer.
    void onUpdate(uint64_t, uint64_t SenderID, std::string_view, Bytebuffer &&Data)
    {
        Friendslist[SenderID].Deserialize(Data);
        Friendslist[SenderID].Lastmodified = uint32_t(time(NULL));
    }

    // Add the callbacks on startup.
    namespace { struct Startup { Startup()
    {
        Communication::addMessagecallback("Socialupdate", onUpdate);
    } }; static Startup Loader{}; }
}

