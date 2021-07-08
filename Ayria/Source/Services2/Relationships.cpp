/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-25
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Relationships
{
    static Hashset<uint32_t> Friendlyclients, Blockedclients;

    // Helper functionallity.
    bool isBlocked(uint32_t SourceID, uint32_t TargetID)
    {
        if (SourceID == Global.ClientID) [[likely]]
            return Blockedclients.contains(TargetID);

        try
        {
            bool isBlocked{};
            Backend::Database() << "SELECT isBlocked FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                                << SourceID << TargetID >> isBlocked;
            return isBlocked;
        } catch (...) {}

        return false;
    }
    bool isFriend(uint32_t SourceID, uint32_t TargetID)
    {
        if (SourceID == Global.ClientID) [[likely]]
            return Friendlyclients.contains(TargetID);

        try
        {
            bool isFriend{};
            Backend::Database() << "SELECT isFriend FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                                << SourceID << TargetID >> isFriend;
            return isFriend;
        } catch (...) {}

        return false;
    }

    // Handle local client-requests.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const JSON::Array_t Request = JSON::Parse(std::string_view(Message, Length));
        const auto SourceID = Clientinfo::getClientID(NodeID);

        for (const auto &Object : Request)
        {
            const auto TargetID = Object.value<uint32_t>("TargetID");
            const auto isBlocked = Object.value<bool>("isBlocked");
            const auto isFriend = Object.value<bool>("isFriend");

            // Update the relationships in the database for the plugins.
            try
            {
                if (Object.contains("isFriend") && Object.contains("isBlocked"))
                {
                    Backend::Database() << "REPLACE INTO Relationships (SourceID, TargetID, isBlocked, isFriend) "
                                           "VALUES (?, ?, ?, ?);" << SourceID << TargetID << isBlocked << isFriend;
                }
                else if (Object.contains("isFriend"))
                {
                    Backend::Database() << "REPLACE INTO Relationships (SourceID, TargetID, isBlocked, isFriend) "
                                           "VALUES (?, ?, ?, ?);" << SourceID << TargetID << false << isFriend;
                }
                else if (Object.contains("isBlocked"))
                {
                    Backend::Database() << "REPLACE INTO Relationships (SourceID, TargetID, isBlocked, isFriend) "
                                           "VALUES (?, ?, ?, ?);" << SourceID << TargetID << isBlocked << false;
                }
            }
            catch (...) {}
        }
    }

    // Notify the other clients about our state.
    static void Updatestate()
    {
        const auto Changes = Backend::getDatabasechanges("Relationships");
        for (const auto &Item : Changes)
        {
            try
            {
                Backend::Database() << "SELECT * FROM Relationships WHERE rowid = ? AND SourceID = ?;"
                                    << Item << Global.ClientID
                                    >> [](uint32_t, uint32_t TargetID, bool isBlocked, bool isFriend)
                                    {
                                        if (isBlocked) Blockedclients.insert(TargetID);
                                        if (isFriend) Friendlyclients.insert(TargetID);
                                    };
            } catch (...) {}
        }

        // Share our relationship with the world.
        JSON::Array_t Relations;
        Relations.reserve(Friendlyclients.size() + Blockedclients.size());

        for (const auto &Item : Friendlyclients)
            Relations.emplace_back(JSON::Object_t({ { "TargetID", Item }, { "isFriend", true } }));
        for (const auto &Item : Blockedclients)
            Relations.emplace_back(JSON::Object_t({ { "TargetID", Item }, { "isBlocked", true } }));

        Backend::Network::Transmitmessage("Relationships::Update", Relations);
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Relationship::Update", Updatehandler);

        // Periodically check for changes.
        Backend::Enqueuetask(30000, Updatestate);

        // Load all the clients relations from the database.
        try
        {
            Backend::Database() << "SELECT * FROM Relationships WHERE SourceID = ?;" << Global.ClientID
                                >> [](uint32_t, uint32_t TargetID, bool isBlocked, bool isFriend)
                                {
                                    if (isBlocked) Blockedclients.insert(TargetID);
                                    if (isFriend) Friendlyclients.insert(TargetID);
                                };
        } catch (...) {}
    }
}
