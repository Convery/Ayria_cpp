/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-27
    License: MIT
*/

#include <Global.hpp>

namespace Services::Matchmaking
{
    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onUpdate(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Server = fromJSON(std::string_view(Message, Length));
            if (!Server || Server->GroupID != LongID) [[unlikely]] return false;

            try
            {
                Backend::Database()
                    << "INSERT OR REPLACE INTO Matchmaking VALUES (?,?,?,?,?,?,?);"
                    << Server->GroupID
                    << Server->Hostaddress
                    << Server->Servername
                    << Server->Provider
                    << Server->GameID
                    << Server->ModID;
            } catch (...) {}

            return true;
        }
        static bool __cdecl onTerminate(uint64_t, const char *LongID, const char *, unsigned int)
        {
            try
            {
                Backend::Database()
                    << "DELETE FROM Matchmaking WHERE GroupID = ?;"
                    << std::string(LongID);
            } catch (...) {}

            // The notification system does not poll deletions, so we need to do that here.
            const auto Tmp = AyriaAPI::Groups::getMemberships(Global.getLongID());
            if (Tmp.end() != std::ranges::find(Tmp, std::string(LongID)))
            {
                Backend::Notifications::Publish("Matchmaking::onTerminate", JSON::Dump(JSON::Value_t{ std::string(LongID) }).c_str());
            }

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl stopServer(JSON::Value_t &&)
        {
            Layer1::Publish("Matchmaking::onTerminate", { "Shutdown" });
            Global.Settings.isHosting = false;
            Clientinfo::triggerUpdate();
            return {};
        }
        static std::string __cdecl updateServer(JSON::Value_t &&Request)
        {
            const auto Server = fromJSON(Request);
            if (!Server) return R"({ "Error" : "Missing arguments" })";

            Layer1::Publish("Matchmaking::onUpdate()", { "Shutdown" });
            Global.Settings.isHosting = true;
            Clientinfo::triggerUpdate();
            return {};
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        static void __cdecl onUpdate(int64_t RowID)
        {
            try
            {
                Serverinfo_t Server{};

                Backend::Database()
                    << "SELECT * FROM Matchmaking WHERE rowid = ?;" << RowID
                    >> [&](const std::string &GroupID, const std::string &Hostaddress, const std::optional<std::string> &Servername, const std::string &Provider, uint32_t GameID, uint32_t ModID)
                    {
                        Server.ModID = ModID;
                        Server.GameID = GameID;
                        Server.GroupID = GroupID;
                        Server.Provider = Provider;
                        Server.Hostaddress = Hostaddress;
                        if (Servername) Server.Servername = *Servername;
                    };

                const auto Tmp = AyriaAPI::Groups::getMemberships(Global.getLongID());
                if (Tmp.end() != std::ranges::find(Tmp, Server.getLongID())) [[unlikely]]
                {
                    Backend::Notifications::Publish("Matchmaking::onUpdate", JSON::Dump(toJSON(Server)).c_str());
                }
            } catch (...) {}
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Matchmaking ("
                "GroupID TEXT PRIMARY KEY REFERENCES Group(GroupID) ON DELETE CASCADE, "
                "Hostaddress TEXT NOT NULL, "
                "Servername TEXT, "
                "Provider TEXT NOT NULL, "
                "GameID INTEGER NOT NULL,"
                "ModID INTEGER DEFAULT 0 );";
        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Matchmaking::Update", Messagehandlers::onUpdate);
        Backend::Messageprocessing::addMessagehandler("Matchmaking::Stop", Messagehandlers::onTerminate);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Matchmaking::updateServer", JSONAPI::updateServer);
        Backend::JSONAPI::addEndpoint("Matchmaking::startServer", JSONAPI::updateServer);
        Backend::JSONAPI::addEndpoint("Matchmaking::stopServer", JSONAPI::stopServer);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Matchmaking", Notifications::onUpdate);
    }
}
