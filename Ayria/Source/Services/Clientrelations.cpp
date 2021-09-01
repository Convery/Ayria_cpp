/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-09-01
    License: MIT
*/

#include <Global.hpp>

namespace Services::Clientrelations
{
    // Let the local network know about us.
    static void insertRelation(uint32_t AccountID, bool isFriend, bool isBlocked)
    {
        const auto Object = JSON::Object_t({
            { "isBlocked", isBlocked },
            { "isFriend", isFriend },
            { "Target", AccountID }
        });

        Communication::Publish("Clientrelations::Insert", JSON::Dump(Object));
    }
    static void __cdecl Handleinsert(uint64_t, uint32_t AccountID, const char *Message, unsigned int Length)
    {
        auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Target = Request.value<uint32_t>("Target");

        // We don't accept deltas, we need all the properties.
        if (!Request.contains("Target") || !Request.contains("isFriend") || !Request.contains("isBlocked")) return;

        try
        {
            Backend::Database()
                << "INSERT OR REPLACE INTO Relations (Source, Target, isFriend, isBlocked) VALUES (?,?,?,?);"
                << AccountID << Target << Request.value<bool>("isFriend") << Request.value<bool>("isBlocked");
        } catch (...) {}

        // Notify plugins that care about this.
        if (Target == Global.getShortID())
        {
            Request["Source"] = AccountID;
            Notifications::Publish("Relations::onChange", JSON::Dump(Request).c_str());
        }
    }

    // Plugin API to modify this clients relations.
    static std::string __cdecl addFirend(JSON::Value_t &&Request)
    {
        const auto AccountID = Request.value<uint32_t>("AccountID");
        if (!AccountID) [[unlikely]] return {};

        insertRelation(AccountID, true, false);
        return {};
    }
    static std::string __cdecl blockUser(JSON::Value_t &&Request)
    {
        const auto AccountID = Request.value<uint32_t>("AccountID");
        if (!AccountID) [[unlikely]] return {};

        insertRelation(AccountID, false, true);
        return {};
    }
    static std::string __cdecl clearRelation(JSON::Value_t &&Request)
    {
        const auto AccountID = Request.value<uint32_t>("AccountID");
        if (!AccountID) [[unlikely]] return {};

        insertRelation(AccountID, false, false);
        return {};
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Relations ("
                "Source INTEGER NOT NULL, "
                "Target INTEGER NOT NULL, "
                "isFriend BOOLEAN NOT NULL, "
                "isBlocked BOOLEAN NOT NULL, "
                "PRIMARY KEY (Source, Target) );";
        } catch (...) {}

        // Access from the plugins.
        API::addEndpoint("Clientrelations::addFriend", addFirend);
        API::addEndpoint("Clientrelations::blockUser", blockUser);
        API::addEndpoint("Clientrelations::clearRelation", clearRelation);

        // Backend handler for database writes.
        Communication::registerHandler("Clientrelations::Insert", Handleinsert, true);
    }
}
