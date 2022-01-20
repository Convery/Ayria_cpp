/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-27
    License: MIT
*/

#include <Global.hpp>

namespace Services::Messaging
{
    // Helpers, includes the groups owner even if not listed as a member (for some reason).
    static std::unordered_set<std::string> getMembers(const std::string &GroupID)
    {
        auto Temp = AyriaAPI::Groups::getMembers(GroupID);
        Temp.push_back(GroupID);

        return { Temp.begin(), Temp.end() };
    }

    // Query the current crypto-base for the group, public groups may use a default key (SHA256(0)).
    static std::string getCryptokey(const std::string &GroupID)
    {
        uint64_t Cryptobase{};
        Backend::Database() << "SELECT Cryptobase FROM Groupkey WHERE GroupID = ?;" << GroupID >> Cryptobase;
        return Hash::SHA256(Cryptobase);
    }

    // Internal access for the services.
    void sendGroupmessage(const std::string &GroupID, uint32_t Messagetype, std::string_view Message)
    {
        // Create a unique seed for the IV and generate the shared key to use.
        const auto IVSeed = Hash::WW32(std::chrono::system_clock::now().time_since_epoch().count());
        const auto Checksum = Hash::WW32(Message);
        const auto Key = getCryptokey(GroupID);
        const auto IV = Hash::SHA256(IVSeed);

         // Save to the database for later lookups.
         Backend::Database()
             << "INSERT OR REPLACE INTO Groupmessage VALUES (?, ?, ?, ?, ?);"
             << Global.getLongID() << GroupID << Messagetype
             << std::chrono::system_clock::now().time_since_epoch().count()
             << Base85::Encode(Message);

         // Should be optimized to be in-place..
         const auto Payload = Base85::Encode(AES::Encrypt_256<char>(Key, IV, Message));

         // Put it in the system.
         Layer1::Publish("Messaging::Groupmessage", JSON::Object_t({
             { "Messagetype", Messagetype },
             { "Checksum", Checksum },
             { "Payload", Payload },
             { "GroupID", GroupID },
             { "IVSeed", IVSeed }
         }));
    }
    void sendClientmessage(const std::string &ClientID, uint32_t Messagetype, std::string_view Message)
    {
        // Create a unique seed for the IV and generate the shared key to use.
        const auto IVSeed = Hash::WW32(std::chrono::system_clock::now().time_since_epoch().count());
        const auto Key = qDSA::Generatesecret(Base58::Decode(ClientID), *Global.Privatekey);
        const auto Checksum = Hash::WW32(Message);
        const auto IV = Hash::SHA256(IVSeed);

        // Save to the database for later lookups.
        Backend::Database()
            << "INSERT OR REPLACE INTO Clientmessage VALUES (?, ?, ?, ?, ?);"
            << Global.getLongID() << ClientID << Messagetype
            << std::chrono::system_clock::now().time_since_epoch().count()
            << Base85::Encode(Message);

        // Should be optimized to be in-place..
        const auto Payload = Base85::Encode(AES::Encrypt_256<char>(Key, IV, Message));

        // Put it in the system.
        Layer1::Publish("Messaging::Clientmessage", JSON::Object_t({
            { "Messagetype", Messagetype },
            { "Checksum", Checksum },
            { "ClientID", ClientID },
            { "Payload", Payload },
            { "IVSeed", IVSeed }
        }));
    }
    void sendMulticlientmessage(const std::unordered_set<std::string> &ClientIDs, uint32_t Messagetype, std::string_view Message)
    {
        for (const auto &ID : ClientIDs)
            sendClientmessage(ID, Messagetype, Message);
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onGroupmessage(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto GroupID = Request.value<std::string>("GroupID");

            // Not intended for us, no need to parse further.
            if (!getMembers(GroupID).contains(Global.getLongID())) return false;

            const auto Messagetype = Request.value<uint32_t>("Messagetype");
            const auto Checksum = Request.value<uint32_t>("Checksum");
            const auto IVSeed = Request.value<uint32_t>("IVSeed");
            auto Payload = Request.value<std::string>("Payload");

            // Generate crypto properties.
            const auto Key = getCryptokey(GroupID);
            const auto IV = Hash::SHA256(IVSeed);

            // Should be optimized to be in-place..
            Payload = AES::Decrypt_256<char>(Key, IV, Base85::Decode(Payload));

            // Failed to decrypt properly, discard it.
            if (Checksum != Hash::WW32(Payload)) [[unlikely]]
                return false;

            // Notify the plugins about receiving this message.
            Layer4::Publish("Messaging::onGroupmessage", JSON::Object_t({
                { "Messagetype", Messagetype },
                { "Timestamp", Timestamp },
                { "GroupID", GroupID },
                { "Payload", Payload },
                { "From", LongID }
            }));

            // And insert into the database for later lookups.
            Backend::Database()
                << "INSERT OR REPLACE INTO Groupmessage VALUES (?, ?, ?, ?, ?);"
                << LongID << GroupID << Messagetype << Timestamp << Base85::Encode(Payload);

            return true;

        }
        static bool __cdecl onClientmessage(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto ClientID = Request.value<std::string>("ClientID");

            // Not intended for us, no need to parse further.
            if (ClientID != Global.getLongID()) return false;

            const auto Messagetype = Request.value<uint32_t>("Messagetype");
            const auto Checksum = Request.value<uint32_t>("Checksum");
            const auto IVSeed = Request.value<uint32_t>("IVSeed");
            auto Payload = Request.value<std::string>("Payload");

            // Generate crypto properties.
            const auto Key = qDSA::Generatesecret(Base58::Decode(std::string_view(LongID)), *Global.Privatekey);
            const auto IV = Hash::SHA256(IVSeed);

            // Should be optimized to be in-place..
            Payload = AES::Decrypt_256<char>(Key, IV, Base85::Decode(Payload));

            // Failed to decrypt properly, discard it.
            if (Checksum != Hash::WW32(Payload)) [[unlikely]]
                return false;

            // Notify the plugins about receiving this message.
            Layer4::Publish("Messaging::onClientmessage", JSON::Object_t({
                { "Messagetype", Messagetype },
                { "Timestamp", Timestamp },
                { "Payload", Payload },
                { "From", LongID }
            }));

            // And insert into the database for later lookups.
            Backend::Database()
                << "INSERT OR REPLACE INTO Clientmessage VALUES (?, ?, ?, ?, ?);"
                << LongID << ClientID << Messagetype << Timestamp << Base85::Encode(Payload);

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl sendClientmessage(JSON::Value_t &&Request)
        {
            if (!Request.contains_all("ClientID", "Messagetype", "Message"))
                return R"({ "Error" : "Required arguments: ClientID, Messagetype, Message" })";

            const auto Message = Request.value<std::string>("Message");
            const auto ClientID = Request.value<std::string>("ClientID");
            const auto Messagetype = Request["Messagetype"].Type == JSON::Type_t::Unsignedint ?
                uint32_t(Request["Messagetype"]) : Hash::WW32(Request["Messagetype"].get<std::u8string>());

            Messaging::sendClientmessage(ClientID, Messagetype, Message);
            return {};
        }
        static std::string __cdecl sendGroupmessage(JSON::Value_t &&Request)
        {
            if (!Request.contains_all("GroupID", "Messagetype", "Message"))
                return R"({ "Error" : "Required arguments: GroupID, Messagetype, Message" })";

            const auto Message = Request.value<std::string>("Message");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Messagetype = Request["Messagetype"].Type == JSON::Type_t::Unsignedint ?
                uint32_t(Request["Messagetype"]) : Hash::WW32(Request["Messagetype"].get<std::u8string>());

            // Basic sanity-checking.
            if (!getMembers(GroupID).contains(Global.getLongID())) [[unlikely]]
                return R"({ "Error" : "Invalid GroupID" })";

            Messaging::sendGroupmessage(GroupID, Messagetype, Message);
            return {};
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Groupkey ("
            "GroupID TEXT PRIMARY KEY REFERENCES Groups(GroupID) ON DELETE CASCADE, "
            "Cryptobase INTEGER DEFAULT 0 );";

        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Clientmessage ("
            "Source TEXT REFERENCES Account(Publickey), "
            "Target TEXT REFERENCES Account(Publickey), "
            "Messagetype INTEGER NOT NULL, "
            "Timestamp INTEGER NOT NULL, "
            "Message TEXT NOT NULL );";

        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Groupmessage ("
            "Source TEXT REFERENCES Account(Publickey), "
            "Target TEXT REFERENCES Groups(GroupID), "
            "Messagetype INTEGER NOT NULL, "
            "Timestamp INTEGER NOT NULL, "
            "Message TEXT NOT NULL );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Messaging::Groupmessage", Messagehandlers::onGroupmessage);
        Layer2::addMessagehandler("Messaging::Clientmessage", Messagehandlers::onClientmessage);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Messaging::sendGroupmessage", JSONAPI::sendGroupmessage);
        Layer3::addEndpoint("Messaging::sendClientmessage", JSONAPI::sendClientmessage);
    }
}
