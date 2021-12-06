/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-06
    License: MIT
*/

#include <Global.hpp>

namespace Services::Groups
{
    static Hashmap<std::string, std::shared_ptr<Group_t>> Groupcache{};
    static Hashset<std::u8string> Groupnamecache{};
    static Hashset<std::string> Publickeycache{};

    // Format between C++ and JSON representations.
    std::optional<Group_t> fromJSON(const JSON::Value_t &Object)
    {
        // The required fields, we wont do partial parsing.
        if (!Object.contains_all("GroupID", "isPublic", "Maxmembers")) [[unlikely]] return {};

        Group_t Group{};
        Group.isPublic = Object.value<bool>("isPublic");
        Group.Maxmembers = Object.value<uint32_t>("Maxmembers");
        Group.Membercount = Object.value<uint32_t>("Membercount");
        Group.GroupID = *Publickeycache.insert(Object.value<std::string>("GroupID")).first;
        Group.Groupname = *Groupnamecache.insert(Object.value<std::u8string>("Groupname")).first;

        return Group;
    }
    std::optional<Group_t> fromJSON(std::string_view JSON)
    {
        return fromJSON(JSON::Parse(JSON));
    }
    JSON::Object_t toJSON(const Group_t &Group)
    {
        return JSON::Object_t({
            { "GroupID", Group.GroupID },
            { "isPublic", Group.isPublic },
            { "Groupname", Group.Groupname },
            { "Maxmembers", Group.Maxmembers },
            { "Membercount", Group.Membercount }
        });
    }

    // Fetch the group by ID, for use with services.
    std::shared_ptr<Group_t> getGroup(const std::string &LongID)
    {
        // Simplest case, we already have the group cached.
        if (Groupcache.contains(LongID)) [[likely]] return Groupcache[LongID];

        // Check if it needs updating.
        const auto Timestamp = (std::chrono::utc_clock::now() - std::chrono::seconds(10)).time_since_epoch().count();
        if (Groupcache.contains(LongID) && Groupcache[LongID]->Lastupdated > Timestamp)
        {
            return Groupcache[LongID];
        }

        // Get the group from the database.
        if (auto Group = fromJSON(AyriaAPI::Groups::Fetch(LongID)))
        {
            Group->Lastupdated = std::chrono::utc_clock::now().time_since_epoch().count();
            return Groupcache.emplace(LongID, std::make_shared<Group_t>(*Group)).first->second;
        }

        return {};
    }

    // Helpers, includes the groups owner even if not listed as a member (for some reason).
    static std::unordered_set<std::string> getModerators(const std::string &GroupID)
    {
        auto Temp = AyriaAPI::Groups::getModerators(GroupID);
        Temp.push_back(GroupID);

        return { Temp.begin(), Temp.end() };
    }
    static std::unordered_set<std::string> getMembers(const std::string &GroupID)
    {
        auto Temp = AyriaAPI::Groups::getMembers(GroupID);
        Temp.push_back(GroupID);

        return { Temp.begin(), Temp.end() };
    }

