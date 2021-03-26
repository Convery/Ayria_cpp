/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-25
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Relationships
{
    using Relationflags_t = union { uint8_t Raw; struct { uint8_t isFriend : 1, isBlocked : 1; }; };
    static Hashset<uint32_t> Friendlyclients, Blockedclients;

    // Helper functionallity.
    bool isBlocked(uint32_t SourceID, uint32_t TargetID)
    {
        if (SourceID == Global.ClientID) [[likely]]
            return Blockedclients.contains(TargetID);

        Relationflags_t Flags{};
        Backend::Database() << "SELECT Flags FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                            << SourceID << TargetID >> Flags.Raw;

        return Flags.isBlocked;
    }
    bool isFriend(uint32_t SourceID, uint32_t TargetID)
    {
        if (SourceID == Global.ClientID) [[likely]]
            return Friendlyclients.contains(TargetID);

        Relationflags_t Flags{};
        Backend::Database() << "SELECT Flags FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
            << SourceID << TargetID >> Flags.Raw;

        return Flags.isFriend;
    }

    // Handle local client-requests.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto SourceID = Request.value<uint32_t>("SourceID");
        const auto TargetID = Request.value<uint32_t>("TargetID");
        const auto isBlocked = Request.value<bool>("isBlocked");
        const auto isFriend = Request.value<bool>("isFriend");
        Relationflags_t Flags{};

        // Let's not bother with fuckery.
        if (SourceID != Clientinfo::getClientID(NodeID))
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // Get the current set of flags.
        Backend::Database() << "SELECT Flags FROM Relationships WHERE SourceID = ? AND TargetID = ?;"
                            << SourceID << TargetID >> Flags.Raw;

        // Update the flags.
        Flags.isFriend = isFriend;
        Flags.isBlocked = isBlocked;

        // Ensure that the flags are available to the plugins.
        Backend::Database() << "REPLACE ( SourceID, TargetID, Flags ) INTO Relationships VALUES (?, ?, ?);"
                            << SourceID << TargetID << Flags.Raw;
    }

    // Send requests to local clients.
    static void Sendupdate(uint32_t TargetID, Relationflags_t Flags)
    {
        // Update the internal state for quicker lookups.
        if (Flags.isBlocked) { Blockedclients.insert(TargetID); Friendlyclients.erase(TargetID); }
        if (Flags.isFriend) { Blockedclients.erase(TargetID); Friendlyclients.insert(TargetID); }

        // Ensure that the flags are available to the plugins.
        Backend::Database() << "REPLACE Flags INTO Relationships ( SourceID, TargetID, Flags ) VALUES (?, ?, ?);"
                            << Global.ClientID << TargetID << Flags.Raw;

        // Notify the other clients.
        const auto Request = JSON::Object_t({
            { "isBlocked", Flags.isBlocked },
            { "SourceID", Global.ClientID },
            { "isFriend", Flags.isFriend },
            { "TargetID", TargetID }
        });
        Backend::Network::Transmitmessage("Relationships::Update", Request);
    }

    // JSON API access for the plugins.
    static std::string __cdecl updateStatus(JSON::Value_t &&Request)
    {
        const auto TargetID = Request.value<uint32_t>("TargetID");
        const auto isBlocked = Request.value<bool>("isBlocked");
        const auto isFriend = Request.value<bool>("isFriend");

        Sendupdate(TargetID, { .isFriend = isFriend, .isBlocked = isBlocked });

        return "{}";
    }
    static std::string __cdecl clearStatus(JSON::Value_t &&Request)
    {
        const auto TargetID = Request.value<uint32_t>("TargetID");

        Sendupdate(TargetID, {});
        return "{}";
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Relationship::Update", Updatehandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Relationships::Update", updateStatus);
        Backend::API::addEndpoint("Relationships::Clear", clearStatus);

        // Load all the clients relations from the database.
        Backend::Database() << "SELECT TargetID, Flags FROM Relationships WHERE SourceID = ?;" << Global.ClientID
                            >> [](uint32_t TargetID, uint8_t Flags)
                            {
                                if (Relationflags_t{ Flags }.isFriend) Friendlyclients.insert(TargetID);
                                if (Relationflags_t{ Flags }.isBlocked) Blockedclients.insert(TargetID);
                            };
    }
}
