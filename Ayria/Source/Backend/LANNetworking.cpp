/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#include <Global.hpp>
#include <openssl/curve25519.h>

// Totally randomly selected constants here..
constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985
constexpr auto Buffersizelimit = 4096;                          // A single virtual page.

namespace Networking
{
    static const sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static Hashmap<uint32_t /* SessionID */, Client_t> LANClients{};
    static size_t Sendersocket{}, Receiversocket{};
    static Hashset<uint32_t> Verifiedclients{};
    static uint32_t LocalsessionID{};

    // 9 byte shared header.
    #pragma pack(push, 1)
    struct Packetflags_t
    {
        uint8_t
            Encrypted : 1,      // Needs Targeted
            Targeted : 1,
            Signed : 1,
            END : 1;
    };
    struct Header_t
    {
        uint32_t SessionID;
        uint32_t Messagetype;
        Packetflags_t Packetflags;

        // if Targeted uint32_t TargetID
        // if Signed uint8_t[64] Signature
    };
    #pragma pack(pop)

    // Register handlers for the different packets.
    static Hashmap<uint32_t /* Messagetype */, Hashset<Callback_t>> Messagehandlers{};
    void Register(std::string_view Identifier, Callback_t Handler)
    {
        return Register(Hash::WW32(Identifier), Handler);
    }
    void Register(uint32_t Identifier, Callback_t Handler)
    {
        Messagehandlers[Identifier].insert(Handler);
    }

    // Broadcast a message to the LAN clients.
    void Publish(std::string_view Identifier, std::string_view Payload, uint64_t TargetID, bool Encrypt, bool Sign)
    {
        return Publish(Hash::WW32(Identifier), Payload, TargetID, Encrypt, Sign);
    }
    void Publish(uint32_t Identifier, std::string_view Payload, uint64_t TargetID, bool Encrypt, bool Sign)
    {
        // One can always dream..
        if (!Base64::isValid(Payload)) [[likely]]
        {
            static std::string Encoded{};
            Encoded = Base64::Encode(Payload);
            Payload = Encoded;
        }

        std::string Packet{};
        Packet.reserve(sizeof(Header_t) + (!!TargetID * sizeof(uint64_t)) + (Encrypt * 32) + (Sign * 64) + Payload.size());

        Header_t Header{ LocalsessionID, Identifier };
        Header.Packetflags.Targeted = !!TargetID;
        Header.Packetflags.Encrypted = Encrypt;
        Header.Packetflags.Signed = Sign;
        Packet.append((char *)&Header, sizeof(Header));

        if (Header.Packetflags.Targeted) [[unlikely]]
        {
            Packet.append((char *)&TargetID, sizeof(TargetID));
        }

        if (Header.Packetflags.Encrypted) [[unlikely]]
        {
            const auto Client = Clientinfo::byAccount(TargetID);
            if (!Client) [[unlikely]] return;

            uint8_t Sharedkey[32]{};
            if (NULL == X25519(Sharedkey, Global.EncryptionkeyPrivate->data(), Client->EncryptionkeyPublic.data()))
                [[unlikely]] return;

            static std::string Encrypted{};
            const auto Key = Hash::SHA1(Sharedkey, 32);
            const auto IV = Hash::Tiger192(Sharedkey, 32);
            Encrypted = AES::Encrypt(Payload.data(), Payload.size(), Key.data(), IV.data());
            Payload = Encrypted;
        }

        if (Header.Packetflags.Signed) [[unlikely]]
        {
            uint8_t Signature[64]{};
            if (NULL == ED25519_sign(Signature, (uint8_t *)Payload.data(), Payload.size(), Global.SigningkeyPrivate->data()))
                [[unlikely]] return;

            Packet.append((char *)Signature, 64);
        }

        // Put the packet on the network.
        sendto(Sendersocket, Packet.data(), (int)Packet.size(), NULL, (sockaddr *)&Multicast, sizeof(Multicast));
    }

    // Drop packets from bad clients on the floor.
    static Hashset<uint32_t> Blacklistedsessions{};
    void Blockclient(uint32_t SessionID) { Blacklistedsessions.insert(SessionID); }
    static bool isBlocked(uint32_t SessionID) { return Blacklistedsessions.contains(SessionID); }

