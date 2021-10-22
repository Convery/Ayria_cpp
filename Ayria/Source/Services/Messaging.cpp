/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-09
    License: MIT
*/

#include <Global.hpp>

namespace Services::Messaging
{
    // Helpers for getting / setting cryptokeys.
    static void setCryptokey(const std::string &GroupID, const std::string &Key)
    {
        try
        {
            Backend::Database()
                << "INSERT INTO Groupkey VALUES(?,?);"
                << GroupID << Key;
        } catch (...) {}
    }
    static std::optional<std::array<uint8_t, 32>> getCryptokey(const std::string &LongID)
    {
        std::string Key{};
        try { Backend::Database() << "SELECT Encryptionkey FROM Groupkey WHERE GroupID = ?;" << LongID >> Key; } catch (...) {}
        if (Key.empty()) return {};

        std::array<uint8_t, 32> Result;
        std::ranges::move(Base85::Decode<uint8_t>(Key), Result.begin());
        return Result;
    }

    // Internal access for the services.
    void sendUsermessage(const std::string &UserID, std::string_view Messagetype, std::string_view Payload)
    {
        // In some cases, of broadcasting, we include ourselves.
        if (UserID == Global.getLongID()) [[unlikely]] return;

        const auto Key = Hash::SHA256(qDSA::Generatesecret(Base58::Decode(UserID), *Global.Privatekey));
        const auto Encrypted = Base85::Encode(AES::Encrypt_256(Key, Payload));
        const auto Object = JSON::Object_t({
            { "Messagetype", Hash::WW32(Messagetype) },
            { "Checksum", Hash::WW32(Payload) },
            { "Payload", Encrypted },
            { "UserID", UserID }
        });

        Layer1::Publish("Usermessage", JSON::Dump(Object));
    }
    void sendGroupmessage(const std::string &GroupID, std::string_view Messagetype, std::string_view Payload)
    {
        const auto Cryptokey = getCryptokey(GroupID);
        const auto Group = Groups::getGroup(GroupID);

        // Group needs encryption and we don't have a key.
        if (!Cryptokey && (!Group || !Group->isPublic)) return;

        const auto Message = Cryptokey ? Base85::Encode(AES::Encrypt_256(*Cryptokey, Payload)) : Base85::Encode(Payload);
        const auto Object = JSON::Object_t({
            { "Messagetype", Hash::WW32(Messagetype) },
            { "Checksum", Hash::WW32(Payload) },
            { "Payload", Message },
            { "GroupID", GroupID }
        });

        Layer1::Publish("Groupmessage", JSON::Dump(Object));
    }
    void sendMultiusermessage(const std::unordered_set<std::string> &UserIDs, std::string_view Messagetype, std::string_view Payload)
    {
        for (const auto &ID : UserIDs) sendUsermessage(ID, Messagetype, Payload);
    }

    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onKeychange(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto Newkey = Request.value<std::string>("Newkey");
            const auto Checksum = Request.value<uint32_t>("Checksum");
            const auto GroupID = Request.value<std::string>("GroupID");

            // Basic sanity checking.
            if (Newkey.empty()) [[unlikely]] return false;
            if (!Base85::isValid(Newkey)) [[unlikely]] return false;
            if (!Groups::getGroup(GroupID)) [[unlikely]] return false;
            if (!AyriaAPI::Groups::getModerators(GroupID).contains(LongID)) [[unlikely]] return false;

            // We can only decrypt if we have the key.
            if (const auto Cryptokey = getCryptokey(GroupID))
            {
                const auto Decrypted = AES::Decrypt_256<char>(*Cryptokey, Base85::Decode(Newkey));
                if (Hash::WW32(Decrypted) != Checksum) [[unlikely]] return false;

                setCryptokey(GroupID, Decrypted);
            }

            return true;
        }
        static bool __cdecl onUsermessage(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto UserID = Request.value<std::string>("UserID");
            const auto Checksum = Request.value<uint32_t>("Checksum");
            const auto Payload = Request.value<std::string>("Payload");
            const auto Messagetype = Request.value<uint32_t>("Messagetype");
            const auto Received = std::chrono::utc_clock::now().time_since_epoch().count();

            // Basic sanity checking.
            if (Payload.empty()) [[unlikely]] return false;
            if (!Base85::isValid(Payload)) [[unlikely]] return false;
            if (!Clientinfo::getClient(UserID)) [[unlikely]] return false;

            if (UserID == Global.getLongID())
            {
                const auto Key = Hash::SHA256(qDSA::Generatesecret(Base58::Decode(std::string(LongID)), *Global.Privatekey));
                const auto Decrypted = AES::Decrypt_256(Key, Base85::Decode(Payload));
                if (Hash::WW32(Decrypted) != Checksum) [[unlikely]] return false;

                try
                {
                    Backend::Database()
                        << "INSERT INTO Usermessages VALUES (?,?,?,?,?,?,?);"
                        << std::string(LongID)
                        << UserID
                        << Messagetype
                        << Checksum
                        << Received
                        << Timestamp
                        << Base85::Encode(Decrypted);
                } catch (...) {};
            }
            else
            {
                try
                {
                    Backend::Database()
                        << "INSERT INTO Usermessages VALUES (?,?,?,?,?,?,?);"
                        << std::string(LongID)
                        << UserID
                        << Messagetype
                        << Checksum
                        << Received
                        << Timestamp
                        << Payload;
                } catch (...) {};
            }

            return true;
        }
        static bool __cdecl onGroupmessage(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Request = JSON::Parse(Message, Length);
            const auto Checksum = Request.value<uint32_t>("Checksum");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Payload = Request.value<std::string>("Payload");
            const auto Messagetype = Request.value<uint32_t>("Messagetype");
            const auto Received = std::chrono::utc_clock::now().time_since_epoch().count();

            // Basic sanity checking.
            if (Payload.empty()) [[unlikely]] return false;
            if (!Base85::isValid(Payload)) [[unlikely]] return false;
            if (!Groups::getGroup(GroupID)) [[unlikely]] return false;
            if (!AyriaAPI::Groups::getMembers(GroupID).contains(LongID)) [[unlikely]] return false;

            // We can only decrypt if we have the key.
            if (const auto Cryptokey = getCryptokey(GroupID))
            {
                const auto Decrypted = AES::Decrypt_256(*Cryptokey, Base85::Decode(Payload));
                if (Hash::WW32(Decrypted) != Checksum) [[unlikely]] return false;

                try
                {
                    Backend::Database()
                        << "INSERT INTO Groupmessages VALUES (?,?,?,?,?,?);"
                        << std::string(LongID)
                        << GroupID
                        << Messagetype
                        << Checksum
                        << Received
                        << Timestamp
                        << Base85::Encode(Decrypted);
                } catch (...) {};
            }
            else
            {
                try
                {
                    Backend::Database()
                        << "INSERT INTO Groupmessages VALUES (?,?,?,?,?,?);"
                        << std::string(LongID)
                        << GroupID
                        << Messagetype
                        << Checksum
                        << Received
                        << Timestamp
                        << Payload;
                } catch (...) {};
            }

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string __cdecl groupInvite(JSON::Value_t &&Request)
        {
            const auto MemberID = Request.value<std::string>("MemberID");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Key = getCryptokey(GroupID);

            const auto Payload = JSON::Dump(JSON::Object_t({
                { "Challenge", Key ? Base58::Encode(*Key) : ""s},
                { "GroupID", GroupID }
            }));

            Messaging::sendUsermessage(MemberID, "Group::Invite", Payload);
            return {};
        }
        static std::string __cdecl groupKeychange(JSON::Value_t &&Request)
        {
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Group = Groups::getGroup(GroupID);
            const auto Cryptokey = getCryptokey(GroupID);

            // Basic sanity checking.
            if (!Group) [[unlikely]] return R"({ "Error" : "Invalid / missing groupID" })";
            if (Group->isPublic) [[unlikely]] return R"({ "Error" : "Group is public." })";
            if (!Cryptokey && GroupID != Global.getLongID()) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";
            if (!AyriaAPI::Groups::getModerators(GroupID).contains(Global.getLongID())) [[unlikely]] return R"({ "Error" : "We don't have permission to do this." })";

            // Generate a random-enough key.
            const auto Newkey = Hash::SHA256(Hash::SHA256(*Global.Privatekey) + Hash::SHA1(GetTickCount64()) + (Cryptokey ? Hash::SHA1(*Cryptokey) : ""));

            // If there's no existing key, we need to message the moderators.
            if (!Cryptokey)
            {
                const auto Payload = JSON::Dump(JSON::Object_t({
                    { "Newkey", Base85::Encode(Newkey) },
                    { "GroupID", GroupID }
                }));

                sendMultiusermessage(AyriaAPI::Groups::getMembers(GroupID), "Group::reKey", Payload);
            }
            else
            {
                const auto Payload = JSON::Dump(JSON::Object_t({
                    { "Newkey", Base85::Encode(Newkey) },
                    { "Checksum", Hash::WW32(Newkey) },
                    { "GroupID", GroupID }
                }));

                Layer1::Publish("Group::reKey", Payload);
            }

            setCryptokey(GroupID, Newkey);
            return {};
        }

        static std::string __cdecl sendUsermessage(JSON::Value_t &&Request)
        {
            const auto Messagetype = Request.value<std::string>("Messagetype");
            const auto Message = Request.value<std::string>("Message");
            const auto UserID = Request.value<std::string>("UserID");

            Messaging::sendUsermessage(UserID, Messagetype, Message);
            return {};
        }
        static std::string __cdecl sendGroupmessage(JSON::Value_t &&Request)
        {
            const auto Messagetype = Request.value<std::string>("Messagetype");
            const auto GroupID = Request.value<std::string>("GroupID");
            const auto Message = Request.value<std::string>("Message");

            Messaging::sendGroupmessage(GroupID, Messagetype, Message);
            return {};
        }
    }

