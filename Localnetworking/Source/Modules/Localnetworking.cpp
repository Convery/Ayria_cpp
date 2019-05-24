/*
    Initial author: Convery (tcn@ayria.se)
    Started: 12-04-2019
    License: MIT
*/

#include "Localnetworking.hpp"
#include "Stdinclude.hpp"
#include "../IServer.hpp"
#include <WinSock2.h>

namespace Localnetworking
{
    std::unordered_map<IServer *, std::vector<uint16_t>> Proxyports;
    std::unordered_map<std::string, IServer *> Serverinstances;

    std::unordered_map<IServer *, size_t> Datagramsockets;
    std::unordered_map<size_t, IServer *> Streamsockets;
    uint16_t BackendTCPport, BackendUDPport;
    std::vector<void *> Pluginslist;
    size_t Listensocket, UDPSocket;

    std::unordered_map<std::string, std::string> Resolvercache;

    // Poll on the local sockets.
    void Serverthread()
    {
        // ~100 FPS incase someone proxies game-data.
        timeval Timeout{ NULL, 10000 };
        char Buffer[8192];
        FD_SET ReadFD;

        while (true)
        {
        // Resent the descriptors.
            FD_ZERO(&ReadFD);

            // Only listen for new connections on this socket.
            FD_SET(Listensocket, &ReadFD);

            // All UDP packets come through the same socket.
            FD_SET(UDPSocket, &ReadFD);

            // While the TCP ones need their own socket, because reasons.
            for (const auto &[Socket, Server] : Streamsockets) FD_SET(Socket, &ReadFD);

            // NOTE(tcn): Not portable! POSIX select timeout is not const *.
            if (select(NULL, &ReadFD, NULL, NULL, &Timeout))
            {
                // If there's data on the listen-socket, it's a new connection.
                if (FD_ISSET(Listensocket, &ReadFD))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
                    if (auto Socket = accept(Listensocket, (SOCKADDR *)&Client, &Clientsize))
                    {
                        // Associate the new socket with a server-instance.
                        for (const auto &[Server, Portlist] : Proxyports)
                        {
                            for (const auto &Port : Portlist)
                            {
                                if (Port == ntohs(Client.sin_port))
                                {
                                    Streamsockets[Socket] = Server;
                                    Server->onConnect();
                                    break;
                                }
                            }
                        }
                    }
                }

                LABEL_LOOP: // If there's data on the stream-sockets then we need to forward it.
                for (const auto &[Socket, Server] : Streamsockets)
                {
                    if (!FD_ISSET(Socket, &ReadFD)) continue;
                    auto Size = recv(Socket, Buffer, 8192, 0);

                    // Connection closed.
                    if (Size <= 0)
                    {
                        // Winsock had some internal issue.
                        if (Size == -1) Infoprint(va("Error on socket 0x%X - %u", WSAGetLastError()));
                        Streamsockets.erase(Socket);
                        closesocket(Socket);
                        goto LABEL_LOOP;
                    }

                    // Normal data.
                    else Server->onStreamwrite(Buffer, Size);
                }

                // All datagram packets come through a single socket.
                if (FD_ISSET(UDPSocket, &ReadFD))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
                    while (recvfrom(UDPSocket, Buffer, 8196, MSG_PEEK, (SOCKADDR *)&Client, &Clientsize) > 0)
                    {
                        auto Size = recvfrom(UDPSocket, Buffer, 8196, 0, (SOCKADDR *)&Client, &Clientsize);

                        // Find the server from the sending-port.
                        for (const auto &[Server, Portlist] : Proxyports)
                        {
                            for (const auto &Port : Portlist)
                            {
                                if (Port == ntohs(Client.sin_port))
                                {
                                    // Forward to the server.
                                    Server->onContextswitch(&Client);
                                    Server->onPacketwrite(Buffer, Size);

                                    // Ensure that there's a socket associated with the server.
                                    auto &Entry = Datagramsockets[Server];
                                    if (Entry == 0)
                                    {
                                        auto Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

                                        // Ensure that the socket is bound somewhere.
                                        Client = { AF_INET, 0 };
                                        Client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                                        bind(Socket, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));

                                        // Query the port assigned to the socket.
                                        getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);
                                        Proxyports[Server].push_back(ntohs(Client.sin_port));

                                        // Associate the server with the socket.
                                        Entry = Socket;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Poll the servers for a single packet and forward it.
            for (const auto &[Server, Socket] : Datagramsockets)
            {
                uint32_t Datasize{ 8192 };
                if (Server->onPacketread(Buffer, &Datasize))
                {
                    // Notify all ports associated with the server.
                    for (const auto &Port : Proxyports[Server])
                    {
                        SOCKADDR_IN Client{ AF_INET, htons(Port) };
                        Client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                        sendto(Socket, Buffer, Datasize, 0, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));
                    }
                }
            }
            for (const auto &[Socket, Server] : Streamsockets)
            {
                uint32_t Datasize{ 8192 };
                if (Server->onStreamread(Buffer, &Datasize))
                {
                    send(Socket, Buffer, Datasize, 0);
                }
            }
        }
    }

    // Initialize the server backends, only TCP and UDP for now.
    void Createbackend(uint16_t TCPPort, uint16_t UDPPort)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // Find an available port for TCP.
        Listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        for (int i = 0; i < 10; ++i)
        {
            SOCKADDR_IN Server{};
            Server.sin_family = AF_INET;
            BackendTCPport = TCPPort + i;
            Server.sin_port = htons(BackendTCPport);
            Server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            if (0 == bind(Listensocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)))
                break;
        }
        listen(Listensocket, 15);

        // Find an available port for UDP.
        UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        for (int i = 0; i < 10; ++i)
        {
            SOCKADDR_IN Server{};
            Server.sin_family = AF_INET;
            BackendUDPport = UDPPort + i;
            Server.sin_port = htons(BackendUDPport);
            Server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            if (0 == bind(UDPSocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)))
                break;
        }

