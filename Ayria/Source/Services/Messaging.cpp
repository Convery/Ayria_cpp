/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-24
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Messaging
{
    // Database helpers.
    static void addUsermessage(uint32_t SourceID, uint32_t TargetID, uint32_t Checksum, uint32_t Timestamp, std::string_view Message)
    {
        try { DB::Query()
            << "INSERT INTO Clientmessages (SourceID, TargetID, Checksum, Timestamp, Message) VALUES (?, ?, ?, ?, ?);"
            << SourceID << TargetID << Checksum << Timestamp << (Base64::isValid(Message) ? Message : Base64::Encode(Message));
        } catch (...) {}
    }
    static void addGroupmessage(uint32_t SourceID, uint64_t GroupID, uint32_t Checksum, uint32_t Timestamp, std::string_view Message)
    {
        try { DB::Query()
            << "INSERT INTO Groupmessages (SourceID, GroupID, Checksum, Timestamp, Message) VALUES (?, ?, ?, ?, ?);"
            << SourceID << GroupID << Checksum << Timestamp << (Base64::isValid(Message) ? Message : Base64::Encode(Message));
        } catch (...) {}
    }

    // Send an instant-message to clients.
    bool Sendusermessage(uint32_t AccountID, std::string_view Message, bool Encrypt)
    {
        // Checksum of the raw input to ensure consistency.
        const auto Checksum = Hash::WW32(Message);
        std::string Tempstorage;

        // Add the message to the database regardless of outcome.
        addUsermessage(Global.AccountID, AccountID, Checksum, time(NULL), Message);

        // Encryption is provided via the clients shared key.
        if (Encrypt)
        {
            const auto Cryptokey = Clientinfo::Derivecryptokey(AccountID);

            // No info in the DB, try again later.
            if (Cryptokey.empty()) return false;

            // Should work well enough for basic privacy.
            AES::AES_t Cryptocontext(Cryptokey.data(), Cryptokey.size(), Cryptokey.data(), Cryptokey.size());
            const auto View = Cryptocontext.Encrypt_192(Message.data(), Message.size());
            Tempstorage = Base64::Encode(View);
            Message = Tempstorage;
        }

        const auto Request = JSON::Object_t({
            { "Checksum", Checksum },
            { "TargetID", AccountID },
            { "SourceID", Global.AccountID },
            { "Timestamp", uint32_t(time(NULL)) },
            { "Message", (Base64::isValid(Message) ? Message : Base64::Encode(Message)) }
        });
        Network::LAN::Publish("AYRIA::Usermessage", JSON::Dump(Request));
        return true;
    }
    bool Sendgroupmessage(uint64_t GroupID, std::string_view Message)
    {
        // if !isInGroup() ret false

        // Checksum of the raw input to ensure consistency.
        const auto Checksum = Hash::WW32(Message);

        // Add the message to the database regardless of outcome.
        addGroupmessage(Global.AccountID, GroupID, Checksum, time(NULL), Message);

        const auto Request = JSON::Object_t({
            { "GroupID", GroupID },
            { "Checksum", Checksum },
            { "SourceID", Global.AccountID },
            { "Timestamp", uint32_t(time(NULL)) },
            { "Message", (Base64::isValid(Message) ? Message : Base64::Encode(Message)) }
        });
        Network::LAN::Publish("AYRIA::Groupmessage", JSON::Dump(Request));
        return true;
    }

    // Callbacks for the LAN messages.
    static void __cdecl Usermessage(unsigned int AccountID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto TargetID = Request.value<uint32_t>("TargetID");
        const auto SourceID = Request.value<uint32_t>("SourceID");

        // Sanity checking..
        if (Global.AccountID != TargetID) return;
        if (AccountID != SourceID) return;

        const auto Checksum = Request.value<uint32_t>("Checksum");
        auto Timestamp = Request.value<uint32_t>("Timestamp");
        auto Payload = Request.value<std::string>("Message");

        // Needs to be decoded.
        if (Hash::WW32(Payload) != Checksum)
        {
            const auto View = Base64::Decode_inplace(Payload.data(), Payload.size());
            Payload.resize(View.size());
        }

        // Needs to be decrypted.
        if (Hash::WW32(Payload) != Checksum)
        {
            const auto Cryptokey = Clientinfo::Derivecryptokey(SourceID);
            if (Cryptokey.empty()) return;  // Nani?

            AES::AES_t Cryptocontext(Cryptokey.data(), Cryptokey.size(), Cryptokey.data(), Cryptokey.size());
            const auto View = Cryptocontext.Decrypt_192(Payload.data(), Payload.size());
            Payload = { View.begin(), View.end() };
        }

        // Nothing more we can do.
        if (Hash::WW32(Payload) != Checksum) return;

        // Prevent fuckery.
        Timestamp = std::max(Timestamp, uint32_t(time(NULL) - 30U));

        // Log this message for later.
        addUsermessage(SourceID, TargetID, Checksum, Timestamp, Payload);

        // Notify the plugins about the relevant info.
        const auto Notification = JSON::Dump(JSON::Object_t({
            { "SourceID", SourceID },
            { "Message", Payload }
        }));
        Postevent("AYRIA::Usermessage", Notification.data(), Notification.size());
    }
    static void __cdecl Groupmessage(unsigned int AccountID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto SourceID = Request.value<uint32_t>("SourceID");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Sanity checking..
        if (AccountID != SourceID) return;
        // if not in group return

        const auto Checksum = Request.value<uint32_t>("Checksum");
        auto Timestamp = Request.value<uint32_t>("Timestamp");
        auto Payload = Request.value<std::string>("Message");

        // Needs to be decoded.
        if (Hash::WW32(Payload) != Checksum)
        {
            const auto View = Base64::Decode_inplace(Payload.data(), Payload.size());
            Payload.resize(View.size());
        }

        // Nothing more we can do.
        if (Hash::WW32(Payload) != Checksum) return;

        // Prevent fuckery.
        Timestamp = std::max(Timestamp, uint32_t(time(NULL) - 30U));

        // Log this message for later.
        addGroupmessage(SourceID, GroupID, Checksum, Timestamp, Message);

        // Notify the plugins about the relevant info.
        const auto Notification = JSON::Dump(JSON::Object_t({
            { "SourceID", SourceID },
            { "GroupID", GroupID },
            { "Message", Payload }
        }));
        Postevent("AYRIA::Groupmessage", Notification.data(), Notification.size());
    }

    // Let the plugins send messages via JSON.
    static std::string __cdecl Sendmessage(JSON::Value_t &&Request)
    {
        const auto toAccountID = Request.value<uint32_t>("toAccountID");
        const auto toGroupID = Request.value<uint32_t>("toGroupID");
        const auto Message = Request.value<std::string>("Message");
        const auto Encrypt = Request.value<bool>("Encrypt");

        bool Result{};
        if (NULL != toAccountID) Result |= Sendusermessage(toAccountID, Message, Encrypt);
        if (NULL != toGroupID) Result |= Sendgroupmessage(toGroupID, Message);

        if (!Result) return R"({ "Error" : "Could not send, try again later." })";
        else return "{}";
    }

    // Set up the service.
    void Initialize()
    {
        // Register the LAN callbacks.
        Network::LAN::Register("AYRIA::Usermessage", Usermessage);
        Network::LAN::Register("AYRIA::Groupmessage", Groupmessage);

        // Register JSON endpoints.
        Backend::API::addEndpoint("Sendmessage", Sendmessage);
    }
}
