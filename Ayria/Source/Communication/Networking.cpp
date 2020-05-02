/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Global.hpp"

namespace Networking
{
    using Node_t = struct { size_t Socket{}; sockaddr_in Address{}; std::queue<std::string> Messagequeue{}; };
    constexpr uint32_t getID(const sockaddr_in &Address) { return Hash::FNV1a_32(&Address, sizeof(Address)); }
    constexpr uint32_t getID(const Node_t *Node) { return Hash::FNV1a_32(&Node->Address, sizeof(Node->Address)); }

    // Port in network-order (big endian), Magic in host-order.
    using Discoverypacket_t = struct { uint16_t Magic, TCPPort; };
    constexpr uint16_t Packetmagic = Hash::FNV1_32("Ayria") >> 16;      // 41700
    constexpr uint16_t Broadcastport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
    const sockaddr_in Broadcast{ AF_INET, htons(Broadcastport), {{.S_addr = INADDR_BROADCAST}} };

    std::unordered_map<std::string, Callback_t> *Handlers;
    std::unordered_map<uint32_t, Node_t> Nodes;
    size_t Listensocket, Broadcastsocket;
    sockaddr_in Clientaddress{};
    FD_SET Activesockets{};
    uint16_t Listenport{};
    bool Initialized{};

    // Find other nodes on the network.
    void doDiscovery()
    {
        assert(Initialized);

        // Discover ourselves so we can filter properly.
        if (Clientaddress.sin_family == 0)
        {
            const Discoverypacket_t Packet{ Packetmagic, Listenport };
            sendto(Broadcastsocket, (const char *)&Packet, sizeof(Discoverypacket_t), MSG_DONTROUTE, (sockaddr *)&Broadcast, sizeof(Broadcast));
        }

        // Non-blocking polling.
        Discoverypacket_t Packet{};
        sockaddr_in Sender{ AF_INET }; int Size{ sizeof(Sender) };
        while (sizeof(Discoverypacket_t) == recvfrom(Broadcastsocket,
            (char *)&Packet, sizeof(Discoverypacket_t), 0, (sockaddr *)&Sender, &Size))
        {
            // Random packets are discarded.
            if (Packet.Magic != Packetmagic) continue;

            // Claim the packet as our own discovery if we haven't.
            if (Clientaddress.sin_family == 0 && Packet.TCPPort == Listenport) Clientaddress = Sender;

            // Filter out our own broadcasts.
            if (FNV::Equal(Sender, Clientaddress)) continue;

            // Re-use the senders struct for TCP.
            Sender.sin_port = Packet.TCPPort;

            // Has this node connected to us already?
            if (Nodes.contains(getID(Sender))) continue;

            // Connect to the node.
            if (const auto Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); Socket != INVALID_SOCKET)
            {
                unsigned long Argument{ 1 };
                if (ioctlsocket(Socket, FIONBIO, &Argument) == SOCKET_ERROR) continue;
                if (NULL == connect(Socket, (const sockaddr *)&Sender, Size))
                {
                    Nodes[getID(Sender)] = { Socket, Sender };
                    FD_SET(Socket, &Activesockets);
                }
            }
        }
    }
    void doListen()
    {
        assert(Initialized);

        timeval Selecttimeout{ NULL, 100 };
        FD_SET ListenFD{}; FD_SET(Listensocket, &ListenFD);
        if (select(1, &ListenFD, nullptr, nullptr, &Selecttimeout))
        {
            // Drop any errors on the floor.
            sockaddr_in Sender{ AF_INET }; int Size{ sizeof(Sender) };
            if (const auto Socket = accept(Listensocket, (sockaddr *)&Sender, &Size); Socket != INVALID_SOCKET)
            {
                Nodes[getID(Sender)] = { Socket, Sender };
                FD_SET(Socket, &Activesockets);
            }
        }
    }
    void doPump()
    {
        assert(Initialized);

        FD_SET ReadFD{ Activesockets }, WriteFD{ Activesockets };
        const auto Count{ Activesockets.fd_count };
        timeval Selecttimeout{ NULL, 100 };

        // If sockets are ready for IO, no need to count, just do for_each.
        if (select(Count, &ReadFD, &WriteFD, nullptr, &Selecttimeout))
        {
            // Simply forward to the node, no processing needed.
            for (auto &[NodeID, Node] : Nodes)
            {
                if (Node.Messagequeue.empty()) continue;
                if (!FD_ISSET(Node.Socket, &WriteFD)) continue;

                // We do not process partial sends, so we assume that all data was sent.
                const auto Status = send(Node.Socket, Node.Messagequeue.front().data(), int(Node.Messagequeue.front().size()), NULL);
                if (Status == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError()) continue;
                if (Status > 0) { Node.Messagequeue.pop(); continue; }

                // Fatal socket-error (or the connection being closed).
                FD_CLR(Node.Socket, &Activesockets);
                closesocket(Node.Socket);
                Nodes.erase(NodeID);
                break;
            }

            // We assume that this is called in non-blocking manner.
            for (auto &[NodeID, Node] : Nodes)
            {
                if (!FD_ISSET(Node.Socket, &ReadFD)) continue;
                const auto Messagesize = recv(Node.Socket, nullptr, NULL, MSG_PEEK);
                if (Messagesize == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError()) continue;

                if (Messagesize > 0)
                {
                    const auto Buffer = std::make_unique<char[]>(Messagesize + 1);
                    recv(Node.Socket, Buffer.get(), Messagesize, NULL);
                    if (!Handlers) continue;

                    // Verify, silently discard.
                    if (!Base64::isValid(Buffer.get())) continue;
                    const auto JSONString = Base64::Decode(Buffer.get());

                    // Nlohmann may throw on bad data.
                    try
                    {
                        const auto Object = nlohmann::json::parse(JSONString.c_str());
                        const auto Subject = Object.value("Subject", std::string());
                        const auto Content = Object.value("Content", std::string());
                        const auto Callback = Handlers->find(Subject);

                        // We just drop packets without a handler.
                        if (Callback != Handlers->end())
                        {
                            Callback->second(Content.c_str());
                        }
                    }
                    catch (...) {};
                    continue;
                }

                // Fatal socket-error (or the connection being closed).
                FD_CLR(Node.Socket, &Activesockets);
                closesocket(Node.Socket);
                Nodes.erase(NodeID);
                break;
            }
        }
    }

    // Set up the networking.
    bool Initialize()
    {
        WSADATA wsaData;
        unsigned long Argument{ 1 };
        (void)WSAStartup(MAKEWORD(2, 2), &wsaData);

        do
        {
            // Raw sockets requires admin privileges on Windows, because reasons.
            Broadcastsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            Listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (Broadcastsocket == INVALID_SOCKET) break;
            if (Listensocket == INVALID_SOCKET) break;

            // Enable binding to the same address, broadcasting, and async operations.
            if (setsockopt(Broadcastsocket, SOL_SOCKET, SO_BROADCAST, (char *)&Argument, sizeof(Argument)) ||
                setsockopt(Broadcastsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument)) ||
                ioctlsocket(Broadcastsocket, FIONBIO, &Argument) == SOCKET_ERROR ||
                ioctlsocket(Listensocket, FIONBIO, &Argument) == SOCKET_ERROR)
                break;

            // Assigns a random port to the listen-socket while binding.
            const sockaddr_in lBroadcast{ AF_INET, htons(Broadcastport) };
            const sockaddr_in Localhost{ AF_INET, 0, {{.S_addr = INADDR_ANY}} };
            if (SOCKET_ERROR == bind(Broadcastsocket, (const sockaddr *)&lBroadcast, sizeof(lBroadcast))) break;
            if (SOCKET_ERROR == bind(Listensocket, (const sockaddr *)&Localhost, sizeof(Localhost))) break;
            if (SOCKET_ERROR == listen(Listensocket, 32)) break;

            // Find where we are actually listening.
            sockaddr_in Info{ AF_INET }; int Size{ sizeof(Info) };
            if (NULL != getsockname(Listensocket, (sockaddr *)&Info, &Size)) break;

            Listenport = Info.sin_port;
            Initialized = true;
            return true;
        }
        while (false);

        Errorprint(va("Initialization of Ayria::Networking failed with %d", WSAGetLastError()));
        assert(false);
        return false;
    }

    // Exported.
    void onFrame()
    {
        if (!Initialized)
            Initialize();
        doDiscovery();
        doListen();
        doPump();
    }
    void addHandler(std::string_view Subject, const Callback_t &Callback)
    {
        if (!Handlers) Handlers = new std::remove_pointer_t<decltype(Handlers)>;
        Handlers->emplace(Subject, Callback);
    }
    void addBroadcast(std::string_view Subject, std::string_view Content)
    {
        auto Object = nlohmann::json::object();
        Object["Subject"] = Subject;
        Object["Content"] = Content;

        const auto Output = Base64::Encode(Object.dump());
        for (auto &[_, Node] : Nodes) Node.Messagequeue.push(Output);
    }
    extern "C" EXPORT_ATTR unsigned short __cdecl getNetworkport() { return Listenport; }
    extern "C" EXPORT_ATTR void __cdecl addNetworklistener(const char *Subject, void *Callback)
    {
        addHandler(Subject, static_cast<void(__cdecl *)(const char *)>(Callback));
    }
    extern "C" EXPORT_ATTR void __cdecl addNetworkbroadcast(const char *Subject, const char *Message)
    {
        addBroadcast(Subject, Message);
    }
}
