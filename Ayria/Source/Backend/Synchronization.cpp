/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

// Winsock and Posix have different ideas about const.
#if defined (_WIN32)
#define __CONST constexpr
#else
#define __CONST
#endif

// Totally randomly selected constants in a non-public range.
constexpr uint32_t Multicastaddress = Endian::toBig(Hash::FNV1_32("Ayria") << 8);           // 228.58.137.0
constexpr uint16_t Multicastport = Endian::toBig(Hash::FNV1_32("Ayria") & 0xFFFF);          // 14985
constexpr sockaddr_in Multicast{ AF_INET, Multicastport, {{.S_addr = Multicastaddress}} };  // IPv4 format.

// DOC(tcn): On-line representation of the packets.
struct Networkpacket_t final
{
    std::array<uint8_t, 64> Signature;
    std::array<uint8_t, 32> Publickey;

    // Signed payload, little endian timestamp, JSON message.
    int64_t Timestamp;
    char Messagedata[0];
};

constexpr auto a = sizeof(cmp::Vector_t<uint8_t, 64>);

namespace Synchronization
{
    // Routers accept sync packets and returns sync packets from other keys.
    // Sockets are non-blocking UDP, with 5 retries for sending.
    static size_t Multicastsocket{}, Routingsocket{};
    static Inlinedvector<sockaddr_in, 2> Routers{};
    static Hashset<std::string> Wantedkeys{};
    static std::atomic_flag newKeys{};

    // A UDP packet is less than 64K.
    constexpr auto Maxbuffersize = 0xFFFF;

    // Publish a packet to the network.
    static void Publish(const JSON::Value_t &Payload)
    {
        char *Packetbuffer{};
        std::unique_ptr<char[]> Heapbuffer{};
        const auto Serialized = Payload.dump();
        const auto Compressmax = int(LZ4_COMPRESSBOUND(Serialized.size()));

        // Should be on the stack in 99% of cases.
        const auto Buffersize = sizeof(Networkpacket_t) + Compressmax;
        if (Buffersize <= Maxbuffersize) [[likely]] Packetbuffer = (char *)alloca(Buffersize);
        else { Heapbuffer = std::make_unique<char[]>(Buffersize); Packetbuffer = Heapbuffer.get(); }

        // Compress the payload.
        const auto Header = reinterpret_cast<Networkpacket_t *>(Packetbuffer);
        const auto Compressedsize = LZ4_compress_default(Serialized.data(), Header->Messagedata, int(Serialized.size()), Compressmax);
        const auto Signedpart = std::span(reinterpret_cast<const uint8_t *>(&Header->Timestamp), sizeof(Header->Timestamp) + Compressedsize);

        // Verify that this packet is in range..
        assert(Maxbuffersize > (sizeof(Networkpacket_t) + Compressedsize));

        // Draw the rest of the owl-packet.
        Header->Timestamp = Endian::toLittle(std::chrono::system_clock::now().time_since_epoch().count());
        Header->Signature = qDSA::Sign(*Global.Publickey, *Global.Privatekey, Signedpart);
        Header->Publickey = *Global.Publickey;

        // Format as a nice little span.
        const auto Fullpacket = std::span(Packetbuffer, sizeof(Networkpacket_t) + Compressedsize);
        const auto Lambda = [&Fullpacket](size_t Socket, const sockaddr_in &Address)
        {
            for (uint8_t Count = 0; Count < 5; ++Count)
            {
                const auto Result = sendto(Socket, Fullpacket.data(), int(Fullpacket.size()), NULL, (const sockaddr *)&Address, sizeof(Address));
                if (SOCKET_ERROR != Result || WSAEWOULDBLOCK != WSAGetLastError()) break;
            }
        };

        // Send to the targets.
        Lambda(Multicastsocket, Multicast);
        std::for_each(std::execution::par_unseq, Routers.begin(), Routers.end(),
                      [&](const sockaddr_in &Address) { Lambda(Routingsocket, Address); });

        // NOTE(tcn): We may want to zero out the buffer in the future.
    }

    // Servers provide (filtered by client ID) access to past packets.
    void addPublickeys(const std::vector<std::string_view> &ClientIDs)
    {
        for (const auto &ClientID : ClientIDs)
        {
            Wantedkeys.insert({ ClientID.data(), ClientID.size() });
        }
        newKeys.test_and_set();
    }
    void addServer(const std::string &Hostname, uint16_t Port)
    {
        sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_addr.s_addr = inet_addr(Hostname.c_str());
        add.sin_port = Endian::toBig(Port);

    }
    void addPublickey(std::string_view ClientID)
    {
        Wantedkeys.insert({ ClientID.data(), ClientID.size() });
        newKeys.test_and_set();
    }

