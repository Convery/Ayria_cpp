/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Backend::Messagebus
{
    // Winsock and Posix have different ideas about const.
    #if defined (_WIN32)
    #define __CONST constexpr
    #else
    #define __CONST
    #endif

    // constexpr implementations of htonX / ntohX utilities (yes, identical bodies).
    template <typename T> constexpr T toNative(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <typename T> constexpr T toNet(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }

    // Totally randomly selected constants in a non-public range.
    constexpr uint32_t Multicastaddress = toNet(Hash::FNV1_32("Ayria") << 8);                   // 228.58.137.0
    constexpr uint16_t Multicastport = toNet<uint16_t>(Hash::FNV1_32("Ayria") & 0xFFFF);        // 14985
    constexpr sockaddr_in Multicast{ AF_INET, Multicastport, {{.S_addr = Multicastaddress}} };  // IPv4 format.

    // Non-blocking sockets, multicasts use a random ID to filter ourselves.
    static size_t Multicastsocket{}, Routersocket{};
    const uint32_t RandomID{ GetTickCount() };

    // Buffer management.
    constexpr auto Overhead = sizeof(RandomID);
    constexpr auto Defaultbuffersize = 4096;

    // TODO(tcn): Investigate what thread-safety guarantees Winsock can provide.
    static Spinlock Threadguard{};

    // Client-selected (trust, but optionally verify) routing-servers.
    static Hashset<sockaddr_in, decltype(WW::Hash), decltype(WW::Equal)> Routers{};
    void removeRouter(const sockaddr_in &Address) { Routers.erase(Address); }
    void addRouter(const sockaddr_in &Address) { Routers.insert(Address); }

    // On-wire structure using big endian for integers.
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
        std::array<uint8_t, 32> Publickey;
        std::array<uint8_t, 64> Signature;
        Payload_t Payload;
    };
    #pragma pack(pop)

    // Share a message with the network.
    void Publish(std::string_view Identifier, std::string_view Payload)
    {
        // One can always dream..
        if (!Base85::isValid(Payload)) [[likely]]
        {
            const auto Size = Base85::Encodesize_padded(Payload.size());
            const auto Buffer = static_cast<char *>(alloca(Size));
            Base85::Encode(Payload, std::span(Buffer, Size));
            Payload = { Buffer, Size };
        }

        // Create the buffer with a bit of overhead in case the subsystem needs to modify it.
        auto Buffer = std::make_unique<char[]>(Overhead + sizeof(Packet_t) + LZ4_COMPRESSBOUND(Payload.size()));
        const auto Packet = reinterpret_cast<Packet_t *>(Buffer.get() + Overhead);

        // Build the packet..
        Packet->Publickey = *Global.Publickey;
        std::ranges::copy(Payload, Packet->Payload.Message);
        Packet->Payload.Messagetype = toNet(Hash::WW32(Identifier));
        Packet->Payload.Timestamp = toNet(std::chrono::utc_clock::now().time_since_epoch().count());
        Packet->Signature = qDSA::Sign(*Global.Publickey, *Global.Privatekey, std::span((uint8_t *)&Packet->Payload, sizeof(Payload_t) + Payload.size()));

        // Save our own packets incase someone requests them later.
        Backend::Database()
            << "INSERT OR REPLACE INTO Messagestream VALUES (?,?,?,?,?,?);"
            << Global.getLongID()
            << Hash::WW32(Identifier)
            << Packet->Payload.Timestamp
            << Base85::Encode(Packet->Signature)
            << Payload
            << true;

        // The payload is Base85 so it should compress reasonably well.
        const auto Compressed = LZ4_compress_default(Payload.data(), Packet->Payload.Message, (int)Payload.size(), LZ4_COMPRESSBOUND((int)Payload.size()));

        // Post the mssage in a different thread as multicast may block to throttle.
        std::thread([](std::unique_ptr<char[]> &&Buffer, int Payloadsize)
        {
            // Forward to the routers, best effort.
            for (const auto &Address : Routers)
            {
                std::scoped_lock Lock(Threadguard);
                sendto(Routersocket, Buffer.get() + Overhead, Payloadsize, NULL, (const sockaddr *)&Address, sizeof(Address));
            }

            // Insert our (somewhat) unique ID into the overhead segment.
            *(uint32_t *)Buffer.get() = toNet(RandomID);

            // And to local clients, may block in case if many routers.
            while (true)
            {
                int Result;
                {
                    std::scoped_lock Lock(Threadguard);
                    Result = sendto(Multicastsocket, Buffer.get(), Payloadsize + Overhead, NULL, (const sockaddr *)&Multicast, sizeof(Multicast));
                }

                if (SOCKET_ERROR != Result || WSAEWOULDBLOCK != WSAGetLastError())
                    break;
            }
        }, std::move(Buffer), Compressed + sizeof(Packet_t)).detach();
    }
    void Publish(std::string_view Identifier, JSON::Object_t &&Payload)
    {
        Publish(Identifier, JSON::Dump(Payload));
    }
    void Publish(std::string_view Identifier, const JSON::Object_t &Payload)
    {
        Publish(Identifier, JSON::Dump(Payload));
    }

    // Process the message and ensure validity.
    static void handleMessage(std::string_view Packet)
    {
        const auto Header = reinterpret_cast<const Packet_t*>(Packet.data());
        auto Payload = Packet.substr(sizeof(Packet_t));

        // Decompress the payload.
        const auto Buffer = std::make_unique<char[]>(Payload.size() * 3);
        const auto Size = LZ4_decompress_safe(Payload.data(), Buffer.get(), (int)Payload.size(), (int)Payload.size() * 3);
        Payload = { Buffer.get(), static_cast<size_t>(Size) };

        // Verify the packets signature (in-case someone forwarded it and it got corrupted).
        if (!qDSA::Verify(Header->Publickey, Header->Signature, Payload)) return;

        // Check that the packet isn't from the future.
        if (toNative(Header->Payload.Timestamp) > (uint64_t)std::chrono::utc_clock::now().time_since_epoch().count()) [[unlikely]]
            return;

        // Update the database's last-seen status for the client, without triggering the forgein-keys.
        Backend::Database() << "INSERT INTO Account VALUES (?, ?) ON CONFLICT DO UPDATE SET Lastseen = ?;"
                            << Base58::Encode(Header->Publickey)
                            << toNative(Header->Payload.Timestamp)
                            << toNative(Header->Payload.Timestamp);

        // Save the packet for later processing.
        Backend::Database()
            << "INSERT OR REPLACE INTO Messagestream VALUES (?,?,?,?,?,?);"
            << Base58::Encode(Header->Publickey)
            << toNative(Header->Payload.Messagetype)
            << toNative(Header->Payload.Timestamp)
            << Base85::Encode(Header->Signature)
            << Payload
            << false;
    }
    static void __cdecl doMulticast()
    {
        // Quick-check for data on the sockets.
        {
            constexpr auto Count{ 1 };
            constexpr timeval Defaulttimeout{};
            fd_set ReadFD{ 1, {Multicastsocket} };
            __CONST auto Timeout{ Defaulttimeout };

            // Nothing to do, early exit.
            if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]]
                return;
        }

        // As we know that we have data available, allocate the default buffer.
        const auto Defaultbuffer = static_cast<char*>(alloca(Defaultbuffersize));
        std::unique_ptr<char[]> Backupbuffer;
        auto Buffer = Defaultbuffer;

        // As the backend is single-threaded, we should throttle fetching.
        for (uint8_t Packetcount = 0; Packetcount < 5; ++Packetcount)
        {
            // Query the size of the packet.
            const auto Packetsize = recvfrom(Multicastsocket, Buffer, Defaultbuffersize, MSG_PEEK, NULL, NULL);

            // Early exit on error or invalid payload.
            if (Packetsize == SOCKET_ERROR || Packetsize < (sizeof(Packet_t) + Overhead))
                return;

            // Going to need a bigger boat.
            if (Packetsize >= Defaultbuffersize) [[unlikely]]
                Buffer = (Backupbuffer = std::make_unique<char[]>(Packetsize + 1)).get();

            // Get the actual packet.
            sockaddr_in Clientinfo{}; int Len = sizeof(Clientinfo);
            if (SOCKET_ERROR == recvfrom(Multicastsocket, Buffer, Packetsize, NULL, PSOCKADDR(&Clientinfo), &Len)) [[unlikely]]
                continue;

            // Extract the senders ID.
            const auto UniqueID = toNative(*(uint32_t *)Buffer);

            // We can use our own packets to 'discover' our internal IP if it's not been initialized already.
            if (!Global.InternalIP && UniqueID == RandomID) [[unlikely]]
                Global.InternalIP = Clientinfo.sin_addr.S_un.S_addr;

            // However, our own packets should still be discarded.
            if (UniqueID == RandomID) [[likely]]
                continue;

            // Forward to the handler for processing.
            handleMessage(std::string_view(Buffer).substr(Overhead));
        }
    }
    static void __cdecl doRouting()
    {
        // Common case.
        if (Routers.empty()) [[likely]]
            return;

        // Quick-check for data on the sockets.
        {
            constexpr auto Count{ 1 };
            constexpr timeval Defaulttimeout{};
            fd_set ReadFD{ 1, {Routersocket} };
            __CONST auto Timeout{ Defaulttimeout };

            // Nothing to do, early exit.
            if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]]
                return;
        }

        // As we know that we have data available, allocate the default buffer.
        const auto Defaultbuffer = static_cast<char*>(alloca(Defaultbuffersize));
        std::unique_ptr<char[]> Backupbuffer;
        auto Buffer = Defaultbuffer;

        // As the backend is single-threaded, we should throttle fetching.
        for (uint8_t Packetcount = 0; Packetcount < 10; ++Packetcount)
        {
            // Query the size of the packet.
            const auto Packetsize = recvfrom(Routersocket, Buffer, Defaultbuffersize, MSG_PEEK, NULL, NULL);

            // Early exit on error or invalid payload.
            if (Packetsize == SOCKET_ERROR || Packetsize < (sizeof(Packet_t)))
                return;

            // Going to need a bigger boat.
            if (Packetsize >= Defaultbuffersize)
                Buffer = (Backupbuffer = std::make_unique<char[]>(Packetsize + 1)).get();

            // Get the actual packet.
            sockaddr_in Clientinfo{}; int Len = sizeof(Clientinfo);
            if (SOCKET_ERROR == recvfrom(Routersocket, Buffer, Packetsize, NULL, PSOCKADDR(&Clientinfo), &Len)) [[unlikely]]
                continue;

            // Sanity checking.
            if (!Routers.contains(Clientinfo)) [[unlikely]]
                continue;

            // Forward to the handler for processing.
            handleMessage(std::string_view(Buffer));
        }
    }

    // Set up winsock.
    void Initialize()
    {
        constexpr sockaddr_in Localhost{ AF_INET, Multicastport, {{.S_addr = INADDR_ANY}} };
        constexpr sockaddr_in Randomhost{ AF_INET, NULL, {{.S_addr = INADDR_ANY}} };
        constexpr ip_mreq Request{ {{.S_addr = Multicastaddress}} };
        unsigned long Argument{ 1 };
        unsigned long Error{ 0 };
        WSADATA Unused;

        // Simple, non-blocking sockets.
        (void)WSAStartup(MAKEWORD(2, 2), &Unused);
        Routersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Multicastsocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Routersocket, FIONBIO, &Argument);
        Error |= ioctlsocket(Multicastsocket, FIONBIO, &Argument);

        // Use a random (ephemeral) configuration for the routing socket.
        Error |= bind(Routersocket, (const sockaddr *)&Randomhost, sizeof(Randomhost));

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Multicastsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&Request, sizeof(Request));
        Error |= setsockopt(Multicastsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Multicastsocket, (const sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            Global.Settings.noNetworking = true;
            closesocket(Multicastsocket);
            closesocket(Routersocket);
            assert(false);
            return;
        }
        else
        {
            Global.Settings.noNetworking = false;
        }

        // Ensure that this is only done once.
        [[maybe_unused]] static bool doOnce = []() -> bool
        {
            // TODO(tcn): Tune the polling rate.
            Backend::Enqueuetask(250, doMulticast);
            Backend::Enqueuetask(200, doRouting);
            return true;
        }();
    }

    // Access from the plugins
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
    }

    #undef __CONST
}
