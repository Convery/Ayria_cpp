/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-14
    License: MIT
*/

#include <Stdinclude.hpp>

namespace Networking
{
    using Node_t = struct { size_t Socket{}; std::queue<std::string> Messagequeue{}; };
    using NodeID_t = uint32_t;
    NodeID_t Nodecount = 0; // TODO(tcn): Move to Global.hpp

    std::unordered_map<NodeID_t, Node_t> Nodes;
    std::vector<NodeID_t> getNodes()
    {
        std::vector<NodeID_t> Result(Nodes.size());
        for (const auto &[ID, _] : Nodes) Result.push_back(ID);
        return Result;
    }




    constexpr uint16_t Broadcastport = Hash::FNV1_32("Ayria") & 0xFFFF;
    size_t Listensocket, Broadcastsocket;
    FD_SET Activesockets{};
    bool Initialized{};

    bool Initialize()
    {
        SOCKADDR_IN Broadcast{ AF_INET, htons(Broadcastport), {{.S_addr = INADDR_BROADCAST}} };
        SOCKADDR_IN Localhost{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
        unsigned long Argument{ 1 };

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
            if (SOCKET_ERROR == bind(Broadcastsocket, (SOCKADDR *)&Broadcast, sizeof(Broadcast))) break;
            if (SOCKET_ERROR == bind(Listensocket, (SOCKADDR *)&Localhost, sizeof(Localhost))) break;
            if (SOCKET_ERROR == listen(Listensocket, 32)) break;

            // MEEP

            Initialized = true;
            return true;
        }
        while (false);

        const auto Errorcode = WSAGetLastError();
        DebugBreak();
        return false;
    }

    void onFrame()
    {
        FD_SET ReadFD{ Activesockets }, WriteFD{ Activesockets };
        const auto Count{ Activesockets.fd_count };
        timeval Selecttimeout{ NULL, 10000 };

        // If sockets are ready for IO, no need to count, just do for_each.
        if (select(Count, &ReadFD, &WriteFD, NULL, &Selecttimeout))
        {
            // Basic socket IO.
            for (auto &[ID, Node] : Nodes)
            {
                if (Node.Messagequeue.empty()) continue;
                if (!FD_ISSET(Node.Socket, &WriteFD)) continue;

                // We do not care for partial sends, assume that everything was sent.
                const auto Status = send(Node.Socket, Node.Messagequeue.front().data(), Node.Messagequeue.front().size(), NULL);
                if (Status == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError()) continue;
                if (Status > 0) { Node.Messagequeue.pop(); continue; }

                // Fatal socket-error (or the connection being closed).
                FD_CLR(Node.Socket, &Activesockets);
                closesocket(Node.Socket);
                Nodes.erase(ID);
                break;
            }
            for (auto &[ID, Node] : Nodes)
            {
                if (!FD_ISSET(Node.Socket, &ReadFD)) continue;

                // We treat the message as Base64 encoded JSON, ignore invalid.
                const auto Messagesize = recv(Node.Socket, nullptr, NULL, MSG_PEEK);
                if (Messagesize == SOCKET_ERROR && WSAEWOULDBLOCK == WSAGetLastError()) continue;
                if (Messagesize > 0)
                {
                    const auto Buffer = std::make_unique<char[]>(Messagesize + 1);
                    recv(Node.Socket, Buffer.get(), Messagesize, NULL);

                    // Verify, silently discard.
                    if (!Base64::isValid(Buffer.get())) continue;
                    const auto JSONString = Base64::Decode(Buffer.get());

                    // Nlohmann may throw on bad data.
                    try
                    {
                        const auto Object = nlohmann::json::parse(JSONString.c_str());
                        const auto Subject = Object.value("Subject", std::string());
                        const auto Content = Object.value("Content", std::string());

                        // TODO(tcn): Forward to a handler of some sort.
                        std::string Response{};

                        if (!Response.empty())
                        {
                            Node.Messagequeue.push(std::move(Response));
                        }
                    }
                    catch (...) {};
                    continue;
                }

                // Fatal socket-error (or the connection being closed).
                FD_CLR(Node.Socket, &Activesockets);
                closesocket(Node.Socket);
                Nodes.erase(ID);
                break;
            }

            // If there's data on the listen-socket, it's a new connection.
            if (FD_ISSET(Listensocket, &ReadFD))
            {
                // Drop any errors on the floor.
                if (const auto Socket = accept(Listensocket, NULL, NULL); Socket != INVALID_SOCKET)
                {
                    Nodes[Nodecount++] = { .Socket = Socket };
                    FD_SET(Socket, &Activesockets);
                }
            }
        }
    }

    // Find peers.
    void onDiscovery()
    {




    }



}


