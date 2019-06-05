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

namespace Internal
{
    constexpr uint16_t Syncport = 0x1234;
    constexpr size_t Buffersize = 8196;

    class Responsequeue
    {
        std::unordered_map<std::string, std::vector<std::string>> Storage;
        mutable std::mutex Mutex;

        public:
        std::vector<std::string> Dequeue(std::string_view Type)
        {
            std::lock_guard<std::mutex> Lock(Mutex);

            auto Entry = Storage.find(Type.data());
            if(Entry != Storage.end())
            {
                std::vector<std::string> Result{};
                Entry->second.swap(Result);
                return Result;
            }

            return {};
        }
        void Enqueue(std::string &&Entry)
        {
            try
            {
                auto Object = nlohmann::json::parse(Entry.c_str());
                if(Object["Event"].is_null()) return;

                std::lock_guard<std::mutex> Lock(Mutex);
                Storage[Object["Event"]].push_back(Entry);
            }
            catch(std::exception &e){ (void)e; }
        }
    };
}

class LANSynchroniser
{
    Internal::Responsequeue Responses{};
    size_t Socket{ INVALID_SOCKET };
    uint32_t GameID{};

    public:
    bool Broadcast(std::string &&Payload)
    {
        // Windows does not like to send partial messages, so prefix the ID.
        Payload.insert(0, (const char *)&GameID, sizeof(uint32_t));

        // Broadcast address.
        SOCKADDR_IN Address{};
        Address.sin_family = AF_INET;
        Address.sin_addr.S_un.S_addr = INADDR_BROADCAST;
        Address.sin_port = htons(Internal::Syncport);

        // Broadcast the payload
        return Payload.size() == size_t(sendto(Socket, Payload.data(), Payload.size(), NULL, (SOCKADDR *)&Address, sizeof(Address)));
    }
    explicit LANSynchroniser(uint32_t GameID) : GameID(GameID)
    {
        WSAData Unused;
        WSAStartup(MAKEWORD(2, 2), &Unused);

        // IPX datagram sockets need to use IPv6 on Windows.
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
        std::thread([](LANSynchroniser *Parent)
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

                Debugprint(va("LANSync: %s", std::string(Buffer.get() + 4, Length - 4).c_str()));
                Parent->Responses.Enqueue(std::string(Buffer.get() + 4, Length - 4));
            }

            Debugprint("LANSync thread exiting.");
        }, this).detach();
    }
    std::vector<std::string> getResponses(std::string_view Type)
    {
        return Responses.Dequeue(Type);
    }
};

#endif
