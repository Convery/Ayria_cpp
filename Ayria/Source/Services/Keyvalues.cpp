/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-25
    License: MIT
*/

#include <Global.hpp>

namespace Services::Keyvalues
{
    // Helper for other services to set presence.
    void setPresence(const std::string &Key, const std::optional<std::string> &Value)
    {
        auto Object = JSON::Object_t({ { "Key", Key }, { "Category", Hash::WW32("AYRIA")} });
        if (Value) Object["Value"] = Encoding::toUTF8(*Value);

        Layer1::Publish("Keyvalues::onInsert", "[ "s + JSON::Dump(Object) + " ]");
    }

    // Helper for conversion.
    static std::optional<uint32_t> toCategory(const JSON::Value_t &Category)
    {
        if (Category.Type == JSON::Type_t::String) return Hash::WW32(Category.get<std::u8string>());
        if (Category.Type == JSON::Type_t::Unsignedint || Category.Type == JSON::Type_t::Signedint)
            return (uint32_t)Category;
        return {};
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onInsert(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

            for (auto &&Item : Request)
            {
                // A key and category is required for presence.
                if (!Item.contains_all("Key", "Category")) [[unlikely]] continue;

                std::optional<std::u8string> Value{};
                if (Item.contains("Value")) Value = Item.value<std::u8string>("Value");

                // Notify the user about the update.
                Item["ClientID"] = std::string(LongID);
                Layer4::Publish("Keyvalues::onInsert", JSON::Dump(Item));

                // Update the database.
                Backend::Database()
                    << "INSERT OR REPLACE INTO Keyvalues VALUES (?,?,?,?);"
                    << LongID << Item.value<uint32_t>("Category") << Item.value<std::u8string>("Key") << Value;
            }

            return !Request.empty();
        }
        static bool __cdecl onErase(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

            for (auto &&Item : Request)
            {
                // A key and category is required for presence.
                if (!Item.contains_all("Key", "Category")) [[unlikely]] continue;

                // Notify the user about the update.
                Item["ClientID"] = std::string(LongID);
                Layer4::Publish("Keyvalues::onErase", JSON::Dump(Item));

                // Update the database.
                Backend::Database()
                    << "DELETE FROM Keyvalues WHERE (ClientID = ? AND Category = ? AMD Key = ?);"
                    << LongID << Item.value<uint32_t>("Category") << Item.value<std::u8string>("Key");
            }

            return !Request.empty();
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl Insert(JSON::Value_t &&Request)
        {
            // Multiple keyvalues.
            if (Request.Type == JSON::Type_t::Array)
            {
                for (const auto &Item : Request.get<JSON::Array_t>())
                {
                    const auto Category = toCategory(Item["Category"]);
                    if (!Category) [[unlikely]] return R"({ "Error" : "Category needs to be a string or integer" })";

                    std::optional<std::u8string> Value{};
                    if (Item.contains("Value"))
                    {
                        Value = Item.value<std::u8string>("Value");
                        if (Value->empty()) Value = {};
                    }

                    Backend::Database()
                        << "INSERT OR REPLACE INTO Keyvalues VALUES (?, ?, ?, ?);"
                        << Global.getLongID() << *Category << Item.value<std::u8string>("Key") << Value;
                }

                Layer1::Publish("Keyvalues::onInsert", JSON::Dump(Request));
                return {};
            }

            // Single keyvalue.
            if (Request.Type == JSON::Type_t::Object)
            {
                const auto Category = toCategory(Request["Category"]);
                if (!Category) [[unlikely]] return R"({ "Error" : "Category needs to be a string or integer" })";

                std::optional<std::u8string> Value{};
                if (Request.contains("Value"))
                {
                    Value = Request.value<std::u8string>("Value");
                    if (Value->empty()) Value = {};
                }

                Backend::Database()
                    << "INSERT OR REPLACE INTO Keyvalues VALUES (?, ?, ?, ?);"
                    << Global.getLongID() << *Category << Request.value<std::u8string>("Key") << Value;

                Layer1::Publish("Keyvalues::onInsert", JSON::Dump(Request));
                return {};
            }

            // Invalid keyvalue.
            return R"({ "Error" : "Keyvalues should be an object or array of objects." })";
        }
        static std::string __cdecl Erase(JSON::Value_t &&Request)
        {
            // Multiple keyvalues.
            if (Request.Type == JSON::Type_t::Array)
            {
                for (const auto &Item : Request.get<JSON::Array_t>())
                {
                    const auto Category = toCategory(Item["Category"]);
                    if (!Category) [[unlikely]] return R"({ "Error" : "Category needs to be a string or integer" })";

                    // Update the database.
                    Backend::Database()
                        << "DELETE FROM Keyvalues WHERE (ClientID = ? AND Category = ? AMD Key = ?);"
                        << Global.getLongID() << Category << Item.value<std::u8string>("Key");
                }

                Layer1::Publish("Keyvalues::onErase", JSON::Dump(Request));
                return {};
            }

            // Single keyvalue.
            if (Request.Type == JSON::Type_t::Object)
            {
                const auto Category = toCategory(Request["Category"]);
                if (!Category) [[unlikely]] return R"({ "Error" : "Category needs to be a string or integer" })";

                // Update the database.
                Backend::Database()
                    << "DELETE FROM Keyvalues WHERE (ClientID = ? AND Category = ? AMD Key = ?);"
                    << Global.getLongID() << Category << Request.value<std::u8string>("Key");

                Layer1::Publish("Keyvalues::onErase", JSON::Dump(Request));
                return {};
            }

            // Invalid keyvalue.
            return R"({ "Error" : "Keyvalues should be an object or array of objects." })";
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Keyvalues ("
            "ClientID TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
            "Category INTEGER NOT NULL, "
            "Key TEXT NOT NULL, "
            "Value TEXT, "
            "UNIQUE (ClientID, Category, Key) );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Keyvalues::onInsert", Messagehandlers::onInsert);
        Layer2::addMessagehandler("Keyvalues::onErase", Messagehandlers::onErase);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Keyvalues::Insert", JSONAPI::Insert);
        Layer3::addEndpoint("Keyvalues::Erase", JSONAPI::Erase);
    }
}
