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

    std::unordered_map<uint32_t, std::unordered_set<Messagecallback_t>> Callbacks;
    std::unordered_map<uint16_t, Multicast_t> Networkgroups;
    static uint32_t RandomID;

    void Registermessagehandler(uint32_t MessageID, Messagecallback_t Callback)
    {
        assert(Callback);
        Callbacks[MessageID].insert(Callback);
    }
    void Sendmessage(uint32_t Messagetype, std::string_view JSONString, uint16_t Port)
    {
        std::string Encoded;

        // Pre-allocate storage for the message rather than trusting STL.
        Encoded.reserve((((JSONString.size() + 2) / 3) * 4) + sizeof(uint64_t));

        // Windows does not like partial messages, so prefix the buffer with ID and type.
        Encoded.append((char *)&RandomID, sizeof(RandomID));
        Encoded.append((char *)&Messagetype, sizeof(Messagetype));
        Encoded.append(Base64::isValid(JSONString) ? JSONString : Base64::Encode(JSONString));

        // Non-blocking send.
        const auto &[Sendersocket, _, Multicast] = Networkgroups[Port];
        sendto(Sendersocket, Encoded.data(), (int)Encoded.size(), 0, (sockaddr *)&Multicast, sizeof(Multicast));
    }
    void Joinmessagegroup(uint16_t Port, uint32_t Address)
    {
        WSADATA WSAData;
        uint32_t Error{ 0 };
        constexpr uint32_t Argument{ 1 };
        size_t Sendersocket, Receiversocket;
        sockaddr_in Localhost{ AF_INET, htons(Port), {{.S_addr = htonl(INADDR_ANY)}} };
        const sockaddr_in Multicast{ AF_INET, htons(Port), {{.S_addr = htonl(Address)}} };

        // We only need WS 1.1, no need for more.
        (void)WSAStartup(MAKEWORD(1, 1), &WSAData);
        Sendersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Receiversocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Sendersocket, FIONBIO, (u_long *)&Argument);
        Error |= ioctlsocket(Receiversocket, FIONBIO, (u_long *)&Argument);

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for devs).
        const auto Request = ip_mreq{ {{.S_addr = htonl(Address)}}, {{.S_addr = htonl(INADDR_ANY)}} };
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // Make a unique identifier for later.
        if (!RandomID) RandomID = Hash::FNV1_32(GetTickCount64() ^ Sendersocket);
        Networkgroups[Port] = { Sendersocket, Receiversocket, Multicast };

        // TODO(tcn): Error checking.
        if (Error) [[unlikely]] { assert(false); }
    }

    // Just for clearer codes..
    using Message_t = struct { uint32_t Messagetype, RandomID; char Payload[1]; };

    // Poll the internal socket(s).
    void Updatenetworking()
    {
        constexpr auto Buffersize = 4096;
        char Buffer[Buffersize];

        for (const auto &[_, Group] : Networkgroups)
        {
            sockaddr_in Sender{ AF_INET };
            int Size{ sizeof(Sender) };

            // MSVC has some issue that casts the int to unsigned without warning; even with /Wall.
            const auto Packetlength = recvfrom(Group.Receiversocket, Buffer, Buffersize, 0, (sockaddr *)&Sender, &Size);
            if (Packetlength < static_cast<int>(sizeof(uint64_t))) continue;

            // Clearer codes, should be optimized away.
            const auto Packet = (Message_t *)Buffer;

            // To ensure that we don't process our own packets.
            if (Packet->RandomID == RandomID) [[likely]] continue;

            // Do we even care for this message?
            if (const auto Result = Callbacks.find(Packet->Messagetype); Result != Callbacks.end())
            {
                // All messages should be base64.
                const auto Decoded = Base64::Decode({ Packet->Payload, size_t(Packetlength - sizeof(uint64_t)) });

                // May have multiple listeners for the same messageID.
                for (const auto Callback : Result->second)
                {
                    Callback(Packet->RandomID, Decoded.c_str());
                }
            }
        }
    }

    // Let's expose this interface to the world.
    namespace API
    {
        // using Messagecallback_t = void(__cdecl *)(uint32_t NodeID, const char *JSONString);
        extern "C" EXPORT_ATTR void __cdecl addNetworklistener(uint32_t MessageID, void *Callback)
        {
            Registermessagehandler(MessageID, (Messagecallback_t)Callback);
        }
    }
}
