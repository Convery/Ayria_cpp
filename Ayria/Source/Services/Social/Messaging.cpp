/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-01
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Messages
{
    using Message_t = struct { uint32_t Timestamp; uint32_t Source, Target; uint64_t GroupID; LZString_t B64Message; };
    static std::vector<Message_t> Messagehistory;
    static size_t Initialcount;

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl getMessages(JSON::Value_t &&Request)
        {
            const auto Timestamp = Request.value<uint32_t>("Timestamp");
            const auto SenderID = Request.value<uint32_t>("SenderID");
            const auto GroupID = Request.value<uint64_t>("GroupID");
            const auto Limit = Request.value<uint32_t>("Limit");

            JSON::Array_t Messages;
            for (const auto &Message : Messagehistory)
            {
                if (Message.Timestamp >= Timestamp)
                {
                    if ((SenderID && Message.Source == SenderID) || (GroupID && Message.GroupID == GroupID))
                    {
                        Messages.emplace_back(JSON::Object_t({
                            { "B64Message", std::string(Message.B64Message) },
                            { "Timestamp", Message.Timestamp },
                            { "SourceID", Message.Source },
                            { "TargetID", Message.Target },
                            { "GroupID", Message.GroupID }
                        }));

                        if (Limit && Messages.size() == Limit) [[unlikely]]
                            break;
                    }
                }
            }

            return JSON::Dump(Messages);
        }
        static std::string __cdecl Sendtogroup(JSON::Value_t &&Request)
        {
            const auto B64Message = Request.value<std::string>("B64Message");
            const auto GroupID = Request.value<uint64_t>("GroupID");

            // Let's not waste bandwidth on this.
            if (!Groups::isMember(GroupID, Global.UserID))
                return R"({ "Error" : "User is not a member of the group." })";

            const auto Payload = JSON::Object_t({
                { "Timestamp", uint32_t(time(NULL)) },
                { "B64Message", B64Message },
                { "GroupID", GroupID }
            });
            Messagehistory.emplace_back(uint32_t(time(NULL)), Global.UserID, 0, GroupID, LZString_t(std::move(B64Message)));
            Backend::Network::Transmitmessage("Groupmessage", Payload);
            return "{}";
        }
        static std::string __cdecl Sendtouser(JSON::Value_t &&Request)
        {
            auto B64Message = Request.value<std::string>("B64Message");
            const auto UserID = Request.value<uint32_t>("UserID");
            const auto Private = Request.value<bool>("Private");

            // Let's not waste bandwidth on this.
            if (!Clientinfo::isClientonline(UserID))
                return R"({ "Error" : "User is not online." })";

            // Save the original message.
            Messagehistory.emplace_back(uint32_t(time(NULL)), Global.UserID, UserID, 0, LZString_t(B64Message));

            // Encrypt the message if needed.
            if (Private)
            {
                const auto Cryptokey = Clientinfo::getCryptokey(UserID);
                if (Cryptokey.empty())
                {
                    Clientinfo::Requestcryptokeys({ UserID });
                    return R"({ "Error" : "User has not shared keys. Try again in a minute." })";
                }

                B64Message = Base64::Encode(PK_RSA::Encrypt(B64Message, Base64::Decode(Cryptokey)));
            }

            const auto Payload = JSON::Object_t({
                { "Timestamp", uint32_t(time(NULL)) },
                { "B64Message", B64Message },
                { "TargetID", UserID },
                { "Private", Private }
            });
            Backend::Network::Transmitmessage("Usermessage", Payload);
            return "{}";
        }
    }

    // Handle incoming packets with message data.
    static void __cdecl onGroupmessage(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Verify that we are interested in this message.
        if (!Groups::isMember(GroupID, Global.UserID))
            return;

        // Verify that there's no fuckery going on.
        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender || !Groups::isMember(GroupID, Sender->UserID))
            return;

        const auto Timestamp = Request.value<uint32_t>("Timestamp");
        const auto B64Message = Request.value<std::string>("B64Message");
        Messagehistory.emplace_back(Timestamp, Sender->UserID, Global.UserID, GroupID, LZString_t(std::move(B64Message)));
    }
    static void __cdecl onUsermessage(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto TargetID = Request.value<uint32_t>("TargetID");

        // Verify that we are interested in this message.
        if (Global.UserID != TargetID) return;

        auto B64Message = Request.value<std::string>("B64Message");
        const auto Timestamp = Request.value<uint32_t>("Timestamp");
        const auto Private = Request.value<bool>("Private");

        if (Private)
        {
            const auto Decoded = Base64::Decode_inplace(B64Message.data(), B64Message.size());
            B64Message = PK_RSA::Decrypt(Decoded, Global.Cryptokeys);
        }

        if (const auto Sender = Clientinfo::getLANClient(NodeID))
        {
            Messagehistory.emplace_back(Timestamp, Sender->UserID, TargetID, 0, LZString_t(std::move(B64Message)));
        }
    }

    // Add the message-handlers and load message history from disk.
    void Initialize()
    {
        const auto HardwareID = Clientinfo::getHardwareID();
        const auto Cryptokey = Hash::Tiger192(HardwareID.data(), HardwareID.size());

        // Load messages from disk.
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Chathistory.json"); !Filebuffer.empty())
        {
            auto Decrypted = AES::Decrypt(Filebuffer.data(), Filebuffer.size(), Cryptokey.data(), Cryptokey.data());
            const JSON::Array_t Messagelog = JSON::Parse(Decrypted);

            // Don't keep this buffer in memory.
            volatile size_t Dontoptimize = Decrypted.size();
            std::memset(Decrypted.data(), 0xCC, Dontoptimize);

            Messagehistory.reserve(Messagelog.size());
            for (const auto &Item : Messagelog)
            {
                // Basic sanity checking.
                if (Item.contains("B64Message") && Item.contains("Timestamp") && Item.contains("SourceID"))
                {
                    const auto B64Message = Item.value<std::string>("B64Message");
                    const auto Timestamp = Item.value<uint32_t>("Timestamp");
                    const auto TargetID = Item.value<uint32_t>("TargetID");
                    const auto SourceID = Item.value<uint32_t>("SourceID");
                    const auto GroupID = Item.value<uint64_t>("GroupID");

                    Messagehistory.emplace_back(Timestamp, SourceID, TargetID, GroupID, LZString_t(B64Message));
                }
            }
        }

        // Track if we have new messages for later.
        Initialcount = Messagehistory.size();

        // Save on exit.
        std::atexit([]()
        {
            // Nothing to do.
            if (Messagehistory.empty())
            {
                std::remove("./Ayria/Chathistory.json");
                return;
            }
            if (Initialcount == Messagehistory.size())
                return;

            const auto HardwareID = Clientinfo::getHardwareID();
            const auto Cryptokey = Hash::Tiger192(HardwareID.data(), HardwareID.size());

            JSON::Array_t Array;
            Array.reserve(Messagehistory.size());
            for (const auto &Message : Messagehistory)
            {
                Array.emplace_back(JSON::Object_t({
                    { "B64Message", std::string(Message.B64Message) },
                    { "Timestamp", Message.Timestamp },
                    { "SourceID", Message.Source },
                    { "TargetID", Message.Target },
                    { "GroupID", Message.GroupID }
                }));
            }

            const auto String = JSON::Dump(Array);
            const auto Encrypted = AES::Encrypt(String.data(), String.size(), Cryptokey.data(), Cryptokey.data());
            FS::Writefile(L"./Ayria/Chathistory.json", Encrypted);
        });

        // Register the networke handlers.
        Backend::Network::Registerhandler("Usermessage", onUsermessage);
        Backend::Network::Registerhandler("Groupmessage", onGroupmessage);

        // JSON endpoints.
        Backend::API::addEndpoint("Social::Messaging::Sendtouser", API::Sendtouser);
        Backend::API::addEndpoint("Social::Messaging::Sendtogroup", API::Sendtogroup);
        Backend::API::addEndpoint("Social::Messaging::getMessages", API::getMessages);
    }
}
