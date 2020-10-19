/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social
{
    std::unordered_map<uint32_t, std::string> Publickeys;
    std::vector<Message_t> Messages;
    RSA *Clientkeypair{};

    // Requests key from Clients, shares own.
    void Syncpublickeys(std::vector<uint32_t> Clients)
    {
        // No need to request keys we already have.
        std::erase_if(Clients, [](const auto &ID) { return Publickeys.contains(ID); });

        // Ensure that we actually have a key to share.
        if (!Clientkeypair) [[unlikely]] Clientkeypair = PK_RSA::Createkeypair(512);

        auto Object = nlohmann::json::object();
        Object["Publickey"] = Base64::Encode(PK_RSA::getPublickey(Clientkeypair));
        Object["Wantedkeys"] = Clients;

        Backend::Sendmessage(Hash::FNV1_32("Syncpublickeys"), Object.dump());
    }
    void __cdecl Keysharehandler(uint32_t NodeID, const char *JSONString)
    {
        const auto ClientID = Clientinfo::toClientID(NodeID);
        if (ClientID == 0) [[unlikely]] return; // WTF?

        const auto Request = ParseJSON(JSONString);
        const auto Publickey = Request.value("Publickey", std::string());
        const auto Wantedclients = Request.value("Wantedkeys", std::vector<uint32_t>());

        if (Publickey.empty()) [[unlikely]] return; // WTF?
        Publickeys[ClientID] = Request["Publickey"];

        const auto Localclient = Clientinfo::getLocalclient();
        for (const auto &Client : Wantedclients)
        {
            if (Client == Localclient->ClientID)
            {
                Syncpublickeys({});
                break;
            }
        }
    }

    // Returns false if clients are offline or missing public key.
    bool Sendinstantmessage(std::vector<uint32_t> Clients, uint32_t Messagetype, std::string_view Message)
    {
        const auto Online = Clientinfo::getNetworkclients();
        std::erase_if(Clients, [&](const auto &ClientID)
        {
            for (const auto &Client : *Online)
                if (Client.ClientID == ClientID)
                    return false;
            return true;
        });
        if (Clients.empty()) return false;

        auto Object = nlohmann::json::object();
        Object["Type"] = Messagetype;
        Object["Message"] = Message;
        Object["Targets"] = Clients;

        Backend::Sendmessage(Hash::FNV1_32("Instantmessage"), Object.dump());
        return true;
    }
    bool Sendinstantmessage_enc(uint32_t Client, uint32_t Messagetype, std::string_view Message)
    {
        if (!Publickeys.contains(Client)) [[unlikely]]
        {
            Syncpublickeys({Client});
            return false;
        }

        const auto Encrypted = PK_RSA::Encrypt(Message, Publickeys[Client]);
        if (Encrypted.empty()) [[unlikely]] return false;

        auto Object = nlohmann::json::object();
        Object["Message"] = Encrypted;
        Object["Type"] = Messagetype;
        Object["Target"] = Client;

        Backend::Sendmessage(Hash::FNV1_32("Instantmessage_enc"), Object.dump());
        return true;
    }

    // Parse and enqueue messages.
    void __cdecl Messagehandler(uint32_t NodeID, const char *JSONString)
    {
        const auto ClientID = Clientinfo::toClientID(NodeID);
        if (ClientID == 0) [[unlikely]] return; // WTF?

        const auto Request = ParseJSON(JSONString);
        const auto Targets = Request.value("Targets", std::vector<uint32_t>());
        const auto Message = Request.value("Message", std::string());
        const auto Type = Request.value("Type", uint32_t());

        const auto OwnID = Clientinfo::getLocalclient()->ClientID;
        for (const auto &Client : Targets)
        {
            if (Client == OwnID)
            {
                Messages.push_back({ ClientID, Type, Message });
                break;
            }
        }
    }
    void __cdecl Messagehandler_enc(uint32_t NodeID, const char *JSONString)
    {
        const auto ClientID = Clientinfo::toClientID(NodeID);
        if (ClientID == 0) [[unlikely]] return; // WTF?

        const auto Request = ParseJSON(JSONString);
        const auto Targets = Request.value("Targets", std::vector<uint32_t>());
        const auto Message = Request.value("Message", std::string());
        const auto Type = Request.value("Type", uint32_t());

        // Sanity-checking.
        if (Message.empty()) [[unlikely]] return;

        const auto OwnID = Clientinfo::getLocalclient()->ClientID;
        for (const auto &Client : Targets)
        {
            if (Client == OwnID)
            {
                const auto Decrypted = PK_RSA::Decrypt(Message, Clientkeypair);
                Messages.push_back({ ClientID, Type, Decrypted });
                break;
            }
        }
    }

    // Returns the chat messages seen this session.
    std::vector<Message_t> Readinstantmessages(uint32_t Offset, uint32_t Count, uint32_t Messagetype, uint32_t SenderID)
    {
        std::vector<Message_t> Result; Result.reserve(Messages.size());

        std::copy_if(Messages.begin(), Messages.end(), std::back_inserter(Result), [&](const auto &Message)
        {
            return (SenderID == 0 || SenderID == Message.Sender) && (Messagetype == 0 || Messagetype == Message.Type);
        });

        if (Offset) Result.erase(Result.begin(), Result.begin() + Offset);
        Result.resize(std::clamp(Result.size(), size_t(), size_t(Count)));

        return Result;
    }

    // Add the message-handlers.
    void Initialize()
    {
        Backend::Registermessagehandler(Hash::FNV1_32("Instantmessage"), Messagehandler);
        Backend::Registermessagehandler(Hash::FNV1_32("Syncpublickeys"), Keysharehandler);
        Backend::Registermessagehandler(Hash::FNV1_32("Instantmessage_enc"), Messagehandler_enc);
    }
}

