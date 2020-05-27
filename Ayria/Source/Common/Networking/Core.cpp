/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-25
    License: MIT
*/

#include "../../Global.hpp"

enum Packetflags_t : uint8_t
{
    Ephemeral = 1,  // Do not track this packet.
    Encrypted = 2,  // RSA encrypted.
    Compressed = 4, // ZSTD compressed (-15)
    RESERVED = 8    // TBD
};
struct Packetheader_t
{
    union
    {
        uint64_t Raw;
        uint64_t
            Magic : 16,         // Static.
            Packetflags : 4,    // 16 states.
            Packetlength : 12,  // Max 4095 bytes.
            Clientrandom : 32;  // Unique on startup.
    };
};

namespace Networking::Core
{
    // Totally randomly selected constants..
    constexpr uint16_t Multicastport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
    constexpr uint32_t Multicastaddress = Hash::FNV1_32("Ayria") << 8;  // 228.58.137.0
    constexpr uint16_t Packetmagic = Hash::FNV1_32("Ayria") >> 16;      // 41700 (LE) | 58530 (BE)

    const sockaddr_in Multicast{ AF_INET, htons(Multicastport), {{.S_addr = htonl(Multicastaddress)}} };
    sockaddr_in Localhost{ AF_INET, htons(Multicastport), {{.S_addr = htonl(INADDR_ANY)}} };
    std::unordered_map<uint32_t, std::function<void(sockaddr_in, const char *)>> Messagehandlers{};
    std::queue<std::string> Outgoingmessages;
    size_t Sendersocket, Receiversocket;

    // Messages are in the format of [UINT32 Type][B64 Data]
    void onRecv(sockaddr_in Client, std::string_view Input)
    {
        // Incase of getsockname failing. TODO(tcn): Refactor this?
        if(Localhost.sin_addr.s_addr == 0) [[unlikely]]
        {
            if(Client.sin_port == Localhost.sin_port)
            {
                Localhost.sin_addr.s_addr = Client.sin_addr.s_addr;
            }
        }

        // We receive our own messages as well, because devs.
        if (FNV::Equal(Client, Localhost)) return;

        // The type of message is the first 32 bits, normally a hash.
        const auto Type = *(uint32_t *)Input.data();
        Input.remove_prefix(sizeof(Type));

        // Maybe this message was for a plugin that isn't enabled.
        const auto Callback = Messagehandlers.find(Type);
        if (Callback == Messagehandlers.end()) [[unlikely]] return;

        // We only send messages in base64, so we can use it as integrity verification.
        if (!Base64::isValid(Input)) [[unlikely]] return;

        // Callbacks take a char * to allow C-APIs.
        Callback->second(Client, Base64::Decode(Input).c_str());
    }
    void Sendmessage(uint32_t Type, std::string_view Input)
    {
        // Windows does not like partial messages, so prefex the buffer.
        auto Message = Base64::Encode(Input);
        Message.insert(0, (char *)&Type, sizeof(Type));

        // Enqueue for later.
        Outgoingmessages.push(Message);
    }
    void Sendmessage(std::string_view Type, std::string_view Input)
    {
        return Sendmessage(Hash::FNV1_32(Type), Input);
    }

    // Internal events.
    void onStartup()
    {
        WSADATA wsaData;
        int32_t Error{ 0 };
        uint32_t Argument{ 1 };

        // We only need WS 1.1, no need for more.
        Error |= WSAStartup(MAKEWORD(1, 1), &wsaData);
        Sendersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Receiversocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Sendersocket, FIONBIO, (u_long *)&Argument);
        Error |= ioctlsocket(Receiversocket, FIONBIO, (u_long *)&Argument);

        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for devs).
        const auto Request = ip_mreq{ {{.S_addr = htonl(Multicastaddress)}}, {{.S_addr = htonl(INADDR_ANY)}} };
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // Bind the sender to a random port and save it for filtering.
        Localhost.sin_port = 0; int Addresslength{ sizeof(Localhost) };
        Error |= bind(Sendersocket, (sockaddr *)&Localhost, sizeof(Localhost));
        Error |= getsockname(Sendersocket, (sockaddr *)&Localhost, &Addresslength);

        // Notify other clients about startup.
        Sendmessage(0, "");

        // TODO: =(
        if (Error) [[unlikely]]
        {

        }
    }
    void onFrame()
    {
        constexpr auto Buffersize = 4096;
        char Buffer[Buffersize];
        int32_t Timeslice = 8;

        // Outgoing data.
        while (Timeslice--)
        {
            if (Outgoingmessages.empty()) [[likely]] break;
            const auto Message = Outgoingmessages.front();

            if (sendto(Sendersocket, Message.data(), Message.size(), 0, (sockaddr *)&Multicast, sizeof(Multicast)))
            {
                Outgoingmessages.pop();
            }
        }

        // Incoming data.
        while (Timeslice--)
        {
            sockaddr_in Sender{ AF_INET };
            int Size{ sizeof(Sender) };

            // MSVC has some issue that casts the int to unsigned without warning.
            const auto Packetlength = recvfrom(Receiversocket, Buffer, Buffersize, 0, (sockaddr *)&Sender, &Size);
            if (Packetlength < static_cast<int>(sizeof(uint32_t))) break;
            onRecv(Sender, std::string_view(Buffer, Packetlength));
        }
    }

    // External events.
    void Registerhandler(uint32_t Type, std::function<void(sockaddr_in, const char *)> Callback)
    {
        Messagehandlers[Type] = Callback;
    }
    void Registerhandler(std::string_view Type, std::function<void(sockaddr_in, const char *)> Callback)
    {
        Messagehandlers[Hash::FNV1_32(Type)] = Callback;
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl addNetworklistener(const char *Subject, void *Callback)
        {
            Networking::Core::Registerhandler(Subject, [=](sockaddr_in, const char *A)
            {
                static_cast<void(__cdecl *)(const char *)>(Callback)(A);
            });
        }
        EXPORT_ATTR void __cdecl addNetworkbroadcast(const char *Subject, const char *Message)
        {
            Networking::Core::Sendmessage(Subject, Message);
        }
    }
}