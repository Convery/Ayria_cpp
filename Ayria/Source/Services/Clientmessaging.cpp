/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-09-01
    License: MIT
*/

#include <Global.hpp>
#include <openssl/curve25519.h>

namespace Services::Clientmessaging
{
    // Timestamp, Messagetype, Message

    // Process incoming messages from the network.
    static void __cdecl HandleUsermessage(uint64_t Timestamp, uint32_t AccountID, const char *Message, unsigned int Length)
    {
        std::array<uint8_t, 32> Encryptionkey{};

        // We can't decrypt anything if we don't have the clients key.
        if (const auto Client = Clientinfo::getClient(AccountID)) Encryptionkey = Client->Encryptionkey;
        else if (const auto Offline = Clientinfo::getOfflineclient(AccountID)) Encryptionkey = Offline.value().Encryptionkey;
        else [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Messagetype = Request.value<uint32_t>("Messagetype");
        const auto Checksum = Request.value<uint32_t>("Checksum");

        uint8_t Secret[32]{};
        if (NULL == X25519(Secret, Global.EncryptionkeyPrivate->data(), Encryptionkey.data())) [[unlikely]] return;

        const auto Sharedkey = Hash::Tiger192(Secret, 32);
        const auto Encrypted = Base85::Decode(Request.value<std::u8string>("Message"));
        const auto Decrypted = AES::Decrypt(Encrypted.data(), Encrypted.size(), Sharedkey.data(), Sharedkey.data());
        if (Checksum != Hash::WW32(Decrypted)) [[unlikely]] return;

        // Save to the DB in plaintext.
        try
        {
            Backend::Database()
                << "INSERT INTO Usermessage (Timestamp, Messagetype, From, Message) VALUES (?,?,?,?);"
                << Timestamp << Messagetype << AccountID << Decrypted;
        } catch (...) {}

        // Notify any callbacks.
        const auto Notification = JSON::Object_t({
            { "Messagetype", Messagetype },
            { "Timestamp", Timestamp },
            { "Message", Decrypted },
            { "From", AccountID }
        });
        Notifications::Publish("Messaging::onUsermessage", JSON::Dump(Notification).c_str());
    }
    static void __cdecl HandleGroupmessage(uint64_t Timestamp, uint32_t AccountID, const char *Message, unsigned int Length)
    {
        assert(false);
    }

    // JSON API for plugins to send messages.
    static std::string __cdecl sendUsermessage(JSON::Value_t &&Request)
    {
        std::array<uint8_t, 32> Encryptionkey{};
        const auto Message = Request.value<std::string>("Message");
        const auto AccountID = Request.value<uint32_t>("Recipient");
        const auto Messagetype = Request.value<uint32_t>("Messagetype");

        // We can't decrypt anything if we don't have the clients key.
        if (const auto Client = Clientinfo::getClient(AccountID)) Encryptionkey = Client->Encryptionkey;
        else if (const auto Offline = Clientinfo::getOfflineclient(AccountID)) Encryptionkey = Offline.value().Encryptionkey;
        else [[unlikely]] return {};

        uint8_t Secret[32]{};
        if (NULL == X25519(Secret, Global.EncryptionkeyPrivate->data(), Encryptionkey.data())) [[unlikely]] return{};

        const auto Checksum = Hash::WW32(Message);
        const auto Sharedkey = Hash::Tiger192(Secret, 32);
        const auto Encrypted = AES::Encrypt(Message.data(), Message.size(), Sharedkey.data(), Sharedkey.data());
        const auto Encoded = Base85::Encode(Encrypted);

        const auto JSON = JSON::Dump(JSON::Object_t({
            { "Messagetype", Messagetype },
            { "Checksum", Checksum },
            { "Message", Encoded }
        }));
        Communication::Publish("Clientmessaging::Usermessage", JSON, AccountID);
        return {};
    }
    static std::string __cdecl sendGroupmessage(JSON::Value_t &&Request)
    {
        assert(false); return {};
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Usermessage ("
                "Messagetype INTEGER NOT NULL, "
                "Timestamp INTEGER NOT NULL, "
                "From INTEGER NOT NULL, "
                "Message TEXT NOT NULL );";

            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Groupmessage ("
                "Messagetype INTEGER NOT NULL, "
                "Timestamp INTEGER NOT NULL, "
                "GroupID INTEGER NOT NULL, "
                "From INTEGER NOT NULL, "
                "Message TEXT NOT NULL );";
        } catch (...) {}

        //
        API::addEndpoint("Clientmessaging::sendUsermessage", sendUsermessage);
        API::addEndpoint("Clientmessaging::sendGroupmessage", sendGroupmessage);

        //
        Communication::registerHandler("Clientmessaging::Usermessage", HandleUsermessage, false);
        Communication::registerHandler("Clientmessaging::Groupmessage", HandleGroupmessage, true);
    }
}
