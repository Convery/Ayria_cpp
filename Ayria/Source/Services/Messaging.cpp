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
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Transient = !Request.value<bool>("Save");

        // Only track valid messages relevant to us.
        if (!GroupID || B64Message.empty() || !Usergroups::isMember(GroupID, Global.ClientID)) return;

        // Ensure that the message are available to the plugins.
        Backend::Database() << "REPLACE (SourceID, B64Message, ProviderID, Timestamp, TargetID, GroupID, Transient) "
                               "INTO Messages VALUES (?, ?, ?, ?, ?, ?, ?);"
                            << Clientinfo::getClientID(NodeID) << B64Message << ProviderID << uint32_t(time(NULL))
                            << TargetID << GroupID << Transient;

        // TODO(tcn): Create some notification system.
    }
    static void __cdecl Usermessagehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        auto B64Message = Request.value<std::string>("B64Message");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto isPrivate = Request.value<bool>("isPrivate");
        const auto Transient = !Request.value<bool>("Save");

        // Only track valid messages relevant to us.
        if (TargetID != Global.ClientID) return;
        if (B64Message.empty()) return;

        // Need to decrypt.
        if (isPrivate)
        {
            const auto Decoded = Base64::Decode_inplace(B64Message.data(), B64Message.size());
            B64Message = PK_RSA::Decrypt(Decoded, Global.Cryptokeys);
        }

        // Ensure that the message are available to the plugins.
        Backend::Database() << "REPLACE (SourceID, B64Message, ProviderID, Timestamp, TargetID, GroupID, Transient) "
                               "INTO Messages VALUES (?, ?, ?, ?, ?, ?, ?);"
                            << Clientinfo::getClientID(NodeID) << B64Message << ProviderID << uint32_t(time(NULL))
                            << TargetID << 0 << Transient;

        // TODO(tcn): Create some notification system.
    }

    // JSON API access for the plugins.
    static std::string __cdecl Sendtogroup(JSON::Value_t &&Request)
    {
        const auto B64Message = Request.value<std::string>("B64Message");
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto GroupID = Request.value<uint64_t>("GroupID");
        const auto Transient = !Request.value<bool>("Save");

        // Ensure that the message are available to the plugins.
        Backend::Database() << "REPLACE (SourceID, B64Message, ProviderID, Timestamp, TargetID, GroupID, Transient) "
                               "INTO Messages VALUES (?, ?, ?, ?, ?, ?, ?);"
                            << Global.ClientID << B64Message << ProviderID << uint32_t(time(NULL))
                            << TargetID << GroupID << Transient;

        // Notify the clients.
        const auto Object = JSON::Object_t({
            { "ProviderID", ProviderID },
            { "B64Message", B64Message },
            { "TargetID", TargetID },
            { "GroupID", GroupID },
            { "Save", !Transient }
        });
        Backend::Network::Transmitmessage("Messaging::Sendtogroup", Object);

        return "{}";
    }
    static std::string __cdecl Sendtouser(JSON::Value_t &&Request)
    {
        const auto ProviderID = Request.value<uint64_t>("ProviderID");
        auto B64Message = Request.value<std::string>("B64Message");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto isPrivate = Request.value<bool>("isPrivate");
        const auto Transient = !Request.value<bool>("Save");

        // Ensure that the message are available to the plugins.
        Backend::Database() << "REPLACE (SourceID, B64Message, ProviderID, Timestamp, TargetID, GroupID, Transient) "
                               "INTO Messages VALUES (?, ?, ?, ?, ?, ?, ?);"
                            << Global.ClientID << B64Message << ProviderID << uint32_t(time(NULL))
                            << TargetID << 0 << Transient;

        // Encrypt with the targets shared key.
        if (isPrivate)
        {
            const auto Cryptokey = Base64::Decode(Clientinfo::getSharedkey(TargetID));
            const auto Encrypted = PK_RSA::Encrypt(B64Message, Cryptokey);
            B64Message = Base64::Encode(Encrypted);
        }

        // Notify the clients.
        const auto Object = JSON::Object_t({
            { "ProviderID", ProviderID },
            { "B64Message", B64Message },
            { "isPrivate", isPrivate },
            { "TargetID", TargetID },
            { "Save", !Transient }
        });
        Backend::Network::Transmitmessage("Messaging::Sendtouser", Object);

        return "{}";
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
    }
}
