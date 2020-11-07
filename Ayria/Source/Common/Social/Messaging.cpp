/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social
{
    namespace Messaging
    {
        std::vector<Message_t> Messages;

        namespace Read
        {
            std::vector<Message_t *> All()
            {
                std::vector<Message_t *> Result; Result.reserve(Messages.size());
                for (auto it = Messages.begin(); it != Messages.end(); ++it)
                {
                    Result.push_back(&*it);
                }
                return Result;
            }
            std::vector<Message_t *> bySenders(const std::vector<uint32_t> &SenderIDs)
            {
                auto Result = All();
                Result.erase(std::remove_if(std::execution::par_unseq, Result.begin(), Result.end(), [&](const auto &MSG)
                {
                    return !std::any_of(SenderIDs.cbegin(), SenderIDs.cend(), [&](const auto &ID)
                    {
                         return ID == MSG->Source.AccountID;
                    });
                }));

                return Result;
            }
            std::vector<Message_t *> byTime(uint32_t First, uint32_t Last, uint32_t SenderID)
            {
                std::vector<Message_t *> Result;

                if (SenderID == 0) Result = All();
                else Result = bySenders({ SenderID });

                Result.erase(std::remove_if(std::execution::par_unseq, Result.begin(), Result.end(), [&](const auto &MSG)
                {
                    return MSG->Timestamp < First || MSG->Timestamp > Last;
                }));

                return Result;
            }
        }
        namespace Send
        {
            // Returns false if clients are offline or missing public key.
            bool toClient(uint32_t ClientID, std::u8string_view Message)
            {
                // We currently have no way of communicating with offline clients.
                if (!Clientinfo::isOnline(ClientID)) return false;

                // Simple identifier, more options can be added to message.
                const auto Object = JSON::Object_t({ {"Target", ClientID }, {"Message", Message} });
                Backend::Sendmessage(Hash::FNV1_32("MSG_toClient"), JSON::Dump(Object));
                return true;
            }
            bool toClientencrypted(uint32_t ClientID, std::u8string_view Message)
            {
                // Key-request is automatically issued if missing, try again later.
                const auto Client = Clientinfo::getClient(ClientID);
                if (!Client->B64Publickey) return false;

                // Simple identifier, more options can be added to message.
                const auto Object = JSON::Object_t({ {"Target", ClientID },
                    {"Message", PK_RSA::Encrypt(Message, Base64::Decode(Client->B64Publickey))} });
                Backend::Sendmessage(Hash::FNV1_32("MSG_toClient_enc"), JSON::Dump(Object));
                return true;
            }
            bool Broadcast(std::vector<uint32_t> ClientIDs, std::u8string_view Message)
            {
                // Remove offline clients from the list.
                std::erase_if(ClientIDs, [&](const auto &Item) { return !Clientinfo::isOnline(Item); });
                if (ClientIDs.empty()) return false;

                // Simple identifier, more options can be added to message.
                const auto Object = JSON::Object_t({ {"Targets", ClientIDs }, {"Message", Message} });
                Backend::Sendmessage(Hash::FNV1_32("MSG_Broadcast"), JSON::Dump(Object));
                return true;
            }
        }

        // Current size of the storage.
        size_t getMessagecount()
        {
            return Messages.size();
        }

        // Parse and enqueue messages.
        void __cdecl Messagehandler(uint32_t NodeID, const char *JSONString)
        {
            const auto Client = Clientinfo::getNetworkclient(NodeID);
            if (!Client) [[unlikely]] return;

            const auto Request = JSON::Parse(JSONString);
            const auto Target = Request.value("Target", uint32_t());
            const auto Message = Request.value("Message", std::u8string());
            auto Targets = Request.value("Targets", std::vector<uint32_t>());
            Targets.push_back(Target);

            const auto Self = Userinfo::getAccount();
            if (std::any_of(Targets.cbegin(), Targets.cend(), [&](const auto &ID) { return ID == Self->ID.AccountID; }))
            {
                if (!Message.empty())
                    Messages.push_back({ (uint32_t)time(NULL), Client->AccountID, Message });
            }
        }
        void __cdecl Messagehandler_enc(uint32_t NodeID, const char *JSONString)
        {
            const auto Client = Clientinfo::getNetworkclient(NodeID);
            if (!Client) [[unlikely]] return;

            const auto Request = JSON::Parse(JSONString);
            const auto Target = Request.value("Target", uint32_t());
            const auto Message = Request.value("Message", std::u8string());

            const auto Self = Userinfo::getAccount();
            if (Target == Self->ID.AccountID)
            {
                const auto Key = Userinfo::getB64Privatekey();
                const auto Decrypted = PK_RSA::Decrypt(Message, Base64::Decode(Key));
                if (!Decrypted.empty())
                    Messages.push_back({ (uint32_t)time(NULL), Client->AccountID,
                        std::u8string((char8_t*)Decrypted.data(), Decrypted.size()) });
            }
        }

        // Add the message-handlers.
        void Initialize()
        {
            Backend::Registermessagehandler(Hash::FNV1_32("MSG_toClient"), Messagehandler);
            Backend::Registermessagehandler(Hash::FNV1_32("MSG_Broadcast"), Messagehandler);
            Backend::Registermessagehandler(Hash::FNV1_32("MSG_toClient_enc"), Messagehandler_enc);
        }
    }
}
