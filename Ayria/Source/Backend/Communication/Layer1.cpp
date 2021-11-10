/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

// Big endian.
static uint16_t Listenport;

namespace LANDiscovery
{
    constexpr uint32_t Discoveryaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
    constexpr uint16_t Discoveryport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985

    static const sockaddr_in Multicast{ AF_INET, htons(Discoveryport), {{.S_addr = htonl(Discoveryaddress)}} };
    static size_t Discoverysocket{};
    static uint32_t RandomID{};

    static uint32_t Tickcount = 9;
    static void __cdecl doNetworking()
    {
        // Only announce every 10 sec.
        if (++Tickcount == 10)
        {
            char Buffer[6]{};
            *(uint32_t *)&Buffer[0] = RandomID;
            *(uint16_t *)&Buffer[4] = Listenport;

            sendto(Discoverysocket, Buffer, 6, NULL, PSOCKADDR(&Multicast), sizeof(Multicast));
            Tickcount = 0;
        }

        // Check for data on the socket.
        FD_SET ReadFD{}; FD_SET(Discoverysocket, &ReadFD);
        constexpr timeval Defaulttimeout{ NULL, 1 };
        const auto Count{ ReadFD.fd_count };
        auto Timeout{ Defaulttimeout };

        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]]
            return;

        while (true)
        {
            char Buffer[6]{};

            sockaddr_in Clientinfo{}; int Len = sizeof(Clientinfo);
            const auto Data = recvfrom(Discoverysocket, Buffer, 6, NULL, PSOCKADDR(&Clientinfo), &Len);
            if (Data < 6) [[unlikely]] break;   // Also covers any errors.

            // We also receive our own requests, but we can use it for IP discovery.
            if (RandomID == *(uint32_t *)&Buffer[0]) [[likely]]
            {
                Global.InternalIP = Clientinfo.sin_addr.S_un.S_addr;
                continue;
            }

            // Try to connect to this client if we haven't already.
            Backend::Messagebus::Connectuser(ntohl(Clientinfo.sin_addr.S_un.S_addr), ntohs(*(uint16_t *)&Buffer[4]));
        }
    }

    void Initialize()
    {
        const sockaddr_in Localhost{ AF_INET, htons(Discoveryport), {{.S_addr = htonl(INADDR_ANY)}} };
        const ip_mreq Request{ {{.S_addr = htonl(Discoveryaddress)}} };
        unsigned long Argument{ 1 };
        unsigned long Error{ 0 };
        WSADATA Unused;

        // We only need WS 1.1, no need for more.
        (void)WSAStartup(MAKEWORD(1, 1), &Unused);
        Discoverysocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Discoverysocket, FIONBIO, &Argument);

        // TODO(tcn): Investigate if WS2's IP_ADD_MEMBERSHIP (12) gets mapped to the WS1's original (5) internally.
        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Discoverysocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Discoverysocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Discoverysocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            closesocket(Discoverysocket);
            assert(false);
            return;
        }

        // Make a unique identifier for our session.
        RandomID = Hash::WW32(GetTickCount64() ^ Discoverysocket);

        // It's unlikely that a new client joins.
        Backend::Enqueuetask(1000, doNetworking);
    }
}

namespace Backend::Messagebus
{
    #pragma pack(push, 1)
    struct Payload_t
    {
        uint32_t Messagetype;
        uint64_t Timestamp;

        #pragma warning(suppress: 4200)
        char Message[0];
    };
    struct Packet_t
    {
        std::array<uint8_t, 64> Signature;
        Payload_t Payload;
    };
    #pragma pack(pop)

    using Node_t = struct { uint32_t IPv4; uint16_t Port; size_t Socket; };
    static Hashmap<std::string, Node_t> Connectednodes;
    static Spinlock Threadsafe;
    static size_t Listensocket;

