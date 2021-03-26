/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-3-24
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Matchmakingsessions
{
    struct Session_t
    {
        uint32_t IPv4;
        uint16_t Port;

        LZString_t B64Gamedata;
        std::u8string Servername, Mapname;
        uint32_t HostID, GameID, ProviderID;
    };
    static Hashmap<uint32_t, Session_t> Localsession{};

    // Insert new data into the database.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Gamedata = Request.value<std::string>("B64Gamedata");
        const auto Servername = Request.value<std::u8string>("Servername");
        const auto ProviderID = Request.value<uint32_t>("ProviderID");
        const auto Mapname = Request.value<std::u8string>("Mapname");
        const auto HostID = Request.value<uint32_t>("HostID");
        const auto GameID = Request.value<uint32_t>("GameID");
        const auto IPv4 = Request.value<uint32_t>("IPv4");
        const auto Port = Request.value<uint16_t>("Port");

        // No fuckery allowed.
        if (Clientinfo::getClientID(NodeID) != HostID) return;

        // Add the entry to the database.
        Backend::Database() << "REPLACE INTO Matchmakingsessions (ProviderID, HostID, Lastupdate, B64Gamedata, "
                               "Servername, GameID, Mapname, IPv4, Port) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ? );"
                            << ProviderID << HostID << uint32_t(time(NULL)) << B64Gamedata << Encoding::toNarrow(Servername)
                            << GameID << Encoding::toNarrow(Mapname) << IPv4 << Port;
    }
    static void __cdecl Terminationhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto ProviderID = Request.value<uint32_t>("ProviderID");

       Backend::Database() << "DELETE FROM Matchmakingsessions WHERE HostID = ? AND ProviderID = ?;"
                           << Clientinfo::getClientID(NodeID) << ProviderID;
    }

    // Send an update if we are active.
    static void Matchmakingupdate()
    {
        if (Localsession.empty()) return;

        for (const auto &[_, Session] : Localsession)
        {
            const auto Request = JSON::Object_t({
                { "B64Gamedata", (std::u8string)Session.B64Gamedata },
                { "Servername", Session.Servername },
                { "ProviderID", Session.ProviderID },
                { "Mapname", Session.Mapname },
                { "HostID", Session.HostID },
                { "GameID", Session.GameID },
                { "IPv4", Session.IPv4 },
                { "Port", Session.Port }
            });

            Backend::Network::Transmitmessage("Matchmaking::Update", Request);
        }
    }

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl Update(JSON::Value_t &&Request)
        {
            const auto ProviderID = Request.value<uint32_t>("ProviderID");
            auto Session = &Localsession[ProviderID];

            Session->Servername = Request.value("Servername", Session->Servername);
            Session->Mapname = Request.value("Mapname", Session->Mapname);
            Session->GameID = Request.value("GameID", Session->GameID);
            Session->IPv4 = Request.value("IPv4", Session->IPv4);
            Session->Port = Request.value("Port", Session->Port);
            Session->ProviderID = ProviderID;

            const auto B64Gamedata = Request.value<std::u8string>("B64Gamedata");
            if (!B64Gamedata.empty())
            {
                Session->B64Gamedata = B64Gamedata;
            }

            Session->HostID = Global.ClientID;
            return "{}";
        }
        static std::string __cdecl Terminate(JSON::Value_t &&Request)
        {
            const auto ProviderID = Request.value<uint32_t>("ProviderID");

            Backend::Network::Transmitmessage("Matchmaking::Terminate", JSON::Object_t({ { "ProviderID", ProviderID } }));
            Localsession.erase(ProviderID);
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
        Backend::API::addEndpoint("Matchmakingsessions::Terminate", API::Terminate);
        Backend::API::addEndpoint("Matchmakingsessions::Update", API::Update);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, Matchmakingupdate);
    }
}
