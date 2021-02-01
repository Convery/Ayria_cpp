/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-30
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Group
{
    std::deque<Task_t<GroupID_t>> asyncCreation;
    Hashmap<uint64_t, Task_t<bool> *> asyncJoin;
    Nodemap<uint64_t, Group_t> Knowngroups;
    std::queue<Joinrequest_t> Joinrequests;

    // Handle the synchronization for LAN clients.
    static void __cdecl Requesthandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Extradata = Request.value<JSON::Value_t>("Extradata");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Join = Request.value<bool>("Join");

        if (Global.UserID == GroupID_t(GroupID).AdminID && Knowngroups.contains(GroupID))
        {
            if (!Join)  removeMember(GroupID_t(GroupID), Sender->UserID);
            else Joinrequests.push({ GroupID_t(GroupID), Sender->UserID, std::move(Extradata) });
        }
    }
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const JSON::Array_t Array = JSON::Parse(std::string_view(Message, Length));
        for (const auto &Item : Array)
        {
            const auto GroupID = Item.value<uint64_t>("GroupID");
            const auto Members = Item.value<JSON::Array_t>("Members");

            // The sender is clearly confused, but we'll continue.
            if (Sender->UserID != GroupID_t(GroupID).AdminID) [[unlikely]]
            {
                Errorprint(va("Client ID %08X sent an invalid group.", Sender->UserID));
                continue;
            }

            auto &Entry = Knowngroups[GroupID];
            if (!Entry.Members) [[unlikely]]
            {
                Entry.Members = std::make_unique<Hashmap<uint32_t, JSON::Value_t>>();
            }

            for (const auto &Member : Members)
            {
                const auto Memberinfo = Member.value<JSON::Value_t>("Memberinfo");
                const auto UserID = Member.value<uint32_t>("UserID");
                Entry.Members->emplace(UserID, Memberinfo);
            }
        }
    }
    static void __cdecl Resulthandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto ClientID = Request.value<uint32_t>("ClientID");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Result = Request.value<bool>("Accepted");

        if (Global.UserID == ClientID)
        {
            if (asyncJoin.contains(GroupID))
            {
                asyncJoin[GroupID]->Result = Result;
                asyncJoin[GroupID]->hasResult.test_and_set();
            }
        }
    }
    static void __cdecl Synchronizegroups()
    {
        const auto ClientID = Global.UserID;
        JSON::Array_t Array;

        for (const auto &[GroupID, Groupdata] : Knowngroups)
        {
            if (GroupID_t(GroupID).AdminID == ClientID)
            {
                JSON::Array_t Memberarray;
                for (const auto &[ID, Data] : *Groupdata.Members)
                {
                    Memberarray.emplace_back(JSON::Object_t({
                        { "Memberinfo", Data },
                        { "UserID", ID }
                    }));
                }

                Array.emplace_back(JSON::Object_t({
                    { "GroupID", Groupdata.GroupID.Raw },
                    { "Members", Memberarray }
                }));
            }
        }

        if (!Array.empty())
        {
            Backend::Network::Sendmessage("Groupupdate", JSON::Dump(Array));
        }
    }

    // Create a new group as host, optionally request a remote host for it.
    const Task_t<GroupID_t> *Creategroup(bool Local, uint8_t Memberlimit)
    {
        auto Async = &asyncCreation.emplace_back();

        if (Local)
        {
            Group_t Newgroup{};
            Newgroup.GroupID.Limit = Memberlimit;
            Newgroup.GroupID.AdminID = Global.UserID;
            Newgroup.GroupID.RoomID = Hash::WW32(GetTickCount64()) & 0xFFFF;
            Newgroup.Members = std::make_unique<Hashmap<uint32_t, JSON::Value_t>>();

            Async->hasResult.test_and_set();
            Async->Result = Newgroup.GroupID;

            Knowngroups.emplace(Newgroup.GroupID.Raw, std::move(Newgroup));
            Synchronizegroups();
        }
        else
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }

        return Async;
    }

    // Manage the group-members for groups this client is admin of, update inserts a new user.
    void updateMember(GroupID_t GroupID, uint32_t ClientID, JSON::Value_t Clientinfo)
    {
        assert(Knowngroups[GroupID.Raw].Members);
        (*Knowngroups[GroupID.Raw].Members)[ClientID].update(Clientinfo);

        if (GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }
        else
        {
            Synchronizegroups();
        }
    }
    void removeMember(GroupID_t GroupID, uint32_t ClientID)
    {
        assert(Knowngroups[GroupID.Raw].Members);
        (*Knowngroups[GroupID.Raw].Members).erase(ClientID);

        if (GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }
        else
        {
            Synchronizegroups();
        }
    }

    // Manage group-membership.
    const Task_t<bool> *Subscribe(GroupID_t GroupID, JSON::Value_t Extradata)
    {
        auto Async = asyncJoin[GroupID.Raw];
        Async = new Task_t<bool>();

        if (GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            Async->hasResult.test_and_set();
            Async->Result = false;
            assert(false);
        }
        else
        {
            const auto Request = JSON::Object_t({
                { "GroupID", GroupID.Raw },
                { "Extradata", Extradata },
                { "Join", true }
            });

            Backend::Network::Sendmessage("Grouprequest", JSON::Dump(Request));
        }

        return Async;
    }
    void Acceptrequest(Joinrequest_t *Request, JSON::Value_t Clientinfo)
    {
        if (Request->GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }
        else
        {
            const auto Object = JSON::Object_t({
                { "GroupID", Request->GroupID.Raw },
                { "ClientID", Request->ClientID },
                { "Accepted", true }
            });

            Backend::Network::Sendmessage("Groupjoinresult", JSON::Dump(Object));
        }

        updateMember(Request->GroupID, Request->ClientID, Clientinfo);
        Joinrequests.pop();
    }
    void Rejectrequest(Joinrequest_t *Request)
    {
        if (Request->GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }
        else
        {
            const auto Object = JSON::Object_t({
                { "GroupID", Request->GroupID.Raw },
                { "ClientID", Request->ClientID },
                { "Accepted", false }
            });

            Backend::Network::Sendmessage("Groupjoinresult", JSON::Dump(Object));
        }

        Joinrequests.pop();
    }
    void Unsubscribe(GroupID_t GroupID)
    {
        if (GroupID.HostID)
        {
            // TODO(tcn): Contact some remote server.
            assert(false);
        }
        else
        {
            const auto Request = JSON::Object_t({
                { "GroupID", GroupID.Raw },
                { "Join", false }
            });

            assert(Knowngroups[GroupID.Raw].Members);
            (*Knowngroups[GroupID.Raw].Members).erase(Global.UserID);
            Backend::Network::Sendmessage("Grouprequest", JSON::Dump(Request));
        }
    }
    Joinrequest_t *getJoinrequest()
    {
        if (Joinrequests.empty()) return {};
        return &Joinrequests.front();
    }

    // List the groups we know of, optionally filtered by Admin or Member.
    std::vector<const Group_t *> List(const uint32_t *byOwner, const uint32_t *byUser)
    {
        std::vector<const Group_t *> Result;
        Result.reserve(8); // Random guess.

        for (const auto &[GroupID, Group] : Knowngroups)
        {
            if (byUser && !(*Knowngroups[GroupID].Members).contains(*byUser)) continue;
            if (byOwner && GroupID_t(GroupID).AdminID != *byOwner) continue;
            Result.emplace_back(&Group);
        }

        return Result;
    }
    bool isGroupmember(GroupID_t GroupID, uint32_t UserID)
    {
        return Knowngroups.contains(GroupID.Raw) && Knowngroups[GroupID.Raw].Members->contains(UserID);
    }

    // Set up message-handlers.
    void Initialize()
    {
        // As we already call sync whenever we update, this is more as a keep-alive.
        Backend::Enqueuetask(10000, Synchronizegroups);

        // Register the status change handlers.
        Backend::Network::addHandler("Groupupdate", Updatehandler);
        Backend::Network::addHandler("Grouprequest", Requesthandler);
        Backend::Network::addHandler("Groupjoinresult", Resulthandler);
    }
}
