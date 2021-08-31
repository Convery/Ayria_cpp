/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-31
    License: MIT
*/

#include <Global.hpp>
#include <openssl/curve25519.h>

namespace Services::Clientinfo
{
    static Hashmap<uint32_t, std::shared_ptr<Client_t>> Activeclients{};
    static Client_t Localclient{};

    // Fetch client info by its ID or list all.
    std::shared_ptr<Client_t> getClient(uint32_t AccountID)
    {
        if (!Activeclients.contains(AccountID)) [[unlikely]] return nullptr;
        return Activeclients[AccountID];
    }
    std::unordered_set<std::shared_ptr<Client_t>> listClients()
    {
        std::unordered_set<std::shared_ptr<Client_t>> Result;
        Result.reserve(Activeclients.size());

        for (const auto &Client : Activeclients | std::views::values)
            Result.insert(Client);
        return Result;
    }

    // Let the local network know about us.
    static void __cdecl Clienthello()
    {
        Localclient.ModID = Global.ModID;
        Localclient.GameID = Global.GameID;
        Localclient.Username = Global.Username->c_str();

        // Expensive so only derive the key when needed.
        if (Localclient.Signingkey != *Global.SigningkeyPublic) [[unlikely]]
        {
            Localclient.Signingkey = *Global.SigningkeyPublic;
            X25519_public_from_private(Localclient.Encryptionkey.data(), Global.EncryptionkeyPrivate->data());
        }

        // Post to the local network.
        Communication::Publish("Clientinfo::Hello", JSON::Dump(toJSON(Localclient)));
    }
    static void __cdecl Clientgoodbye()
    {
        // Post to the local network.
        Communication::Publish("Clientinfo::Goodbye", "Sign me");
    }

    // Handle incoming messages.
    static void __cdecl Handlegoodbye(uint32_t AccountID, const char *, unsigned int)
    {
        Activeclients.erase(AccountID);
    }
    static void __cdecl Handlehello(uint32_t AccountID, const char *Message, unsigned int Length)
    {
        const auto [Client, Valid] = fromJSON(std::string_view(Message, Length));

        // Sanity-checking.
        if (!Valid) [[unlikely]] return;
        if (AccountID != Hash::WW32(Client.Signingkey)) [[unlikely]] return;

        // Save for later.
        Activeclients[AccountID] = std::make_shared<Client_t>(Client);
    }

    // JSON API for accessing the client state and updating settings.
    static std::string __cdecl setGameinfo(JSON::Value_t &&Request)
    {
        Global.GameID = Request.value("GameID", Global.GameID);
        Global.ModID = Request.value("ModID", Global.ModID);
        return {};
    }
    static std::string __cdecl setGamestate(JSON::Value_t &&Request)
    {
        Global.Settings.isHosting = Request.value("isHosting", Global.Settings.isHosting);
        Global.Settings.isIngame = Request.value("isIngame", Global.Settings.isIngame);
        return {};
    }
    static std::string __cdecl setSocialstate(JSON::Value_t &&Request)
    {
        Global.Settings.isPrivate = Request.value("isPrivate", Global.Settings.isPrivate);
        Global.Settings.isAway = Request.value("isAway", Global.Settings.isAway);
        return {};
    }
    static std::string __cdecl getLocalclient(JSON::Value_t &&)
    {
        return JSON::Dump(toJSON(Localclient));
    }
    static std::string __cdecl getActiveclients(JSON::Value_t &&)
    {
        JSON::Array_t Result{}; Result.reserve(Activeclients.size());
        for (const auto &Client : Activeclients | std::views::values)
            Result.emplace_back(toJSON(*Client));
        return JSON::Dump(Result);
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        // Add generalized handlers for the communication systems (currently LAN only).
        Communication::registerHandler("Clientinfo::Goodbye", Handlegoodbye, true);
        Communication::registerHandler("Clientinfo::Hello", Handlehello, true);

        // JSON access from the plugins, setting should only be called from Platformwrapper or equivalent.
        API::addEndpoint("Clientinfo::getActiveclients", getActiveclients);
        API::addEndpoint("Clientinfo::getLocalclient", getLocalclient);
        API::addEndpoint("Clientinfo::setSocialstate", setSocialstate);
        API::addEndpoint("Clientinfo::setGamestate", setGamestate);
        API::addEndpoint("Clientinfo::setGameinfo", setGameinfo);

        // Notify the other clients about start and termination.
        Backend::Enqueuetask(5000, Clienthello);
        std::atexit(Clientgoodbye);
    }
}