    static std::string doHandshake(size_t Socket)
    {
        struct { std::array<uint8_t, 64> Signature; std::array<uint8_t, 32> Publickey; } Ours{}, Theirs{};

        const auto Sig = qDSA::Sign(*Global.Publickey, *Global.Privatekey, *Global.Publickey);
        std::ranges::copy(*Global.Publickey, Ours.Publickey.begin());
        std::ranges::move(Sig, Ours.Signature.begin());

        do
        {
            if (static_cast<int>(sizeof(Ours)) != send(Socket, (char *)&Ours, sizeof(Ours), NULL)) [[unlikely]] break;
            if (static_cast<int>(sizeof(Theirs)) != recv(Socket, (char *)&Theirs, sizeof(Theirs), NULL)) [[unlikely]] break;
            if (!qDSA::Verify(Theirs.Publickey, Theirs.Signature, Theirs.Publickey)) [[unlikely]] break;

            return Base58::Encode<char>(Theirs.Publickey);
        } while (false);

        return {};
    }
    void Connectuser(uint32_t IPv4, uint16_t Port)
    {
        std::thread([](uint32_t IPv4, uint16_t Port)
        {
            const sockaddr_in Hostinfo{ AF_INET, htons(Port), {{.S_addr = htonl(IPv4)}} };

            // Check if we already have a connection to this client.
            for (const auto &Node : Connectednodes | std::views::values)
            {
                if (Node.IPv4 == Hostinfo.sin_addr.S_un.S_addr && Node.Port == Hostinfo.sin_port)
                    return;
            }

            const auto Socket = socket(AF_INET, SOCK_STREAM, 0);
            if (Socket == INVALID_SOCKET) return;

            do
            {
                if (0 != connect(Socket, PSOCKADDR(&Hostinfo), sizeof(Hostinfo))) [[unlikely]]
                    break;

                const auto PK = doHandshake(Socket);
                if (PK.empty()) [[unlikely]] break;

                std::scoped_lock Lock(Threadsafe);
                if (Connectednodes.contains(PK)) closesocket(Connectednodes[PK].Socket);
                Connectednodes[PK] = { Hostinfo.sin_addr.S_un.S_addr, Hostinfo.sin_port, Socket };
                return;

            } while (false);

            closesocket(Socket);
        }, IPv4, Port).detach();
    }
    static void Acceptconnection()
    {
        std::thread([]()
        {
            sockaddr_in Sockinfo{}; int Len = sizeof(Sockinfo);
            const auto Socket = accept(Listensocket, PSOCKADDR(&Sockinfo), &Len);

            const auto PK = doHandshake(Socket);
            if (PK.empty()) [[unlikely]]
            {
                closesocket(Socket);
                return;
            }

            std::scoped_lock Lock(Threadsafe);
            if (Connectednodes.contains(PK)) closesocket(Connectednodes[PK].Socket);
            Connectednodes[PK] = { Sockinfo.sin_addr.S_un.S_addr, Sockinfo.sin_port, Socket };
        }).detach();
    }

    static Hashset<std::string> Cachedclients{};
    void Publish(std::string_view Identifier, std::string_view Payload)
    {
        // One can always dream..
        if (!Base85::isValid(Payload)) [[likely]]
        {
            const auto Size = Base85::Encodesize_padded(Payload.size());
            const auto Buffer = static_cast<char*>(alloca(Size));
            Base85::Encode(Payload, std::span(Buffer, Size));
            Payload = { Buffer, Size };
        }

        // Create the buffer with a bit of overhead in case the subsystem needs to modify it.
        auto Buffer = std::make_unique<char []>(sizeof(Packet_t) + LZ4_COMPRESSBOUND(Payload.size()));
        const auto Packet = reinterpret_cast<Packet_t*>(Buffer.get());

        // Build the packet..
        std::ranges::copy(Payload, Packet->Payload.Message);
        Packet->Payload.Messagetype = Hash::WW32(Identifier);
        Packet->Payload.Timestamp = std::chrono::utc_clock::now().time_since_epoch().count();
        Packet->Signature = qDSA::Sign(*Global.Publickey, *Global.Privatekey, std::span((uint8_t *)&Packet->Payload, sizeof(Payload_t) + Payload.size()));

        // Save our own packets so we can have unified processing.
        try
        {
            Backend::Database()
                << "INSERT INTO Messagestream VALUES (?,?,?,?,?,?);"
                << Global.getLongID() << Hash::WW32(Identifier) << Packet->Payload.Timestamp
                << Base85::Encode<char>(Packet->Signature)
                << std::string(Payload) << false;
        } catch (...) {}

        // Hard-lock: if networking is disabled, or the client is private, don't send anything.
        if (Global.Settings.noNetworking || Global.Settings.isPrivate) [[unlikely]] return;

        // The payload is Base85 so it should compress reasonably well.
        const auto Compressed = LZ4_compress_default(Payload.data(), Packet->Payload.Message, (int)Payload.size(), LZ4_COMPRESSBOUND((int)Payload.size()));
        std::thread([](std::unique_ptr<char[]> &&Buffer, int Payloadsize)
        {
            for (const auto &Node : Connectednodes | std::views::values)
            {
                send(Node.Socket, Buffer.get(), Payloadsize, NULL);
            }
        }, std::move(Buffer), Compressed + sizeof(Packet_t)).detach();
    }
    static void handleMessage(const std::string &PK, std::string_view Packet)
    {
        std::array<uint8_t, 32> Publickey; Base58::Decode(PK, Publickey.data());
        const auto Header = reinterpret_cast<const Packet_t*>(Packet.data());
        auto Payload = Packet.substr(sizeof(Packet_t));

        // Decompress the payload.
        const auto Buffer = std::make_unique<char[]>(Payload.size() * 3);
        const auto Size = LZ4_decompress_safe(Payload.data(), Buffer.get(), (int)Payload.size(), (int)Payload.size() * 3);
        Payload = { Buffer.get(), static_cast<size_t>(Size) };

        // Verify the packets signature (in-case someone forwarded it and it got corrupted).
        if (!qDSA::Verify(Publickey, Header->Signature, Payload)) return;

        // Check that the packet isn't from the future.
        if (Header->Payload.Timestamp > (uint64_t)std::chrono::utc_clock::now().time_since_epoch().count()) [[unlikely]] return;

        // Ensure that the PK exists in the DB.
        if (!Cachedclients.contains(PK) && !AyriaAPI::Clientinfo::Find(PK)) [[unlikely]]
        {
            try { Backend::Database() << "INSERT INTO Account VALUES (?);" << PK; } catch (...) {}
            Cachedclients.insert(PK);
        }

        // Save the packet for our internal synchronization.
        try
        {
            Backend::Database()
                << "INSERT INTO Messagestream VALUES (?,?,?,?,?,?);"
                << PK << Header->Payload.Messagetype << Header->Payload.Timestamp
                << Base85::Encode<char>(Header->Signature)
                << std::string(Payload) << false;
        } catch (...) {}
    }

