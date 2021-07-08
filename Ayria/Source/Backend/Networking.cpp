/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-23
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// Totally randomly selected constants here..
constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985

#pragma pack(push, 1)
struct LANpacket_t { uint32_t SessionID; uint32_t Identifier; uint16_t Payloadlength; char Payload[1]; };
#pragma pack(pop)

namespace Network::LAN
{
    static sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static Hashmap<uint32_t, Hashset<Callback_t>> Messagehandlers{};
    static std::unordered_map<uint32_t, IN_ADDR> Peerlist{};
    static size_t Sendersocket{}, Receiversocket{};
    static Hashset<uint32_t> Blacklistedclients{};
    static uint32_t SessionID{};

    // Handlers for the different packets.
    void Register(std::string_view Identifier, Callback_t Handler)
    {
        return Register(Hash::WW32(Identifier), Handler);
    }
    void Register(uint32_t Identifier, Callback_t Handler)
    {
        Messagehandlers[Identifier].insert(Handler);
    }

    // Broadcast a message to the LAN clients.
    void Publish(std::string_view Identifier, std::string_view Payload)
    {
        return Publish(Hash::WW32(Identifier), Payload);
    }
    void Publish(uint32_t Identifier, std::string_view Payload)
    {
        // Developers should know better than to send 4KB as a single packet.
        const auto Totalsize = static_cast<int>(10 + Payload.size());
        assert(Totalsize < Buffersize);

        // If the client wants to remain private, don't post anything.
        if (Global.Settings.noNetworking) [[unlikely]] return;
        if (Global.Settings.isPrivate) [[unlikely]] return;

        auto Packet = (LANpacket_t *)alloca(Totalsize);
        Packet->SessionID = SessionID;
        Packet->Identifier = Identifier;
        Packet->Payloadlength = Payload.size();
        std::memcpy(Packet->Payload, Payload.data(), Payload.size());

        // We are in non-blocking mode, so may need a retry or two.
        while (true)
        {
            const auto Sent = sendto(Sendersocket, (char *)Packet, Totalsize, 0, (sockaddr *)&Multicast, sizeof(Multicast));
            if (Sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) continue;

            assert(Sent == Totalsize);
            break;
        }
    }

    // List or block LAN clients by their SessionID.
    const std::unordered_map<uint32_t, IN_ADDR> &getPeers()
    {
        return Peerlist;
    }
    IN_ADDR getPeer(uint32_t SessionID)
    {
        if (!Peerlist.contains(SessionID)) [[unlikely]] return {};
        return Peerlist[SessionID];
    }
    void Blockpeer(uint32_t SessionID)
    {
        Blacklistedclients.insert(SessionID);
    }

    // Fetch as many messages as possible from the network.
    static void __cdecl Pollnetwork()
    {
        // There is no network available at this time.
        if (Global.Settings.noNetworking) [[unlikely]] return;

        constexpr timeval Defaulttimeout{ NULL, 1 };
        constexpr auto Buffersizelimit = 4096;
        auto Timeout{ Defaulttimeout };
        auto Count{ 1 };

        // Check for data on the active socket.
        FD_SET ReadFD{}; FD_SET(Receiversocket, &ReadFD);
        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;

        // Allocate data on the stack rather than heap as it's only 4Kb.
        const auto Buffer = alloca(Buffersizelimit);

        // Get all available packets.
        while (true)
        {
            sockaddr_in Clientaddress; int Addresslen = sizeof(Clientaddress);
            const auto Packetlength = recvfrom(Receiversocket, (char *)Buffer, Buffersizelimit, NULL, (sockaddr *)&Clientaddress, &Addresslen);
            if (Packetlength < 10) break;   // Also covers any errors.

            // Sanity-checking..
            const auto Packet = (LANpacket_t *)Buffer;
            if (Packet->SessionID == 0) [[unlikely]] continue;                          // Borked client.
            if (Packet->SessionID == SessionID) [[likely]] continue;                    // Our own packet.
            if (!Messagehandlers.contains(Packet->Identifier)) [[unlikely]] continue;   // Not a packet we can handle.
            if (Blacklistedclients.contains(Packet->SessionID)) [[unlikely]] continue;  // Our client does not like them.
            if (Packet->Payloadlength > (Buffersizelimit - 10)) [[unlikely]] continue;  // Packet is larger than our buffer.

            // Save the peers address incase someone needs it.
            Peerlist[Packet->SessionID] = Clientaddress.sin_addr;

            // Special case: Clientinfo needs to use the SessionID.
            if (Packet->Identifier == Hash::WW32("AYRIA::Clienthello"))
            {
                // Let the compiler decide how to vectorize this..
                std::ranges::for_each(Messagehandlers[Packet->Identifier],
                    [&](const Callback_t &CB) { if (CB) [[likely]] CB(Packet->SessionID, Packet->Payload, Packet->Payloadlength); });
            }
            else
            {
                // Get the associated account ID.
                const auto AccountID = Services::Clientinfo::getAccountID(Packet->SessionID);

                // Let the compiler decide how to vectorize this..
                std::ranges::for_each(Messagehandlers[Packet->Identifier],
                    [&](const Callback_t &CB) { if (CB) [[likely]] CB(AccountID, Packet->Payload, Packet->Payloadlength); });
            }
        }
    }

    // Set up the network.
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
        Error |= ioctlsocket(Sendersocket, FIONBIO, &Argument);
        Error |= ioctlsocket(Receiversocket, FIONBIO, &Argument);

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for devs).
        // TODO(tcn): Investigate if WS2's IP_ADD_MEMBERSHIP (12) gets mapped to the WS1's original (5) internally.
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            Global.Settings.noNetworking = true;
            closesocket(Receiversocket);
            closesocket(Sendersocket);
            assert(false);
            return;
        }

        // Make a unique identifier for later.
        SessionID = Hash::WW32(GetTickCount64() ^ Sendersocket);

        // Should only need to check for packets 10 times a second.
        Backend::Enqueuetask(100, Pollnetwork);
        Global.Settings.noNetworking = false;
    }

    // Helper for the plugins to send and recive.
    namespace API
    {
        extern "C" EXPORT_ATTR void __cdecl registerLANCallback(const char *Messageidentifier, void(__cdecl *Callback)(unsigned int AccountID, const char *Message, unsigned int Length))
        {
            assert(Messageidentifier);
            Register(std::string_view(Messageidentifier), Callback);
        }
        extern "C" EXPORT_ATTR void __cdecl publishLANPacket(const char *Messageidentifier, const char *JSONPayload)
        {
            assert(Messageidentifier);
            const std::string_view Payload = JSONPayload ? JSONPayload : std::string_view();
            Publish(std::string_view(Messageidentifier), Payload);
        }
    }
}

namespace Network::WAN
{
    // Set up the network.
    void Initialize() {}
}