    // Internal.
    void addMember(const std::string &GroupID, const std::string &MemberID)
    {
        const auto Moderators = getModerators(GroupID);
        const auto Group = getGroup(GroupID);

        // Sanity checking.
        if (!Group) [[unlikely]] return;
        if (Group->isFull()) [[unlikely]] return;
        if (!Moderators.contains(Global.getLongID())) [[unlikely]] return;

        const auto Request = JSON::Object_t({
            { "MemberID", MemberID },
            { "GroupID", GroupID }
        });
        Layer1::Publish("Group::Join", JSON::Dump(Request));
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onJoin(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(std::string_view(Message, Length));
            const auto MemberID = Request.value<std::string>("MemberID");
            const auto isModerator = Request.value<bool>("isModerator");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return false;
            if (Group->isFull()) [[unlikely]] return false;

            // getModerators also includes the owner.
            const auto Moderators = getModerators(GroupID);

            // Only moderators can add users to private groups.
            if (!Group->isPublic && !Moderators.contains(LongID)) [[unlikely]] return false;

            // Only the groups creator can modify moderators.
            if (isModerator && GroupID != LongID) [[unlikely]] return false;
            if (Moderators.contains(MemberID) && GroupID != LongID) [[unlikely]] return false;

            // Verify that the client wants to join this group.
            const auto Presence = AyriaAPI::Keyvalue::Get(MemberID, Hash::WW32("AYRIA"), u8"Grouprequest_" + Encoding::toUTF8(GroupID));
            if (MemberID != LongID && !Presence) [[unlikely]] return false;

            // Are we interested in this group?
            if (getMembers(GroupID).contains(Global.getLongID()))
            {
                const auto Notification = JSON::Object_t({
                    { "isModerator", isModerator },
                    { "MemberID", MemberID },
                    { "GroupID", GroupID }
                });

                Layer4::Publish("Group::onJoin", Notification);
            }

            // And store in the database.
            Backend::Database()
                << "INSERT INTO Groupmember VALUES (?,?,?);"
                << GroupID << MemberID << isModerator;

            return true;
        }
        static bool __cdecl onLeave(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(std::string_view(Message, Length));
            const auto MemberID = Request.value<std::string>("MemberID");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return false;

            // No extra permission is needed to leave.
            if (MemberID == LongID)
            {
                Backend::Database()
                    << "DELETE FROM Groupmember WHERE (GroupID = ? AND MemberID = ?);"
                    << GroupID << MemberID;
                return true;
            }

            // Only the admin can kick a moderator.
            const auto Moderators = getModerators(GroupID);
            if (Moderators.contains(MemberID) && GroupID != LongID) return false;

            // And naturally only moderators can kick users.
            if (!Moderators.contains(LongID) && GroupID != LongID) return false;

            // Are we interested in this group?
            if (getMembers(GroupID).contains(Global.getLongID()))
            {
                const auto Notification = JSON::Object_t({
                    { "MemberID", MemberID },
                    { "GroupID", GroupID }
                });

                Layer4::Publish("Group::onLeave", Notification);
            }

            // And store in the database.
            Backend::Database()
                << "DELETE FROM Groupmember WHERE (GroupID = ? AND MemberID = ?);"
                << GroupID << MemberID;

            return true;
        }
        static bool __cdecl onUpdate(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(std::string_view(Message, Length));
            const auto Groupname = Request.value<std::u8string>("Groupname");
            const auto Maxmembers = Request.value<uint32_t>("Maxmembers");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto isPublic = Request.value<bool>("isPublic");

            // Sanity checking.
            const auto Group = getGroup(GroupID);
            if (!Group && GroupID != LongID) [[unlikely]] return false;

            // Most properties can only be changed by the admin.
            if (GroupID == LongID)
            {
                auto New = fromJSON(Request);
                if (!New) [[unlikely]] return false;

                New->Lastupdated = std::chrono::utc_clock::now().time_since_epoch().count();
                Groupcache[GroupID] = std::make_shared<Group_t>(*New);

                // Are we interested in this group?
                if (getMembers(GroupID).contains(Global.getLongID()))
                {
                    Layer4::Publish("Group::onUpdate", toJSON(*New));
                }

                // And store in the database.
                Backend::Database() <<
                    "INSERT INTO Group VALUES (?, ?, ?, ?) ON CONFLICT DO "
                    "UPDATE SET Groupname = ?, Maxmembers = ?, isPublic = ?;"
                    << GroupID << Groupname << Maxmembers << isPublic
                    << Groupname << Maxmembers << isPublic;

                return true;
            }

            // Can't change anything..
            if (!getModerators(GroupID).contains(LongID)) [[unlikely]] return false;

            // Unmodifiable values.
            if (Group->Groupname != Groupname) return false;
            if (Group->isPublic != isPublic) return false;

            auto New = fromJSON(Request);
            if (!New) [[unlikely]] return false;

            New->Lastupdated = std::chrono::utc_clock::now().time_since_epoch().count();
            Groupcache[GroupID] = std::make_shared<Group_t>(*New);

            // Are we interested in this group?
            if (getMembers(GroupID).contains(Global.getLongID()))
            {
                Layer4::Publish("Group::onUpdate", toJSON(*New));
            }

            // And store in the database.
            Backend::Database() <<
                "INSERT INTO Group VALUES (?, ?, ?, ?) ON CONFLICT DO "
                "UPDATE SET Groupname = ?, Maxmembers = ?, isPublic = ?;"
                << GroupID << Groupname << Maxmembers << isPublic
                << Groupname << Maxmembers << isPublic;

            return true;
        }
        static bool __cdecl onDestroy(uint64_t, const char *LongID, const char *, unsigned int)
        {
            const auto Group = getGroup(LongID);

            // No need to save this message.
            if (!Group) [[unlikely]] return false;

            // Are we interested in this group?
            if (getMembers(LongID).contains(Global.getLongID()))
            {
                Layer4::Publish("Group::onDelete", toJSON(*Group));
            }

            // And update the database.
            Backend::Database() << "DELETE FROM Group WHERE GroupID = ?;" << LongID;
            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl onJoin(JSON::Value_t &&Request)
        {
            const auto Challenge = Request.value<std::string>("Challenge");
            const auto MemberID = Request.value<std::string>("MemberID");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Moderators = getModerators(GroupID);
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return R"({ "Error" : "Invalid group ID." })";
            if (Group->isFull()) [[unlikely]] return R"({ "Error" : "Group is full." })";

            // Permission check.
            if (!Group->isPublic && !Moderators.contains(Global.getLongID())) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";
            if (Request.contains("isModerator") && GroupID != Global.getLongID()) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";

            // For a private group, a moderator needs to invite us.
            if (!Group->isPublic && MemberID != Global.getLongID())
            {
                // Set presence so they know that we are interested.
                Keyvalues::setPresence("Grouprequest_"s + GroupID, {});

                // Challenge may be null, but is probably an agreed upon password. Implementation dependent.
                const auto Object = JSON::Object_t({ { "GroupID", GroupID }, { "Challenge", Challenge } });
                Messaging::sendMulticlientmessage(Moderators, Hash::WW32("Group::Joinrequest"), JSON::Dump(Object));
                return {};
            }

            // And store in the database.
            Backend::Database()
                << "INSERT INTO Groupmember VALUES (?,?,?);"
                << GroupID << MemberID << bool(Request["isModerator"]);

            // While for public ones, we can just add ourselves.
            Layer1::Publish("Group::Join", JSON::Dump(Request));
            return {};
        }
        static std::string __cdecl onLeave(JSON::Value_t &&Request)
        {
            const auto MemberID = Request.value("MemberID", Global.getLongID());
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return R"({ "Error" : "Invalid group ID." })";
            if (!getMembers(GroupID).contains(MemberID)) [[unlikely]] return R"({ "Error" : "Not a member of group." })";

            // And store in the database.
            Backend::Database()
                << "DELETE FROM Groupmember WHERE (GroupID = ? AND MemberID = ?);"
                << GroupID << MemberID;

            // Could also check if the MemberID is that of a moderator, but meh..
            Layer1::Publish("Group::Leave", JSON::Object_t({ {"MemberID", MemberID}, {"GroupID", GroupID} }));
            return {};
        }
        static std::string __cdecl onUpdate(JSON::Value_t &&Request)
        {
            const auto Group = fromJSON(Request);

            // Sanity checking.
            if (!Group) [[unlikely]] return R"({ "Required" : "["GroupID", "isPublic", "Maxmembers"]" })";

            // And store in the database.
            Backend::Database()
                << "INSERT OR REPLACE INTO Group VALUES (?, ?, ?, ?);"
                << Group->GroupID << Group->Groupname << Group->Maxmembers << Group->isPublic;

            Layer1::Publish("Group::Update", JSON::Dump(toJSON(*Group)));
            return {};
        }
        static std::string __cdecl onDestroy(JSON::Value_t &&)
        {
            // Sanity checking.
            if (!getGroup(Global.getLongID())) [[unlikely]] return R"({ "Error" : "No active group for this client." })";

            // And update the database.
            Backend::Database() << "DELETE FROM Group WHERE GroupID = ?;" << Global.getLongID();

            Layer1::Publish("Group::Destroy", "");
            return {};
        }

        static std::string __cdecl reKeygroup(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Cryptobase = Request["Cryptobase"].Type == JSON::Type_t::Unsignedint ?
                uint64_t(Request["Cryptobase"]) : Hash::WW64(Request["Cryptobase"].get<std::u8string>());

            // Basic sanity-checking.
            if (!getModerators(GroupID).contains(Global.getLongID())) [[unlikely]]
                return R"({ "Error" : "Invalid GroupID" })";

            const auto Object = JSON::Object_t({ {"Cryptobase", Cryptobase} });
            Messaging::sendGroupmessage(GroupID, Hash::WW32("Ayria::reKey"), JSON::Dump(Object));
            return {};
        }
    }

    // Layer 4 interactions.
    namespace Notifications
    {
        static void __cdecl onKeychange(const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto Messagetype = Request.value<uint32_t>("Messagetype");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto ClientID = Request.value<std::string>("From");

            // Only interested in a single message.
            if (Messagetype != Hash::WW32("Ayria::reKey")) [[likely]]
                return;

            // Sanity checking.
            if (!getModerators(GroupID).contains(ClientID)) [[unlikely]]
                return;

            const auto Payload = JSON::Parse(Request.value<std::string>("Payload"));
            const auto Cryptobase = Payload["Cryptobase"];

            // Either the database is out of sync, or the client is malicious.
            if (!Cryptobase || Cryptobase.Type != JSON::Type_t::Unsignedint) [[unlikely]]
            {
                // TODO(tcn): Maybe report this to some security service that tracks bad clients..
                Warningprint(va("Client %s wants to re-key group %s, but they are not a moderator..", ClientID.c_str(), GroupID.c_str()));
                return;
            }

            // Add the new value to the database.
            Backend::Database()
                << "INSERT OR REPLACE INTO Groupkey VALUES (?, ?);"
                << GroupID << uint64_t(Cryptobase);
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Group ("
            "GroupID TEXT PRIMARY KEY REFERENCES Account(Publickey) ON DELETE CASCADE, "
            "Groupname TEXT "
            "Maxmembers INTEGER NOT NULL, "
            "isPublic BOOLEAN NOT NULL );";

        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Groupmember ("
            "GroupID TEXT REFERENCES Group(GroupID) ON DELETE CASCADE, "
            "MemberID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
            "isModerator BOOLEAN DEFAULT false, "
            "UNIQUE (GroupID, MemberID) );";

        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Groupkey ("
            "GroupID TEXT PRIMARY KEY REFERENCES Group(GroupID) ON DELETE CASCADE, "
            "Cryptobase INTEGER DEFAULT 0 );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Group::Join", Messagehandlers::onJoin);
        Layer2::addMessagehandler("Group::Leave", Messagehandlers::onLeave);
        Layer2::addMessagehandler("Group::Update", Messagehandlers::onUpdate);
        Layer2::addMessagehandler("Group::Destroy", Messagehandlers::onDestroy);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Groups::Join", JSONAPI::onJoin);
        Layer3::addEndpoint("Groups::Leave", JSONAPI::onLeave);
        Layer3::addEndpoint("Groups::reKey", JSONAPI::reKeygroup);
        Layer3::addEndpoint("Groups::addMember", JSONAPI::onJoin);
        Layer3::addEndpoint("Groups::kickMember", JSONAPI::onLeave);
        Layer3::addEndpoint("Groups::Updategroup", JSONAPI::onUpdate);
        Layer3::addEndpoint("Groups::Creategroup", JSONAPI::onUpdate);
        Layer3::addEndpoint("Groups::Destroygroup", JSONAPI::onDestroy);

        // Process Layer 4 updates.
        Layer4::Subscribe("Messaging::onGroupmessage", Notifications::onKeychange);
    }
}
