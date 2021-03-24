/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-30
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Groups
{
    constexpr uint8_t Localhost = 0, Clanhost = 0xFF;
    struct Joinrequest_t
    {
        uint64_t GroupID;
        uint32_t ClientID;
        JSON::Value_t Extradata;
    };
    union GroupID_t
    {
        uint64_t Raw;
        struct
        {
            uint32_t AdminID;   // Creator.
            uint16_t RoomID;    // Random.
            uint8_t HostID;     // Server, localhost, or clan.
            uint8_t Limit;      // Users.
        };
    };
    struct Group_t
    {
        GroupID_t Identifier;
        std::u8string Friendlyname;
        Hashset<uint32_t> MemberIDs;
        Hashmap<uint32_t, JSON::Value_t> Memberdata;
    };
    static Hashmap<uint64_t, Group_t> Knowngroups{};
    static Hashset<uint64_t> Recentlyupdated{};

    // Verify membership.
    bool isMember(uint64_t GroupID, uint32_t UserID)
    {
        if (Knowngroups.contains(GroupID))
        {
            return Knowngroups[GroupID].MemberIDs.contains(UserID);
        }

        return false;
    }
    bool isAdmin(uint64_t GroupID, uint32_t UserID)
    {
        return GroupID_t{ GroupID }.AdminID == UserID && Knowngroups.contains(GroupID);
    }

    // Notify the world about our groups.
    static void onGroupstatuschange(uint64_t GroupID)
    {
        if (!isAdmin(GroupID, Global.UserID)) return;
        if (!Knowngroups.contains(GroupID)) return;

        const auto &Group = Knowngroups[GroupID];
        std::vector<uint32_t> MemberIDs{};
        JSON::Object_t Memberdata{};

        MemberIDs.reserve(Group.MemberIDs.size());
        for (const auto &ID : Group.MemberIDs) MemberIDs.emplace_back(ID);
        for (const auto &[ID, Data] : Group.Memberdata) Memberdata[va("%u", ID)] = Data;

        // Keep the data as a string.
        const auto JSONData = JSON::Dump(Memberdata);

        // Save information about the group for later.
        Backend::Database() << "insert or replace into Groups values (GroupID, Lastseen, MemberIDs, Memberdata, Groupname) values(?, ?, ?, ?, ?);"
                            << GroupID << uint32_t(time(NULL)) << MemberIDs << JSONData << Encoding::toNarrow(Group.Friendlyname);

        // Announce the change to LAN clients.
        const auto Request = JSON::Object_t({
            { "Groupname", Group.Friendlyname },
            { "Memberdata", JSONData },
            { "MemberIDs", MemberIDs },
            { "GroupID", GroupID }
        });
        Backend::Network::Transmitmessage("Groups::Statuschange", Request);
        Recentlyupdated.insert(GroupID);
    }
    static void onJoinrequest(uint64_t GroupID, JSON::Value_t Extradata)
    {
        const auto Request = JSON::Object_t({
            { "Extradata", Extradata },
            { "GroupID", GroupID }
        });

        Backend::Network::Transmitmessage("Groups:Joinrequest", Request);
    }
    static void __cdecl Statushandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Sender = Clientinfo::getLANClient(NodeID);

        if (!Sender || Sender->UserID != GroupID_t{ GroupID }.AdminID)
            return;

        const auto MemberIDs = Request.value<JSON::Array_t>("MemberIDs");
        const auto Memberdata = Request.value<std::string>("Memberdata");
        const auto Groupname = Request.value<std::string>("Groupname");

        // Save information about the group for later.
        Backend::Database() << "insert or replace into Groups values (GroupID, Lastseen, MemberIDs, Memberdata, Groupname) values (?, ?, ?, ?, ?);"
                            << GroupID << uint32_t(time(NULL)) << MemberIDs << Memberdata << Groupname;

        const JSON::Object_t Map = JSON::Parse(Memberdata);
        Group_t Group{};

        for (const auto &[Key, Value] : Map) Group.Memberdata[std::atol(Key.c_str())] = Value;
        for (const auto &ID : MemberIDs) Group.MemberIDs.insert(ID);
        Group.Friendlyname = Encoding::toUTF8(Groupname);
        Group.Identifier.Raw = GroupID;

        Knowngroups[GroupID] = Group;
    }
    static void __cdecl Joinhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Extradata = Request.value<JSON::Value_t>("Extradata");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Sanity checking.
        if (isAdmin(GroupID, Global.UserID)) return;

        // Limited to LAN clients for now.
        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender) return;

        Backend::Database() << "insert or replace into Grouprequests values (Lastseen, GroupID, UserID, Extradata) values (?, ?, ?, =);"
                            << uint32_t(time(NULL)) << GroupID << Sender->UserID << Extradata.dump();
    }
    static void __cdecl Leavehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Sanity checking.
        if (isAdmin(GroupID, Global.UserID)) return;
        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender) return;

        // Clean up the leaving clients state.
        Knowngroups[GroupID].Memberdata.erase(Sender->UserID);
        Knowngroups[GroupID].MemberIDs.erase(Sender->UserID);
        onGroupstatuschange(GroupID);
    }

    // Update once in a while.
    static void Announce()
    {
        // If we are hidden, don't communicate.
        if (Global.Stateflags.isPrivate) return;

        // Clear outdated groups.
        Backend::Database() << "select GroupID from Groups where Lastseen < ?;" << uint32_t(time(NULL) - 20) >>
        [&](uint64_t ID)
        {
            Knowngroups.erase(ID);
            Backend::Database() << "delete from Groups where GroupID = ?;" << ID;
        };

        // Notify about each of our groups.
        std::for_each(std::execution::par_unseq, Knowngroups.begin(), Knowngroups.end(),
        [](const auto &Pair)
        {
            const auto &[ID, Group] = Pair;
            if (GroupID_t{ ID }.AdminID == Global.UserID && !Recentlyupdated.contains(ID))
            {
                onGroupstatuschange(ID);
            }
        });
        Recentlyupdated.clear();
    }

    // JSON interface for the plugins.
    namespace API
    {
        // Create a new group as host, optionally request a remote host for it.
        static std::string __cdecl Creategroup(JSON::Value_t &&Request)
        {
            const auto Groupname = Request.value<std::u8string>("Groupname");
            const auto Memberlimit = Request.value<uint8_t>("Memberlimit");
            const auto isLocalhost = Request.value<bool>("isLocalhost");
            const auto isClan = Request.value<bool>("isClan");

            // Sanity checking.
            if (!Global.Stateflags.isOnline && !isLocalhost) [[unlikely]]
                return R"({ "Error" : "Can only request hosting if online." })";

            if (!isLocalhost)
            {
                // TODO(tcn): Request a server somewhere.
                return R"({ "Error" : "Not implemented." })";
            }

            Group_t Newgroup{};
            Newgroup.Identifier.HostID = isClan ? Clanhost : Localhost;
            Newgroup.Identifier.RoomID = Hash::WW32(GetTickCount());
            Newgroup.Identifier.AdminID = Global.UserID;
            Newgroup.MemberIDs.insert(Global.UserID);
            Newgroup.Identifier.Limit = Memberlimit;
            Newgroup.Friendlyname = Groupname;

            Knowngroups[Newgroup.Identifier.Raw] = Newgroup;
            onGroupstatuschange(Newgroup.Identifier.Raw);

            return va("{ \"GroupID\" : %016X }", Newgroup.Identifier.Raw);
        }
        static std::string __cdecl Deletegroup(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            if (isAdmin(GroupID, Global.UserID)) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    Knowngroups.erase(GroupID);
                }
            }

            return "{}";
        }

        // Manage the group-members for groups this client is admin of.
        static std::string __cdecl addMember(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto UserID = Request.value<uint32_t>("UserID");

            if (isAdmin(GroupID, Global.UserID)) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Knowngroups.contains(GroupID)) [[likely]]
                    {
                        Knowngroups[GroupID].MemberIDs.insert(UserID);
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

            if (isAdmin(GroupID, Global.UserID)) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Knowngroups.contains(GroupID)) [[likely]]
                    {
                        Knowngroups[GroupID].Memberdata.erase(UserID);
                        Knowngroups[GroupID].MemberIDs.erase(UserID);
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
                if (Knowngroups.contains(GroupID)) [[likely]]
                {
                    const auto Group = &Knowngroups[GroupID];
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

            if (isAdmin(GroupID, Global.UserID)) [[likely]]
            {
                // If hosted remotely.
                if (GroupID_t{ GroupID }.HostID) [[unlikely]]
                {
                    // TODO(tcn): Notify a server or something.
                    return R"({ "Error" : "Not implemented." })";
                }
                else
                {
                    if (Knowngroups.contains(GroupID)) [[likely]]
                    {
                        Knowngroups[GroupID].Memberdata[UserID] = Memberdata;
                        onGroupstatuschange(GroupID);
                    }
                }
            }

            return "{}";
        }

        // Manage invitations from other users.
        static std::string __cdecl getJoinrequests(JSON::Value_t &&)
        {
            JSON::Array_t Result;

            Backend::Database() << "select * from Grouprequests;" >>
            [&](uint32_t, uint64_t GroupID, uint32_t ClientID, const std::string &Extradata)
            {
                Result.emplace_back(JSON::Object_t({
                    { "Extradata", JSON::Parse(Extradata) },
                    { "ClientID", ClientID },
                    { "GroupID", GroupID }
                }));
            };

            return JSON::Dump(Result);
        }
        static std::string __cdecl answerJoinrequestequest(JSON::Value_t &&Request)
        {
            const auto ClientID = Request.value<uint32_t>("ClientID");
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto Accepted = Request.value<bool>("Accepted");

            // If hosted remotely.
            if (GroupID_t{ GroupID }.HostID) [[unlikely]]
            {
                // TODO(tcn): Notify a server or something.
                return R"({ "Error" : "Not implemented." })";
            }
            else
            {
                uint32_t Count{};
                if (GroupID)
                    Backend::Database() << "select count(*) from Grouprequests where GroupID = ? and ClientID = ?;" << GroupID << ClientID >> Count;
                else
                    Backend::Database() << "select count(*) from Grouprequests where ClientID = ?;" << ClientID >> Count;

                // If it's even a valid request.
                if (Count && Accepted) addMember(std::move(Request));

                // Clear this and any outdated.
                Backend::Database() << "delete from Grouprequests where GroupID = ? or Lastseen < ?;" << GroupID << uint32_t(time(NULL) - 300);
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
                if (Knowngroups.contains(GroupID))
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
                Backend::Network::Transmitmessage("Groups::Leave", JSON::Object_t({{ "GroupID", GroupID } }));
            }

            return "{}";
        }

        // List the groups that we know of.
        static std::string __cdecl Listgroups(JSON::Value_t &&)
        {
            JSON::Array_t Groups;
            Groups.reserve(Knowngroups.size());

            for (const auto &[GroupID, Entry] : Knowngroups)
            {
                std::vector<uint32_t> MemberIDs;
                MemberIDs.reserve(Entry.MemberIDs.size());

                for (const auto &ID : Entry.MemberIDs)
                    MemberIDs.emplace_back(ID);

                Groups.emplace_back(JSON::Object_t({
                    { "Groupname", Entry.Friendlyname },
                    { "MemberIDs", MemberIDs },
                    { "GroupID", GroupID }
                }));
            }

            return JSON::Dump(Groups);
        }
    }

    // Set up message-handlers.
    void Initialize()
    {
        Backend::Database() << "create table if not exists Groups ("
                               "GroupID integer primary key unique not null, "
                               "Lastseen integer, "
                               "MemberIDs blob, "
                               "Memberdata text, "
                               "Groupname text);";
        Backend::Database() << "create table if not exists Grouprequests ("
                               "Lastseen integer, "
                               "GroupID integer, "
                               "UserID integer, "
                               "Extradata text);";

        // Load existing data from disk.
        Backend::Database() << "select * from Groups;" >>
        [](uint64_t GroupID, uint32_t, const std::vector<uint32_t> &MemberIDs, const std::string &Memberdata, const std::string &Groupname)
        {
            Group_t Group{};
            Group.Identifier.Raw = GroupID;
            Group.Friendlyname = Encoding::toUTF8(Groupname);

            Group.MemberIDs.reserve(MemberIDs.size());
            for (const auto &ID : MemberIDs) Group.MemberIDs.insert(ID);

            const JSON::Object_t Map = JSON::Parse(Memberdata);
            for (const auto &[Key, Value] : Map) Group.Memberdata[std::atol(Key.c_str())] = Value;

            Knowngroups.emplace(GroupID, std::move(Group));
        };

        // Member-data can be large so limit updates.
        Backend::Enqueuetask(10000, Announce);

        // Register the status change handlers.
        Backend::Network::Registerhandler("Groups::Leave", Leavehandler);
        Backend::Network::Registerhandler("Groups::Joinrequest", Joinhandler);
        Backend::Network::Registerhandler("Groups::Statuschange", Statushandler);

        // JSON endpoints.
        Backend::API::addEndpoint("Groups::Creategroup", API::Creategroup);
        Backend::API::addEndpoint("Groups::Deletegroup", API::Deletegroup);
        Backend::API::addEndpoint("Groups::addMember", API::addMember);
        Backend::API::addEndpoint("Groups::removeMember", API::removeMember);
        Backend::API::addEndpoint("Groups::getMemberdata", API::getMemberdata);
        Backend::API::addEndpoint("Groups::setMemberdata", API::setMemberdata);
        Backend::API::addEndpoint("Groups::getJoinrequests", API::getJoinrequests);
        Backend::API::addEndpoint("Groups::answerJoinrequestequest", API::answerJoinrequestequest);
        Backend::API::addEndpoint("Groups::Requestjoin", API::Requestjoin);
        Backend::API::addEndpoint("Groups::Notifyleave", API::Notifyleave);
        Backend::API::addEndpoint("Groups::Listgroups", API::Listgroups);
    }
}
