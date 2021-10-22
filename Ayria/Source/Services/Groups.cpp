/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-06
    License: MIT
*/

#include <Global.hpp>

namespace Services::Groups
{
    static Hashmap<std::string, std::shared_ptr<Group_t>> Groupcache{};

    // Fetch the group by ID, for use with services.
    std::shared_ptr<Group_t> getGroup(const std::string &LongID)
    {
        // Simplest case, we already have the group cached.
        if (Groupcache.contains(LongID)) [[likely]] return Groupcache[LongID];

        // Get the group from the database.
        if (const auto JSON = AyriaAPI::Groups::Find(LongID))
        {
            const auto Group = fromJSON(JSON);
            if (!Group) [[unlikely]] return {};

            return Groupcache.emplace(LongID, std::make_shared<Group_t>(*Group)).first->second;
        }

        return {};
    }

    // Internal.
    void addMember(const std::string &GroupID, const std::string &MemberID)
    {
        const auto Group = getGroup(GroupID);
        const auto Moderators = AyriaAPI::Groups::getModerators(GroupID);

        // Sanity checking.
        if (!Group) [[unlikely]] return;
        if (Group->isFull) [[unlikely]] return;
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
            if (Group->isFull) [[unlikely]] return false;

            // getModerators also includes the owner.
            const auto Moderators = AyriaAPI::Groups::getModerators(GroupID);

            // Only moderators can add users to private groups.
            if (!Group->isPublic && !Moderators.contains(LongID)) [[unlikely]] return false;

            // Only the groups creator can modify moderators.
            if (isModerator && GroupID != LongID) [[unlikely]] return false;
            if (Moderators.contains(MemberID) && GroupID != LongID) [[unlikely]] return false;

            // Verify that the client wants to join this group.
            if (MemberID != LongID && !AyriaAPI::Presence::Value(LongID, "Grouprequest_"s + GroupID, "AYRIA")) [[unlikely]] return false;

            try
            {
                Backend::Database()
                    << "INSERT INTO Groupmember VALUES (?,?,?);"
                    << MemberID << GroupID << isModerator;
            } catch (...) {}

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
                try
                {
                    Backend::Database()
                        << "DELETE FROM Groupmember WHERE (GroupID = ? AND MemberID = ?);"
                        << GroupID << MemberID;
                } catch (...) {}
                return true;
            }

            // Only the admin can kick a moderator.
            const auto Moderators = AyriaAPI::Groups::getModerators(GroupID);
            if (Moderators.contains(MemberID) && GroupID != LongID) return false;

            // And naturally only moderators can kick users.
            if (!Moderators.contains(LongID) && GroupID != LongID) return false;

