/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-06
    License: MIT
*/

#include <Global.hpp>

namespace Services::Presence
{
    // Helper for other services to set presence.
    void setPresence(const std::string &Key, const std::string &Category, const std::optional<std::string> &Value)
    {
        auto Object = JSON::Object_t({ { "Key", Key }, { "Category", Category } });
        if (Value) Object["Value"] = *Value;

        Backend::Messagebus::Publish("Presence::Insert", "[ "s + JSON::Dump(Object) + " ]");
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onInsert(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

            for (const auto &Item : Request)
            {
                // A key and category is required for presence.
                if (!Item.contains_all("Key", "Category")) [[unlikely]] continue;

                std::optional<std::string> Value{};
                if (Item.contains("Value")) Value = Item.value<std::string>("Value");

                try
                {
                    Backend::Database()
                        << "INSERT OR REPLACE INTO Presence VALUES (?,?,?,?);"
                        << std::string(LongID) << Item.value<std::string>("Category") << Item.value<std::string>("Key") << Value;
                } catch (...) {}
            }

            return !Request.empty();
        }
        static bool __cdecl onErase(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

            try
            {
                for (const auto &Item : Request)
                {
                    // A key and category is required for presence.
                    if (!Item.contains_all("Key", "Category")) [[unlikely]] continue;

                    Backend::Database()
                        << "DELETE FROM Presence WHERE (OwnerID = ? AND Category = ? AMD Key = ?);"
                        << std::string(LongID) << Item.value<std::string>("Category") << Item.value<std::string>("Key");
                }
            } catch (...) {}

            return !Request.empty();
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl Insert(JSON::Value_t &&Request)
        {
            if (Request.Type == JSON::Type_t::Array)
            {
                Backend::Messagebus::Publish("Presence::Insert", JSON::Dump(Request));
            }
            else if (Request.Type == JSON::Type_t::Object)
            {
                Backend::Messagebus::Publish("Presence::Insert", "[ "s + JSON::Dump(Request) + " ]");
            }
            else
            {
                return R"({ "Error" : "Presence should be an object or array of objects." })";
            }

            return {};
        }
        static std::string __cdecl Erase(JSON::Value_t &&Request)
        {
            if (Request.Type == JSON::Type_t::Array)
            {
                Backend::Messagebus::Publish("Presence::Erase", JSON::Dump(Request));
            }
            else if (Request.Type == JSON::Type_t::Object)
            {
                Backend::Messagebus::Publish("Presence::Erase", "[ "s + JSON::Dump(Request) + " ]");
            }
            else
            {
                return R"({ "Error" : "Presence should be an object or array of objects." })";
            }

            return {};
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        // Could be a bit spammy, the plugin will have to do its own filtering.
        static void __cdecl onUpdate(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Presence WHERE rowid = ?;" << RowID
                    >> [&](const std::string &OwnerID, const std::string &Category, const std::string &Key, const std::optional<std::string> &Value)
                    {
                        auto Object = JSON::Object_t({ { "ClientID", OwnerID }, { "Key", Key }, { "Category", Category } });
                        if (Value) Object["Value"] = *Value;

                        Backend::Notifications::Publish("Presence::Update", JSON::Dump(Object).c_str());
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
                "CREATE TABLE IF NOT EXISTS Presence ("
                "OwnerID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Category TEXT NOT NULL, "
                "Key TEXT NOT NULL, "
                "Value TEXT, "
                "UNIQUE (OwnerID, Category, Key) );";
        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Presence::Insert", Messagehandlers::onInsert);
        Backend::Messageprocessing::addMessagehandler("Presence::Erase", Messagehandlers::onErase);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Presence::Insert", JSONAPI::Insert);
        Backend::JSONAPI::addEndpoint("Presence::Erase", JSONAPI::Erase);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Relation", Notifications::onUpdate);
    }
}
