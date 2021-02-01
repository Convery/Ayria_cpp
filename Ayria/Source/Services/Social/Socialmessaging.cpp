/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-01
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Messages
{
    //using Message_t = struct { uint32_t Timestamp; uint32_t Source, Target; std::string B64Message; };

    static std::vector<Message_t> Messagehistory;
    static std::vector<Message_t> Pendingprivate;
    static std::vector<Message_t> Pendingpublic;
    static std::vector<Message_t> Pendinggroup;

    // Helpers for serialization.
    static inline Message_t Deserialize(const JSON::Value_t &Object)
    {
        return Message_t
        {
            Object.value<uint32_t>("Timestamp"),
            Object.value<uint32_t>("Source"),
            Object.value<uint32_t>("Target"),
            Object.value<uint64_t>("GroupID"),
            Object.value<std::string>("Message")
        };
    }
    static inline JSON::Object_t Serialize(const Message_t &Message)
    {
        return JSON::Object_t({
            { "Timestamp", Message.Timestamp },
            { "Message", Message.B64Message },
            { "GroupID", Message.GroupID },
            { "Source", Message.Source },
            { "Target", Message.Target }
        });
    }

    // Handle the synchronization for LAN clients.
    static void __cdecl Messagehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Checksum = Request.value<uint32_t>("Checksum");
        const auto Private = Request.value<bool>("isPrivate");
        auto lMessage = Deserialize(Request);

        // Either the sender messed up or this is not for us.
        if (Sender->UserID != lMessage.Source) return;
        if (Global.UserID != lMessage.Target) return;

        if (Private)
        {
            const auto Tmp = Base64::Decode(lMessage.B64Message);
            lMessage.B64Message  = PK_RSA::Decrypt(Tmp, Global.Cryptokeys);

            if (Hash::WW32(lMessage.B64Message) != Checksum)
            {
                Errorprint(va("Client ID %08X sent an invalid private message.", Sender->UserID));
                return;
            }
        }

        lMessage.GroupID = 0;
        Messagehistory.emplace_back(std::move(lMessage));
    }
    static void __cdecl Grouphandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        auto lMessage = Deserialize(Request);

        // The sender messed up or this is not for us.
        if (Sender->UserID != lMessage.Source) return;
        if (!Group::isGroupmember({ lMessage.GroupID }, Global.UserID)) return;
        if (!Group::isGroupmember({ lMessage.GroupID }, Sender->UserID)) return;

        Messagehistory.emplace_back(std::move(lMessage));
    }

    namespace Send
    {
        void toGroup(uint64_t GroupID, std::string_view B64Message)
        {
            // Sanity checking.
            if (!Group::isGroupmember({ GroupID }, Global.UserID))
            {
                Errorprint(va("Tried to send a message to a group we are not a member of."));
                return;
            }

            const Message_t Message{ uint32_t(time(NULL)), Global.UserID, 0, GroupID, std::string(B64Message) };
            Pendinggroup.emplace_back(std::move(Message));
        }
        void toClient(uint32_t ClientID, std::string_view B64Message)
        {
            const Message_t Message{ uint32_t(time(NULL)), Global.UserID, ClientID, 0, std::string(B64Message) };
            Pendingpublic.emplace_back(std::move(Message));
        }
        void toClientencrypted(uint32_t ClientID, std::string_view B64Message)
        {
            const Message_t Message{ uint32_t(time(NULL)), Global.UserID, ClientID, 0, std::string(B64Message) };
            Pendingprivate.emplace_back(std::move(Message));
        }
    }

    // Read messages optionally filtered by group, sender, and/or timeframe.
    std::vector<const Message_t *> Read(const std::pair<uint32_t, uint32_t> *Timeframe,
        const uint64_t *GroupID,
        const uint32_t *Source)
    {
        // Apparently this mess is easier for the compiler to reason about..
        #pragma region Avert_thine_eyes
        const auto byTime = [Start = Timeframe->first, Stop = Timeframe->second](const auto &Item) -> bool
        {
            return Item.Timestamp >= Start && Item.Timestamp <= Stop;
        };
        const auto byGroup = [ID = *GroupID](const auto &Item) -> bool
        {
            return Item.GroupID == ID;
        };
        const auto bySender = [ID = *Source](const auto &Item) -> bool
        {
            return Item.Source == ID;
        };
        const auto all = [](const auto &Item) -> bool
        {
            return true;
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
        #pragma endregion

        if (!Timeframe && Source)  return toVector(Messagehistory | std::views::filter(byTime) | std::views::filter(bySender));
        if (Timeframe  && GroupID) return toVector(Messagehistory | std::views::filter(byTime) | std::views::filter(byGroup));
        if (Timeframe  && Source)  return toVector(Messagehistory | std::views::filter(bySender));
        if (!Timeframe && GroupID) return toVector(Messagehistory | std::views::filter(byGroup));
        return toVector(Messagehistory);
    }

    // Try to send all pending messages.
    static void Sendmessages()
    {
        for (auto &Item : Pendinggroup)
        {
            Backend::Network::Sendmessage("Groupmessage", JSON::Dump(Serialize(Item)));
            Item.B64Message.clear();
        }
        for (auto &Item : Pendingpublic)
        {
            Backend::Network::Sendmessage("Clientmessage", JSON::Dump(Serialize(Item)));
            Item.B64Message.clear();
        }
        for (auto &Item : Pendingprivate)
        {
            for (const auto &Client : Clientinfo::Listclients())
            {
                if (Item.Target == Client->UserID)
                {
                    if (!Client->Sharedkey) Clientinfo::Requestcryptokeys({ Client->UserID });
                    else
                    {
                        const auto Checksum = Hash::WW32(Item.B64Message);
                        const auto Key = Base64::Decode(*Client->Sharedkey);
                        const auto Enc = PK_RSA::Encrypt(Item.B64Message, Key);

                        Item.B64Message = Base64::Encode(Enc);

                        auto Request = Serialize(Item);
                        Request["Checksum"] = Checksum;
                        Request["isPrivate"] = true;

                        Backend::Network::Sendmessage("Clientmessage", JSON::Dump(Request));
                        Item.B64Message.clear();
                    }
                }
            }
        }

        // Clear any sent messages.
        std::erase_if(Pendinggroup, [](const auto &Item) { return Item.B64Message.empty(); });
        std::erase_if(Pendingpublic, [](const auto &Item) { return Item.B64Message.empty(); });
        std::erase_if(Pendingprivate, [](const auto &Item) { return Item.B64Message.empty(); });
    }

    // Add the message-handlers and load pending messages from disk.
    void Initialize()
    {
        const auto HardwareID = Clientinfo::getHardwareID();
        const auto Cryptokey = Hash::Tiger192(HardwareID.data(), HardwareID.size());

        // Load messages from disk.
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Chatlog.json"); !Filebuffer.empty())
        {
            auto Decrypted = AES::Decrypt(Filebuffer.data(), Filebuffer.size(), Cryptokey.data(), Cryptokey.data());
            const JSON::Array_t Messagelog = JSON::Parse(Decrypted);

            // Don't keep this buffer in memory.
            volatile size_t Dontoptimize = Decrypted.size();
            std::memset(Decrypted.data(), 0xCC, Dontoptimize);

            Messagehistory.reserve(Messagelog.size());
            for (const auto &Item : Messagelog) Messagehistory.emplace_back(Deserialize(Item));
        }
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Pendingchat.json"); !Filebuffer.empty())
        {
            auto Decrypted = AES::Decrypt(Filebuffer.data(), Filebuffer.size(), Cryptokey.data(), Cryptokey.data());
            const auto Buffer = JSON::Parse(Decrypted);

            // Don't keep this buffer in memory.
            volatile size_t Dontoptimize = Decrypted.size();
            std::memset(Decrypted.data(), 0xCC, Dontoptimize);

            const auto Public = Buffer.value<JSON::Array_t>("Pendingpublic");
            const auto Private = Buffer.value<JSON::Array_t>("Pendingprivate");

            Pendingpublic.reserve(Public.size());
            Pendingprivate.reserve(Private.size());

            for (const auto &Item : Public) Pendingpublic.emplace_back(Deserialize(Item));
            for (const auto &Item : Private) Pendingprivate.emplace_back(Deserialize(Item));
        }

        // Save on exit.
        std::atexit([]()
        {
            const auto HardwareID = Clientinfo::getHardwareID();
            const auto Cryptokey = Hash::Tiger192(HardwareID.data(), HardwareID.size());

            if (!Messagehistory.empty())
                {
                    JSON::Array_t Array;
                    Array.reserve(Messagehistory.size());
                    for (const auto &Item : Messagehistory)
                        Array.emplace_back(Serialize(Item));

                    const auto String = JSON::Dump(Array);
                    const auto Encrypted = AES::Encrypt(String.data(), String.size(), Cryptokey.data(), Cryptokey.data());
                    FS::Writefile(L"./Ayria/Chatlog.json", Encrypted);
                }

            JSON::Object_t Pending;
            if (!Pendingpublic.empty())
                {
                    JSON::Array_t Array;
                    Array.reserve(Pendingpublic.size());
                    for (const auto &Item : Pendingpublic)
                        Array.emplace_back(Serialize(Item));
                    Pending["Pendingpublic"] = Array;
                }
            if (!Pendingprivate.empty())
                {
                    JSON::Array_t Array;
                    Array.reserve(Pendingprivate.size());
                    for (const auto &Item : Pendingprivate)
                        Array.emplace_back(Serialize(Item));
                    Pending["Pendingprivate"] = Array;
                }

            if (!Pending.empty())
            {
                const auto String = JSON::Dump(Pending);
                const auto Encrypted = AES::Encrypt(String.data(), String.size(), Cryptokey.data(), Cryptokey.data());
                FS::Writefile(L"./Ayria/Pendingchat.json", Encrypted);
            }
        });

        // Try to send the pending messages occasionally.
        Backend::Enqueuetask(1000, Sendmessages);

        // Handle the synchronization for LAN clients.
        Backend::Network::addHandler("Groupmessage", Grouphandler);
        Backend::Network::addHandler("Clientmessage", Messagehandler);
    }
}
