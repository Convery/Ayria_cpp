/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include "../Global.hpp"

constexpr uint16_t Packetmagic = Hash::FNV1_32("Ayria") >> 16;      // 41700
constexpr uint16_t Broadcastport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
const sockaddr_in Broadcast{ AF_INET, htons(Broadcastport), {{.S_addr = INADDR_BROADCAST}} };

// Only tracks the info needed to keep a connection.
using Node_t = struct { size_t Socket; bool Connected; sockaddr_in Address; std::queue<std::string> Messagequeue; };

// Base message format, other modules handle the content.
inline std::string Encode(std::string_view Subject, std::string_view Content)
{
    auto Object = nlohmann::json::object();
    Object["Subject"] = Subject;
    Object["Content"] = Content;
    return Base64::Encode(Object.dump());
}
inline std::pair<std::string, std::string> Decode(std::string_view Input)
{
    if (!Base64::isValid(Input)) return {};
    const auto String = Base64::Decode(Input);

    try
    {
        const auto Object = nlohmann::json::parse(String.c_str());
        return std::make_pair(
            Object.value("Subject", std::string()),
            Object.value("Content", std::string())
        );
    } catch(...){}

    return {};
}

using Header_t = struct { uint16_t Magic, Random; };

namespace Network
{
    std::unordered_map<std::string, std::function<void(const char *)>> *Handlers;
    std::queue<std::string> Outgoingmessages{};
    std::unordered_set<uint16_t> Greeted{};
    std::vector<std::string> Greetings{};
    size_t Broadcastsocket{};
    uint16_t Selfrandom{};

    // Internal exports.
    void onFrame()
    {
        assert(Handlers);
        assert(Selfrandom);

        sockaddr_in Sender{ AF_INET };
        int Size{ sizeof(Sender) };
        char Buffer[4096];

        // Process incoming messages.
        while (true)
        {
            const auto Messagesize = recvfrom(Broadcastsocket, Buffer, 4095, 0, (sockaddr *)&Sender, &Size);

            if (Messagesize < int(sizeof(Header_t))) break;
            Buffer[Messagesize] = '\0';

            // Random packets and our own are discarded.
            const auto Packetheader = (Header_t *)Buffer;
            if (Packetheader->Magic != Packetmagic) continue;
            if (Packetheader->Random == Selfrandom) continue;

            // If this was the first message, greet the client.
            if (!Greeted.contains(Packetheader->Random)) [[unlikely]]
            {
                Greeted.insert(Packetheader->Random);
                for (const auto &Item : Greetings)
                {
                    // Drop the packets on error, no greeting for them.
                    sendto(Broadcastsocket, Item.data(), Item.size(), 0, (sockaddr *)&Sender, Size);
                }
            }

            // Verify, silently discard.
            const auto [Subject, Content] = Decode(Buffer + sizeof(Header_t));
            const auto Callback = Handlers->find(Subject);
            if (Callback != Handlers->end()) [[likely]]
            {
                Callback->second(Content.c_str());
            }
        }

        // Process outgoing messages.
        while (true)
        {
            if (Outgoingmessages.empty()) [[likely]] break;

            // Increase recv-buffer later if needed.
            const auto &Message = Outgoingmessages.front();
            assert(Message.size() < 4096);

            // Broadcast as much as possible, stop on error.
            if (int(Message.size()) != sendto(Broadcastsocket, Message.data(), Message.size(), 0,
                                             (sockaddr *)&Broadcast, sizeof(Broadcast))) break;

            Outgoingmessages.pop();
        }
    }
    void onStartup()
    {
        WSADATA wsaData;
        unsigned long Argument{ 1 };
        (void)WSAStartup(MAKEWORD(2, 2), &wsaData);

        do
        {
            // Raw sockets requires admin privileges on Windows, so UDP it is.
            Broadcastsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (Broadcastsocket == INVALID_SOCKET) break;

            // Enable binding to the same address, broadcasting, and async operations.
            if (setsockopt(Broadcastsocket, SOL_SOCKET, SO_BROADCAST, (char *)&Argument, sizeof(Argument)) ||
                setsockopt(Broadcastsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument)) ||
                ioctlsocket(Broadcastsocket, FIONBIO, &Argument) == SOCKET_ERROR)
                break;

            // Windows does not like binding to INADDR_BROADCAST..
            const sockaddr_in lBroadcast{ AF_INET, htons(Broadcastport) };
            if (SOCKET_ERROR == bind(Broadcastsocket, (const sockaddr *)&lBroadcast, sizeof(lBroadcast))) break;

            // We only have 16 bits of randomness, so can't do much better than this.
            Selfrandom = std::chrono::high_resolution_clock::now().time_since_epoch().count() & 0xFFFF;

            // Announce that we are alive.
            addBroadcast("", "");

            return;
        }
        while (false);

        Errorprint(va("Initialization of Ayria::Networking failed with %d", WSAGetLastError()));
    }

    // Maybe exposed to other modules.
    void addHandler(std::string_view Subject, std::function<void(const char *)> Callback)
    {
        if (!Handlers) Handlers = new std::remove_pointer_t<decltype(Handlers)>;
        Handlers->emplace(Subject, Callback);
    }
    void addBroadcast(std::string_view Subject, std::string_view Content)
    {
        assert(Selfrandom);

        auto Message = Encode(Subject, Content);
        Header_t Packetheader{ Packetmagic, Selfrandom };
        Message.insert(0, (const char *)&Packetheader, sizeof(Packetheader));

        Outgoingmessages.push(Message);
    }
    void addGreeting(std::string_view Subject, std::string_view Content)
    {
        assert(Selfrandom);

        auto Message = Encode(Subject, Content);
        Header_t Packetheader{ Packetmagic, Selfrandom };
        Message.insert(0, (const char *)&Packetheader, sizeof(Packetheader));

        Greetings.push_back(Message);
    }
}

namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl addNetworklistener(const char *Subject, void *Callback)
        {
            Network::addHandler(Subject, static_cast<void(__cdecl *)(const char *)>(Callback));
        }
        EXPORT_ATTR void __cdecl addNetworkbroadcast(const char *Subject, const char *Message)
        {
            Network::addBroadcast(Subject, Message);
        }
    }
}