    // Service access to the LAN clients.
    namespace Clientinfo
    {
        const Client_t *byAccount(AccountID_t AccountID)
        {
            const uint8_t Wantedcount = !!AccountID.AyriaID + !!AccountID.KeyID;
            if (!Wantedcount) return nullptr;

            for (const auto &[Session, Client] : LANClients)
            {
                if (Wantedcount == 1 && AccountID.AyriaID && AccountID.AyriaID == Client.AccountID.AyriaID)
                    return &Client;
                else if (Wantedcount == 1 && AccountID.KeyID && AccountID.KeyID == Client.AccountID.KeyID)
                    return &Client;
                else if (AccountID.KeyID == Client.AccountID.KeyID && AccountID.AyriaID == Client.AccountID.AyriaID)
                    return &Client;
            }

            return nullptr;
        }
        const Client_t *bySession(uint32_t SessionID)
        {
            if (LANClients.contains(SessionID)) [[likely]]
                return &LANClients.at(SessionID);
            return nullptr;
        }
    }

    // Fetch as many messages as possible from the network.
    static void __cdecl Pollnetwork()
    {
        // There is no network available at this time.
        if (Global.Settings.noNetworking) [[unlikely]] return;

        // Check for data on the active socket.
        {
            FD_SET ReadFD{}; FD_SET(Receiversocket, &ReadFD);
            constexpr timeval Defaulttimeout{ NULL, 1 };
            auto Timeout{ Defaulttimeout };
            const auto Count{ 1 };

            if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;
        }

        // Allocate packet-data on the stack rather than heap as it's only 4Kb.
        const auto Buffer = alloca(Buffersizelimit);

        // Get all available packets.
        while (true)
        {
            sockaddr_in Clientaddress; int Addresslen = sizeof(Clientaddress);
            const auto Packetlength = recvfrom(Receiversocket, (char *)Buffer, Buffersizelimit, NULL, (sockaddr *)&Clientaddress, &Addresslen);
            if (Packetlength < static_cast<int>(sizeof(Header_t))) break;  // Also covers any errors.

            #define Consumestream(...) *(__VA_ARGS__ *)Stream.data(); Stream.remove_prefix(sizeof(__VA_ARGS__));
            std::string_view Stream((char *)Buffer, Packetlength);
            const auto Header = Consumestream(Header_t);

            // First packet from our client, save our IP.
            if (NULL == Global.InternalIP && Header.SessionID == LocalsessionID) [[unlikely]]
            {
                Global.InternalIP = Clientaddress.sin_addr.S_un.S_addr;
                continue;
            }

            // Sanity checking for the header.
            if (Header.SessionID == 0) [[unlikely]] continue;                           // Borked client.
            if (isBlocked(Header.SessionID)) [[likely]] continue;                       // Ignored session, likely our own.
            if (Header.Packetflags.Encrypted && !Header.Packetflags.Targeted) continue; // We have no idea how to decrypt this.

            // Client heartbeat, save their information.
            if (Header.Messagetype == Hash::WW32("Clienthello"))
            {
                if (Stream.size() < sizeof(Client_t)) [[unlikely]] continue;
                auto Client = Consumestream(Client_t);
                Client.InternalIP = Clientaddress.sin_addr.S_un.S_addr;

                // Sanity check.
                if (Client.AccountID.KeyID != Hash::WW32(Client.SigningkeyPublic)) [[unlikely]] continue;
                if (!Verifiedclients.contains(Client.AccountID.AyriaID))
                {
                    // TODO(tcn): Validate authentication.
                    Client.AccountID.AyriaID = 0;
                }

                // TODO(tcn): Check emplace result and notify callbacks.
                LANClients.emplace(Header.SessionID, std::move(Client));
                continue;
            }

            // Client terminated, remove them.
            if (Header.Messagetype == Hash::WW32("Clientgoodbye")) [[unlikely]]
            {
                // TODO(tcn): Notify callbacks.
                if (Header.SessionID == 0) [[unlikely]] continue;
                LANClients.erase(Header.SessionID);
                continue;
            }

            // More checking for odd packets.
            if (!LANClients.contains(Header.SessionID)) [[unlikely]] continue;          // Unknown client, needs a hello first.
            if (!Messagehandlers.contains(Header.Messagetype)) [[unlikely]] continue;   // Not a packet we know how to handle.

            // Update the timestamp for the client, even if the payload is corrupted.
            LANClients.at(Header.SessionID).Lastmessage = (uint32_t)time(NULL);
            const auto Client = &LANClients.at(Header.SessionID);

            // If this is a targeted message, check that it's for us.
            if (Header.Packetflags.Targeted)
            {
                if (Stream.size() < sizeof(uint64_t)) [[unlikely]] continue;
                const auto TargetID = Consumestream(uint64_t);
                if (TargetID != Global.AccountID) continue;
            }

            // If the message is signed, verify.
            if (Header.Packetflags.Signed)
            {
                if (Stream.size() < sizeof(std::array<uint8_t, 64>)) [[unlikely]] continue;
                const auto Signature = Consumestream(std::array<uint8_t, 64>);

                if (NULL == ED25519_verify((uint8_t *)Stream.data(), Stream.size(), Signature.data(), Client->SigningkeyPublic.data()))
                    [[unlikely]] continue;
            }

            // Decrypt the payload if needed.
            if (Header.Packetflags.Encrypted)
            {
                uint8_t Sharedkey[32]{};
                if (NULL == X25519(Sharedkey, Global.EncryptionkeyPrivate->data(), Client->EncryptionkeyPublic.data()))
                    [[unlikely]] continue;

                const auto Key = Hash::SHA1(Sharedkey, 32);
                const auto IV = Hash::Tiger192(Sharedkey, 32);

                static std::string Decrypted;
                Decrypted = AES::Decrypt(Stream.data(), Stream.size(), Key.data(), IV.data());
                Stream = Decrypted;
            }

            // Basic sanity checking before processing the packet, if they can't even format the payload we don't care.
            if (!Base64::isValid(Stream)) [[unlikely]] continue;

            // Decode the incoming message and forward it to the handler(s).
            const auto Decoded = Base64::Decode_inplace((char *)Stream.data(), Stream.size());
            std::ranges::for_each(Messagehandlers[Header.Messagetype],
                                  [&](const auto &CB) { if (CB) [[likely]] CB(Client, Decoded.data(), (uint32_t)Decoded.size()); });
        }
    }

