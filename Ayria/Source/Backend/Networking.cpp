/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend::Network
{
    // Just for clearer code..
    using Message_t = struct { uint32_t RandomID, Messagetype; char Payload[1]; };

    // Totally randomly selected constants..
    constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
    constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985

    static sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static Hashmap<uint32_t, Hashset<Callback_t>> Callbacks{};
    static size_t Sendersocket, Receiversocket;
    static Hashset<uint32_t> Blacklist{};
    static uint32_t RandomID;

    // Callbacks for the syncing multi-casts.
    void Sendmessage(const char *Identifier, std::string_view JSONString)
    {
        assert(Identifier);
        std::string Encoded;
        const auto Messagetype = Hash::WW32(Identifier);

        // Windows does not like partial messages, so prefix the buffer with ID and type.
        if (Base64::isValid(JSONString)) [[unlikely]]
        {
            const auto Size = LZ4_compressBound(int(JSONString.size()));

            Encoded.reserve(Size + sizeof(uint64_t));
            Encoded.append((char *)&RandomID, sizeof(RandomID));
            Encoded.append((char *)&Messagetype, sizeof(Messagetype));
            Encoded.resize(Size + sizeof(uint64_t));

            LZ4_compress_default(JSONString.data(), Encoded.data() + sizeof(uint64_t), int(JSONString.size()), Size);
        }
        else
        {
            const auto String = Base64::Encode(JSONString);
            const auto Size = LZ4_compressBound(int(String.size()));

            Encoded.reserve(Size + sizeof(uint64_t));
            Encoded.append((char *)&RandomID, sizeof(RandomID));
            Encoded.append((char *)&Messagetype, sizeof(Messagetype));
            Encoded.resize(Size + sizeof(uint64_t));

            LZ4_compress_default(String.data(), Encoded.data() + sizeof(uint64_t), int(String.size()), Size);
        }

        sendto(Sendersocket, Encoded.data(), int(Encoded.size()), 0, (sockaddr *)&Multicast, sizeof(Multicast));
    }
    void addHandler(const char *Identifier, Callback_t Callback)
    {
        assert(Identifier); assert(Callback);
        Callbacks[Hash::WW32(Identifier)].insert(Callback);
    }

    // Export functionality to the plugins.
    namespace Export
    {
        extern "C" EXPORT_ATTR void __cdecl addMessagehandler(const char *Messagetype, const void *Callback)
        {
            if (!Messagetype || !Callback) return;
            Network::addHandler(Messagetype, Callback_t(Callback));
        }
        static std::string __cdecl Sendmessage(const char *JSONString)
        {
            const auto Request = JSON::Parse(JSONString);
            const auto Message = Request.value("Message", std::string());
            const auto Identifier = Request.value("Identifier", std::string());

            Network::Sendmessage(Identifier.c_str(), Message);
            return ""s;
        }
    }

    // Poll the internal socket.
    void Updatenetworking()
    {
        constexpr timeval Defaulttimeout{ NULL, 10000 };
        constexpr auto Buffersizelimit = 4096;
        auto Timeout{ Defaulttimeout };
        constexpr auto Count{ 1 };

        // Check for data on the active socket.
        FD_SET ReadFD{}; FD_SET(Receiversocket, &ReadFD);
        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;

        // Stack allocate as we have data.
        const auto Buffer = alloca(Buffersizelimit);

        // Poll for all packets.
        while (true)
        {
            sockaddr_in Sender{ AF_INET };
            int Size{ sizeof(Sender) };

            // MSVC has some issue that casts the int to unsigned without warning; even with /Wall.
            const auto Packetlength = recvfrom(Receiversocket, (char *)Buffer, Buffersizelimit, NULL, (sockaddr *)&Sender, &Size);
            if (Packetlength < static_cast<int>(sizeof(uint64_t))) break;

            // For clearer code, should be optimized away.
            const auto Packet = (Message_t *)Buffer;

            // To ensure that we don't process our own packets.
            if (Packet->RandomID == RandomID) [[likely]] continue;

            // Blocked clients shouldn't be processed.
            if (Blacklist.contains(Packet->RandomID)) [[unlikely]] continue;

            // Do we even care for this message?
            if (const auto Result = Callbacks.find(Packet->Messagetype); Result != Callbacks.end()) [[likely]]
            {
                // All messages should be LZ4 compressed.
                const auto Decodebuffer = std::make_unique<char[]>(Packetlength * 3);
                const auto Decodesize = LZ4_decompress_safe(Packet->Payload, Decodebuffer.get(),
                                        static_cast<int>(Packetlength - sizeof(uint64_t)), Packetlength * 3);
                if (Decodesize <= 0) [[unlikely]] continue;

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

        // Export the network to the plugins.
        API::addHandler("Network::Sendmessage", Export::Sendmessage);

        Enqueuetask(0, Updatenetworking);
    }
    void Blockclient(uint32_t NodeID)
    {
        Blacklist.insert(NodeID);
    }
}
