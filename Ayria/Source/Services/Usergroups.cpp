/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Usergroups
{
    static Hashmap<uint64_t /* GroupID */, Hashset<uint32_t>> Groupmembers;

    // Helper functionallity.
    bool isMember(uint64_t GroupID, uint32_t ClientID)
    {
        if (GroupID_t{ GroupID }.AdminID == ClientID)
            return true;

        for (const auto &ID : Groupmembers[GroupID])
        {
            if (ID == ClientID)
                return true;
        }
        return false;
    }

    // Updates about other groups.
    static void __cdecl Leavehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // We only care about requests for our groups.
        if (!isMember(GroupID, Global.ClientID)) return;

        // No need to force an update, other clients are notified later.
        const auto ClientID = Clientinfo::getClientID(NodeID);
        Groupmembers[GroupID].erase(ClientID);

        // TODO(tcn): Notify the plugins.
    }
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto MemberIDs = Request.value<std::vector<uint32_t>>("MemberIDs");
        const auto Memberdata = Request.value<std::string>("Memberdata");
        const auto Groupdata = Request.value<std::string>("Groupdata");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto GameID = Request.value<uint32_t>("GameID");
        const auto Sender = Clientinfo::getClientID(NodeID);

        // We only accept updates from the host.
        if (GroupID_t{ GroupID }.AdminID != Sender) return;

        // Update the database so the plugins can partake.
        try
        {
            Backend::Database()
                << "REPLACE INTO Usergroups (GroupID, Lastupdate, GameID, MemberIDs, Memberdata, Groupdata) "
                   "VALUES (?, ?, ?, ?, ?, ?);"
                << GroupID << time(NULL) << GameID << MemberIDs << Memberdata << Groupdata;
        } catch (...) {}
    }
    static void __cdecl Joinrequesthandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Extradata = Request.value<std::string>("Extradata");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // We only care about requests for our groups.
        if (GroupID_t{ GroupID }.AdminID != Global.ClientID) return;

        // Update the database so the plugins can process this.
        try
        {
            Backend::Database()
                << "INSERT INTO Grouprequests (ClientID, GroupID, Timestamp, Extradata) VALUES (?, ?, ?, ?);"
                << Clientinfo::getClientID(NodeID) << GroupID << time(NULL) << Extradata;
        } catch (...) {}
    }

    // Poll for changes and notify the other clients.
    static void Updatestate()
    {
        // Process group updates.
        const auto Groupupdates = Backend::getDatabasechanges("Usergroups");
        for (const auto &Row : Groupupdates)
        {
            try
            {
                uint64_t GroupID; std::vector<uint32_t> vMemberIDs;

                Backend::Database() << "SELECT GroupID, MemberIDs FROM Usergroups WHERE rowid = ?;"
                                    << Row >> std::tie(GroupID, vMemberIDs);

                Hashset<uint32_t> MemberIDs;
                MemberIDs.reserve(vMemberIDs.size());
                for (const auto &Item : vMemberIDs)
                    MemberIDs.insert(Item);

                auto Oldmembers = Groupmembers[GroupID];
                Groupmembers[GroupID] = MemberIDs;

                // Check for changes in the members.
                for (const auto &Item : Oldmembers) MemberIDs.erase(Item);
                for (const auto &Item : MemberIDs) Oldmembers.erase(Item);

                // TODO(tcn): Notify the plugins about clients joining.
                if (!MemberIDs.empty()) {}

                // TODO(tcn): Notify the plugins about clients leaving.
                if (!Oldmembers.empty()) {}
            }
            catch (...) {}
        }

        // Process new group requests.
        const auto Grouprequests = Backend::getDatabasechanges("Grouprequests");
        for (const auto &Item : Grouprequests)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Grouprequests WHERE rowid = ?"
                    << Item << Global.ClientID
                    >> [](uint32_t ClientID, uint64_t GroupID, uint32_t, const std::string &Extradata)
                    {
                        if (ClientID == Global.ClientID)
                        {
                            const auto Request = JSON::Object_t({
                                { "Extradata", Extradata },
                                { "GroupID", GroupID }
                            });
                            Backend::Network::Transmitmessage("Usergroups::Joinrequest", Request);
                        }
                        else
                        {
                            // TODO(tcn): Notify the plugins.
                        }
                    };
            }
            catch (...) {}
        }

        // Notify the clients about our current groups.
        for (const auto &[GroupID, _] : Groupmembers)
        {
            if (GroupID_t{ GroupID }.AdminID != Global.ClientID) continue;

            try
            {
                Backend::Database() << "SELECT * FROM Usergroups WHERE GroupID = ?;" << GroupID
                    >> [](uint64_t GroupID, uint32_t, uint32_t GameID, std::vector<uint32_t> MemberIDs,
                          const std::string &Memberdata, const std::string &Groupdata)
                    {
                        const auto Request = JSON::Object_t({
                            { "Memberdata", Memberdata },
                            { "Groupdata", Groupdata },
                            { "MemberIDs", MemberIDs },
                            { "GroupID", GroupID },
                            { "GameID", GameID }
                        });
                        Backend::Network::Transmitmessage("Usergroups::Update", Request);
                    };
            }
            catch (...) {}
        }
    }

    // JSON API access for the plugins.
    static std::string __cdecl Leavegroup(JSON::Value_t &&Request)
    {
        const auto GroupID = Request.value<uint64_t>("GroupID");
        Backend::Network::Transmitmessage("Usergroups::Notifyleave", JSON::Object_t({ { "GroupID", GroupID } }));
        return "{}";
    }
    static std::string __cdecl Requestjoin(JSON::Value_t &&Request)
    {
        const auto Extradata = Request.value<std::string>("Extradata");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Notify the other clients.
        const auto Response = JSON::Object_t({
            { "Extradata", Extradata },
            { "GroupID", GroupID }
        });
        Backend::Network::Transmitmessage("Usergroups::Joinrequest", Response);

        // Add a copy to the database incase plugins want to verify.
        try
        {
            Backend::Database()
                << "INSERT INTO Grouprequests (ClientID, GroupID, Timestamp, Extradata) VALUES (?, ?, ?, ?);"
                << Global.ClientID << GroupID << time(NULL) << Extradata;
        } catch (...) {}

        return "{}";
    }
    static std::string __cdecl Creategroup(JSON::Value_t &&Request)
    {
        const auto Groupdata = Request.value<std::string>("Groupdata");
        const auto Memberlimit = Request.value<uint8_t>("Memberlimit");
        const auto Grouptype = Request.value<uint8_t>("Grouptype");
        const auto GameID = Request.value<uint32_t>("GameID");

        GroupID_t GroupID{};
        GroupID.Type = Grouptype;
        GroupID.Limit = Memberlimit;
        GroupID.AdminID = Global.ClientID;
        GroupID.RoomID = GetTickCount() & 0xFFFF;

        // TODO(tcn): Depending on grouptype, request hosting and update the GroupID.

        // Add the group to the database.
        try
        {
            Backend::Database() << "REPLACE INTO Usergroups (GroupID, Lastupdate, GameID, Groupdata) VALUES (?, ?, ?, ?);"
                                << GroupID.Raw << time(NULL) << GameID << Groupdata;
        } catch (...) {}

        return "{}";
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Usergroups::Joinrequest", Joinrequesthandler);
        Backend::Network::Registerhandler("Usergroups::Notifyleave", Leavehandler);
        Backend::Network::Registerhandler("Usergroups::Update", Updatehandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Usergroups::Requestjoin", Requestjoin);
        Backend::API::addEndpoint("Usergroups::Leavegroup", Leavegroup);
        Backend::API::addEndpoint("Usergroups::Create", Creategroup);

        // Send an update once in a while.
        Backend::Enqueuetask(5000, Updatestate);

        // Load all the groups from the database.
        try
        {
            Backend::Database()
                << "SELECT GroupID, Members FROM Usergroups;"
                >> [](uint64_t GroupID, const std::vector<uint32_t> &Members)
                {
                    auto Entry = &Groupmembers[GroupID];
                    Entry->reserve(Members.size());
                    for (const auto &Item : Members) Entry->insert(Item);
                };
        }
        catch (...) {}
    }
}
