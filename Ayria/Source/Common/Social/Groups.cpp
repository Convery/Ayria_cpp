/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-09
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social::Groups
{
    using Groupinfo_t = struct { std::unordered_set<AyriaID_t, decltype(WW::Hash), decltype(WW::Equal)> Members; uint8_t Memberlimit; };
    static absl::flat_hash_map<AyriaID_t, Groupinfo_t, decltype(WW::Hash), decltype(WW::Equal)> LANGroups, WANGroups;
    static std::vector<AyriaID_t> Joinrequests;
    static Groupinfo_t Localgroup{};
    static AyriaID_t LocalID{};

    // Send an update once in a while.
    static void Sendupdate()
    {
        if (LocalID.Raw == 0) [[likely]] return;

        const JSON::Object_t Request({
            { "GroupID", LocalID.Raw },
            { "Memberlimit", Localgroup.Memberlimit },
            { "Members", JSON::Array_t(Localgroup.Members.begin(), Localgroup.Members.end()) }
            });

        Backend::Sendmessage(Hash::WW32("Localgroup::Update"), JSON::Dump(Request), Backend::Matchmakeport);
    }

    AyriaID_t Createglobal(uint8_t Memberlimit)
    {
        // TODO(tcn): Fetch data from a global tracker.
        (void)Memberlimit;
        return LocalID;
    }
    AyriaID_t Createlocal(uint8_t Memberlimit)
    {
        Localgroup = { { Userinfo::getAccount()->ID }, Memberlimit };
        LocalID = Userinfo::getAccount()->ID;
        Sendupdate();
        return LocalID;
    }
    void Destroygroup()
    {
        Localgroup.Members.clear();
        LocalID.Raw = 0;

        // TODO(tcn): Notify a remote server.
    }

    bool Subscribe(AyriaID_t GroupID)
    {
        if (Accountflags_t{ .Raw = GroupID.Accountflags }.isGroup)
        {
            if (!LANGroups.contains(GroupID) || LANGroups[GroupID].Memberlimit <= LANGroups[GroupID].Members.size())
                return false;

            const JSON::Object_t Request({ { "GroupID", GroupID.Raw }, { "Join", true } });
            Backend::Sendmessage(Hash::WW32("Localgroup::Subscription"), JSON::Dump(Request), Backend::Matchmakeport);
        }
        else
        {
            if (!WANGroups.contains(GroupID) || WANGroups[GroupID].Memberlimit <= WANGroups[GroupID].Members.size())
                return false;

            // TODO(tcn): Ask a remote server.
        }

        return true;
    }
    bool Unsubscribe(AyriaID_t GroupID)
    {
        if (Accountflags_t{ .Raw = GroupID.Accountflags }.isGroup)
        {
            if (!LANGroups.contains(GroupID) || LANGroups[GroupID].Memberlimit <= LANGroups[GroupID].Members.size())
                return false;

            const JSON::Object_t Request({ { "GroupID", GroupID.Raw }, { "Join", false } });
            Backend::Sendmessage(Hash::WW32("Localgroup::Subscription"), JSON::Dump(Request), Backend::Matchmakeport);
        }
        else
        {
            if (!WANGroups.contains(GroupID) || WANGroups[GroupID].Memberlimit <= WANGroups[GroupID].Members.size())
                return false;

            // TODO(tcn): Ask a remote server.
        }

        return true;
    }
    std::vector< AyriaID_t> getSubscriptions()
    {
        const auto ClientID = Userinfo::getAccount()->ID;
        std::vector<AyriaID_t> Result;
        Result.reserve(4);

        for (auto it = LANGroups.cbegin(); it != LANGroups.cend(); ++it)
            if (it->second.Members.contains(ClientID))
                Result.push_back(it->first);

        for (auto it = WANGroups.cbegin(); it != WANGroups.cend(); ++it)
            if (it->second.Members.contains(ClientID))
                Result.push_back(it->first);

        return Result;
    }
    std::vector<AyriaID_t> getMembers(AyriaID_t GroupID)
    {
        if (GroupID.Raw == Userinfo::getAccount()->ID.Raw)
            return { Localgroup.Members.cbegin(), Localgroup.Members.cend() };

        if (LANGroups.contains(GroupID))
            return { LANGroups[GroupID].Members.cbegin(), LANGroups[GroupID].Members.cend() };

        if (WANGroups.contains(GroupID))
            return { WANGroups[GroupID].Members.cbegin(), WANGroups[GroupID].Members.cend() };

        return {};
    }

    // Manage the local group.
    void addUser(AyriaID_t UserID)
    {
        std::erase_if(Joinrequests, [UserID](const auto &ID) { return ID.Raw == UserID.Raw; });
        Localgroup.Members.insert(UserID);
        Sendupdate();
    }
    void removeUser(AyriaID_t UserID)
    {
        Localgroup.Members.erase(UserID);
        Sendupdate();
    }
    const std::vector<AyriaID_t> *getRequests()
    {
        return &Joinrequests;
    }

    // Network messages, TODO(tcn): poll remote servers.
    static void __cdecl Subscriptionhandler(uint32_t NodeID, const char *JSONString)
    {
        if (LocalID.Raw == 0) [[likely]] return;

        const auto Request = JSON::Parse(JSONString);
        const auto GroupID = Request.value("GroupID", uint64_t());

        if (AyriaID_t{ .Raw = GroupID }.AccountID == Userinfo::getAccount()->ID.AccountID) [[unlikely]]
        {
            if (const auto Client = Clientinfo::getNetworkclient(NodeID)) [[likely]]
            {
                if (!Request.value("Join", false)) removeUser(Client->UserID);
                else Joinrequests.push_back(Client->UserID);
            }
        }
    }
    static void __cdecl Updatehandler(uint32_t NodeID, const char *JSONString)
    {
        const auto Request = JSON::Parse(JSONString);
        const auto GroupID = Request.value("GroupID", uint64_t());
        const auto Members = Request.value("Members", JSON::Array_t());
        const auto Memberlimit = Request.value("Memberlimit", uint8_t());

        if (const auto Client = Clientinfo::getNetworkclient(NodeID)) [[likely]]
        {
            if (Client->UserID.Raw == GroupID) [[likely]]
            {
                auto &Entry = LANGroups[Client->UserID];
                Entry.Memberlimit = Memberlimit;
                Entry.Members.clear();

                for (const auto &Item : Members)
                    LANGroups[Client->UserID].Members.insert(AyriaID_t(Item));
            }
        }
    }

    // Add the message-handlers.
    void Initialize()
    {
        Backend::Enqueuetask(5000, Sendupdate);

        // Register message-handlers.
        Backend::Registermessagehandler(Hash::WW32("Localgroup::Update"), Updatehandler);
        Backend::Registermessagehandler(Hash::WW32("Localgroup::Subscription"), Subscriptionhandler);
    }
}