    static void __cdecl doNetworking()
    {
        fd_set ReadFD{};
        FD_SET(Listensocket, &ReadFD);
        for (const auto &Node : Connectednodes)
            FD_SET(Node.second.Socket, &ReadFD);

        // Check for data on the sockets.
        constexpr timeval Defaulttimeout{ NULL, 1 };
        const auto Count{ ReadFD.fd_count };
        auto Timeout{ Defaulttimeout };

        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]]
            return;

        if (FD_ISSET(Listensocket, &ReadFD))
            Acceptconnection();

        for (const auto &[PK, Node] : Connectednodes)
        {
            if (!FD_ISSET(Node.Socket, &ReadFD)) [[likely]] continue;

            uint16_t Wanted{};
            const auto Return = recv(Node.Socket, (char *)&Wanted, sizeof(Wanted), MSG_PEEK);
            if (Return < static_cast<int>(sizeof(Wanted))) [[unlikely]]
            {
                if (Return == 0) // Connection closed.
                {
                    closesocket(Node.Socket);
                    Connectednodes.erase(PK);
                    break;
                }
                continue;
            }

            DWORD Available{};
            if (SOCKET_ERROR == ioctlsocket(Node.Socket, FIONREAD, &Available)) [[unlikely]]
                continue;

            const int Total = ntohs(Wanted) + sizeof(Wanted);
            if ((int)Available >= Total) [[likely]]
            {
                const auto Buffer = static_cast<char*>(alloca(Total));

                if (Total > recv(Node.Socket, Buffer, Total, NULL)) [[unlikely]]
                    break;

                handleMessage(PK, { Buffer, Wanted });
                break;
            }
        }
    }
    void Initialize(bool doLANDiscovery)
    {
        do
        {
            WSADATA Unused;
            (void)WSAStartup(MAKEWORD(1, 1), &Unused);

            Listensocket = socket(AF_INET, SOCK_STREAM, 0);
            if (Listensocket == INVALID_SOCKET) break;

            BOOL Argument{ 0 }; // Enable Nagles algorithm.
            if (SOCKET_ERROR == setsockopt(Listensocket, IPPROTO_TCP, TCP_NODELAY, (char *)&Argument, sizeof(Argument))) [[unlikely]]
                break;

            Argument = 1; // Enable periodic pings.
            if (SOCKET_ERROR == setsockopt(Listensocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&Argument, sizeof(Argument))) [[unlikely]]
                break;

            // Bind to a random port.
            const sockaddr_in Localhost{ AF_INET, 0, {{.S_addr = htonl(INADDR_ANY)}} };
            if (SOCKET_ERROR == bind(Listensocket, PSOCKADDR(&Localhost), sizeof(Localhost))) [[unlikely]]
                break;

            // As a client, we shouldn't have too many incoming connections.
            if (SOCKET_ERROR == listen(Listensocket, 5)) [[unlikely]]
                break;

            // Fetch the random port we are bound to.
            sockaddr_in Sockinfo{}; int Len = sizeof(Sockinfo);
            if (SOCKET_ERROR == getsockname(Listensocket, PSOCKADDR(&Sockinfo), &Len)) [[unlikely]]
                break;

            // Should only need to check for packets 5 times a second.
            Backend::Enqueuetask(200, doNetworking);
            Global.Settings.noNetworking = false;
            Listenport = Sockinfo.sin_port;

            if (doLANDiscovery) LANDiscovery::Initialize();
        } while (false);

        Global.Settings.noNetworking = true;
        closesocket(Listensocket);
        Listenport = 0;
        return;
    }

    namespace Export
    {
        extern "C" EXPORT_ATTR void __cdecl publishMessage(const char *Identifier, const char *Message, unsigned int Length)
        {
            if (!Identifier || !Message) [[unlikely]]
            {
                assert(false);
                return;
            }

            Publish(Identifier, { Message, Length });
        }
        extern "C" EXPORT_ATTR void __cdecl connectUser(const char *IPv4, const char *Port)
        {
            if (!IPv4 || !Port) [[unlikely]]
            {
                assert(false);
                return;
            }

            Connectuser(inet_addr(IPv4), htons(std::atoi(Port)));
        }
    }
}
