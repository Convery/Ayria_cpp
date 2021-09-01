/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-13
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Services
{
    namespace Fileshare { void Initialize(); }
    namespace Usergroups { void Initialize(); }
    namespace Matchmaking { void Initialize(); }
    namespace Clientpresence { void Initialize(); }
    namespace Clientrelations { void Initialize(); }
    namespace Clientmessaging { void Initialize(); }

    namespace Clientinfo
    {
        struct Client_t
        {
            uint32_t GameID, ModID;
            std::u8string Username;
            std::array<uint8_t, 32> Signingkey;
            std::array<uint8_t, 32> Encryptionkey;

            // Internal.
            uint64_t Timestamp;
        };

        // Format as JSON so that other tools can read it.
        inline std::pair<Client_t, bool> fromJSON(const JSON::Value_t &Object)
        {
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
        inline std::pair<Client_t, bool> fromJSON(std::string_view JSON)
        {
            return fromJSON(JSON::Parse(JSON));
        }
        inline JSON::Object_t toJSON(const Client_t &Client)
        {
            return JSON::Object_t({
                { "ModID", Client.ModID },
                { "GameID", Client.GameID },
                { "Username", Client.Username },
                { "Signingkey", Base85::Encode(Client.Signingkey) },
                { "Encryptionkey", Base85::Encode(Client.Encryptionkey) }
                });
        }

        // Fetch client info by its ID or list all.
        std::shared_ptr<Client_t> getClient(uint32_t AccountID);
        std::unordered_set<std::shared_ptr<Client_t>> listClients();
        std::optional<Client_t> getOfflineclient(uint32_t AccountID);

        // Add the handlers and tasks.
        void Initialize();
    }

    inline void Initialize()
    {
        Clientinfo::Initialize();
        Clientpresence::Initialize();
        Clientrelations::Initialize();
        Clientmessaging::Initialize();
    }
}
