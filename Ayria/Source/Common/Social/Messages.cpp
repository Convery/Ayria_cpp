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

    // Helpers.
    JSON::Object_t toObject(const Message_t &Message)
    {
        return JSON::Object_t({
            { "Timestamp", Message.Timestamp },
            { "Source", Message.Source.Raw },
            { "Target", Message.Target.Raw },
            { "Message", Message.Message}
            });
    }
    Message_t toMessage(const JSON::Value_t &Object)
    {
        Message_t Newmessage;
        Newmessage.Message = Object.value("Message", std::u8string());
        Newmessage.Timestamp = Object.value("Timestamp", uint32_t());
        Newmessage.Target.Raw = Object.value("Target", uint64_t());
        Newmessage.Source.Raw = Object.value("Source", uint64_t());
        return Newmessage;
    }

    // std::optional was being a pain on MSVC, so pointers it is.
    std::vector<const Message_t *> Read(const std::pair<uint32_t, uint32_t> *Timeframe,
        const std::vector<uint64_t> *Source,
        const uint64_t *GroupID)
    {
        // Apparently this mess is easier for the compiler to reason about..
        #pragma region Avert_thine_eyes

        const auto byTime = [Start = Timeframe->first, Stop = Timeframe->second](const auto &Item) -> bool
        {
            return Item.Timestamp < Start || Item.Timestamp > Stop;
        };
        const auto byGroup = [ID = *GroupID](const auto &Item) -> bool
        {
            return Item.Target.Raw == ID;
        };
        const auto bySender = [Source](const auto &Item) -> bool
        {
            return std::ranges::any_of(*Source, [ID = Item.Target](const auto &lItem)
            {
                return lItem == ID.Raw;
            });
        };
        const auto toVector = []<typename T>(T View)
        {
            std::vector<const std::ranges::range_value_t<T> *> Vec;

            if constexpr (std::ranges::sized_range<T>)
            {
                Vec.reserve(std::ranges::size(View));
            }

            for (auto it = View.begin(); it != View.end(); ++it)
                Vec.push_back(&*it);

            return Vec;
        };

        if (GroupID) [[unlikely]]
        {
            if (!Timeframe) return toVector(Groupmessages | std::views::filter(byGroup));
            return toVector(Groupmessages | std::views::filter(byGroup) | std::views::filter(byTime));
        }

        if (Source) [[likely]]
        {
            if (!Timeframe) return toVector(Usermessages | std::views::filter(bySender));
            return toVector(Usermessages | std::views::filter(bySender) | std::views::filter(byTime));
        }

        if (!Timeframe) return toVector(Usermessages | std::views::filter([](const auto &) { return true; }));
        return toVector(Usermessages | std::views::filter(byTime));
        #pragma endregion
    }

    namespace Send
    {
        void toGroup(AyriaID_t GroupID, std::u8string_view Message)
        {
            const Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID, GroupID, Message.data() };

            const auto Memberships = Groups::getSubscriptions();
            if (std::any_of(Memberships.begin(), Memberships.end(),
            [&](const AyriaID_t &ID) { return ID.Raw == GroupID.Raw; })) [[likely]]
            {
                Backend::Sendmessage(Hash::FNV1_32("Groupmessage"), JSON::Dump(toObject(lMessage)));
            }
            else
            {
                Grouppending.push_back(lMessage);
            }
        }
        void toClient(AyriaID_t ClientID, std::u8string_view Message)
        {
            const Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID, ClientID, Message.data() };

            if (Clientinfo::isOnline(ClientID.AccountID)) [[likely]]
            {
                Backend::Sendmessage(Hash::FNV1_32("Publicmessage"), JSON::Dump(toObject(lMessage)));
            }
            else
            {
                Publicpending.push_back(lMessage);
            }
        }
        void toClientencrypted(AyriaID_t ClientID, std::u8string_view Message)
        {
            Message_t lMessage{ uint32_t(time(NULL)), Userinfo::getAccount()->ID, ClientID, Message.data() };

            if (const auto Client = Clientinfo::getClient(ClientID.AccountID)) [[likely]]
            {
                if (Client->B64Sharedkey) [[likely]]
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

    // Try to send messages.
    static void __cdecl Processpending()
    {
        // No work needed. Yey.
        if (Grouppending.empty() && Publicpending.empty() && Privatepending.empty()) [[likely]]
            return;

        if (!Grouppending.empty())
        {
            const auto Memberships = Groups::getSubscriptions();
            std::erase_if(Grouppending, [&Memberships](const auto &Message)
            {
                if (std::any_of(Memberships.begin(), Memberships.end(),
                [&](const AyriaID_t &ID) { return ID.Raw == Message.Target.Raw; })) [[unlikely]]
                {
                    Backend::Sendmessage(Hash::FNV1_32("Groupmessage"), JSON::Dump(toObject(Message)));
                    return true;
                }
                return false;
            });
        }

        if (!Publicpending.empty())
        {
            std::erase_if(Publicpending, [](const auto &Message)
            {
                if (Clientinfo::isOnline(Message.Target.AccountID)) [[unlikely]]
                {
                    Backend::Sendmessage(Hash::FNV1_32("Publicmessage"), JSON::Dump(toObject(Message)));
                    return true;
                }
                return false;
            });
        }

        if (!Privatepending.empty())
        {
            std::erase_if(Publicpending, [](auto &Message)
            {
                if (const auto Client = Clientinfo::getClient(Message.Target.AccountID)) [[unlikely]]
                {
                    if (Client->B64Sharedkey) [[likely]]
                    {
                        Message.Message = Encoding::toUTF8(PK_RSA::Encrypt(Message.Message, Base64::Decode(Client->B64Sharedkey)));
                        Backend::Sendmessage(Hash::FNV1_32("Privatemessage"), JSON::Dump(toObject(Message)));
                        return true;
                    }
                }
                return false;
            });
        }
    }

    // API-handlers.
    static void __cdecl Grouphandler(uint32_t, const char *JSONString)
    {
        const auto Message = toMessage(JSON::Parse(JSONString));
        const auto Memberships = Groups::getSubscriptions();

        if (std::any_of(Memberships.begin(), Memberships.end(),
            [&](const AyriaID_t &ID) { return ID.Raw== Message.Target.Raw; }))
        {
            Groupmessages.push_back(Message);
        }
    }
    static void __cdecl Publichandler(uint32_t, const char *JSONString)
    {
        const auto Message = toMessage(JSON::Parse(JSONString));

        if (Message.Target.Raw != Userinfo::getAccount()->ID.Raw) [[likely]]
            return;

        Usermessages.push_back(Message);
    }
    static void __cdecl Privatehandler(uint32_t, const char *JSONString)
    {
        auto Message = toMessage(JSON::Parse(JSONString));

        if (Message.Target.Raw != Userinfo::getAccount()->ID.Raw) [[likely]]
            return;

        const auto PK = Base64::Decode(Userinfo::getB64Privatekey());
        Message.Message = Encoding::toUTF8(PK_RSA::Decrypt(Message.Message, PK));
        Usermessages.push_back(Message);
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

        // Periodically try to send any pending messages.
        Backend::Enqueuetask(30000, Processpending);
    }
}
