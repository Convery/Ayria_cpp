/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-27
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Clientinfo
{
    std::vector<Client_t> Localclients, Remoteclients;
    Hashmap<uint32_t, uint32_t> Networkmap;
    Hashmap<uint32_t, uint32_t> Lastseen;
    Hashset<std::u8string> Clientnames;

    // LAN clients are identified by their random ID.
    std::vector<Client_t *> Listclients(bool onlyLAN)
    {
        std::vector<Client_t *> Result;
        Result.reserve(Localclients.size() + (!onlyLAN * Remoteclients.size()));

        for (auto &Client : Localclients)
            Result.emplace_back(&Client);

        if (!onlyLAN)
            for (auto &Client : Remoteclients)
                Result.emplace_back(&Client);

        return Result;
    }
    Client_t *getLocalclient(uint32_t NodeID)
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

    // Periodically clean up the maps.
    static void Clearoutdated()
    {
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
    };

    // Handle local client discovery.
    static void __cdecl sendDiscovery()
    {
        const auto Object = JSON::Object_t({
            { "Username", std::u8string_view(Global.Username) },
            { "isOnline", Global.isOnline},
            { "UserID", Global.UserID }
        });

        Backend::Network::Sendmessage("Clientdiscovery", JSON::Dump(Object));
    }
    static void __cdecl Discoveryhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Username = Request.value<std::u8string>("Username");
        const auto isOnline = Request.value<bool>("isOnline");
        const auto UserID = Request.value<uint32_t>("UserID");

        // If the client can't even format a simple request, block them.
        if (!UserID || Username.empty()) [[unlikely]]
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // If we need to add a client..
        if (!Networkmap.contains(NodeID)) [[unlikely]]
        {
            Localclients.emplace_back(UserID, nullptr, &*Clientnames.insert(Username).first, nullptr);
            Networkmap.emplace(NodeID, UserID);
        }

        // Update their last-seen timestamp.
        const auto Client = getLocalclient(NodeID);
        Lastseen[Client->UserID] = uint32_t(time(NULL));

        // Need to request and verify their ticket.
        if (isOnline && !Client->Authticket) [[unlikely]]
        {
            // TODO(tcn): Create some auth service to handle this.
        }
    }

    //
    void Initializediscovery()
    {
        Backend::Enqueuetask(5000, sendDiscovery);
        Backend::Enqueuetask(5000, Clearoutdated);
        Backend::Network::addHandler("Clientdiscovery", Discoveryhandler);
    }
}
