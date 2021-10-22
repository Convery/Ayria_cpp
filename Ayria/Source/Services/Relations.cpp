/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-06
    License: MIT
*/

#include <Global.hpp>

namespace Services::Relations
{
    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onUpdate(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            // We don't accept deltas, we need all the properties.
            const auto Request = JSON::Parse(std::string_view(Message, Length));
            if (!Request.contains_all("Target", "isFriend", "isBlocked")) [[unlikely]] return false;

            try
            {
                Backend::Database()
                    << "INSERT OR REPLACE INTO Relation VALUES (?,?,?,?);"
                    << std::string(LongID) << Request.value<std::string>("Target")
                    << Request.value<bool>("isBlocked") << Request.value<bool>("isFriend");
            } catch (...) {}

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string Wrapper(JSON::Value_t &&Request, bool isFriend, bool isBlocked)
        {
            // Not sure what the caller expects of us..
            if (!Request.contains("ClientID")) [[unlikely]]
                return R"({ "Error" : "No client ID provided." })";

            // Validate that the client is in the DB.
            if (!Clientinfo::getClient(Request.value<std::string>("ClientID"))) [[unlikely]]
                return R"({ "Error" : "Client ID is not in the DB (yet?)." })";

            const auto Object = JSON::Object_t({
                { "Target", Request["ClientID"] },
                { "isBlocked", isBlocked },
                { "isFriend", isFriend }
            });
            Backend::Messagebus::Publish("Relation::Update", JSON::Dump(Object));
            return {};
        }

        static std::string __cdecl Clear(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), false, false);
        }
        static std::string __cdecl Block(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), false, true);
        }
        static std::string __cdecl Befriend(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), true, false);
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        static void __cdecl onUpdate(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Relation WHERE rowid = ?;" << RowID
                    >> [](const std::string &Source, const std::string &Target, bool isBlocked, bool isFriend)
                    {
                        // Not interested in this.
                        if (Target != Global.getLongID()) return;

                        // We are now mutual friends.
                        if (isFriend && AyriaAPI::Relations::Get(Target, Source).first)
                        {
                            Backend::Notifications::Publish("Relation::onFriendship", JSON::Dump(JSON::Value_t{ Source }).c_str());
                            return;
                        }

                        // They asked to be our friend.
                        if (isFriend)
                        {
                            Backend::Notifications::Publish("Relation::onFriendrequest", JSON::Dump(JSON::Value_t{ Source }).c_str());
                            return;
                        }

                        // NOTE(tcn): Not sure if we should publish this change. It's not secret, but maybe unnecessary.
                        if (isBlocked)
                        {
                            Backend::Notifications::Publish("Relation::onBlocked", JSON::Dump(JSON::Value_t{ Source }).c_str());
                            return;
                        }

                        // Relation state cleared, interpret that how you will..
                        Backend::Notifications::Publish("Relation::onReset", JSON::Dump(JSON::Value_t{ Source }).c_str());
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
                "CREATE TABLE IF NOT EXISTS Relation ("
                "Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Target TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "isBlocked BOOLEAN NOT NULL, "
                "isFriend BOOLEAN NOT NULL, "
                "UNIQUE (Source, Target) );";
        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Relation::Update", Messagehandlers::onUpdate);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Relation::Clear", JSONAPI::Clear);
        Backend::JSONAPI::addEndpoint("Relation::Block", JSONAPI::Block);
        Backend::JSONAPI::addEndpoint("Relation::Befriend", JSONAPI::Befriend);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Relation", Notifications::onUpdate);
    }
}
