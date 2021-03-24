/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-27
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Clientinfo
{
    static std::vector<Client_t> Localclients{}, Remoteclients{};
    static Hashmap<uint32_t, uint32_t> Networkmap{};
    static Hashmap<uint32_t, uint32_t> Lastseen{};
    static Hashset<std::u8string> Clientnames{};

    // Fetch information about a client.
    bool isClientonline(uint32_t UserID)
    {
        const bool A = std::any_of(std::execution::par_unseq, Localclients.begin(), Localclients.end(),
                                   [=](const Client_t &Item) { return UserID == Item.UserID; });
        const bool B = std::any_of(std::execution::par_unseq, Remoteclients.begin(), Remoteclients.end(),
                                   [=](const Client_t &Item) { return UserID == Item.UserID; });
        return A || B;
    }
    Client_t *getLANClient(uint32_t NodeID)
    {
        if (Networkmap.contains(NodeID)) [[likely]]
        {
            const auto Result = std::find_if(std::execution::par_unseq, Localclients.begin(), Localclients.end(),
            [ID = Networkmap[NodeID]](const Client_t &Item) { return Item.UserID == ID; });

        if (Result != Localclients.end()) [[likely]]
            return &*Result;
        }

        return {};
    }
    Client_t *getWANClient(uint32_t UserID)
    {
        const auto Result = std::find_if(std::execution::par_unseq, Remoteclients.begin(), Remoteclients.end(),
            [=](const Client_t &Item) { return Item.UserID == UserID; });

        if (Result != Remoteclients.end()) [[likely]]
            return &*Result;

        return {};
    }
    bool isClientauthenticated(uint32_t UserID)
    {
        // If the clients info has been supplied from a server, they are authenticated.
        return std::any_of(std::execution::par_unseq, Remoteclients.begin(), Remoteclients.end(),
                           [=](const Client_t &Item) { return UserID == Item.UserID; });
    }

    // Update information about the world.
    static void Updateclients()
    {
        if (Global.Stateflags.isPrivate) [[unlikely]] return;

        const auto Request = JSON::Object_t({
            { "Username", std::u8string_view(Global.Username) },
            { "isOnline", Global.Stateflags.isOnline},
            { "UserID", Global.UserID }
        });
        Backend::Network::Transmitmessage("Clientinfo::Discovery", Request);

        if (Global.Stateflags.isOnline)
        {
            // TODO(tcn): Forward our information to some server.
        }

        // Clear outdated clients.
        const auto Currenttime = time(NULL);
        for (const auto &[ClientID, Timestamp] : Lastseen)
        {
            if ((Timestamp + 15) < Currenttime)
            {
                std::erase_if(Remoteclients, [&](const auto &Item) { return Item.UserID == ClientID; });
                std::erase_if(Localclients, [&](const auto &Item) { return Item.UserID == ClientID; });

                for (const auto &[Node, Client] : Networkmap)
                {
                    if (ClientID == Client)
                    {
                        Networkmap.erase(Node);
                        break;
                    }
                }
            }
        }
    }

    // Handle local discovery notifications.
    static void __cdecl Discoveryhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Username = Request.value<std::u8string>("Username");
        const auto UserID = Request.value<uint32_t>("UserID");

        // If the client can't even format a simple request, block them.
        if (!UserID || Username.empty()) [[unlikely]]
        {
            Infoprint(va("Blocking client %08X due to invalid discovery request.", UserID));
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // If the user doesn't want to be associated, block them.
        if (Social::Relations::isBlocked(UserID)) [[unlikely]]
        {
            Infoprint(va("Blocking client %08X due to relationship status.", UserID));
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // If we need to add a client..
        if (!Networkmap.contains(NodeID)) [[unlikely]]
        {
            // Notify the developers about this.
            Debugprint(va("Found a new LAN client with ID %08X", UserID));
            Localclients.emplace_back(UserID, &*Clientnames.insert(Username).first);
            Networkmap.emplace(NodeID, UserID);
        }

        // Update their last-seen timestamp.
        const auto Client = getLANClient(NodeID);
        Lastseen[Client->UserID] = uint32_t(time(NULL));

        // Update the database for the plugins.
        Backend::Database() << "insert or replace into Onlineclients (ClientID, Lastseen, Uername, Local) values (?, ?, ?, ?);"
                            << Client->UserID << Lastseen[Client->UserID] << Encoding::toNarrow(*Client->Username) << true;
    }

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl isClientonline(JSON::Value_t &&Request)
        {
            const auto ClientID = Request.value<uint32_t>("ClientID");

            const auto Response = JSON::Object_t({ { "isOnline", Clientinfo::isClientonline(ClientID)} });
            return JSON::Dump(Response);
        }
        static std::string __cdecl getLocalclients(JSON::Value_t &&)
        {
            JSON::Array_t Result;
            Result.reserve(Localclients.size());

            for (const auto &Client : Localclients)
            {
                const auto Cryptokey = getCryptokey(Client.UserID);
                Result.emplace_back(JSON::Object_t({
                    { "Username", Client.Username ? *Client.Username : u8""s },
                    { "UserID", Client.UserID },
                    { "Sharedkey", Cryptokey }
                }));
            }

            return JSON::Dump(Result);
        }
        static std::string __cdecl getAccountinfo(JSON::Value_t &&)
        {
            const auto Response = JSON::Object_t({
                { "Username", std::u8string(Global.Username) },
                { "Locale", std::u8string(Global.Locale) },
                { "AccountID", Global.UserID }
            });

            return JSON::Dump(Response);
        }
    }

    //
    void Initializediscovery()
    {
        // Persistent database entry of known clients.
        Backend::Database() << "create table Onlineclients ("
                               "ClientID integer primary key unique not null, "
                               "Lastseen integer, "
                               "Username text, "
                               "Local boolean);";

        Backend::Enqueuetask(5000, Updateclients);
        Backend::Network::Registerhandler("Clientinfo::Discovery", Discoveryhandler);

        Backend::API::addEndpoint("Clientinfo::isClientonline", API::isClientonline);
        Backend::API::addEndpoint("Clientinfo::getAccountinfo", API::getAccountinfo);
        Backend::API::addEndpoint("Clientinfo::getLocalclients", API::getLocalclients);
    }
}
