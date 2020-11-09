/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-09
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social::Messages
{
    static std::vector<Message_t> Grouppending, Publicpending, Privatepending;
    static std::vector<Message_t> Usermessages, Groupmessages;
    static std::vector<const Message_t *> Allmessages;
    bool hasDirtymessages{ true };

    static JSON::Object_t toObject(const Message_t &Message)
    {
        return JSON::Object_t({
            { "Timestamp", Message.Timestamp },
            { "Source", Message.Source.Raw },
            { "Target", Message.Target.Raw },
            { "Message", Message.Message}
            });
    }
    static Message_t toMessage(const JSON::Value_t &Object)
    {
        Message_t Newmessage;
        Newmessage.Message = Object.value("Message", std::u8string());
        Newmessage.Timestamp = Object.value("Timestamp", uint32_t());
        Newmessage.Target.Raw = Object.value("Target", uint64_t());
        Newmessage.Source.Raw = Object.value("Source", uint64_t());
        return Newmessage;
    }

    namespace Read
    {
        std::vector<const Message_t *> All()
        {
            if (!hasDirtymessages) [[likely]]
                return Allmessages;

            Allmessages.clear();
            Allmessages.reserve(Usermessages.size() + Groupmessages.size());

            for (auto it = Usermessages.cbegin(); it != Usermessages.cend(); ++it)
                Allmessages.push_back(&*it);

            for (auto it = Groupmessages.cbegin(); it != Groupmessages.cend(); ++it)
                Allmessages.push_back(&*it);

            hasDirtymessages = false;
            return Allmessages;
        }
        std::vector<const Message_t *> New(uint32_t Lasttime)
        {
            auto Messages = All();
            std::erase_if(Messages, [Lasttime](const Message_t *Message) { return Message->Timestamp <= Lasttime; });
            return Messages;
        }
        std::vector<const Message_t *> Filtered(std::optional<std::pair<uint32_t, uint32_t>> Timeframe,
            std::optional<std::vector<uint32_t>> Source,
            std::optional<AyriaID_t> GroupID)
        {
            auto Messages = All();

            if (Timeframe.has_value())
            {
                const auto &[Start, Stop] = Timeframe.value();
                std::erase_if(Messages, [Start](const Message_t *Message) { return Message->Timestamp <= Start; });
                std::erase_if(Messages, [Stop](const Message_t *Message) { return Message->Timestamp >= Stop; });
            }

            if (GroupID.has_value())
            {
                const auto ID = GroupID.value();
                std::erase_if(Messages, [ID](const Message_t *Message) { return Message->Target.Raw != ID.Raw; });
            }

            if (Source.has_value())
            {
                for (const auto &ID : Source.value())
                {
                    std::erase_if(Messages, [ID](const Message_t *Message) { return Message->Source.AccountID != ID; });
                }
            }

            return Messages;
        }
    }
    namespace Send
    {
        void toGroup(uint32_t GroupID, std::u8string_view Message)
        {
            const Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID.Raw, GroupID, Message.data() };

            const auto Memberships = Groups::getSubscriptions();
            if (std::any_of(Memberships.begin(), Memberships.end(),
                [&](const AyriaID_t &ID) { return ID.AccountID == GroupID; }))
            {
                Backend::Sendmessage(Hash::FNV1_32("Groupmessage"), JSON::Dump(toObject(lMessage)));
            }
            else
            {
                Grouppending.push_back(lMessage);
            }
        }
        void toClient(uint32_t ClientID, std::u8string_view Message)
        {
            const Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID.Raw, ClientID, Message.data() };

            if (Clientinfo::isOnline(ClientID))
            {
                Backend::Sendmessage(Hash::FNV1_32("Publicmessage"), JSON::Dump(toObject(lMessage)));
            }
            else
            {
                Publicpending.push_back(lMessage);
            }
        }
        void toClientencrypted(uint32_t ClientID, std::u8string_view Message)
        {
            Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID.Raw, ClientID, Message.data() };

            if (const auto Client = Clientinfo::getClient(ClientID))
            {
                if (Client->B64Sharedkey)
                {
                    lMessage.Message = Encoding::toUTF8(PK_RSA::Encrypt(Message, Base64::Decode(Client->B64Sharedkey)));
                    Backend::Sendmessage(Hash::FNV1_32("Privatemessage"), JSON::Dump(toObject(lMessage)));
                }
            }
            else
            {
                Privatepending.push_back(lMessage);
            }
        }
    }

    // API-handlers.
    static void __cdecl Grouphandler(uint32_t NodeID, const char *JSONString)
    {
        const auto Message = toMessage(JSON::Parse(JSONString));
        const auto Memberships = Groups::getSubscriptions();

        if (std::any_of(Memberships.begin(), Memberships.end(),
            [&](const AyriaID_t &ID) { return ID.Raw== Message.Target.Raw; }))
        {
            Groupmessages.push_back(Message);
            hasDirtymessages = true;
        }
    }
    static void __cdecl Publichandler(uint32_t NodeID, const char *JSONString)
    {
        const auto Message = toMessage(JSON::Parse(JSONString));

        if (Message.Target.Raw != Userinfo::getAccount()->ID.Raw) [[likely]]
            return;

        Usermessages.push_back(Message);
        hasDirtymessages = true;
    }
    static void __cdecl Privatehandler(uint32_t NodeID, const char *JSONString)
    {
        auto Message = toMessage(JSON::Parse(JSONString));

        if (Message.Target.Raw != Userinfo::getAccount()->ID.Raw) [[likely]]
            return;

        const auto PK = Base64::Decode(Userinfo::getB64Privatekey());
        Message.Message = Encoding::toUTF8(PK_RSA::Decrypt(Message.Message, PK));
        Usermessages.push_back(Message);
        hasDirtymessages = true;
    }

    // Add the message-handlers and load messages from disk.
    void Initialize()
    {
        // Messages on disk are encrypted and shared between users.
        const auto HWID = Userinfo::getB64HardwareID();
        const auto Key = Encoding::toUTF8(Hash::Tiger192(HWID.data(), HWID.size()));
        AES::AES_t Crypto(Key, Key);

        // Load messages from disk.
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Messagehistory.json"); !Filebuffer.empty())
        {
            const auto Decrypted = Crypto.Decrypt_128(Filebuffer.data(), Filebuffer.length());

            const auto Object = JSON::Parse(Encoding::toNarrow(Decrypted));
            const auto Userarray = Object.value("Usermessages", JSON::Array_t());
            const auto Grouparray = Object.value("Groupmessages", JSON::Array_t());

            Groupmessages.reserve(Grouparray.size());
            Usermessages.reserve(Userarray.size());

            for (const auto &Item : Grouparray) Groupmessages.push_back(toMessage(Item));
            for (const auto &Item : Userarray) Usermessages.push_back(toMessage(Item));
        }
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Messagequeue.json"); !Filebuffer.empty())
        {
            const auto Decrypted = Crypto.Decrypt_128(Filebuffer.data(), Filebuffer.length());

            const auto Object = JSON::Parse(Encoding::toNarrow(Decrypted));
            const auto Grouparray = Object.value("Groupmessages", JSON::Array_t());
            const auto Publicarray = Object.value("Publicmessages", JSON::Array_t());
            const auto Privatearray = Object.value("Privatemessages", JSON::Array_t());

            Grouppending.reserve(Grouparray.size());
            Publicpending.reserve(Publicarray.size());
            Privatepending.reserve(Privatearray.size());

            for (const auto &Item : Grouparray) Grouppending.push_back(toMessage(Item));
            for (const auto &Item : Publicarray) Publicpending.push_back(toMessage(Item));
            for (const auto &Item : Privatearray) Privatepending.push_back(toMessage(Item));
        }

        // Save all unsent messages when the app terminates.
        std::atexit([]()
        {
            // No work needed. Yey.
            if (Grouppending.empty() && Publicpending.empty() && Privatepending.empty())
                {
                    std::remove("./Ayria/Messagequeue.json");
                    return;
                }

            JSON::Object_t Pending;
            if (!Grouppending.empty())
            {
                JSON::Array_t Array; Array.reserve(Grouppending.size());
                for (const auto &Item : Grouppending)
                    Array.push_back(toObject(Item));
                Pending["Groupmessages"] = Array;
            }
            if (!Publicpending.empty())
            {
                JSON::Array_t Array; Array.reserve(Publicpending.size());
                for (const auto &Item : Publicpending)
                    Array.push_back(toObject(Item));
                Pending["Publicmessages"] = Array;
            }
            if (!Privatepending.empty())
            {
                JSON::Array_t Array; Array.reserve(Privatepending.size());
                for (const auto &Item : Privatepending)
                    Array.push_back(toObject(Item));
                Pending["Privatemessages"] = Array;
            }

            // Messages on disk are encrypted and shared between users.
            const auto HWID = Userinfo::getB64HardwareID();
            const auto Key = Encoding::toUTF8(Hash::Tiger192(HWID.data(), HWID.size()));
            AES::AES_t Crypto(Key, Key);

            const auto Plain = JSON::Dump(Pending);
            FS::Writefile(L"./Ayria/Messagequeue.json", Crypto.Encrypt_128(Plain.data(), Plain.size()));
        });

        // Register message-handlers.
        Backend::Registermessagehandler(Hash::FNV1_32("Groupmessage"), Grouphandler);
        Backend::Registermessagehandler(Hash::FNV1_32("Publicmessage"), Publichandler);
        Backend::Registermessagehandler(Hash::FNV1_32("Privatemessage"), Privatehandler);
    }
}