    // Listen for incoming messages.
    static Hashmap<uint32_t, Hashset<Callback_t>> syncListeners{};
    static Hashmap<uint32_t, Hashset<Callback_t>> pluginListeners{};
    static Hashmap<uint32_t, Hashset<Callback_t>> routingListeners{};
    void addSynclistener(std::string_view Tablename, Callback_t Callback)
    {
        syncListeners[Hash::WW32(Tablename)].insert(Callback);
    }
    void addPluginlistener(std::string_view Messagetype, Callback_t Callback)
    {
        pluginListeners[Hash::WW32(Messagetype)].insert(Callback);
    }
    void addRoutinglistener(std::string_view Messagetype, Callback_t Callback)
    {
        routingListeners[Hash::WW32(Messagetype)].insert(Callback);
    }

    // Helpers for publishing certain packets.
    void sendRoutingpacket(std::string_view Messagetype, JSON::Object_t &&Payload)
    {
        Payload["Messagetype"] = Hash::WW32(Messagetype);
        Payload["Routercount"] = Routers.size();
        return Publish(Payload);
    }
    void sendSyncpacket(int Operation, std::string_view Tablename, const JSON::Array_t &Values)
    {
        Publish(JSON::Object_t{ { "Operation", Operation }, { "Table", Hash::WW32(Tablename) }, { "Values", Values } });
    }
    void sendPluginpacket(std::string_view Messagetype, std::string_view Pluginname, JSON::Object_t &&Payload)
    {
        Payload["Messagetype"] = Hash::WW32(Messagetype);
        Payload["Pluginname"] = std::string(Pluginname);
        return Publish(Payload);
    }

    // Separate handlers for the different packets.
    inline void handleSyncpacket(int64_t Timestamp, int64_t RowID, const std::u8string &SenderID, const JSON::Value_t &Payload)
    {
        // Required fields.
        if (!Payload.contains_all("Operation", "Table", "Values")) return;

        // Notify any listeners.
        std::ranges::for_each(syncListeners[Payload["Table"]], [&](const auto &CB)
        {
            CB(Timestamp, RowID, SenderID, Payload);
        });
    }
    inline void handlePluginpacket(int64_t Timestamp, int64_t RowID, const std::u8string &SenderID, const JSON::Value_t &Payload)
    {
        // Required fields.
        if (!Payload.contains_all("Messagetype", "Senderplugin")) [[likely]] return;

        // Notify any listeners.
        std::ranges::for_each(pluginListeners[Payload["Messagetype"]], [&](const auto &CB)
        {
            CB(Timestamp, RowID, SenderID, Payload);
        });
    }
    inline void handleRouterpacket(int64_t Timestamp, int64_t RowID, const std::u8string &SenderID, const JSON::Value_t &Payload)
    {
        // Required fields.
        if (!Payload.contains_all("Messagetype", "Routercount")) [[likely]] return;

        // Notify any listeners.
        std::ranges::for_each(routingListeners[Payload["Messagetype"]], [&](const auto &CB)
        {
            CB(Timestamp, RowID, SenderID, Payload);
        });
    }

    // Parse and insert into the database.
    static void savePacket(std::span<const char> Packetdata)
    {
        // Malformed packet, no need to bother with it.
        if (Packetdata.size() < sizeof(Networkpacket_t)) [[unlikely]]
            return;

        // Pre-process the data for use with the database.
        const auto Header = reinterpret_cast<const Networkpacket_t *>(Packetdata.data());
        const auto Signature = Base85::Encode(Header->Signature);
        const auto Publickey = Base58::Encode(Header->Publickey);

        // If we are in routing mode, we may get duplicates, so check the DB.
        if (!!Global.Settings.enableRouting)
        {
            static auto CheckPS = Core::QueryDB() << "SELECT COUNT(*) FROM Syncpackets WHERE (Publickey = ? AND Signature = ?) LIMIT 1;";
            size_t Exists = 0;

            CheckPS << Publickey << Signature >> Exists;
            if (!!Exists) [[unlikely]] return;
        }

        // Corrupted or invalid packet, discard.
        const auto Signedpart = Packetdata.subspan(offsetof(Networkpacket_t, Timestamp));
        if (!qDSA::Verify(Header->Publickey, Header->Signature, Signedpart)) [[unlikely]]
            return;

        // Get the payloads properties.
        const auto Compressed = Packetdata.subspan(sizeof(Networkpacket_t));
        const auto Compressedsize = int(Compressed.size());

        // Decompress the payload.
        const auto Buffersize = int(Compressedsize * 3);
        const auto Payloadbuffer = std::make_unique<char[]>(Buffersize);
        const int Decompressedsize = LZ4_decompress_safe(Compressed.data(), Payloadbuffer.get(), Compressedsize, Buffersize);

        // Create prepared statements for the queries.
        static auto UpdatePS = Core::QueryDB() << "INSERT INTO Accounts (Publickey, Timestamp) VALUES (?,?) ON CONFLICT DO UPDATE SET Timestamp = ?;";
        static auto InsertPS = Core::QueryDB() << "INSERT INTO Syncpackets VALUES (?,?,?,?,?);";

        // Update the users activity tracker in the database.
        const auto Timestamp = Endian::fromLittle(Header->Timestamp);
        UpdatePS << Publickey << Timestamp << Timestamp;
        UpdatePS.Execute();

        // Insert the static parts of the packet.
        const auto Decompressed = std::span{ Payloadbuffer.get(), size_t(Decompressedsize) };
        InsertPS << Publickey << Timestamp << Signature << Base85::Encode(Decompressed);

        // If this is our own packet, it's already processed.
        InsertPS << (0 == std::memcmp(Global.Publickey->data(), Header->Publickey.data(), 32));
        InsertPS.Execute();
    }

