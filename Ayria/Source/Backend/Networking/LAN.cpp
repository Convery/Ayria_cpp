/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-30
    License: MIT
*/

#include <Global.hpp>
#include <openssl/curve25519.h>

// NOTE(tcn): While Hyrum's law is generally not respected, port should be updated for breaking changes.
constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985
constexpr auto Buffersizelimit = 8192;                          // Two virtual pages (if the compiler is smart enough).

namespace Networking::LAN
{
    struct Packet_t { uint32_t UniqueID; Communication::Packet_t Packet; };

    static const sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static size_t Sendersocket{}, Receiversocket{};
    static uint32_t RandomID{};

    // Publish a message to the local multicast group.
    bool Publish(std::unique_ptr<char []> &&Buffer, size_t Buffersize, size_t Overhead)
    {
        assert(Overhead >= sizeof(uint32_t));

        // The buffer should have been created with enough overhead so we can insert our ID..
        std::memmove(Buffer.get() + sizeof(uint32_t), Buffer.get(), Buffersize - Overhead);
        *reinterpret_cast<uint32_t*>(Buffer.get()) = RandomID;

        // Put the packet on the network.
        const auto Result = sendto(Sendersocket, Buffer.get(), (int)(Buffersize + sizeof(uint32_t)), NULL, (sockaddr *)&Multicast, sizeof(Multicast));
        return Result == static_cast<int>(Buffersize + sizeof(uint32_t));
    }

    // Fetch any and all new packets.
    void __cdecl Pollnetwork()
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

        // Allocate packet-data on the stack rather than heap as it's 'only' 8Kb.
        const auto Buffer = (char *)alloca(Buffersizelimit);

        // Get all available packets.
        while (true)
        {
            sockaddr_in Clientaddress{}; int Addresslen = sizeof(Clientaddress);
            const auto Packetlength = recvfrom(Receiversocket, Buffer, Buffersizelimit, NULL, (sockaddr *)&Clientaddress, &Addresslen);
            if (Packetlength < static_cast<int>(sizeof(Packet_t))) break;  // Also covers any errors.
            const auto Packet = (Packet_t *)Buffer;

            // Depending on the router, a multicast can act as a broadcast.
            if (Packet->UniqueID == RandomID) [[likely]]
            {
                // Save our internal IP for later.
                if (NULL == Global.InternalIP) [[unlikely]]
                {
                    Global.InternalIP = Clientaddress.sin_addr.S_un.S_addr;
                }
                continue;
            }

            const auto Payload = std::string_view(Buffer + sizeof(Packet_t), Packetlength - sizeof(Packet_t));
            Communication::savePacket(&Packet->Packet, Payload, Communication::Source_t::LAN);
        }
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
        (void)WSAStartup(MAKEWORD(1, 1), &Unused);
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
            Global.Settings.noNetworking = true;
            closesocket(Receiversocket);
            closesocket(Sendersocket);
            assert(false);
            return;
        }

        // Make a unique identifier for our session.
        RandomID = Hash::WW32(GetTickCount64() ^ Sendersocket);

        // Should only need to check for packets 4 times a second.
        Backend::Enqueuetask(250, Pollnetwork);
        Global.Settings.noNetworking = false;
    }
}
