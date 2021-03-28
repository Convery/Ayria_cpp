/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Usergroups
{
    static Hashmap<uint64_t /* GroupID */, std::unordered_set<uint32_t>> Groupmembers;
    static Hashmap<uint64_t /* GroupID */, std::u8string> Groupnames;

    // Helper functionallity.
    bool isMember(uint64_t GroupID, uint32_t ClientID)
    {
        if (GroupID_t{ GroupID }.AdminID == Global.ClientID)
            return Groupmembers[GroupID].contains(ClientID);

        std::vector<uint32_t> Members{};
        Backend::Database() << "SELECT Members FROM Usergroups WHERE GroupID = ?;"
                            << GroupID >> Members;

        return Members.cend() != std::find(Members.cbegin(), Members.cend(), ClientID);
    }

    // Send requests to local clients.
    static void Sendupdate(uint64_t GroupID)
    {
        if (GroupID == 0)
        {
            for (const auto &[ID, _] : Groupnames)
            {
                const auto Response = JSON::Object_t({
                    { "Groupname", Groupnames[ID] },
                    { "Members", Groupmembers[ID] },
                    { "GroupID", ID }
                });
                Backend::Network::Transmitmessage("Usergroups::Update", Response);
            }
        }
        else
        {
            const auto Response = JSON::Object_t({
                { "Groupname", Groupnames[GroupID] },
                { "Members", Groupmembers[GroupID] },
                { "GroupID", GroupID }
            });
            Backend::Network::Transmitmessage("Usergroups::Update", Response);
        }
    }

    // Handle local client-requests.
    static void __cdecl Leavehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // If we are even intested in this group.
        if (GroupID_t{ GroupID }.AdminID != Global.ClientID) return;

        Groupmembers[GroupID].erase(Clientinfo::getClientID(NodeID));
        Sendupdate(GroupID);
    }
    static void __cdecl Statushandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Members = Request.value<std::vector<uint32_t>>("Members");
        const auto Groupname = Request.value<std::string>("Groupname");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Let's not bother with fuckery.
        if (GroupID_t{ GroupID }.AdminID != Clientinfo::getClientID(NodeID))
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // Ensure that the groups are available to the plugins.
        Backend::Database() << "REPLACE ( GroupID, Lastupdate, Groupname, Members ) INTO Usergroups VALUES (?, ?, ?, ?);"
                            << GroupID << uint32_t(time(NULL)) << Groupname << Members;
    }
    static void __cdecl Requesthandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Extradata = Request.value<std::string>("B64Extradata");
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Are we even interested in this?
        if (GroupID_t{ GroupID }.AdminID != Global.ClientID) return;

        // Ensure that the requests are available to the plugins.
        Backend::Database() << "REPLACE ( GroupID, UserID, ProviderID, Timestamp, B64Extradata ) INTO Grouprequests VALUES (?, ?, ?, ?, ?);"
                            << GroupID << Clientinfo::getClientID(NodeID) << ProviderID << uint32_t(time(NULL)) << B64Extradata;
    }
    static void __cdecl Responsehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto isAccepted = Request.value<bool>("isAccepted");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Let's not bother with fuckery.
        if (GroupID_t{ GroupID }.AdminID != Clientinfo::getClientID(NodeID))
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // TODO(tcn): Create some notification system.

        // Clean up the database.
        Backend::Database() << "DELETE FROM Grouprequests WHERE GroupID = ? AND UserID = ? AND ProviderID = ?;"
                            << GroupID << Clientinfo::getClientID(NodeID) << ProviderID;
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
        const auto B64Extradata = Request.value<std::string>("B64Extradata");
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Notify the other clients.
        const auto Response = JSON::Object_t({
            { "B64Extradata", B64Extradata },
            { "ProviderID", ProviderID },
            { "GroupID", GroupID }
        });
        Backend::Network::Transmitmessage("Usergroups::Joinrequest", Response);

        // Ensure that the requests are available to the plugins.
        Backend::Database() << "REPLACE ( GroupID, UserID, ProviderID, Timestamp, B64Extradata ) INTO Grouprequests VALUES (?, ?, ?, ?, ?);"
                            << GroupID << Global.ClientID << ProviderID << uint32_t(time(NULL)) << B64Extradata;

        return "{}";
    }
    static std::string __cdecl Creategroup(JSON::Value_t &&Request)
    {
        const auto Groupname = Request.value<std::u8string>("Groupname");
        const auto Memberlimit = Request.value<uint8_t>("Memberlimit");
        const auto Grouptype = Request.value<uint8_t>("Grouptype");

        GroupID_t GroupID{};
        GroupID.Type = Grouptype;
        GroupID.Limit = Memberlimit;
        GroupID.AdminID = Global.ClientID;
        GroupID.RoomID = GetTickCount() & 0xFFFF;
        Groupnames[GroupID.Raw] = Groupname;

        Backend::Database() << "REPLACE ( GroupID, Lastupdate, Groupname ) INTO Usergroups VALUES (?, ?, ?);"
                            << GroupID.Raw << uint32_t(time(NULL)) << Encoding::toNarrow(Groupname);

        return "{}";
    }
    static std::string __cdecl updateStatus(JSON::Value_t &&Request)
    {
        const auto Members = Request.value<std::vector<uint32_t>>("Members");
        const auto Groupname = Request.value<std::string>("Groupname");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Ensure that the groups are available to the plugins.
        Backend::Database() << "REPLACE ( GroupID, Lastupdate, Groupname, Members ) INTO Usergroups VALUES (?, ?, ?, ?);"
                            << GroupID << uint32_t(time(NULL)) << Groupname << Members;

        // Notify the other clients.
        auto Set = &Groupmembers[GroupID]; Set->clear();
        for (const auto &Item : Members) Set->insert(Item);
        Sendupdate(GroupID);

        return "{}";
    }
    static std::string __cdecl answerRequest(JSON::Value_t &&Request)
    {
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto isAccepted = Request.value<bool>("isAccepted");
        const auto ClientID = Request.value<uint32_t>("ClientID");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Notify the other clients.
        const auto Response = JSON::Object_t({
            { "ProviderID", ProviderID },
            { "isAccepted", isAccepted },
            { "GroupID", GroupID }
        });
        Backend::Network::Transmitmessage("Usergroups::Joinresponse", Response);

        // Clean up the database.
        Backend::Database() << "DELETE FROM Grouprequests WHERE GroupID = ? AND UserID = ? AND ProviderID = ?;"
                            << GroupID << ClientID << ProviderID;

        // Notify the other clients.
        Groupmembers[GroupID].insert(ClientID);
        Sendupdate(GroupID);

        return JSON::Value_t(Groupmembers[GroupID]).dump();
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Usergroups::Joinresponse", Responsehandler);
        Backend::Network::Registerhandler("Usergroups::Joinrequest", Requesthandler);
        Backend::Network::Registerhandler("Usergroups::Notifyleave", Leavehandler);
        Backend::Network::Registerhandler("Usergroups::Update", Statushandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Usergroups::Answerrequest", answerRequest);
        Backend::API::addEndpoint("Usergroups::Requestjoin", Requestjoin);
        Backend::API::addEndpoint("Usergroups::Leavegroup", Leavegroup);
        Backend::API::addEndpoint("Usergroups::Update", updateStatus);
        Backend::API::addEndpoint("Usergroups::Create", Creategroup);

        // Send an update once in a while.
        Backend::Enqueuetask(5000, []() { Sendupdate(0); });

        // Load all the clients from the database.
        Backend::Database() << "SELECT GroupID, Members FROM Usergroups;"
                            >> [](const uint64_t &GroupID, const std::vector<uint32_t> &Members)
                            {
                                if (GroupID_t{ GroupID }.AdminID == Global.ClientID)
                                {
                                    auto Set = &Groupmembers[GroupID];
                                    Set->reserve(Members.size());

                                    for (const auto &Item : Members)
                                        Set->insert(Item);
                                }
                            };
    }
}
