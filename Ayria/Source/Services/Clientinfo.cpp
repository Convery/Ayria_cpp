/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Services::Clientinfo
{
    // Cache strings and clients we've seen so far.
    static Hashmap<std::string, std::shared_ptr<Client_t>> Clientcache{};
    static Hashset<std::u8string> Usernamecache{};
    static Hashset<std::string> Publickeycache{};

    // Format between C++ and JSON representations.
    std::optional<Client_t> fromJSON(const JSON::Value_t &Object)
    {
        // Can only parse if all the required data is available.
        if (!Object.contains_all("ClientID", "GameID", "ModID", "isIngame", "isAway")) [[unlikely]] return {};

        Client_t Client{};
        Client.ModID = Object["ModID"];
        Client.GameID = Object["GameID"];
        Client.isAway = Object["isAway"];
        Client.isIngame = Object["isIngame"];
        Client.Lastseen = Object.value("Lastseen", int64_t{});
        Client.ClientID = *Publickeycache.insert(Object.value<std::string>("ClientID")).first;
        Client.Username = *Usernamecache.insert(Object.value<>("Username", std::u8string{})).first;

        return Client;
    }
    std::optional<Client_t> fromJSON(std::string_view JSON)
    {
        return fromJSON(JSON::Parse(JSON));
    }
    JSON::Object_t toJSON(const Client_t &Client)
    {
        return JSON::Object_t({
            { "Username", Client.Username },
            { "Lastseen", Client.Lastseen },
            { "ClientID", Client.ClientID },
            { "isIngame", Client.isIngame },
            { "GameID", Client.GameID },
            { "isAway", Client.isAway },
            { "ModID", Client.ModID }
        });
    }

    // Format our own info as a client struct.
    static const Client_t &getLocalclient()
    {
        static Client_t Local;
        Local = {
            *Usernamecache.insert(Global.Username->c_str()).first,
            *Publickeycache.insert(Global.getLongID()).first,
            Global.GameID,
            Global.ModID,
            !!Global.Settings.isIngame,
            !!Global.Settings.isAway
        };
        return Local;
    }

    // Fetch the client by ID, for use with services.
    std::shared_ptr<Client_t> getClient(const std::string &LongID)
    {
        // Special case for when this function is used to get ourselves.
        if (LongID == Global.getLongID()) [[unlikely]]
        {
            return Clientcache.emplace(LongID, std::make_shared<Client_t>(getLocalclient())).first->second;
        }

        // Check if it needs updating.
        const auto Timestamp = (std::chrono::system_clock::now() - std::chrono::seconds(10)).time_since_epoch().count();
        if (Clientcache.contains(LongID) && Clientcache[LongID]->Lastupdated > Timestamp)
        {
            return Clientcache[LongID];
        }

        // Else we query the database for more info.
        if (auto Client = fromJSON(AyriaAPI::Clientinfo::Fetch(LongID)))
        {
            if (Client->Lastseen < Timestamp) Client->Lastupdated = 0xFFFFFFFF;
            return Clientcache.emplace(LongID, std::make_shared<Client_t>(*Client)).first->second;
        }

        return {};
    }

    // Internal helper to trigger an update when the global state changes.
    void triggerUpdate()
    {
        Layer1::Publish("Client::Update", toJSON(getLocalclient()));
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onLeave(uint64_t, const char *LongID, const char *, unsigned int)
        {
            Layer4::Publish("Client::onLeave", JSON::Object_t({ {"ClientID", LongID } }));
            Clientcache.erase(LongID);
            return true;
        }
        static bool __cdecl onUpdate(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Parsed = fromJSON(std::string_view(Message, Length));
            auto Client = Parsed.value_or(Client_t{});

            // Sanity-checking.
            if (!Parsed) [[unlikely]] return false;
            if (Client.getLongID() != LongID) [[unlikely]] return false;

            // Check how much we care for this user.
            do
            {
                // Maybe some plugin is interested in this..
                if (LongID == Global.getLongID())
                {
                    Layer4::Publish("Client::Selfupdate", toJSON(Client));
                    break;
                }

                // Relationships.
                const auto Relation = AyriaAPI::Clientrelations::Get(LongID, Global.getLongID());

                // Ignore blocked clients.
                if (Relation.second) break;

                // A friend or someone we stalk.
                if (Relation.second)
                {
                    Layer4::Publish("Client::Friendupdate", toJSON(Client));
                    break;
                }

                // Just some random..
                Layer4::Publish("Client::Clientupdate", toJSON(Client));

            } while (false);

            // Update the client in the datbase.
            Backend::Database()
                << "INSERT OR REPLACE INTO Clientsocial VALUES(?, ?, ?);"
                << Client.ClientID << Client.Username << Client.isAway;
            Backend::Database()
                << "INSERT OR REPLACE INTO Clientgaming VALUES(?, ?, ?, ?);"
                << Client.ClientID << Client.GameID << Client.ModID << Client.isIngame;

            // And our cache.
            Client.Lastupdated = std::chrono::system_clock::now().time_since_epoch().count();
            Client.Lastseen = Client.Lastupdated;
            Clientcache[Client.ClientID] = std::make_shared<Client_t>(Client);

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
                { "pruneDB", Global.Settings.pruneDB },
                { "isAway", Global.Settings.isAway },
                { "InternalIP", Global.InternalIP },
                { "ExternalIP", Global.ExternalIP },
                { "ShortID", Global.getShortID() },
                { "LongID", Global.getLongID() }
            });

            return JSON::Dump(Object);
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        // Create persistent tables for the users.
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Clientsocial ( "
            "ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey), "
            "Username TEXT, "
            "isAway BOOLEAN );";
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Clientgaming ( "
            "ClientID TEXT PRIMARY KEY REFERENCES Account(Publickey), "
            "GameID INTEGER, "
            "ModID INTEGER, "
            "isIngame BOOLEAN );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Client::Leave", Messagehandlers::onLeave);
        Layer2::addMessagehandler("Client::Update", Messagehandlers::onUpdate);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Client::setGameinfo", JSONAPI::setGameinfo);
        Layer3::addEndpoint("Client::setGamestate", JSONAPI::setGamestate);
        Layer3::addEndpoint("Client::setSocialstate", JSONAPI::setSocialstate);
        Layer3::addEndpoint("Client::getLocalclient", JSONAPI::getLocalclient);

        // Notify the other clients when we join and leave.
        std::atexit([]() { Layer1::Publish("Client::Leave", "{}"); });
        triggerUpdate();
    }
}
