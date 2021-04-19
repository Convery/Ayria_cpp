/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-3-24
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Matchmakingsessions
{
    struct Localsession_t
    {
        bool Active;
        uint16_t Port;
        uint32_t HostID, GameID, Address;
        LZString_t Gamedata;
    };
    static Localsession_t Localsession{};

    // Handle incoming updates from the other clients.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Gamedata = Request.value<std::string>("Gamedata");
        const auto Hostport = Request.value<uint16_t>("Hostport");
        const auto GameID = Request.value<uint32_t>("GameID");
        const auto HostID = Clientinfo::getClientID(NodeID);

        // Re-use the clients local address if no public one is provided.
        auto Hostaddress = Request.value<uint32_t>("Hostaddress");
        if (!Hostaddress) Hostaddress = Backend::Network::Nodeaddress(NodeID).S_un.S_addr;

        // Add the entry so that the plugins can query for it.
        try
        {
            Backend::Database()
                << "REPLACE INTO Matchmakingsessions (HostID, Lastupdate, GameID, Hostaddress, Hostport, Gamedata) "
                   "VALUES (?, ?, ?, ?, ?, ?);"
                << HostID << time(NULL) << GameID << Hostaddress << Hostport << Gamedata;
        } catch (...) {}
    }
    static void __cdecl Terminationhandler(unsigned int NodeID, const char *, unsigned int)
    {
        // TODO(tcn): Notify the plugins about this event.
        const auto HostID = Clientinfo::getClientID(NodeID);

        try { Backend::Database() << "DELETE FROM Matchmakingsessions WHERE HostID = ?;" << HostID; }
        catch (...) {}
    }

    // Notify the other clients about our state once in a while.
    static void Updatestate()
    {
        const auto Sessionchanges = Backend::getDatabasechanges("Matchmakingsessions");

        // TODO(tcn): Maybe notify the plugins about changes rather than just checking ours.
        if (Localsession.Active)
        {
            for (const auto &Item : Sessionchanges)
            {
                try
                {
                    Backend::Database()
                        << "SELECT * FROM Matchmakingsessions WHERE rowid = ? AND HostID = ?;"
                        << Item << Global.ClientID
                        >> [](uint32_t, uint32_t, uint32_t GameID, uint32_t Hostaddress, uint16_t Hostport, std::string &&Gamedata)
                        {
                            Localsession.Address = Hostaddress;
                            Localsession.Gamedata = Gamedata;
                            Localsession.GameID = GameID;
                            Localsession.Port = Hostport;
                        };
                }
                catch (...) {}
            }
        }

        // Notify the other clients about our session.
        if (Localsession.Active)
        {
            const auto Request = JSON::Object_t({
                { "Gamedata", (std::string)Localsession.Gamedata },
                { "Hostaddress", Localsession.Address },
                { "Hostport", Localsession.Port },
                { "GameID", Localsession.GameID }
            });
            Backend::Network::Transmitmessage("Matchmakingsessions::Update", Request);
        }
    }

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl Starthosting(JSON::Value_t &&Request)
        {
            const auto Gamedata = Request.value<std::string>("Gamedata");
            const auto Hostport = Request.value<uint16_t>("Hostport");
            const auto HostID = Global.ClientID;

            // Re-use the clients local address if no public one is provided.
            auto Hostaddress = Request.value<uint32_t>("Hostaddress");
            if (!Hostaddress) Hostaddress = Global.Publicaddress;

            // A game-ID is required.
            auto GameID = Request.value<uint32_t>("GameID");
            if (!GameID) GameID = Global.GameID;

            // Active needs to be set last incase of data-races.
            Localsession.Address = Hostaddress;
            Localsession.Gamedata = Gamedata;
            Localsession.GameID = GameID;
            Localsession.HostID = HostID;
            Localsession.Port = Hostport;
            Localsession.Active = true;

            // Add the entry so that the plugins can query for it.
            try
            {
                Backend::Database()
                    << "REPLACE INTO Matchmakingsessions (HostID, Lastupdate, GameID, Hostaddress, Hostport, Gamedata) "
                       "VALUES (?, ?, ?, ?, ?, ?);"
                    << HostID << 0 << GameID << Hostaddress << Hostport << Gamedata;
            } catch (...) {}

            // TODO(tcn): Maybe notify the other plugins about this?
            return "{}";
        }
        static std::string __cdecl Terminate(JSON::Value_t &&)
        {
            Localsession.Active = false;
            Backend::Network::Transmitmessage("Matchmaking::Terminate", {});

            try { Backend::Database() << "DELETE FROM Matchmakingsessions WHERE HostID = ?;" << Global.ClientID; }
            catch (...) {}

            // TODO(tcn): Notify the other plugins about this.
            return "{}";
        }
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Matchmakingsessions::Terminate", Terminationhandler);
        Backend::Network::Registerhandler("Matchmakingsessions::Update", Updatehandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Matchmakingsessions::Starthosting", API::Starthosting);
        Backend::API::addEndpoint("Matchmakingsessions::Terminate", API::Terminate);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, Updatestate);
    }
}
