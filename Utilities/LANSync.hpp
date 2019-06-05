/*
    Initial author: Convery (tcn@ayria.se)
    Started: 29-05-2019
    License: MIT

    Just a simple utility to sync across the local net.
*/

#pragma once
#if defined _WIN32
#include <Stdinclude.hpp>
#include <WinSock2.h>

namespace LANSync
{
    struct Message_t
    {
        std::string Category;
        std::string Subtype;
        std::string Sender;
        std::string Data;
    };

    namespace Internal
    {
        constexpr uint16_t Syncport = 0x1234;
        constexpr size_t Buffersize = 8196;

        class Responsequeue
        {
            std::unordered_map<std::string, std::vector<Message_t>> Storage;
            mutable std::mutex Mutex;

            public:
            std::vector<Message_t> Dequeue(std::string_view Category)
            {
                std::lock_guard<std::mutex> Lock(Mutex);

                auto Entry = Storage.find(Category.data());
                if (Entry != Storage.end())
                {
                    std::vector<Message_t> Result{};
                    Entry->second.swap(Result);
                    return Result;
                }

                return {};
            }
            void Enqueue(Message_t &&Message)
            {
                std::lock_guard<std::mutex> Lock(Mutex);
                Storage[Message.Category].push_back(Message);
            }
        };
    }

    class Server
    {
        Internal::Responsequeue Responses{};
        size_t Socket{ INVALID_SOCKET };
        uint32_t GameID{};

        public:
        explicit Server(uint32_t GameID) : GameID(GameID)
        {
            WSAData Unused;
            WSAStartup(MAKEWORD(2, 2), &Unused);

            // Would have preferred IPX, but UDP will have to do.
            Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (Socket == INVALID_SOCKET) return;

            // Enable broadcasting.
            unsigned long Command = TRUE;
            setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (char *)&Command, sizeof(Command));
            setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&Command, sizeof(Command));

            // Only listen on our port.
            SOCKADDR_IN Address{};
            Address.sin_family = AF_INET;
            Address.sin_port = htons(Internal::Syncport);
            bind(Socket, (SOCKADDR *)&Address, sizeof(Address));

            // Poll for packets in the background.
            std::thread([](Server *Parent)
            {
                thread_local auto Buffer{ std::make_unique<char[]>(Internal::Buffersize) };

                while(true)
                {
                    // Blocking socket.
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
                    const auto Length = recvfrom(Parent->Socket, (char *)Buffer.get(), Internal::Buffersize, 0, (SOCKADDR *)&Client, &Clientsize);
                    if (Length == 0 || Length == SOCKET_ERROR) break;

                    // Is this packet for us to play with?
                    if (*(uint32_t *)Buffer.get() != Parent->GameID) continue;

                    // Try to parse the message.
                    try
                    {
                        Message_t Message;
                        Message.Sender = va("%s:%u", inet_ntoa(Client.sin_addr), ntohl(Client.sin_port));

                        auto Object = nlohmann::json::parse(Buffer.get() + 4);
                        Message.Category = Object.value("Category", "");
                        Message.Subtype = Object.value("Subtype", "");
                        Message.Data = Object.value("Data", "");

                        Debugprint(va("LANSync: \"%s\"", Message.Category.c_str()));
                        Parent->Responses.Enqueue(std::move(Message));
                    }
                    catch (std::exception &e) { (void)e; }
                }

                Debugprint("LANSync thread exiting.");
            }, this).detach();
        }
        std::vector<Message_t> getResponses(std::string_view Category)
        {
            return Responses.Dequeue(Category);
        }
        bool Broadcast(std::string &&Category, std::string &&Subtype, std::string &&Data) const
        {
            // Create the message in JSON so developers can read it.
            auto Message = nlohmann::json::object();
            Message["Category"] = std::move(Category);
            Message["Subtype"] = std::move(Subtype);

            // We only allow base64 data to be sent, obviously.
            if(Base64::Validate(Data)) Message["Data"] = std::move(Data);
            else Message["Data"] = Base64::Encode(Data);

            // Windows does not like to send partial messages, so prefix the ID.
            auto Payload = Message.dump().insert(0, (const char *)&GameID, sizeof(uint32_t));

            // Broadcast address.
            SOCKADDR_IN Address{};
            Address.sin_family = AF_INET;
            Address.sin_addr.S_un.S_addr = INADDR_BROADCAST;
            Address.sin_port = htons(Internal::Syncport);

            // Broadcast the payload
            return Payload.size() == size_t(sendto(Socket, Payload.data(), Payload.size(), NULL, (SOCKADDR *)&Address, sizeof(Address)));
        }
    };
}

#endif
