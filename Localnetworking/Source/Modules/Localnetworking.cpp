/*
    Initial author: Convery (tcn@ayria.se)
    Started: 12-04-2019
    License: MIT
*/

#include "../Localnetworking.hpp"
#include "Stdinclude.hpp"
#include "../IServer.hpp"
#include <WinSock2.h>

namespace Localnetworking
{
    robin_hood::unordered_flat_map<std::string, std::string> Proxyaddresses;
    robin_hood::unordered_flat_map<std::string, uint16_t> Proxyports;
    robin_hood::unordered_flat_map<std::string, size_t> Proxysockets;
    robin_hood::unordered_flat_map<std::string, void *> Servers;
    std::vector<void *> Pluginslist;

    // Internal polling.
    size_t Listensocket, UDPSocket;
    IServer *Findserver(std::string_view Address)
    {
        for (const auto &[Real, Proxy] : Proxyaddresses)
        {
            if (Proxy == Address)
            {
                if (auto Server = (IServer *)Servers[Real])
                {
                    return Server;
                }
            }
        }

        return (IServer *)Servers[Address.data()];
    }
    void Serverthread()
    {
        SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
        char Buffer[8192];
        FD_SET ReadFD;

        while (true)
        {
            FD_ZERO(&ReadFD);

            // Only listen for new connections on this socket.
            FD_SET(Listensocket, &ReadFD);

            // All UDP packets come through the same socket.
            FD_SET(UDPSocket, &ReadFD);

            // While the TCP ones need their own socket, because reasons.
            for (const auto &[_, FD] : Proxysockets) FD_SET(FD, &ReadFD);

            // Ask Windows nicely for the status.
            if (auto Count = select(NULL, &ReadFD, NULL, NULL, NULL))
            {
                // If there's data on the listen-socket, it's a new connection.
                if (FD_ISSET(Listensocket, &ReadFD))
                {
                    Count--; Clientsize = sizeof(SOCKADDR_IN);
                    if (auto Socket = accept(Listensocket, (SOCKADDR *)&Client, &Clientsize))
                    {
                        // Blocking is bad.
                        unsigned long Noblocky = 1;
                        ioctlsocket(Socket, FIONBIO, &Noblocky);

                        // Find the hostname from the port.
                        for (const auto &[Host, Port] : Proxyports)
                        {
                            if (Port == ntohs(Client.sin_port))
                            {
                                // Remove the proxy port as it's now irrelevant.
                                Proxysockets[Host] = Socket;
                                Proxyports.erase(Host);
                                break;
                            }
                        }
                    }
                }

                // If there's data on the stream-sockets then we need to forward it.
                for (const auto &[Host, Socket] : Proxysockets)
                {
                    if (!FD_ISSET(Socket, &ReadFD)) continue; Count--;
                    if (auto Size = recv(Socket, Buffer, 8192, 0))
                    {
                        if (auto Server = Findserver(Host))
                        {
                            Server->onStreamwrite(Buffer, Size);
                        }
                    }
                }

                // If we still have unhandled FDs, it's for datagram.
                while (Count-- && FD_ISSET(UDPSocket, &ReadFD))
                {
                    Clientsize = sizeof(SOCKADDR_IN);
                    if (auto Size = recvfrom(UDPSocket, Buffer, 8196, 0, (SOCKADDR *)&Client, &Clientsize))
                    {
                        for (const auto &[Host, Port] : Proxyports)
                        {
                            if (Port == ntohs(Client.sin_port))
                            {
                                if (auto Server = Findserver(Host))
                                {
                                    Server->onPacketwrite(Buffer, Size);
                                }
                            }
                        }
                    }
                }
            }

            // NOTE(tcn): We might want to tweak this delay later.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Poll the servers for data.
            for (const auto &[Host, FD] : Proxysockets)
            {
                if (auto Server = Findserver(Host))
                {
                    uint32_t Datasize{ 8192 };
                    while (Server->onStreamread(Buffer, &Datasize))
                    {
                        send(FD, Buffer, Datasize, 0);
                        Datasize = 8192;
                    }
                }
            }
            for (const auto &[Host, Port] : Proxyports)
            {
                if (auto Server = Findserver(Host))
                {
                    uint32_t Datasize{ 8192 };
                    while (Server->onPacketread(Buffer, &Datasize))
                    {
                        Client.sin_family = AF_INET;
                        Client.sin_port = htons(Port);
                        Client.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
                        sendto(UDPSocket, Buffer, Datasize, 0, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));

                        Datasize = 8192;
                    }
                }
            }
        }
    }

    // Initialize the server backends, only TCP and UDP for now.
    void Createbackend(uint16_t TCPPort, uint16_t UDPPort)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKADDR_IN Server{};
        Server.sin_family = AF_INET;
        Server.sin_port = htons(TCPPort);
        Server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

        Listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        bind(Listensocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN));
        listen(Listensocket, 5);

        Server.sin_port = htons(UDPPort);
        UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(UDPSocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN));

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

        std::thread(Serverthread).detach();
    }

    // Check if we are going to proxy any connection-property.
    bool isAddressproxied(std::string_view Address)
    {
        for (const auto &[Host, Proxy] : Proxyaddresses)
            if (Proxy == Address)
                return true;

        // Allow hosts to be an IP.
        return isHostproxied(Address);
    }
    bool isHostproxied(std::string_view Hostname)
    {
        for (const auto &[Host, _] : Servers)
            if (Host == Hostname)
                return true;

        // Ask the plugins nicely if they have a server for us.
        for (const auto &Item : Pluginslist)
        {
            if (auto Callback = GetProcAddress((HMODULE)Item, "Createserver"))
            {
                if (auto Server = (reinterpret_cast<IServer *(*)(const char *)>(Callback))(Hostname.data()))
                {
                    Servers[Hostname.data()] = Server;
                    return true;
                }
            }
        }

        return false;
    }

    // Create or fetch a unique property for the connection.
    std::string getProxyaddress(std::string_view Hostname)
    {
        static uint16_t Counter = 0;

        if (Proxyaddresses[Hostname.data()] == std::string())
        {
            Counter++;  // If we ever create 64K connections, overflowing is the least of our problems.
            Proxyaddresses[Hostname.data()] = va("240.0.%u.%u", HIBYTE(Counter), LOBYTE(Counter));
        }

        return Proxyaddresses[Hostname.data()];
    }

    // Associate a port with an address.
    void Associateport(std::string_view Address, uint16_t Port)
    {
        Proxyports[Address.data()] = Port;
    }
    std::string Addressfromport(uint16_t Port)
    {
        for (const auto &[Address, P] : Proxyports)
        {
            if (Port == P) return Address;
        }

        return {};
    }
}
