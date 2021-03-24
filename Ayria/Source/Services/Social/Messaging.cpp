/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-01
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Messages
{
    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl Sendtogroup(JSON::Value_t &&Request)
        {
            const auto B64Message = Request.value<std::string>("B64Message");
            const auto GroupID = Request.value<uint64_t>("GroupID");

            // Let's not waste bandwidth on this.
            if (!Groups::isMember(GroupID, Global.UserID))
                return R"({ "Error" : "User is not a member of the group." })";

            // Save the message in the database.
            Backend::Database() << "insert into Socialmessages (Timestamp, SourceID, TargetID, "
                                   "GroupID, B64Message) values (?, ?, ?, ?, ?);"
                                << uint32_t(time(NULL)) << Global.UserID  << 0 << GroupID << B64Message;

            // Send to the local network only, for now.
            const auto Payload = JSON::Object_t({
                { "Timestamp", uint32_t(time(NULL)) },
                { "B64Message", B64Message },
                { "GroupID", GroupID }
            });
            Backend::Network::Transmitmessage("Messaging::Groupmessage", Payload);
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

            // Save the message in the database.
            Backend::Database() << "insert into Socialmessages (Timestamp, SourceID, TargetID, "
                                   "GroupID, B64Message) values (?, ?, ?, ?, ?);"
                                << uint32_t(time(NULL)) << Global.UserID  << UserID << 0 << B64Message;

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

            // Send to the local network only, for now.
            const auto Payload = JSON::Object_t({
                { "Timestamp", uint32_t(time(NULL)) },
                { "B64Message", B64Message },
                { "TargetID", UserID },
                { "Private", Private }
            });
            Backend::Network::Transmitmessage("Messaging::Usermessage", Payload);
            return "{}";
        }
    }

    // Handle incoming packets with message data.
    static void __cdecl onGroupmessage(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Message = Request.value<std::string>("B64Message");
        const auto Timestamp = Request.value<uint32_t>("Timestamp");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Verify that we are interested in this message.
        if (!Groups::isMember(GroupID, Global.UserID))
            return;

        // Verify that there's no fuckery going on.
        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender || !Groups::isMember(GroupID, Sender->UserID))
            return;

        // Let's not save blocked clients.
        if (Social::Relations::isBlocked(Sender->UserID))
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // Save the message in the database.
        Backend::Database() << "insert into Socialmessages (Timestamp, SourceID, TargetID, "
                               "GroupID, B64Message) values (?, ?, ?, ?, ?);"
                            << Timestamp << Sender->UserID << Global.UserID << GroupID << B64Message;
    }
    static void __cdecl onUsermessage(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Timestamp = Request.value<uint32_t>("Timestamp");
        auto B64Message = Request.value<std::string>("B64Message");
        const auto TargetID = Request.value<uint32_t>("TargetID");
        const auto Private = Request.value<bool>("Private");

        // Verify that we are interested in this message.
        if (Global.UserID != TargetID) return;

        // Private messages are encrypted.
        if (Private)
        {
            const auto Decoded = Base64::Decode_inplace(B64Message.data(), B64Message.size());
            B64Message = PK_RSA::Decrypt(Decoded, Global.Cryptokeys);
        }

        // We can only handle local clients for now.
        if (const auto Sender = Clientinfo::getLANClient(NodeID))
        {
            // Let's not save blocked clients.
            if (Social::Relations::isBlocked(Sender->UserID))
            {
                Backend::Network::Blockclient(NodeID);
                return;
            }

            // Save the message in the database.
            Backend::Database() << "insert into Socialmessages (Timestamp, SourceID, TargetID, "
                                   "GroupID, B64Message) values (?, ?, ?, ?, ?);"
                                << Timestamp << Sender->UserID << Global.UserID << 0 << B64Message;
        }
        else
        {
            // TODO(tcn): Implement this.
            assert(false);
        }
    }

    // Add the message-handlers and load message history from disk.
    void Initialize()
    {
        // Persistent database entry for the message history.
        Backend::Database() << "create table if not exists Socialmessages ("
                               "Timestamp integer, "
                               "SourceID integer, "
                               "TargetID integer, "
                               "GroupID integer, "
                               "B64Message text);";

        // Register the network handlers.
        Backend::Network::Registerhandler("Messaging::Usermessage", onUsermessage);
        Backend::Network::Registerhandler("Messaging::Groupmessage", onGroupmessage);

        // JSON endpoints.
        Backend::API::addEndpoint("Messaging::Sendtouser", API::Sendtouser, R"({ "UserID" : 1234, "B64Message" : "==", "Private" : false })");
        Backend::API::addEndpoint("Messaging::Sendtogroup", API::Sendtogroup, R"({ "GroupID" : 1234, "B64Message" : "==" })");
    }
}
