/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend::Network
{
    // Totally randomly selected constants..
    constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
    constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985

    static sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static Hashmap<uint32_t, Hashset<Callback_t>> Messagehandlers{};
    static Hashset<uint32_t> Blacklistedclients{};
    static size_t Sendersocket, Receiversocket;
    static uint32_t RandomID{};

    // static void __cdecl Callback(unsigned int NodeID, const char *Message, unsigned int Length);
    // using Callback_t = void(__cdecl *)(unsigned int NodeID, const char *Message, unsigned int Length);
    void Registerhandler(std::string_view Identifier, Callback_t Handler)
    {
        const auto ID = Hash::WW32(Identifier);
        Messagehandlers[ID].insert(Handler);
    }

    // Formats and LZ compresses the message.
    void Transmitmessage(std::string_view Identifier, JSON::Value_t &&Message)
    {
        const auto Messagetype = Hash::WW32(Identifier);
        const auto Encoded = Base64::Encode(JSON::Dump(Message));
        const int Headersize = sizeof(Messagetype) + sizeof(RandomID);
        const auto Messagesize = LZ4_compressBound(int(Encoded.size()));
        auto Packet = std::make_unique<char[]>(Messagesize + Headersize);

        std::memcpy(Packet.get() + 0, &RandomID, sizeof(RandomID));
        std::memcpy(Packet.get() + sizeof(RandomID), &Messagetype, sizeof(Messagetype));
        LZ4_compress_default(Encoded.data(), Packet.get() + Headersize, (int)Encoded.size(), Messagesize + Headersize);

        sendto(Sendersocket, Packet.get(), int(Messagesize + Headersize), 0, (sockaddr *)&Multicast, sizeof(Multicast));
    }

    // Decodes and calls the handler for a packet.
    static void Pollnetwork()
    {
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
            const auto Packetlength = recvfrom(Receiversocket, (char *)Buffer, Buffersizelimit, NULL, NULL, NULL);
            if (Packetlength < 8) break;

            // For slightly cleaner code, should be optimized out.
            using Message_t = struct { uint32_t RandomID, Messagetype; char Payload[1]; };
            const auto Packet = (Message_t *)Buffer;

            // To ensure that we don't process our own packets.
            if (Packet->RandomID == RandomID) [[likely]] continue;

            // Blocked clients shouldn't be processed.
            if (Blacklistedclients.contains(Packet->RandomID)) [[unlikely]] continue;

            // Do we even care for this message?
            if (const auto Result = Messagehandlers.find(Packet->Messagetype); Result != Messagehandlers.end()) [[likely]]
            {
                // All messages should be LZ4 compressed.
                const auto Decodebuffer = std::make_unique<char[]>(Packetlength * 3);
                const auto Decodesize = LZ4_decompress_safe(Packet->Payload, Decodebuffer.get(),
                                                            static_cast<int>(Packetlength - 8), Packetlength * 3);

                // Possibly corrupted packet.
                if (Decodesize <= 0) [[unlikely]]
                {
                    Debugprint(va("Packet from ID %08X failed to decode.", Packet->RandomID));
                    continue;
                }

                    // All messages should be base64, in-place decode zero-terminates.
                    const auto Decoded = Base64::Decode_inplace(Decodebuffer.get(), Decodesize);

                    // May have multiple listeners for the same messageID.
                    for (const auto Callback : Result->second)
                    {
                        Callback(Packet->RandomID, Decoded.data(), (unsigned int)Decoded.size());
                    }
            }
        }
    }

    // Prevent packets from being processed.
    void Blockclient(uint32_t NodeID)
    {
        Blacklistedclients.insert(NodeID);
    }

    // Exported functionallity.
    namespace API
    {
        extern "C" EXPORT_ATTR void __cdecl addHandler(const char *Messagetype,
                               void(__cdecl *Callback)(unsigned int NodeID, const char *Message, unsigned int Length))
        {
            Registerhandler(Messagetype, Callback);
        }
        static std::string __cdecl Sendmessage(JSON::Value_t &&Request)
        {
            const auto Messagetype = Request.value<std::string>("Messagetype");
            const auto Message = Request.value<std::string>("Message");
            Transmitmessage(Messagetype, Message);

            return "{}";
        }
    }

    // Initialize the system.
    void Initialize()
    {
        WSADATA WSAData;
        unsigned long Error{ 0 };
        unsigned long Argument{ 1 };
        sockaddr_in Localhost{ AF_INET, htons(Syncport), {{.S_addr = htonl(INADDR_ANY)}} };

        // We only need WS 1.1, no need for more.
        Error |= WSAStartup(MAKEWORD(1, 1), &WSAData);
        Sendersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Receiversocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Sendersocket, FIONBIO, &Argument);
        Error |= ioctlsocket(Receiversocket, FIONBIO, &Argument);

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for devs).
        const auto Request = ip_mreq{ {{.S_addr = htonl(Syncaddress)}}, {{.S_addr = htonl(INADDR_ANY)}} };
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // Make a unique identifier for later.
        RandomID = Hash::WW32(GetTickCount64() ^ Sendersocket);

        // TODO(tcn): Error checking.
        if (Error) [[unlikely]] { assert(false); }

        // Should only need to check for packets 10 times a second.
        Enqueuetask(100, Pollnetwork);

        // JSON API.
        Backend::API::addEndpoint("Network::Sendmessage", API::Sendmessage, R"({ "Messagetype" : "Func", "Message" : "String" })");
    }
}
