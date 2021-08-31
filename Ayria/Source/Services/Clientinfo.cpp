/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-31
    License: MIT
*/

#include <Global.hpp>
#include <openssl/curve25519.h>

namespace Services::Clientinfo
{
    static Hashmap<uint32_t, Client_t> Activeclients{};
    static Client_t Localclient{};

    // Format as JSON so that other tools can read it.
    static std::string toJSON(const Client_t &Client)
    {
        return JSON::Dump(JSON::Object_t({
            { "ModID", Client.ModID },
            { "GameID", Client.GameID },
            { "Username", Client.Username },
            { "Signingkey", Base85::Encode(Client.Signingkey) },
            { "Encryptionkey", Base85::Encode(Client.Encryptionkey) }
        }));
    }
    static std::pair<Client_t, bool> fromJSON(std::string_view JSON)
    {
        const auto Object = JSON::Parse(JSON);

        // Verify that all fields are included.
        const auto Valid = Object.contains("ModID") && Object.contains("GameID") &&
                Object.contains("Username") && Object.contains("Signingkey") &&
                Object.contains("Encryptionkey");

        Client_t Client{};
        Client.ModID = Object.value<uint32_t>("ModID");
        Client.GameID = Object.value<uint32_t>("GameID");
        Client.Username = Object.value<std::u8string>("Username");
        std::ranges::move(Base85::Decode<uint8_t>(Object.value<std::u8string>("Signingkey")), Client.Signingkey.data());
        std::ranges::move(Base85::Decode<uint8_t>(Object.value<std::u8string>("Encryptionkey")), Client.Encryptionkey.data());

        return { Client, Valid };
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
        Communication::Publish("Clientinfo::Hello", toJSON(Localclient));
    }
    static void __cdecl Clientgoodbye()
    {
        // Post to the local network.
        Communication::Publish("Clientinfo::Goodbye", "Sign me");
    }

    // Handle incoming messages.
    static void __cdecl Handlehello(uint32_t AccountID, const char *Message, unsigned int Length)
    {
        const auto [Client, Valid] = fromJSON(std::string_view(Message, Length));

        // Sanity-checking.
        if (!Valid) [[unlikely]] return;
        if (AccountID != Hash::WW32(Client.Signingkey)) [[unlikely]] return;

        // Save for later.
        Activeclients[AccountID] = Client;
    }
    static void __cdecl Handlegoodbye(uint32_t AccountID, const char *, unsigned int)
    {
        Activeclients.erase(AccountID);
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        // Notify the other clients about start and termination.
        Backend::Enqueuetask(5000, Clienthello);
        std::atexit(Clientgoodbye);

        // Add generalized handlers for the communication systems.
        Communication::registerHandler("Clientinfo::Hello", Handlehello, true);
        Communication::registerHandler("Clientinfo::Goodbye", Handlegoodbye, true);

        // TODO(tcn): JSON API
    }
}