    // Share our info once in a while.
    static void __cdecl Announceclient()
    {
        static Client_t Self = []()
        {
            Client_t Tmp{};
            X25519_public_from_private(Tmp.EncryptionkeyPublic.data(), Global.EncryptionkeyPrivate->data());
            return Tmp;
        }();

        std::memcpy(Self.Username.data(), Global.Username->data(), std::min(Self.Username.size(), Global.Username->size()));
        std::memcpy(Self.SigningkeyPublic.data(), Global.SigningkeyPublic->data(), Global.SigningkeyPublic->size());
        Self.AccountID = Global.AccountID;
        Self.GameID = Global.GameID;
        Self.ModID = Global.ModID;

        // Publish
        Publish("Clienthello", { (char *)&Self, sizeof(Self) });
    }

    // Initialize the system.
    void Initialize()
    {
        WSADATA Unused;
        unsigned long Error{ 0 };
        unsigned long Argument{ 1 };
        const ip_mreq Request{ {{.S_addr = htonl(Syncaddress)}} };
        const sockaddr_in Localhost{ AF_INET, htons(Syncport), {{.S_addr = htonl(INADDR_ANY)}} };

        // We only need WS 1.1, no need to load more modules.
        WSAStartup(MAKEWORD(1, 1), &Unused);
        Sendersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Receiversocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Receiversocket, FIONBIO, &Argument);

        // TODO(tcn): Investigate if WS2's IP_ADD_MEMBERSHIP (12) gets mapped to the WS1's original (5) internally.
        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            //Global.Settings.noNetworking = true;
            closesocket(Receiversocket);
            closesocket(Sendersocket);
            assert(false);
            return;
        }

        // Make a unique identifier for our session.
        LocalsessionID = Hash::WW32(GetTickCount64() ^ Sendersocket);

        // Ensure that we don't process our own packets.
        Blacklistedsessions.insert(LocalsessionID);

        // Should only need to check for packets 5 times a second.
        Backend::Enqueuetask(5000, Announceclient);
        Backend::Enqueuetask(200, Pollnetwork);
        Global.Settings.noNetworking = false;
    }
}