    // Process the database for new messages.
    static void __cdecl doSynchronization()
    {
        Inlinedvector<int64_t, 10> Processed{};

        // Create prepared statements for the queries.
        static auto QueryPS = Core::QueryDB() << "SELECT rowid, SenderID, Timestamp, Payload FROM Syncpackets WHERE (isProcessed = false) ORDER BY Timestamp LIMIT 10;";
        static auto UpdatePS = Core::QueryDB() << "UPDATE Syncpackets SET isProcessed = true WHERE rowid = ?;";

        // Get up to 10 unprocessed packets from the DB.
        QueryPS >> [&](int64_t rowid, const std::u8string &SenderID, int64_t Timestamp, std::u8string &&Payload)
        {
            // Mark as checked.
            Processed.emplace_back(rowid);

            // Decode generates less data, so we can do it in-place.
            const auto Decoded = Base85::Decode(std::span(Payload), std::span(Payload));
            const auto Object = JSON::Parse(Decoded);

            // Forward to whomever is interested.
            handleSyncpacket(Timestamp, rowid, SenderID, Object);
            handlePluginpacket(Timestamp, rowid, SenderID, Object);
            handleRouterpacket(Timestamp, rowid, SenderID, Object);
        };

        // Mark the packets as processed.
        for (const auto &RowID : Processed)
        {
            UpdatePS << RowID;
            UpdatePS.Execute();
        }
    }

    // Poll for new packets on the network.
    static void __cdecl doNetworking()
    {
        // Quick-check for data on the sockets.
        {
            constexpr auto Count{ 1 };
            constexpr timeval Defaulttimeout{};
            __CONST auto Timeout{ Defaulttimeout };
            fd_set ReadFD{ 2, {Multicastsocket, Routingsocket} };

            // Nothing to do, early exit.
            if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]]
                return;
        }

        // As we know that we have data available, allocate the maximum size.
        const auto Packetbuffer = static_cast<char*>(alloca(Maxbuffersize));

        // As the backend is single-threaded, we should throttle fetching.
        for (uint8_t Count = 0; Count < 5; ++Count)
        {
            const auto Lambda = [&](size_t Socket)
            {
                const auto Packetsize = recvfrom(Socket, Packetbuffer, Maxbuffersize, NULL, nullptr, nullptr);
                if (Packetsize < static_cast<int>(sizeof(Networkpacket_t))) return;

                // Forward to the handler.
                savePacket({ Packetbuffer, size_t(Packetsize) });
            };

            Lambda(Routingsocket);
            Lambda(Multicastsocket);
        }
    }

    // Notify the routers about a new packet.
    static void __cdecl reSubscribe()
    {
        if (newKeys.test() && !Routers.empty()) [[unlikely]]
        {
            JSON::Array_t Keys; Keys.reserve(Wantedkeys.size());
            for (const auto &Key : Wantedkeys)
                Keys.emplace_back(Key);
            newKeys.clear();

            // No
            sendRoutingpacket("Router::Subscribe", JSON::Object_t{ {"Keys", Keys} });
        }
    }

    // Initialize networking.
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
        Routingsocket = socket(AF_INET, SOCK_DGRAM, 0);
        Multicastsocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Routingsocket, FIONBIO, &Argument);
        Error |= ioctlsocket(Multicastsocket, FIONBIO, &Argument);

        // Use a random (ephemeral) configuration for the routing socket.
        Error |= bind(Routingsocket, (const sockaddr *)&Randomhost, sizeof(Randomhost));

        // Join the multicast group, reuse the address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Multicastsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&Request, sizeof(Request));
        Error |= setsockopt(Multicastsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&Argument, sizeof(Argument));
        Error |= bind(Multicastsocket, reinterpret_cast<const sockaddr *>(&Localhost), sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        Global.Settings.noNetworking = !!Error;
        if (Error) [[unlikely]]
        {
            closesocket(Multicastsocket);
            closesocket(Routingsocket);
            assert(false);
            return;
        }

        // Ensure that this is only done once.
        [[maybe_unused]] static bool doOnce = []() -> bool
        {
            // Ensure that we exist in the database.
            const auto Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            Core::QueryDB()
                << "INSERT INTO Accounts (Publickey, Timestamp) VALUES (?,?) ON CONFLICT DO UPDATE SET Timestamp = ?;"
                << Global.getLongID() << Timestamp << Timestamp;

            // TODO(tcn): Tune the polling rate.
            Core::Enqueuetask(2000, reSubscribe);
            Core::Enqueuetask(150, doNetworking);
            Core::Enqueuetask(250, doSynchronization);
            return true;
        }();
    }
}
