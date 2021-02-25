/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-30
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Groups
{
    union GroupID_t
    {
        uint64_t Raw;
        struct
        {
            uint32_t AdminID;   // Creator.
            uint16_t RoomID;    // Random.
            uint8_t HostID;     // Server, 0 == localhost.
            uint8_t Limit;      // Users.
        };
    };
    struct Group_t
    {
        GroupID_t GroupID;
        std::u8string Groupname;
        Hashset<uint32_t> MemberIDs;
        Hashmap<uint32_t, JSON::Value_t> Memberdata;
    };
    struct Joinrequest_t
    {
        uint64_t GroupID;
        uint32_t ClientID;
        JSON::Value_t Extradata;
    };

    static Hashmap<uint64_t, Group_t> Localgroups{}, Remotegroups{};
    static std::vector<Joinrequest_t> Joinrequests{};
    static Hashmap<uint64_t, uint32_t> Lastseen{};
    static uint32_t Lastupdate{};
    static bool isDirty{};

    // Verify membership.
    bool isMember(uint64_t GroupID, uint32_t UserID)
    {
        if (Localgroups.contains(GroupID))
        {
            return Localgroups[GroupID].MemberIDs.contains(UserID);
        }

        if (Remotegroups.contains(GroupID))
        {
            return Remotegroups[GroupID].MemberIDs.contains(UserID);
        }

        return false;
    }
    bool isAdmin(uint64_t GroupID, uint32_t UserID)
    {
        return GroupID_t{ GroupID }.AdminID == UserID && (Localgroups.contains(GroupID) || Remotegroups.contains(GroupID));
    }

    // Notify the world about our groups.
    static void onGroupstatuschange(uint64_t GroupID)
    {
        const auto Active = Localgroups.contains(GroupID);
        std::vector<uint32_t> Members{};

        if (Active)
        {
            const auto Group = &Localgroups[GroupID];
            Members.reserve(Group->MemberIDs.size());

            for (const auto &ID : Group->MemberIDs)
                Members.emplace_back(ID);
        }

        const auto Request = JSON::Object_t({
            { "GroupID", GroupID },
            { "Members", Members},
            { "Active", Active }
        });

        Backend::Network::Transmitmessage("Groupstatuschange", Request);
    }
    static void onJoinrequest(uint64_t GroupID, JSON::Value_t Extradata)
    {
        const auto Request = JSON::Object_t({
            { "Extradata", Extradata },
            { "GroupID", GroupID }
        });

        Backend::Network::Transmitmessage("Groupjoinrequest", Request);
    }
    static void __cdecl Statushandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Members = Request.value<JSON::Value_t>("Members");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Active = Request.value<bool>("Active");

        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender || Sender->UserID != GroupID_t{ GroupID }.AdminID) return;
        if (!Active) { Localgroups.erase(GroupID); return; }

        Localgroups[GroupID].MemberIDs = Members;
    }
    static void __cdecl Joinhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Extradata = Request.value<JSON::Value_t>("Extradata");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender) return;

        if (Global.UserID == GroupID_t{ GroupID }.AdminID && Localgroups.contains(GroupID))
        {
            Joinrequests.emplace_back(GroupID, Sender->UserID, Extradata);
        }
    }
    static void __cdecl Leavehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");

        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender) return;

        if (Global.UserID == GroupID_t{ GroupID }.AdminID && Localgroups.contains(GroupID))
        {
            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
            }
            else
            {
                if (Localgroups.contains(GroupID)) [[likely]]
                {
                    Localgroups[GroupID].Memberdata.erase(Sender->UserID);
                    Localgroups[GroupID].MemberIDs.erase(Sender->UserID);
                    onGroupstatuschange(GroupID);
                }
            }
        }
    }
    static void __cdecl Memberdatahandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Memberdata = Request.value<JSON::Array_t>("Memberdata");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender || Sender->UserID != GroupID_t{ GroupID }.AdminID) return;

        auto Group = &Localgroups[GroupID];
        for (const auto &Item : Memberdata)
        {
            const auto Data = Item.value<JSON::Value_t>("Data");
            const auto ID = Item.value<uint32_t>("ID");
            Group->Memberdata[ID] = Data;
        }
    }

    // Update once in a while.
    static void Announce()
    {
        if (Global.Stateflags.isPrivate) return;

        if (std::any_of(std::execution::par_unseq, Localgroups.begin(), Localgroups.end(),
            [](const auto &Item) { return Global.UserID == GroupID_t{ Item.second.GroupID }.HostID; }))
        {
            if (isDirty || (GetTickCount() - Lastupdate) > 9'000)
            {
                Lastupdate = GetTickCount();
                isDirty = false;

                for (const auto &[ID, Group] : Localgroups)
                {
                    if (GroupID_t{ ID }.HostID == Global.UserID)
                    {
                        JSON::Array_t Memberdata;
                        Memberdata.reserve(Group.Memberdata.size());

                        for (const auto &[Key, Value] : Group.Memberdata)
                        {
                            Memberdata.emplace_back(JSON::Object_t({
                                { "Data", Value},
                                { "ID", Key }
                            }));
                        }

                        const auto Request = JSON::Object_t({
                            { "Memberdata", Memberdata },
                            { "GroupID", ID }
                        });
                        Backend::Network::Transmitmessage("Groupmemberupdate", Request);
                    }
                }

                if (Global.Stateflags.isOnline)
                {
                    // TODO(tcn): Notify some server.
                }
            }
        }
    }

    // JSON interface for the plugins.
    namespace API
    {
        // Create a new group as host, optionally request a remote host for it.
        static std::string __cdecl Creategroup(JSON::Value_t &&Request)
        {
            const auto Groupname = Request.value<std::u8string>("Groupname");
            const auto Memberlimit = Request.value<uint8_t>("Memberlimit");
            const auto Selfhosted = Request.value<bool>("Selfhosted");

            // Sanity checking.
            if (!Global.Stateflags.isOnline && !Selfhosted) [[unlikely]]
                return R"({ "Error" : "Can only request hosting if online." })";

            if (!Selfhosted)
            {
                // TODO(tcn): Request a server somewhere.
                return R"({ "Error" : "Not implemented." })";
            }

            Group_t Newgroup{};
            Newgroup.GroupID.RoomID = Hash::WW32(GetTickCount());
            Newgroup.GroupID.AdminID = Global.UserID;
            Newgroup.MemberIDs.insert(Global.UserID);
            Newgroup.GroupID.Limit = Memberlimit;
            Newgroup.Groupname = Groupname;

            Localgroups[Newgroup.GroupID.Raw] = Newgroup;
            onGroupstatuschange(Newgroup.GroupID.Raw);

            return va("{ \"GroupID\" : %016X }", Newgroup.GroupID.Raw);
        }
        static std::string __cdecl Deletegroup(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            if (GroupID_t{ GroupID }.AdminID == Global.UserID) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    Localgroups.erase(GroupID);
                    onGroupstatuschange(GroupID);
                }
            }

            return "{}";
        }

        // Manage the group-members for groups this client is admin of.
        static std::string __cdecl addMember(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");

            if (GroupID_t{ GroupID }.AdminID == Global.UserID) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Localgroups.contains(GroupID)) [[likely]]
                    {
                        Localgroups[GroupID].MemberIDs.insert(UserID);
                        onGroupstatuschange(GroupID);
                    }
                }
            }

            return "{}";
        }
        static std::string __cdecl removeMember(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");

            if (GroupID_t{ GroupID }.AdminID == Global.UserID) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Localgroups.contains(GroupID)) [[likely]]
                    {
                        Localgroups[GroupID].MemberIDs.erase(UserID);
                        onGroupstatuschange(GroupID);
                    }
                }
            }

            return "{}";
        }

        // Modify or fetch the members shared data.
        static std::string __cdecl getMemberdata(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");

            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
                return R"({ "Error" : "Not implemented." })";
            }
            else
            {
                if (Localgroups.contains(GroupID)) [[likely]]
                {
                    const auto Group = &Localgroups[GroupID];
                    if (Group->Memberdata.contains(UserID))
                    {
                        return JSON::Dump(Group->Memberdata[UserID]);
                    }
                }
            }

            return "{}";
        }
        static std::string __cdecl setMemberdata(JSON::Value_t &&Request)
        {
            const auto Memberdata = Request.value<JSON::Value_t>("Memberdata");
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");

            if (GroupID_t{ GroupID }.AdminID == Global.UserID) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Localgroups.contains(GroupID)) [[likely]]
                    {
                        Localgroups[GroupID].Memberdata[UserID] = Memberdata;
                        isDirty = true;
                    }
                }
            }

            return "{}";
        }

        // Manage invitations from other users.
        static std::string __cdecl getJoinrequests(JSON::Value_t &&)
        {
            JSON::Array_t Result;
            Result.reserve(Joinrequests.size());

            for (const auto &Item : Joinrequests)
            {
                Result.emplace_back(JSON::Object_t({
                    { "Extradata", Item.Extradata },
                    { "ClientID", Item.ClientID },
                    { "GroupID", Item.GroupID }
                }));
            }

            return JSON::Dump(Result);
        }
        static std::string __cdecl answerJoinrequestequest(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");
            const auto Accepted = Request.value<bool>("Accepted");

            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
                return R"({ "Error" : "Not implemented." })";
            }
            else
            {

                if (std::any_of(std::execution::par_unseq, Joinrequests.begin(), Joinrequests.end(),
                    [=](const Joinrequest_t &Item) { return Item.GroupID == GroupID && Item.ClientID == UserID; }))
                {
                    addMember(std::move(Request));

                    std::erase_if(Joinrequests,
                        [=](const Joinrequest_t &Item) { return Item.GroupID == GroupID && Item.ClientID == UserID; });
                }
            }

            return "{}";
        }

        // Let the hosts know about us.
        static std::string __cdecl Requestjoin(JSON::Value_t &&Request)
        {
            const auto Extradata = Request.value<JSON::Value_t>("Extradata");
            const auto GroupID = Request.value<uint64_t>("GroupID");

            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
                return R"({ "Error" : "Not implemented." })";
            }
            else
            {
                if (Localgroups.contains(GroupID))
                {
                    onJoinrequest(GroupID, Extradata);
                }
                else
                {
                    return R"({ "Error" : "No such group." })";
                }
            }

            return "{}";
        }
        static std::string __cdecl Notifyleave(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");

            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
                return R"({ "Error" : "Not implemented." })";
            }
            else
            {
                Backend::Network::Transmitmessage("Groupleave", JSON::Object_t({{ "GroupID", GroupID } }));
            }

            return "{}";
        }

        // List the groups that we know of.
        static std::string __cdecl Listgroups(JSON::Value_t &&)
        {
            JSON::Array_t Local, Remote;
            Local.reserve(Localgroups.size());
            Remote.reserve(Remotegroups.size());

            for (const auto &[GroupID, Entry] : Localgroups)
            {
                std::vector<uint32_t> Members;
                Members.reserve(Entry.MemberIDs.size());

                for (const auto &ID : Entry.MemberIDs)
                    Members.emplace_back(ID);

                Local.emplace_back(JSON::Object_t({
                    { "Groupname", Entry.Groupname },
                    { "GroupID", GroupID },
                    { "Members", Members }
                }));
            }
            for (const auto &[GroupID, Entry] : Remotegroups)
            {
                std::vector<uint32_t> Members;
                Members.reserve(Entry.MemberIDs.size());

                for (const auto &ID : Entry.MemberIDs)
                    Members.emplace_back(ID);

                Remote.emplace_back(JSON::Object_t({
                    { "Groupname", Entry.Groupname },
                    { "GroupID", GroupID },
                    { "Members", Members }
                }));
            }

            const auto Result = JSON::Object_t({
                { "Remote", Remote },
                { "Local", Local }
            });

            return JSON::Dump(Result);
        }
    }

    // Set up message-handlers.
    void Initialize()
    {
        // Member-data can be large so limit updates.
        Backend::Enqueuetask(5000, Announce);

        // Register the status change handlers.
        Backend::Network::Registerhandler("Groupleave", Leavehandler);
        Backend::Network::Registerhandler("Groupjoinrequest", Joinhandler);
        Backend::Network::Registerhandler("Groupstatuschange", Statushandler);
        Backend::Network::Registerhandler("Groupmemberupdate", Memberdatahandler);

        // JSON endpoints.
        Backend::API::addEndpoint("Social::Groups::Creategroup", API::Creategroup);
        Backend::API::addEndpoint("Social::Groups::Deletegroup", API::Deletegroup);
        Backend::API::addEndpoint("Social::Groups::addMember", API::addMember);
        Backend::API::addEndpoint("Social::Groups::removeMember", API::removeMember);
        Backend::API::addEndpoint("Social::Groups::getMemberdata", API::getMemberdata);
        Backend::API::addEndpoint("Social::Groups::setMemberdata", API::setMemberdata);
        Backend::API::addEndpoint("Social::Groups::getJoinrequests", API::getJoinrequests);
        Backend::API::addEndpoint("Social::Groups::answerJoinrequestequest", API::answerJoinrequestequest);
        Backend::API::addEndpoint("Social::Groups::Requestjoin", API::Requestjoin);
        Backend::API::addEndpoint("Social::Groups::Notifyleave", API::Notifyleave);
        Backend::API::addEndpoint("Social::Groups::Listgroups", API::Listgroups);
    }
}
