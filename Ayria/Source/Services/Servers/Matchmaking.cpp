/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-3-24
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Matchmaking
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

        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender || Sender->UserID != HostID) return;

        // Add the entry to the database.
        Backend::Database() << "insert or replace into Matchmakingsessions (HostID, ProviderID, B64Gamedata"
                               "Lastseen, Servername, GameID, Mapname, IPv4, Port) values (?, ?, ?, "
                               "?, ?, ?, ?, ?, ?);" << HostID << ProviderID << B64Gamedata
                               << time(NULL) << Encoding::toNarrow(Servername) << GameID
                               << Encoding::toNarrow(Mapname) << IPv4 << Port;
    }
    static void __cdecl Terminationhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto ProviderID = Request.value<uint32_t>("ProviderID");

        if (const auto Sender = Clientinfo::getLANClient(NodeID))
            Backend::Database() << "delete from Matchmakingsessions where HostID = ? and ProviderID = ?;"
                                << Sender->UserID << ProviderID;
    }

    // Send an update if we are active.
    static void Matchmakingupdate()
    {
        if (Localsession.empty()) return;

        for (const auto &[_, Session] : Localsession)
        {
            const auto Request = JSON::Object_t({
                { "B64Gamedata", std::string(Session.B64Gamedata) },
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

            const auto B64Gamedata = Request.value<std::string>("B64Gamedata");
            if (!B64Gamedata.empty())
            {
                Session->B64Gamedata = B64Gamedata;
            }

            Session->HostID = Global.UserID;
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


    void Initialize()
    {
        // Transient database entry of known sessions.
        Backend::Database() << "create table Matchmakingsessions ("
                               "HostID integer not null, "
                               "ProviderID integer, "
                               "B64Gamedata text, "
                               "Lastseen integer, "
                               "Servername text, "
                               "GameID integer, "
                               "Mapname text, "
                               "IPv4 integer, "
                               "Port integer);";

        Backend::Enqueuetask(5000, Matchmakingupdate);

        Backend::Network::Registerhandler("Matchmaking::Update", Updatehandler);
        Backend::Network::Registerhandler("Matchmaking::Terminate", Terminationhandler);

        Backend::API::addEndpoint("Matchmaking::Update", API::Update);
        Backend::API::addEndpoint("Matchmaking::Terminate", API::Terminate);
    }
}
