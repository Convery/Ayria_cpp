/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-03
    License: MIT
*/

#include "../Localnetworking.hpp"
#include <Ws2tcpip.h>

namespace Winsock
{
    #define Calloriginal(Name) ((decltype(Name) *)Originalfunctions[#Name])
    std::unordered_map<size_t, IServer::Endpoints_t> Datagramsockets;
    std::unordered_map<std::string_view, void *> Originalfunctions;
    std::unordered_map<std::string, std::string> Proxyaddresses;
    std::unordered_map<size_t, IServer::Address_t> Bindings;
    SOCKADDR_IN Localhost{};

    // Utility functionality.
    inline std::string getAddress(const sockaddr *Sockaddr)
    {
        const auto Address = std::make_unique<char[]>(INET6_ADDRSTRLEN);
        if (Sockaddr->sa_family == AF_INET6) inet_ntop(AF_INET6, &((struct sockaddr_in6 *)Sockaddr)->sin6_addr, Address.get(), INET6_ADDRSTRLEN);
        else inet_ntop(AF_INET, &((struct sockaddr_in *)Sockaddr)->sin_addr, Address.get(), INET6_ADDRSTRLEN);
        return std::string(Address.get());
    }
    inline uint16_t getPort(const size_t Socket)
    {
        SOCKADDR_IN Client{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        int Clientsize{ sizeof(SOCKADDR_IN) };

        // Assign a random port to the socket for identification..
        bind(Socket, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));
        getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);

