/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Messaging
{
    // Handle local client-requests.
    static void __cdecl Groupmessagehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Message = Request.value<std::string>("B64Message");
        const auto Messagetype = Request.value<uint32_t>("Messagetype");
        const auto Transient = Request.value<bool>("Transient");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Only track valid messages relevant to us.
        if (!GroupID || B64Message.empty() || !Usergroups::isMember(GroupID, Global.ClientID)) return;

        // Add the message to the database so the plugins can access it.
        try { Backend::Database() << "REPLACE INTO Groupmessages (Messagetype, Timestamp, SenderID, GroupID, B64Message, Transient)"
                                     "VALUES (?, ?, ?, ?, ?, ?);"
                                  << Messagetype << time(NULL) << Clientinfo::getClientID(NodeID) << GroupID << B64Message << Transient;
        } catch (...) {}
    }
    static void __cdecl Usermessagehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Messagetype = Request.value<uint32_t>("Messagetype");
        auto B64Message = Request.value<std::string>("B64Message");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto Transient = !Request.value<bool>("Transient");
        const auto isPrivate = Request.value<bool>("isPrivate");

        // Only track valid messages relevant to us.
        if (TargetID != Global.ClientID) return;
        if (B64Message.empty()) return;

        // Need to decrypt.
        if (isPrivate)
        {
            const auto Decoded = Base64::Decode_inplace(B64Message.data(), B64Message.size());
            B64Message = PK_RSA::Decrypt(Decoded, Global.Cryptokeys);
        }

        // Add the message to the database so the plugins can access it.
        try { Backend::Database() << "REPLACE INTO Usermessages (Messagetype, Timestamp, SenderID, TargetID, B64Message, Transient)"
                                     "VALUES (?, ?, ?, ?, ?, ?);"
                                  << Messagetype << time(NULL) << Clientinfo::getClientID(NodeID) << TargetID << B64Message << Transient;
        } catch (...) {}
    }

    // JSON API access for the plugins.
    static std::string __cdecl Sendtogroup(JSON::Value_t &&Request)
    {
        const auto B64Message = Request.value<std::string>("B64Message");
        const auto Messagetype = Request.value<uint32_t>("Messagetype");
        const auto Transient = Request.value<bool>("Transient");
        const auto GroupID = Request.value<uint64_t>("GroupID");

        // Add the message to the database so the plugins can access it.
        try { Backend::Database() << "REPLACE INTO Groupmessages (Messagetype, Timestamp, SenderID, GroupID, B64Message, Transient)"
                                     "VALUES (?, ?, ?, ?, ?, ?);"
                                     << Messagetype << time(NULL) << Global.ClientID << GroupID << B64Message << Transient;
        } catch (...) {}

        // Notify the clients.
        const auto Object = JSON::Object_t({
            { "Messagetype", Messagetype },
            { "B64Message", B64Message },
            { "Transient", Transient },
            { "GroupID", GroupID }
        });
        Backend::Network::Transmitmessage("Messaging::Sendtogroup", Object);

        return "{}";
    }
    static std::string __cdecl Sendtouser(JSON::Value_t &&Request)
    {
        const auto Messagetype = Request.value<uint32_t>("Messagetype");
        auto B64Message = Request.value<std::string>("B64Message");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto Transient = !Request.value<bool>("Transient");
        const auto isPrivate = Request.value<bool>("isPrivate");

        // Add the message to the database so the plugins can access it.
        try { Backend::Database() << "REPLACE INTO Usermessages (Messagetype, Timestamp, SenderID, TargetID, B64Message, Transient)"
                                     "VALUES (?, ?, ?, ?, ?, ?);"
                                  << Messagetype << time(NULL) << Global.ClientID << TargetID << B64Message << Transient;
        } catch (...) {}

        // Encrypt with the targets shared key.
        if (isPrivate)
        {
            const auto Encrypted = PK_RSA::Encrypt(B64Message, Clientinfo::getSharedkey(TargetID));
            B64Message = Base64::Encode(Encrypted);
        }

        // Notify the clients.
        const auto Object = JSON::Object_t({
            { "Messagetype", Messagetype },
            { "B64Message", B64Message },
            { "Transient", Transient },
            { "isPrivate", isPrivate },
            { "TargetID", TargetID }
        });
        Backend::Network::Transmitmessage("Messaging::Sendtouser", Object);

        return "{}";
    }

    // Check for updates and notify the plugins.
    static void Checkmessages()
    {
        const auto Groupmessages = Backend::getDatabasechanges("Groupmessages");
        const auto Usermessages = Backend::getDatabasechanges("Usermessages");

        // TODO(tcn): Query by rowid when notifications are implemented.
        for (const auto &Item : Groupmessages) {}
        for (const auto &Item : Usermessages) {}
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Messaging::Sendtogroup", Groupmessagehandler);
        Backend::Network::Registerhandler("Messaging::Sendtouser", Usermessagehandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Messaging::Sendtogroup", Sendtogroup);
        Backend::API::addEndpoint("Messaging::Sendtouser", Sendtouser);

        // Notify the plugins when we have a new message.
        Backend::Enqueuetask(300, Checkmessages);
    }
}
