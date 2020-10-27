/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-27
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common.hpp"

namespace Social
{
    std::vector<Relation_t> Friends;
    uint64_t Lastupdate{};

    // Manage the relations we know of, updates in the background.
    bool addRelation(uint32_t UserID, std::u8string Username, Relationflags_t Flags)
    {
        if (Flags.Raw == 0) Flags.isFriend = true;

        // Search the LAN for missing info.
        if (UserID == 0 || Username.empty())
        {
            const auto &[ID, Name] = getUser(UserID, Username);
            if (Username.empty()) Username = Name;
            if (!UserID) UserID = ID;
        }

        if (UserID)
        {
            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Object = nlohmann::json::object({ { "Username", Username }, { "UserID", UserID }, { "Flags", Flags.Raw } });
                const auto Result = Callback(Ayria.toFunctionID("addRelation"), DumpJSON(Object).c_str());
                return !!std::strstr(Result, "OK");
            }
        }

        return false;
    }
    void removeRelation(uint32_t UserID, const std::u8string &Username)
    {
        if (UserID)
        {
            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Object = nlohmann::json::object({ { "Username", Username }, { "UserID", UserID } });
                Callback(Ayria.toFunctionID("removeRelation"), DumpJSON(Object).c_str());
            }
        }
    }
    void blockUser(uint32_t UserID, const std::u8string &Username)
    {
        if (UserID)
        {
            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Object = nlohmann::json::object({ { "Username", Username }, { "UserID", UserID } });
                Callback(Ayria.toFunctionID("blockUser"), DumpJSON(Object).c_str());
            }
        }
    }

    Userinfo_t getUser(uint32_t UserID, std::u8string Username)
    {
        if (const auto Callback = Ayria.API_Client) [[likely]]
        {
            const auto Request = nlohmann::json::object({ { "Username", Username }, { "UserID", UserID } });
            const auto Result = Callback(Ayria.toFunctionID("getClient"), DumpJSON(Request).c_str());
            const auto Object = ParseJSON(Result);

            return { uint32_t(Object.value("AccountID", uint64_t()) & 0xFFFFFFFF), Object.value("Username", std::u8string()) };
        }

        return {};
    }
    std::vector<Relation_t *> getFriends()
    {
        std::vector<Relation_t *> Result;
        Result.reserve(Friends.size());
        for (auto it = Friends.begin(); it != Friends.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }
    void doFrame()
    {
        const auto Currentclock{ GetTickCount64() };
        if (Currentclock - Lastupdate > 5000)
        {
            if (const auto Callback = Ayria.API_Social) [[likely]]
            {
                const auto Result = Callback(Ayria.toFunctionID("Friendslist"), nullptr);
                const auto Array = ParseJSON(Result);

                Friends.clear();
                for (const auto &Item : Array)
                {
                    Friends.push_back(Relation_t(Item));
                }
            }

            Lastupdate = Currentclock;
        }
    }
}
