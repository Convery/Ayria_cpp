/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    using Multicast_t = struct { size_t Sendersocket, Receiversocket; sockaddr_in Address; };

    static std::unordered_map<uint32_t, std::unordered_set<Messagecallback_t>> Callbacks;
    static std::unordered_map<uint16_t, Multicast_t> Networkgroups;
    static std::unordered_set<uint32_t> Blacklist;
    static FD_SET Activesockets;
    static uint32_t RandomID;

    void Sendmessage(uint32_t Messagetype, std::string_view JSONString, uint16_t Port)
    {
        std::string Encoded;

        // Windows does not like partial messages, so prefix the buffer with ID and type.
        if (Base64::isValid(JSONString)) [[unlikely]]
        {
            const auto Size = LZ4_compressBound(JSONString.size());

            Encoded.reserve(Size + sizeof(uint64_t));
            Encoded.append((char *)&RandomID, sizeof(RandomID));
            Encoded.append((char *)&Messagetype, sizeof(Messagetype));
            Encoded.resize(Size + sizeof(uint64_t));

            LZ4_compress_default(JSONString.data(), Encoded.data() + sizeof(uint64_t), JSONString.size(), Size);
        }
        else
        {
            const auto String = Base64::Encode(JSONString);
            const auto Size = LZ4_compressBound(String.size());

            Encoded.reserve(Size + sizeof(uint64_t));
            Encoded.append((char *)&RandomID, sizeof(RandomID));
            Encoded.append((char *)&Messagetype, sizeof(Messagetype));
            Encoded.resize(Size + sizeof(uint64_t));

            LZ4_compress_default(String.data(), Encoded.data() + sizeof(uint64_t), String.size(), Size);
        }

        // Non-blocking send.
        const auto &[Sendersocket, _, Multicast] = Networkgroups[Port];
        sendto(Sendersocket, Encoded.data(), (int)Encoded.size(), 0, (sockaddr *)&Multicast, sizeof(Multicast));
    }
    void Registermessagehandler(uint32_t MessageID, Messagecallback_t Callback)
    {
        assert(Callback);
        Callbacks[MessageID].insert(Callback);
    }
    void Joinmessagegroup(uint16_t Port, uint32_t Address)
    {
        WSADATA WSAData;
        uint32_t Error{ 0 };
        constexpr uint32_t Argument{ 1 };
        sockaddr_in Localhost{ AF_INET, htons(Port), {{.S_addr = htonl(INADDR_ANY)}} };
        const sockaddr_in Multicast{ AF_INET, htons(Port), {{.S_addr = htonl(Address)}} };

        // We only need WS 1.1, no need for more.
        (void)WSAStartup(MAKEWORD(1, 1), &WSAData);
        const auto Sendersocket{ socket(AF_INET, SOCK_DGRAM, 0) };
        const auto Receiversocket{ socket(AF_INET, SOCK_DGRAM, 0) };
        Error |= ioctlsocket(Sendersocket, FIONBIO, (u_long *)&Argument);
        Error |= ioctlsocket(Receiversocket, FIONBIO, (u_long *)&Argument);

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for devs).
        const auto Request = ip_mreq{ {{.S_addr = htonl(Address)}}, {{.S_addr = htonl(INADDR_ANY)}} };
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // Make a unique identifier for later.
        if (!RandomID) RandomID = Hash::WW32(GetTickCount64() ^ Sendersocket);
        Networkgroups[Port] = { Sendersocket, Receiversocket, Multicast };
        FD_SET(Receiversocket, &Activesockets);

        // TODO(tcn): Error checking.
        if (Error) [[unlikely]] { assert(false); }
    }
    void Blockclient(uint32_t NodeID)
    {
        Blacklist.insert(NodeID);
    }

    // Just for clearer codes..
    using Message_t = struct { uint32_t RandomID, Messagetype; char Payload[1]; };

    // Poll the internal socket(s).
    void __cdecl Updatenetworking()
    {
        constexpr timeval Defaulttimeout{ NULL, 10000 };
        const auto Count{ Activesockets.fd_count + 1 };
        constexpr auto Buffersizelimit = 4096;
        FD_SET ReadFD{ Activesockets };
        auto Timeout{ Defaulttimeout };

        // Check for data on the active sockets.
        if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;

        // Stack allocate as we have data.
        const auto Buffer = alloca(Buffersizelimit);

        // Poll each available socket.
        for (const auto &[_, Group] : Networkgroups)
        {
            sockaddr_in Sender{ AF_INET };
            int Size{ sizeof(Sender) };

            // MSVC has some issue that casts the int to unsigned without warning; even with /Wall.
            const auto Packetlength = recvfrom(Group.Receiversocket, (char *)Buffer, Buffersizelimit, NULL, (sockaddr *)&Sender, &Size);
            if (Packetlength < static_cast<int>(sizeof(uint64_t))) continue;

            // Clearer codes, should be optimized away.
            const auto Packet = (Message_t *)Buffer;

            // To ensure that we don't process our own packets.
            if (Packet->RandomID == RandomID) [[likely]] continue;

            // Blocked clients shouldn't be processed.
            if (Blacklist.contains(Packet->RandomID)) [[unlikely]] continue;

            // Do we even care for this message?
            if (const auto Result = Callbacks.find(Packet->Messagetype); Result != Callbacks.end()) [[likely]]
            {
                // All messages should be LZ4 compressed.
                const auto Decodebuffer = std::make_shared<char[]>(Packetlength * 3);
                const auto Decodesize = LZ4_decompress_safe(Packet->Payload, Decodebuffer.get(), Packetlength - sizeof(uint64_t), Packetlength * 3);
                if (Decodesize < 0) [[unlikely]] continue;

                // All messages should be base64, inplace decode zero-terminates.
                const auto Decoded = Base64::Decode_inplace(Decodebuffer.get(), Decodesize);

                // May have multiple listeners for the same messageID.
                for (const auto Callback : Result->second)
                {
                    Callback(Packet->RandomID, Decoded.data());
                }
            }
        }
    }

    // Let's expose this interface to the world.
    namespace API
    {
        // using Messagecallback_t = void(__cdecl *)(uint32_t NodeID, const char *JSONString);
        extern "C" EXPORT_ATTR void __cdecl addNetworklistener(uint32_t MessageID, const void *Callback)
        {
            Registermessagehandler(MessageID, (Messagecallback_t)Callback);
        }
    }
}