            try
            {
                Backend::Database()
                    << "DELETE FROM Groupmember WHERE (GroupID = ? AND MemberID = ?);"
                    << GroupID << MemberID;
            } catch (...) {}
            return true;
        }
        static bool __cdecl onUpdate(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(std::string_view(Message, Length));
            const auto Groupname = Request.value<std::string>("Groupname");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto isPublic = Request.value<bool>("isPublic");
            const auto isFull = Request.value<bool>("isFull");
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return false;

            // The name and visibility can only be changed by the admin.
            if (Group->Groupname != Groupname && GroupID != LongID) return false;
            if (Group->isPublic != isPublic && GroupID != LongID) return false;

            // Moderators can set if the group is full though.
            if (!AyriaAPI::Groups::getModerators(GroupID).contains(GroupID)) return false;

            try
            {
                const auto &Entry = Groupcache[GroupID];
                Entry->Groupname = Groupname;
                Entry->isPublic = isPublic;
                Entry->isFull = isFull;

                Backend::Database()
                    << "INSERT OR REPLACE INTO Group VALUES(?,?,?,?);"
                    << GroupID << Groupname << isPublic << isFull;

                if (isPublic)
                {
                    Backend::Database()
                        << "DELETE FROM Groupkey WHERE GroupID = ?;"
                        << GroupID;
                }
            } catch (...) {}
            return true;
        }
        static bool __cdecl onDestroy(uint64_t, const char *LongID, const char *, unsigned int)
        {
            // No need to save this message.
            if (!getGroup(LongID)) [[unlikely]] return false;

            try
            {
                Backend::Database()
                    << "DELETE FROM Group WHERE GroupID = ?;"
                    << std::string(LongID);
            } catch (...) {};
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
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            const auto Moderators = AyriaAPI::Groups::getModerators(GroupID);
            if (!Group) [[unlikely]] return R"({ "Error" : "Invalid group ID." })";
            if (Group->isFull) [[unlikely]] return R"({ "Error" : "Group is full." })";
            if (!Group->isPublic && !Moderators.contains(Global.getLongID())) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";
            if (Request.contains("isModerator") && GroupID != Global.getLongID()) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";

            // For a private group, we need to ask a moderator to add us.
            if (!Group->isPublic && MemberID != Global.getLongID())
            {
                // Set presence so they know that we are interested.
                Presence::setPresence("Grouprequest_"s + GroupID, "AYRIA", "");

                // Challenge may be null, but is probably an agreed upon password. Implementation dependent.
                const auto Object = JSON::Object_t({ { "GroupID", GroupID }, { "Challenge", Challenge } });
                Messaging::sendMultiusermessage(Moderators, "Group::Joinrequest", JSON::Dump(Object));
                return {};
            }

            // While for public ones, we can just add ourselves.
            Layer1::Publish("Group::Join", JSON::Dump(Request));
            return {};
        }
        static std::string __cdecl onLeave(JSON::Value_t &&Request)
        {
            const auto MemberID = Request.value<std::string>("MemberID");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Group = getGroup(GroupID);

            // Sanity checking.
            if (!Group) [[unlikely]] return R"({ "Error" : "Invalid group ID." })";
            if (AyriaAPI::Groups::getMembers(GroupID).contains(MemberID)) [[unlikely]] return R"({ "Error" : "Invalid member ID." })";

            Layer1::Publish("Group::Leave", JSON::Dump(Request));
            return {};
        }
        static std::string __cdecl onUpdate(JSON::Value_t &&Request)
        {
            const auto Group = fromJSON(Request);

            // Sanity checking.
            if (!Group) [[unlikely]] return R"({ "Required" : "["GroupID", "Groupname", "isPublic", "isFull"]" })";
            if (Group->GroupID != Global.getLongID()) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";

            Backend::Messagebus::Publish("Group::Update", JSON::Dump(toJSON(*Group)));
            return {};
        }
        static std::string __cdecl onDestroy(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<std::string>("GroupID");

            // Sanity checking.
            if (!getGroup(GroupID)) [[unlikely]] return R"({ "Error" : "Invalid group ID." })";
            if (Global.getLongID() != GroupID) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";

            Layer1::Publish("Group::Destroy", JSON::Dump(Request));
            return {};
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        static void __cdecl onGroupupdate(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Group WHERE rowid = ?;" << RowID
                    >> [](const std::string &GroupID, const std::string &Groupname, bool isPublic, bool isFull, uint32_t Membercount)
                    {
                        if (!AyriaAPI::Groups::getMembers(GroupID).contains(Global.getLongID())) return;

                        const auto Notification = JSON::Object_t({
                            { "Membercount", Membercount },
                            { "Groupname", Groupname },
                            { "isPublic", isPublic },
                            { "GroupID", GroupID },
                            { "isFull", isFull }
                        });

                        Backend::Notifications::Publish("Group::onUpdate", JSON::Dump(Notification).c_str());

                    };
            } catch (...) {}
        }
        static void __cdecl onMemberupdate(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Groupmember WHERE rowid = ?;" << RowID
                    >> [](const std::string &MemberID, const std::string &GroupID, bool isModerator)
                    {
                        if (!AyriaAPI::Groups::getMembers(GroupID).contains(Global.getLongID())) return;

                        const auto Notification = JSON::Object_t({
                            { "isModerator", isModerator },
                            { "MemberID", MemberID },
                            { "GroupID", GroupID }
                        });

                        Backend::Notifications::Publish("Group::onMember", JSON::Dump(Notification).c_str());

                    };
            } catch (...) {}
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Group ("
                "GroupID TEXT PRIMARY KEY REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Groupname TEXT NOT NULL, "
                "isPublic BOOLEAN, "
                "isFull BOOLEAN, "
                "Membercount INTEGER DEFAULT 0 );";

            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Groupmember ("
                "MemberID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "GroupID TEXT REFERENCES Group(GroupID) ON DELETE CASCADE, "
                "isModerator BOOLEAN DEFAULT false, "
                "UNIQUE (GroupID, MemberID) );";

            Backend::Database() <<
                "CREATE TRIGGER IF NOT EXISTS MemberINC "
                "AFTER INSERT ON Groupmember "
                "BEGIN "
                "UPDATE Group SET Membercount = (Membercount + 1) "
                "WHERE GroupID = new.GroupID; "
                "END;";

            Backend::Database() <<
                "CREATE TRIGGER IF NOT EXISTS MemberDEC "
                "AFTER DELETE ON Groupmember "
                "BEGIN "
                "UPDATE Group SET Membercount = (Membercount - 1) "
                "WHERE GroupID = old.GroupID; "
                "END;";

        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Group::Join", Messagehandlers::onJoin);
        Backend::Messageprocessing::addMessagehandler("Group::Leave", Messagehandlers::onLeave);
        Backend::Messageprocessing::addMessagehandler("Group::Update", Messagehandlers::onUpdate);
        Backend::Messageprocessing::addMessagehandler("Group::Destroy", Messagehandlers::onDestroy);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Groups::Join", JSONAPI::onJoin);
        Backend::JSONAPI::addEndpoint("Groups::Leave", JSONAPI::onLeave);
        Backend::JSONAPI::addEndpoint("Groups::addMember", JSONAPI::onJoin);
        Backend::JSONAPI::addEndpoint("Groups::kickMember", JSONAPI::onLeave);
        Backend::JSONAPI::addEndpoint("Groups::Updategroup", JSONAPI::onUpdate);
        Backend::JSONAPI::addEndpoint("Groups::Creategroup", JSONAPI::onUpdate);
        Backend::JSONAPI::addEndpoint("Groups::Destroygroup", JSONAPI::onDestroy);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Group", Notifications::onGroupupdate);
        Backend::Notifications::addProcessor("Groupmember", Notifications::onMemberupdate);
    }
}
