/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-12-05
    License: MIT
*/

#include <Global.hpp>

namespace Services::Matchmaking
{
    namespace Local
    {
        static std::unordered_set<uint32_t> Providers{};
        static std::string Hostaddress{};
        static uint64_t Lastupdate{};
        static bool isActive{};

        // Notify the network that we are still active.
        static void Announce(bool Force = false)
        {
            // Nothing to do here.
            if (!isActive) [[likely]] return;

            const auto Currenttime = GetTickCount64();
            if (Force || (Currenttime - Lastupdate > 15000)) [[unlikely]]
            {
                const auto Object = JSON::Object_t({
                    { "Hostaddress", Hostaddress },
                    { "GameID", Global.GameID },
                    { "Providers", Providers }
                });

                Layer1::Publish("Matchmaking::onStartup", Object);
                Lastupdate = Currenttime;

                // Ensure that the database is synchronized.
                Backend::Database()
                    << "INSERT OR REPLACE INTO Matchmaking VALUES (?, ?, ?, ?);"
                    << Global.getLongID()
                    << Hostaddress
                    << Providers
                    << Global.GameID;
            }
        }
    }

    // Helpers for type-conversions.
    static std::optional<uint32_t> toProvider(const JSON::Value_t &Provider)
    {
        if (Provider.Type == JSON::Type_t::String) return Hash::WW32(Provider.get<std::u8string>());
        if (Provider.Type == JSON::Type_t::Unsignedint || Provider.Type == JSON::Type_t::Signedint)
            return (uint32_t)Provider;
        return {};
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onStartup(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto GameID = Request.value<uint32_t>("GameID");
            const auto Hostaddress = Request.value<std::string>("Hostaddress");
            const auto Providers = Request.value<std::vector<uint32_t>>("Providers");

            // Sanity checking, may tweek the providers in the future.
            if (Providers.size() > 5) [[unlikely]] return false;
            if (Hostaddress.empty()) [[unlikely]] return false;

            // Save to the database.
            Backend::Database()
                << "INSERT OR REPLACE INTO Matchmaking VALUES (?, ?, ?, ?);"
                << LongID
                << Hostaddress
                << Providers
                << GameID;

            return true;
        }
        static bool __cdecl onShutdown(uint64_t, const char *LongID, const char *, unsigned int)
        {
            Backend::Database() << "DELETE FROM Matchmaking WHERE GroupID = ?;" << LongID;
            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl Startserver(JSON::Value_t &&Request)
        {
            const auto Hostaddress = Request.value<std::string>("Hostaddress");
            const auto Providers = Request.value<JSON::Array_t>("Providers");
            const auto Group = Groups::getGroup(Global.getLongID());

            // Sanity-checking, need to know the host, even if it's our own IP.
            if (Local::isActive) [[unlikely]] return R"({ "Error" : "Server is already active. Shut down the server first." })";
            if (Providers.empty()) [[unlikely]] return R"({ "Error" : "Need at least one provider." })";
            if (!Group) [[unlikely]] return R"({ "Error" : "No active group for this server." })";
            if (Hostaddress.empty() || !Hostaddress.contains(':')) [[unlikely]]
                return R"({ "Error" : "Needs a Hostaddress string in the form of IP:Port" })";

            // Parse both integers and strings.
            for (const auto &Item : Providers)
            {
                if (const auto Provider = toProvider(Item)) [[likely]] Local::Providers.insert(*Provider);
                else return R"({ "Error" : "Provider needs to be a string or integer" })";
            }

            // Announce to the network.
            Local::Hostaddress = Hostaddress;
            Local::isActive = true;
            Local::Announce(true);
            return {};
        }
        static std::string __cdecl addProvider(JSON::Value_t &&Request)
        {
            if (!Local::isActive) return R"({ "Error" : "No active server" })";

            if (const auto Provider = toProvider(Request["Provider"])) [[likely]] Local::Providers.insert(*Provider);
            else return R"({ "Error" : "Provider needs to be a string or integer" })";

            Local::Announce(true);
            return {};
        }
        static std::string __cdecl removeProvider(JSON::Value_t &&Request)
        {
            if (!Local::isActive) return R"({ "Error" : "No active server" })";

            if (const auto Provider = toProvider(Request["Provider"])) [[likely]] Local::Providers.erase(*Provider);
            else return R"({ "Error" : "Provider needs to be a string or integer" })";

            Local::Announce(true);
            return {};
        }
        static std::string __cdecl Shutdownserver(JSON::Value_t &&)
        {
            Backend::Database() << "DELETE FROM Matchmaking WHERE GroupID = ?;" << Global.getLongID();
            Layer1::Publish("Matchmaking::onShutdown", "");
            Local::Hostaddress.clear();
            Local::Providers.clear();
            Local::isActive = false;
            return {};
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        // Keep a persitent table around for matchmaking.
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Matchmaking ("
            "GroupID TEXT PRIMARY KEY REFERENCES Group(GroupID), "
            "Hostaddress TEXT UNIQUE, "
            "Providers BLOB, "
            "GameID INTEGER );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Matchmaking::onStartup", Messagehandlers::onStartup);
        Layer2::addMessagehandler("Matchmaking::onShutdown", Messagehandlers::onShutdown);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Matchmaking::Startserver", JSONAPI::Startserver);
        Layer3::addEndpoint("Matchmaking::addProvider", JSONAPI::addProvider);
        Layer3::addEndpoint("Matchmaking::removeProvider", JSONAPI::removeProvider);
        Layer3::addEndpoint("Matchmaking::Shutdownserver", JSONAPI::Shutdownserver);

        std::atexit([]() { Layer1::Publish("Matchmaking::onShutdown", ""); });
        Backend::Enqueuetask(5000, []() { Local::Announce(); });
    }
}