    // Layer 4 interaction.
    namespace Notifications
    {
        static void __cdecl onUsermessage(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Usermessages WHERE rowid = ?;" << RowID
                    >> [](const std::string &Source, const std::string &Target, uint32_t Messagetype, uint32_t Checksum, uint64_t, uint64_t, const std::string &Message)
                    {
                        if (Target == Global.getLongID())
                        {
                            if (Checksum != Hash::WW32(Base85::Decode(Message))) [[unlikely]] return;

                            const auto Notification = JSON::Object_t({
                                { "Messagetype", Messagetype },
                                { "Message", Message },
                                { "Source", Source }
                            });

                            Backend::Notifications::Publish("onUsermessage", JSON::Dump(Notification).c_str());
                        }
                    };
            } catch (...) {}
        }
        static void __cdecl onGroupmessage(int64_t RowID)
        {
            try
            {
                Backend::Database()
                    << "SELECT * FROM Groupmessages WHERE rowid = ?;" << RowID
                    >> [](const std::string &Source, const std::string &Target, uint32_t Messagetype, uint32_t Checksum, uint64_t, uint64_t, const std::string &Message)
                    {
                        if (Checksum != Hash::WW32(Base85::Decode(Message))) [[unlikely]] return;

                        if (AyriaAPI::Groups::getMembers(Target).contains(Global.getLongID()))
                        {
                            const auto Notification = JSON::Object_t({
                                { "Messagetype", Messagetype },
                                { "Message", Message },
                                { "Source", Source }
                            });

                            Backend::Notifications::Publish("onGroupmessage", JSON::Dump(Notification).c_str());
                        }
                    };
            } catch (...) {}
        }
    }

    // Listen for special notifications.
    namespace Subscriptions
    {
        static void __cdecl onKeychange(const char *JSONString)
        {
            const auto Notification = JSON::Parse(JSONString);
            if (Hash::WW32("Group::reKey") != Notification.value<uint32_t>("Messagetype")) [[likely]] return;

            const auto Payload = JSON::Parse(Base85::Decode(Notification.value<std::string>("Message")));
            const auto Sender = Notification.value<std::string>("Source");
            const auto GroupID = Payload.value<std::string>("GroupID");
            const auto Newkey = Payload.value<std::string>("Newkey");

            if (!AyriaAPI::Groups::getModerators(GroupID).contains(Sender)) [[unlikely]] return;
            setCryptokey(GroupID, Newkey);
        }
        static void __cdecl onRequest(const char *JSONString)
        {
            const auto Notification = JSON::Parse(JSONString);
            if (Hash::WW32("Group::Joinrequest") != Notification.value<uint32_t>("Messagetype")) [[likely]] return;

            const auto Payload = JSON::Parse(Base85::Decode(Notification.value<std::string>("Message")));
            const auto Challenge = Payload.value<std::string>("Challenge");
            const auto Sender = Notification.value<std::string>("Source");
            const auto GroupID = Payload.value<std::string>("GroupID");

            if (const auto Key = getCryptokey(GroupID); Challenge == (Key ? Base58::Encode(*Key) : ""s))
                Groups::addMember(GroupID, Sender);
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Groupkey ("
                "GroupID TEXT PRIMARY KEY REFERENCES Group(GroupID) ON DELETE CASCADE, "
                "Encryptionkey TEXT NOT NULL);";

            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Usermessages ("
                "Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Target TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Messagetype INTEGER, "
                "Checksum INTEGER, "
                "Received INTEGER, "
                "Sent INTEGER, "
                "Message TEXT, "
                "UNIQUE (Source, Target, Sent, Messagetype) );";

            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Groupmessages ("
                "Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
                "Target TEXT REFERENCES Group(GroupID) ON DELETE CASCADE, "
                "Messagetype INTEGER, "
                "Checksum INTEGER, "
                "Received INTEGER, "
                "Sent INTEGER, "
                "Message TEXT, "
                "UNIQUE (Source, Target, Sent, Messagetype) );";

        } catch (...) {}

        // Parse Layer 2 messages.
        Backend::Messageprocessing::addMessagehandler("Group::reKey", Messagehandlers::onKeychange);
        Backend::Messageprocessing::addMessagehandler("Usermessage", Messagehandlers::onUsermessage);
        Backend::Messageprocessing::addMessagehandler("Groupmessage", Messagehandlers::onGroupmessage);

        // Accept Layer 3 calls.
        Backend::JSONAPI::addEndpoint("Group::Invite", JSONAPI::groupInvite);
        Backend::JSONAPI::addEndpoint("Group::reKey", JSONAPI::groupKeychange);
        Backend::JSONAPI::addEndpoint("sendUsermessage", JSONAPI::sendUsermessage);
        Backend::JSONAPI::addEndpoint("sendGroupmessage", JSONAPI::sendGroupmessage);

        // Process Layer 4 notifications.
        Backend::Notifications::addProcessor("Usermessages", Notifications::onUsermessage);
        Backend::Notifications::addProcessor("Groupmessages", Notifications::onGroupmessage);

        // Listen for special notifications.
        Backend::Notifications::Subscribe("onUsermessage", Subscriptions::onRequest);
        Backend::Notifications::Subscribe("onUsermessage", Subscriptions::onKeychange);
    }
}