        return Client.sin_port;
    }

    // Streamed connections just proxies the port.
    int __stdcall Connectsocket(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        auto Readable = getAddress(Name);

        // Replace proxy-hosts for Localnetworking's sake.
        if (auto Result = Proxyaddresses.find(Readable); Result != Proxyaddresses.end())
            Readable = Result->second;

        // Fetch or create an associated server.
        if (Localnetworking::isProxiedhost(Readable))
        {
            // Notify the backend about an incoming connection.
            Localnetworking::Associateport(Readable, getPort(Socket));

            // Replace the target with our IPv4 proxy.
            Namelength = sizeof(Localhost);
            Name = (sockaddr *)&Localhost;
        }

        // Let Windows do the dirty work..
        const auto Result = Calloriginal(connect)(Socket, Name, Namelength);
        const auto Lasterror = WSAGetLastError(); // Debugprint may invalidate this.
        Debugprint(va("Connecting (0x%X) to %s - %s", Socket, Readable.c_str(), Result ? "FAILED" : "SUCCESS"));

        // Some silly firewalls delay the connection longer than the non-blocking timeout.
        if (Result == -1 && Localnetworking::isProxiedhost(Readable)) return 0;

        WSASetLastError(Lasterror);
        return Result;
    }

    // We don't allow binding to internal IPs.
    int __stdcall Bind(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        static auto lClient = Localhost;
        const auto Readable = getAddress(Name);

        // Rather than tracking our internal address, just set it to loopback when binding.
        if (std::strstr(Readable.c_str(), "192.168.") || std::strstr(Readable.c_str(), "10."))
        {
            // Route to localhost with the clients port.
            lClient.sin_port = ((SOCKADDR_IN *)Name)->sin_port;
            Namelength = sizeof(lClient);
            Name = (sockaddr *)&lClient;
        }

        if (auto Result = Calloriginal(bind)(Socket, Name, Namelength); Result == 0)
        {
            // Add it to the list of known bound sockets.
            SOCKADDR_IN Client{};
            int Clientsize{ sizeof(SOCKADDR_IN) };
            getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);
            auto Clientaddress = IServer::Address_t{ Client.sin_addr.S_un.S_addr, Client.sin_port };

            // Verify that the socket isn't being reused.
            for (auto &[Localsocket, Info] : Datagramsockets)
            {
                if (FNV::Equal(Clientaddress, Info.Client))
                {
                    //Datagramsockets.erase(Localsocket);
                    break;
                }
            }

            Bindings[Socket] = Clientaddress;
        }
        else return Result;
        return 0;
    }

    // Datagram sockets get proxied as stream-sockets for simplicity.
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, sockaddr *To, int Tolength)
    {
        // If we are proxying this address, we know it's going to be internal.
        if (auto Result = Proxyaddresses.find(getAddress(To)); Result != Proxyaddresses.end())
        {
            IServer::Address_t Serveraddress = { ((sockaddr_in *)To)->sin_addr.S_un.S_addr, ((sockaddr_in *)To)->sin_port };

            // Find the proxy-socket if already created.
            for (auto &[Localsocket, Info] : Datagramsockets)
            {
                if (FNV::Equal(Serveraddress, Info.Server))
                {
                    // Update the binding if needed..
                    SOCKADDR_IN Client{};
                    int Clientsize{ sizeof(SOCKADDR_IN) };
                    getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);
                    Info.Client = { Client.sin_addr.s_addr, Client.sin_port };

                    return send(Localsocket, Buffer, Length, 0);
                }
            }

            // Find where the socket is bound.
            SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
            getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);

            // Create a new socket for UDP over TCP.
            Socket = Localnetworking::Createsocket(Serveraddress, Result->second);
            auto &Entry = Datagramsockets[Socket];
            Entry.Client = { Client.sin_addr.s_addr, Client.sin_port };
            Entry.Server = Serveraddress;

            return send(Socket, Buffer, Length, 0);
        }

        return Calloriginal(sendto)(Socket, Buffer, Length, Flags, To, Tolength);
    }
    int __stdcall Receivefrom(size_t Socket, char *Buffer, int Length, int Flags, sockaddr *From, int *Fromlength)
    {
        FD_SET ReadFD{};
        const timeval Timeout{ NULL, 1 };
        for (const auto &[Localsocket, _] : Datagramsockets) FD_SET(Localsocket, &ReadFD);

        // Check if there's any data on the internal sockets.
        if (ReadFD.fd_count && select(NULL, &ReadFD, NULL, NULL, &Timeout))
        {
            for (const auto &[Localsocket, Info] : Datagramsockets)
            {
                // We assume that the socket is bound, because it should be.
                if (!FD_ISSET(Localsocket, &ReadFD)) continue;
                auto Binding = Bindings[Socket];

                // IPv4 of NULL means any address.
                if (Binding.Port != Info.Client.Port) continue;
                if (Binding.IPv4 && Binding.IPv4 != Info.Client.IPv4) continue;

                // We know there's data on the socket, so just return the length and server-address.
                *((SOCKADDR_IN *)From) = { AF_INET, Info.Server.Port, {{.S_addr = Info.Server.IPv4}} };
                return recv(Localsocket, Buffer, Length, 0);
            }
        }

        return Calloriginal(recvfrom)(Socket, Buffer, Length, Flags, From, Fromlength);
    }

    // Create a proxy-address for the host if it's proxied.
    hostent *__stdcall Gethostbyname(const char *Hostname)
    {
        Debugprint(va("Performing hostname lookup for \"%s\"", Hostname));
        if (!Localnetworking::isProxiedhost(Hostname)) return Calloriginal(gethostbyname)(Hostname);

        // Resolve to a known host and replace the address.
        auto Result = Calloriginal(gethostbyname)("localhost");
        Result->h_name = const_cast<char *>(Hostname);
        Result->h_addr_list[1] = nullptr;
        Result->h_addrtype = AF_INET;
        *Result->h_aliases = nullptr;
        Result->h_length = 4;

        SOCKADDR_IN Client{ AF_INET, 0, {{.S_addr = htonl(240 << 24 | Hash::FNV1a_32(Hostname) >> 8)} } };
        ((in_addr *)Result->h_addr_list[0])->s_addr = Client.sin_addr.S_un.S_addr;
        Proxyaddresses[getAddress((sockaddr *)&Client)] = Hostname;

        return Result;
    }
    int __stdcall Getaddrinfo(const char *Nodename, const char *Servicename, ADDRINFOA *Hints, ADDRINFOA **Result)
    {
        Debugprint(va("Performing hostname lookup for \"%s\"", Nodename));
        if (!Localnetworking::isProxiedhost(Nodename)) return Calloriginal(getaddrinfo)(Nodename, Servicename, Hints, Result);

        // Resolve to a known host and replace the address.
        auto lResult = Calloriginal(getaddrinfo)("localhost", Servicename, Hints, Result);

        // Possibly leak some memory if the setup is silly.
        ((sockaddr_in *)(*Result)->ai_addr)->sin_addr.S_un.S_addr = htonl(240 << 24 | Hash::FNV1a_32(Nodename) >> 8);
        (*Result)->ai_next = NULL;

        Proxyaddresses[getAddress(((sockaddr *)(*Result)->ai_addr))] = Nodename;
        return lResult;
    }

    // For debugging only.
    #if !defined(NDEBUG)
    size_t __stdcall Createsocket(int Family, int Type, int Protocol)
    {
        const auto Result = Calloriginal(socket)(Family, Type, Protocol);

        Debugprint(va("Creating a %s socket for %s over %s protocol (ID 0x%X)",
                   [&]() { if (Family == AF_INET) return "IPv4"s; if (Family == AF_INET6) return "IPv6"s; if (Family == AF_IPX) return "IPX "s; return va("%u", Family); }().c_str(),
                   [&]() { if (Type == SOCK_STREAM) return "streaming"s; if (Type == SOCK_DGRAM) return "datagrams"s; return va("%u", Type); }().c_str(),
                   [&]()
        {
            switch (Protocol)
            {
                // NOTE(tcn): Case 0 (IPPROTO_IP) is listed as IPPROTO_HOPOPTS, but everyone uses it as 'any' protocol.
                case IPPROTO_IP: return "Any"s; case IPPROTO_ICMP: return "ICMP"s; case IPPROTO_TCP: return "TCP"s; case IPPROTO_UDP: return "UDP"s;
                case /*NSPROTO_IPX*/ 1000: return "IPX"s; case /*NSPROTO_SPX*/ 1256: return "SPX"s; case /*NSPROTO_SPXII*/ 1257: return "SPX2"s;
                default: return va("%u", Protocol);
            }
        }().c_str(), Result));

        /*
            TODO(tcn):
            Given that Windows Vista+ doesn't support the IPX protocol anymore,
            we should probably create a IPX<->UDP solution for such sockets.
            It was a very popular protocol in older games for LAN-play.
        */

        return Result;
    }
    #endif
}

