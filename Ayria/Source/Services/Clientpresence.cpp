/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-09-01
    License: MIT
*/

#include <Global.hpp>

namespace Services::Clientpresence
{
    // Let the local network know about us.
    static void __cdecl Handleinsert(uint64_t, uint32_t AccountID, const char *Message, unsigned int Length)
    {
        const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

        try
        {
            for (const auto &Item : Request)
            {
                const auto Provider = Item.value<std::string>("Provider");
                const auto Value = Item.value<std::string>("Value");
                const auto Key = Item.value<std::string>("Key");

                Backend::Database()
                    << "INSERT OR REPLACE INTO Presence (AccountID, Provider, Key, Value) VALUES (?,?,?,?);"
                    << AccountID << Provider << Key << Value;
            }
        } catch (...) {}
    }
    static void __cdecl Handledelete(uint64_t, uint32_t AccountID, const char *Message, unsigned int Length)
    {
        const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));

        try
        {
            for (const auto &Item : Request)
            {
                const auto Provider = Item.value<std::string>("Provider");
                const auto Key = Item.value<std::string>("Key");

                Backend::Database()
                    << "DELETE FROM Presence WHERE (AccountID = ? AND Provider = ? AMD Key = ?);"
                    << AccountID << Provider << Key;
            }
        } catch (...) {}
    }

    // Plugin API to modify this clients relations.
    static std::string __cdecl addPresence(JSON::Value_t &&Request)
    {
        if (Request.Type == JSON::Type_t::Array)
        {
            Communication::Publish("Clientpresence::Insert", JSON::Dump(Request));
        }
        else if (Request.Type == JSON::Type_t::Object)
        {
            Communication::Publish("Clientpresence::Insert", "[ "s + JSON::Dump(Request) + " ]");
        }

        return {};
    }
    static std::string __cdecl deletePresence(JSON::Value_t &&Request)
    {
        if (Request.Type == JSON::Type_t::Array)
        {
            Communication::Publish("Clientpresence::Delete", JSON::Dump(Request));
        }
        else if (Request.Type == JSON::Type_t::Object)
        {
            Communication::Publish("Clientpresence::Delete", "[ "s + JSON::Dump(Request) + " ]");
        }

        return {};
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Presence ("
                "AccountID INTEGER NOT NULL, "
                "Provider TEXT NOT NULL, "
                "Key TEXT NOT NULL, "
                "Value TEXT NOT NULL, "
                "PRIMARY KEY (AccountID, Provider, Key) );";
        } catch (...) {}

        // Access from the plugins.
        API::addEndpoint("Clientpresence::addPresence", addPresence);
        API::addEndpoint("Clientpresence::deletePresence", deletePresence);

        // Backend handler for database writes.
        Communication::registerHandler("Clientpresence::Insert", Handleinsert, true);
        Communication::registerHandler("Clientpresence::Delete", Handledelete, true);
    }
}
