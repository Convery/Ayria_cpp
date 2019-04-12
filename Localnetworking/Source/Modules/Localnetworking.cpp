/*
    Initial author: Convery (tcn@ayria.se)
    Started: 12-04-2019
    License: MIT
*/

#include "../Stdinclude.hpp"
#include "../IServer.h"
#include <WinSock2.h>

namespace Localnetworking
{
    std::unordered_map<std::string_view, std::string> Proxyaddresses;
    std::unordered_map<std::string_view, uint16_t> Proxyports;
    std::unordered_map<std::string_view, size_t> Proxysockets;
    std::unordered_map<std::string_view, void *> Servers;
    std::vector<void *> Pluginslist;

    // Internal polling.
    size_t Listensocket, UDPSocket;
    void Serverthread()
    {
        SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
        char Buffer[8192];
        FD_SET WriteFD;
        FD_SET ReadFD;

        while (true)
        {
            FD_ZERO(&WriteFD);
            FD_ZERO(&ReadFD);

            // Only listen for new connections on this socket.
            FD_SET(Listensocket, &ReadFD);

            // All UDP packets come through the same socket.
            FD_SET(UDPSocket, &ReadFD);

            // While the TCP ones need their own socket, because reasons.
            for(const auto &[_, FD] : Proxysockets) FD_SET(FD, &ReadFD);

            // Ask Windows nicely for the status.
            if (auto Count = select(NULL, &ReadFD, &WriteFD, NULL, NULL))
            {
                // New connection, yey.
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
                                Proxysockets[Host] = Socket;
                                break;
                            }
                        }
                    }
                }

                // Iterate over the stream-sockets returned.
                for (const auto &[Host, Socket] : Proxysockets)
                {
                    if (FD_ISSET(Socket, &ReadFD))
                    {
                        Count--;
                        if (auto Size = recv(Socket, Buffer, 8192, 0) > 0)
                        {
                            if (auto Server = (IServer *)Servers[Host])
                            {
                                Server->onStream(Socket, Buffer, Size);
                            }
                        }
                    }
                }

                // If there's any unhandled results, they are all UDP.
                while (Count--)
                {
                    if (FD_ISSET(UDPSocket, &ReadFD))
                    {
                        Clientsize = sizeof(SOCKADDR_IN);
                        if (auto Size = recvfrom(UDPSocket, Buffer, 8196, 0, (SOCKADDR *)&Client, &Clientsize))
                        {
                            for (const auto &[Host, Port] : Proxyports)
                            {
                                if (Port == ntohs(Client.sin_port))
                                {
                                    if (auto Server = (IServer *)Servers[Host])
                                    {
                                        Debugprint(va("Got a packet from proxyport %u", ntohs(Client.sin_port)));
                                        Server->onDatagram(Port, Buffer, Size);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // NOTE(tcn): We might want to tweak this delay later.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Initialize the server backends on port 4200 and 4201 respectively.
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
                    Servers[Hostname] = Server;
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

        if (Proxyaddresses[Hostname] == std::string())
        {
            Counter++;  // If we ever create 64K connections, overflowing is the least of our problems.
            Proxyaddresses[Hostname] = va("240.0.%u.%u", HIBYTE(Counter), LOBYTE(Counter));
        }

        return Proxyaddresses[Hostname];
    }
    uint16_t getProxyport(std::string_view Address)
    {
        static size_t Counter = 0;

        if (Proxyports[Address] == 0)
        {
            Proxyports[Address] = 4202 + Counter++;
        }

        return Proxyports[Address];
    }

    // Associate a port with an address.
    void Associateport(std::string_view Address, uint16_t Port)
    {
        Proxyports[Address] = Port;
    }

    /*
        Localnetworking outline:
        1. Have a TCP and UDP server listen on localhost:50000 and localhost:50001
        2. When resolving a hostname, return an invalid IP associated with the name.
        3a. When connecting or sending, bind the socket to a unique port.
        3b. In the server, forward to a server identified by the port.
    */
}
