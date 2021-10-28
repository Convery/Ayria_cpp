/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Services::Clientinfo
{
    // Cached clients are those we've seen / queried this session..
    static Hashmap<std::string, std::shared_ptr<Client_t>> Clientcache{};

    // Fetch the client by ID, for use with services.
    std::shared_ptr<Client_t> getClient(const std::string &LongID)
    {
        // Rare but possible.
        if (LongID == Global.getLongID()) [[unlikely]]
        {
            Client_t Localclient{ Global.GameID, Global.ModID, Global.getClientflags(), Global.getLongID(), Encoding::toNarrow(*Global.Username) };
            return Clientcache.emplace(Localclient.getLongID(), std::make_shared<Client_t>(Localclient)).first->second;
        }

        // Simplest case, we already have the client cached.
        if (Clientcache.contains(LongID)) [[likely]]
            return Clientcache[LongID];

        // Get the client from the database.
        if (const auto JSON = AyriaAPI::Clientinfo::Find(LongID))
        {
            const auto Client = fromJSON(JSON);
            if (!Client) [[unlikely]] return {};

            return Clientcache.emplace(Client->getLongID(), std::make_shared<Client_t>(*Client)).first->second;
        }

        return {};
    }

    // Internal helper to trigger an update when the global state changes.
    void triggerUpdate()
    {
        Backend::Messagebus::Publish("Client::Update", JSON::Dump(toJSON(*getClient(Global.getLongID()))));
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onLeave(uint64_t, const char *LongID, const char *, unsigned int)
        {
            Backend::Notifications::Publish("Client::onLeave", va(R"({ "ClientID" : "%s" })", LongID).c_str());
            Clientcache.erase(LongID);
            return true;
        }
        static bool __cdecl onUpdate(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Parsed = fromJSON(std::string_view(Message, Length));
            const auto Client = Parsed.value_or(Client_t{});

            // Sanity-checking.
            if (!Parsed) [[unlikely]] return false;
            if (Client.getLongID() != LongID) [[unlikely]] return false;
            if (Clientcache.contains(LongID) && Clientcache[LongID]->Timestamp > Timestamp) [[unlikely]] return false;

            // Only save for later lookups if the client is indeed active (i.e. we are not just processing the backlog).
            if (Timestamp > uint64_t((std::chrono::utc_clock::now() - std::chrono::minutes(5)).time_since_epoch().count()))
                Clientcache.emplace(LongID, std::make_shared<Client_t>(Client)).first->second->Timestamp = Timestamp;

            // Insert into the database.
            try
            {
                Backend::Database()
                    << "INSERT OR REPLACE INTO Clients VALUES (?,?,?,?,?);"
                    << Client.Publickey
                    << Client.GameID
                    << Client.ModID
                    << Client.Flags
                    << Encoding::toNarrow(Client.Username);
            } catch (...) {}

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl setGameinfo(JSON::Value_t &&Request)
        {
            Global.GameID = Request.value("GameID", Global.GameID);
            Global.ModID = Request.value("ModID", Global.ModID);
            triggerUpdate();
            return {};
        }
        static std::string __cdecl setGamestate(JSON::Value_t &&Request)
        {
            Global.Settings.isHosting = Request.value("isHosting", Global.Settings.isHosting);
            Global.Settings.isIngame = Request.value("isIngame", Global.Settings.isIngame);
            triggerUpdate();
            return {};
        }
        static std::string __cdecl setSocialstate(JSON::Value_t &&Request)
        {
            Global.Settings.isPrivate = Request.value("isPrivate", Global.Settings.isPrivate);
            Global.Settings.isAway = Request.value("isAway", Global.Settings.isAway);
            triggerUpdate();
            return {};
        }
        static std::string __cdecl getLocalclient(JSON::Value_t &&)
        {
            const auto Object = JSON::Object_t({
                { "enableIATHooking", Global.Settings.enableIATHooking },
                { "enableFileshare", Global.Settings.enableFileshare },
                { "noNetworking", Global.Settings.noNetworking },
                { "Username", std::u8string(*Global.Username) },
                { "isPrivate", Global.Settings.isPrivate },
                { "isHosting", Global.Settings.isHosting },
                { "isIngame", Global.Settings.isIngame },
                { "isAway", Global.Settings.isAway },
                { "InternalIP", Global.InternalIP },
                { "ExternalIP", Global.ExternalIP },
                { "ShortID", Global.getShortID() },
                { "LongID", Global.getLongID() }
            });

            return JSON::Dump(Object);
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        static void __cdecl onUpdate(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Client WHERE rowid = ?;" << RowID
                    >> [](const std::string &ClientID, uint32_t GameID, uint32_t ModID, uint32_t Flags, const std::string &Username)
                    {
                        const auto Client = getClient(ClientID);
                        if (!Client) [[unlikely]] return; // WTF?

                        JSON::Object_t Delta{ {"ClientID", ClientID} };
                        if (Username != Client->Username) { Delta["Username"] = Username; Client->Username = Username; }
                        if (GameID != Client->GameID) { Delta["GameID"] = GameID; Client->GameID = GameID; }
                        if (ModID != Client->ModID) { Delta["ModID"] = ModID; Client->ModID = ModID; }
                        if (Flags != Client->Flags) { Delta["Flags"] = Flags; Client->Flags = Flags; }

                        Backend::Notifications::Publish("Client::onUpdate", JSON::Dump(Delta).c_str());
                    };
            } catch (...) {}
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        // Create a persistent table for the clients.
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Client ("
                "ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey) ON DELETE CASCADE,"
                "GameID INTEGER NOT NULL,"
                "ModID INTEGER NOT NULL,"
                "Flags INTEGER NOT NULL,"
                "Username TEXT NOT NULL );";
        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Client::Leave", Messagehandlers::onLeave);
        Backend::Messageprocessing::addMessagehandler("Client::Update", Messagehandlers::onUpdate);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Client::setGameinfo", JSONAPI::setGameinfo);
        Backend::JSONAPI::addEndpoint("Client::setGamestate", JSONAPI::setGamestate);
        Backend::JSONAPI::addEndpoint("Client::setSocialstate", JSONAPI::setSocialstate);
        Backend::JSONAPI::addEndpoint("Client::getLocalclient", JSONAPI::getLocalclient);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Client", Notifications::onUpdate);

        // Notify the other clients when we join and leave.
        std::atexit([]() { Backend::Messagebus::Publish("Client::Leave", {}); });
        Backend::Messagebus::Publish("Client::Update", JSON::Dump(toJSON(*getClient(Global.getLongID()))));
    }
}