namespace Localnetworking
{
    // Do hooking
    void Initializewinsock()
    {
        // Proxy Winsocks exports with our own information.
        const auto Hook = [&](const char *Name, void *Target)
        {
            auto Address1 = GetProcAddress(LoadLibraryA("wsock32.dll"), Name);
            auto Address2 = GetProcAddress(LoadLibraryA("ws2_32.dll"), Name);
            if (Address1 == Address2) Address2 = 0;

            if (Address1)
            {
                Mhook_SetHook((void **)&Address1, Target);
                Winsock::Originalfunctions[Name] = (void *)Address1;
            }
            if (Address2)
            {
                Mhook_SetHook((void **)&Address2, Target);
                Winsock::Originalfunctions[Name] = (void *)Address2;
            }
        };

        Hook("gethostbyname", (void *)Winsock::Gethostbyname);
        Hook("getaddrinfo", (void *)Winsock::Getaddrinfo);
        Hook("connect", (void *)Winsock::Connectsocket);
        Hook("recvfrom", (void *)Winsock::Receivefrom);
        Hook("sendto", (void *)Winsock::Sendto);
        Hook("bind", (void *)Winsock::Bind);

        // For debugging.
        #if !defined(NDEBUG)
        Hook("socket", Winsock::Createsocket);
        #endif

        // Delayed initialization as a global will have Localnetworking::Backendport set to NULL.
        Winsock::Localhost = { AF_INET, htons(Localnetworking::Backendport), {{.S_addr = htonl(INADDR_LOOPBACK)}} };
    }
}