        // Should not block.
        unsigned long Noblocky = 1;
        ioctlsocket(UDPSocket, FIONBIO, &Noblocky);

        // Add a default entry for localhost proxying.
        Resolvercache["240.0.0.1"] = "127.0.0.1";

        // Load all plugins from disk.
        constexpr const char *Pluignextension = sizeof(void *) == sizeof(uint32_t) ? ".Localnet32" : ".Localnet64";
        auto Results = FS::Findfiles("./Ayria/Plugins", Pluignextension);
        for (const auto &Item : Results)
        {
            auto Module = LoadLibraryA(va("./Ayria/Plugins/%s", Item.c_str()).c_str());
            if (Module)
            {
                Infoprint(va("Loaded localnet plugin \"%s\"", Item.c_str()));
                Pluginslist.push_back(Module);
            }
        }

        // Notify the developer.
        Debugprint(va("Spawning server on TCP %u and UDP %u", BackendTCPport, BackendUDPport));

        // Keep the polling away from the main thread.
        std::thread(Serverthread).detach();
    }

    // Resolve the address and associated ports, creates a new address if needed.
    void Associateport(std::string_view Address, uint16_t Port)
    {
        // Try to resolve directly.
        auto Hostname = Serverinstances.find(Address.data());
        if (Hostname != Serverinstances.end())
        {
            Proxyports[Hostname->second].push_back(Port);
            return;
        }

        // Else we check the cache.
        auto Resolved = Resolvercache.find(Address.data());
        if (Resolved != Resolvercache.end())
        {
            return Associateport(Resolved->second, Port);
        }
    }
    std::string getAddress(std::string_view Hostname)
    {
        // Check if we've resolved the hostname.
        auto Resolved = Resolvercache.find(Hostname.data());
        if (Resolved != Resolvercache.end())
            return getAddress(Resolved->second);

        // Resolve IPs same as hostnames.
        auto Address = Serverinstances.find(Hostname.data());
        if (Address != Serverinstances.end())
            return Address->first;

        // Create a new address for this host in the internal IPaddress range.
        static uint16_t Counter = 2; // 240.0.0.1 is reserved for localhost.
        return va("240.0.%u.%u", HIBYTE(Counter), LOBYTE(Counter++));
    }
    std::string_view getAddress(uint16_t Senderport)
    {
        for (const auto &[Server, Portlist] : Proxyports)
        {
            for (const auto &Port : Portlist)
            {
                if (Senderport == Port)
                {
                    for (const auto &[Hostname, Instance] : Serverinstances)
                    {
                        if (Instance == Server)
                        {
                            return Hostname;
                        }
                    }
                }
            }
        }

        return "";
    }
    bool isProxiedhost(std::string_view Hostname)
    {
        // Is the hostname in the list?
        if (Resolvercache.end() != Resolvercache.find(Hostname.data())) return true;
        if (Serverinstances.end() != Serverinstances.find(Hostname.data())) return true;

        // Keep a blacklist so we don't spam lookups.
        static std::vector<std::string> Blacklist;
        for (const auto &Item : Blacklist)
            if (Item == Hostname)
                return false;

        // Ask the plugins nicely if they have a server for us.
        for (const auto &Item : Pluginslist)
        {
            if (auto Callback = GetProcAddress((HMODULE)Item, "Createserver"))
            {
                if (auto Server = (reinterpret_cast<IServer *(*)(const char *)>(Callback))(Hostname.data()))
                {
                    const auto Proxyaddress = getAddress(Hostname);
                    Resolvercache[Hostname.data()] = Proxyaddress;
                    Serverinstances[Proxyaddress.data()] = Server;

                    return true;
                }
            }
        }

        // Blacklist the host so we don't have to ask the plugins again.
        Blacklist.push_back(Hostname.data());
        return false;
    }
}
