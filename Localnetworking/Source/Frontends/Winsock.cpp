/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-03
    License: MIT
*/

#include "../Localnetworking.hpp"
#include <Ws2Tcpip.h>

namespace Winsock
{
    #define Calloriginal(Name) ((decltype(Name) *)Originalfunctions[#Name])
    Hashmap<size_t, sockaddr_in> Directedconnections;
    Hashmap<std::string, void *> Originalfunctions;
    Hashmap<size_t, sockaddr_in> Connections;
    Hashmap<size_t, sockaddr_in> Bindings;
    Hashset<size_t> Datagramsockets;

    // Utility functionality.
    inline std::string getAddress(const struct sockaddr *Sockaddr)
    {
        char Address[INET6_ADDRSTRLEN]{};

        if (Sockaddr->sa_family == AF_INET6)
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)Sockaddr)->sin6_addr, Address, INET6_ADDRSTRLEN);
        else
            inet_ntop(AF_INET, &((struct sockaddr_in *)Sockaddr)->sin_addr, Address, INET6_ADDRSTRLEN);

        return Address;
    }
    inline std::string getPort(const struct sockaddr *Sockaddr)
    {
        if (Sockaddr->sa_family == AF_INET)
            return va("%u", ntohs(((struct sockaddr_in *)Sockaddr)->sin_port));
        return va("%u", ntohs(((struct sockaddr_in6 *)Sockaddr)->sin6_port));
    }

    // Connects proxied sockets to the backend.
    int __stdcall Connectsocket(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        // If proxied, handle the connection internally. We assume the struct is always IPv4.
        if (const auto Proxyserver = Localnetworking::getProxyserver((sockaddr_in *)Name))
        {
            Debugprint("Connecting to proxied server: "s + Proxyserver->Hostname);

            // For some edge-cases devs call connect on a datagram socket and we need to proxy.
            if (Datagramsockets.contains(Socket)) [[unlikely]]
            {
                Directedconnections[Socket] = *(sockaddr_in *)Name;
            }
            else
            {
                Localnetworking::Connectserver(Socket, Proxyserver->Instance);
            }
            return 0;
        }

        // Let Windows do the heavy lifting instead.
        if (const auto Result = Calloriginal(connect)(Socket, Name, Namelength))
        {
            // Debugprint may invalidate the last-error, so we had to save it.
            const auto Lasterror = WSAGetLastError();

            // Not actually a failure.
            if (Lasterror != WSAEWOULDBLOCK)
                Debugprint("Failed to connect to: "s + getAddress(Name) + ":" + getPort(Name));

            WSASetLastError(Lasterror);
            return Result;
        }

        // Our own system connects to localhost a lot, so ignore that.
        if (((sockaddr_in *)Name)->sin_addr.s_addr != ntohl(INADDR_LOOPBACK))
            Debugprint("Connected to: "s + getAddress(Name) + ":" + getPort(Name));

        return 0;
    }

    // Internal addresses are abstracted to localhost for easier management.
    int __stdcall Bind(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        const auto Readableaddress = getAddress(Name);

        // Windows IPv6 sockets should be in dual-stack mode before binding.
        if(Name->sa_family == AF_INET6)
        {
            const DWORD Argument{};
            setsockopt(Socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&Argument, sizeof(Argument));
        }

        // We don't allow binding to internal addresses, just localhost.
        if (0 == std::memcmp(Readableaddress.c_str(), "192.168.", 8) || 0 == std::memcmp(Readableaddress.c_str(), "10.", 3))
        {
            if (Name->sa_family == AF_INET) ((sockaddr_in *)Name)->sin_addr.s_addr = INADDR_LOOPBACK;
        }

        const auto Result = Calloriginal(bind)(Socket, Name, Namelength);
        Bindings[Socket] = *(sockaddr_in *)Name;
        return Result;
    }

    // Datagram sockets are abstracted to stream-sockets so we can provide a unified interface.
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, struct sockaddr *To, int Tolength)
    {
        if (const auto Proxyserver = Localnetworking::getProxyserver((sockaddr_in *)To))
        {
            // See if we already have an established connection.
            for (const auto &Item : Connections)
            {
                if (FNV::Equal(Item.second.sin_addr, ((sockaddr_in *)To)->sin_addr))
                {
                    return send(Item.first, Buffer, Length, NULL);
                }
            }

            // Create a new one if needed and send to the socket.
            const auto Localsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            Localnetworking::Connectserver(Localsocket, Proxyserver->Instance);

            unsigned long Argument{ 1 };
            ioctlsocket(Localsocket, FIONBIO, &Argument);
            Connections[Localsocket] = *(sockaddr_in *)To;
            return send(Localsocket, Buffer, Length, NULL);
        }

        return Calloriginal(sendto)(Socket, Buffer, Length, Flags, To, Tolength);
    }
    int __stdcall Receivefrom(size_t Socket, char *Buffer, int Length, int Flags, struct sockaddr *From, int *Fromlength)
    {
        FD_SET ReadFD{};
        const timeval Timeout{ NULL, 1 };
        for (const auto &Item : Connections) FD_SET(Item.first, &ReadFD);

        // Check if there's any data on the internal sockets.
        if (select(NULL, &ReadFD, NULL, NULL, &Timeout))
        {
            for (const auto &Item : Connections)
            {
                // IPv4 of NULL means any address.
                if (!FD_ISSET(Item.first, &ReadFD)) continue;
                //if (Bindings[Socket].sin_port != Item.second.sin_port) continue;
                if (Bindings[Socket].sin_addr.s_addr && !FNV::Equal(Bindings[Socket].sin_addr, Item.second.sin_addr)) continue;

                if (Fromlength) *Fromlength = sizeof(SOCKADDR_IN);
                if (From)*((sockaddr_in *)From) = Item.second;
                return recv(Item.first, Buffer, Length, 0);
            }
        }

        return Calloriginal(recvfrom)(Socket, Buffer, Length, Flags, From, Fromlength);
    }

    // Create a proxy-address for the host if it's proxied.
    hostent *__stdcall Gethostbyname(const char *Hostname)
    {
        const auto Proxyserver = Localnetworking::getProxyserver(Hostname);
        Debugprint(va("Performing hostname lookup for \"%s\"", Hostname));

        // Not our problem =)
        if (!Proxyserver) return Calloriginal(gethostbyname)(Hostname);

        // Resolve to a known host and replace the address.
        const auto Result = Calloriginal(gethostbyname)("localhost");
        Result->h_name = const_cast<char *>(Hostname);
        Result->h_addr_list[1] = nullptr;
        Result->h_addrtype = AF_INET;
        *Result->h_aliases = nullptr;
        Result->h_length = 4;

        ((in_addr *)Result->h_addr_list[0])->s_addr = Proxyserver->Address.sin_addr.s_addr;
        return Result;
    }
    int __stdcall Getaddrinfo(const char *Nodename, const char *Servicename, ADDRINFOA *Hints, ADDRINFOA **Result)
    {
        const auto Proxyserver = Localnetworking::getProxyserver(Nodename);
        Debugprint(va("Performing hostname lookup for \"%s\"", Nodename));

        // Not our problem =)
        if (!Proxyserver) return Calloriginal(getaddrinfo)(Nodename, Servicename, Hints, Result);

        // Resolve to a known host and replace the address.
        const auto lResult = Calloriginal(getaddrinfo)("localhost", Servicename, Hints, Result);

        // Possibly leak some memory if the setup is silly.
        ((sockaddr_in *)(*Result)->ai_addr)->sin_addr.s_addr = Proxyserver->Address.sin_addr.s_addr;
        (*Result)->ai_next = NULL;
        return lResult;
    }

    // Hackery to support directed connections.
    int __stdcall Send(size_t Socket, const char *Buffer, int Length, int Flags)
    {
        if (!Datagramsockets.contains(Socket)) [[likely]] return Calloriginal(send)(Socket, Buffer, Length, Flags);
        return Sendto(Socket, Buffer, Length, Flags, (sockaddr *)&Directedconnections[Socket], sizeof(sockaddr_in));
    }
    int __stdcall Receive(size_t Socket, char *Buffer, int Length, int Flags)
    {
        if (!Datagramsockets.contains(Socket)) [[likely]] return Calloriginal(recv)(Socket, Buffer, Length, Flags);
        return Receivefrom(Socket, Buffer, Length, Flags, nullptr, nullptr);
    }

    // Mainly for debug-logging.
    size_t __stdcall Createsocket(int Family, int Type, int Protocol)
    {
        const auto Result = Calloriginal(socket)(Family, Type, Protocol);

        Debugprint(va("Creating a %s socket for %s over %s protocol (ID 0x%X)",
        [&]()
        {
            if (Family == AF_INET) return "IPv4"s;
            if (Family == AF_INET6) return "IPv6"s;
            if (Family == AF_IPX) return "IPX "s;
            return va("%u", Family);
        }().c_str(),
        [&]()
        {
            if (Type == SOCK_STREAM) return "streaming"s;
            if (Type == SOCK_DGRAM) return "datagrams"s;
            return va("%u", Type);
        }().c_str(),
        [&]()
        {
             switch (Protocol)
             {
                 // NOTE(tcn): Case 0 (IPPROTO_IP) is listed as IPPROTO_HOPOPTS, but everyone uses it as 'any' protocol.
                 case IPPROTO_IP: return "Any"s;
                 case IPPROTO_ICMP: return "ICMP"s;
                 case IPPROTO_TCP: return "TCP"s;
                 case IPPROTO_UDP: return "UDP"s;
                 case /*NSPROTO_IPX*/ 1000: return "IPX"s;
                 case /*NSPROTO_SPX*/ 1256: return "SPX"s;
                 case /*NSPROTO_SPXII*/ 1257: return "SPX2"s;
                 default: return va("%u", Protocol);
             }
        }().c_str(),
        Result));

        /*
            TODO(tcn):
            Given that Windows Vista+ doesn't support the IPX protocol anymore,
            we should probably create a IPX<->UDP solution for such sockets.
            It was a very popular protocol in older games for LAN-play.
        */

        // For some edge-cases devs call connect on a datagram socket and we need to track that.
        if (Type == SOCK_DGRAM) Datagramsockets.insert(Result);

        return Result;
    }
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
                const auto Trampoline = Hooking::Stomphook(Address1, Target);
                Winsock::Originalfunctions[Name] = Trampoline;
                assert(Trampoline);
            }
            if (Address2)
            {
                const auto Trampoline = Hooking::Stomphook(Address2, Target);
                Winsock::Originalfunctions[Name] = Trampoline;
                assert(Trampoline);
            }
        };

        Hook("gethostbyname", (void *)Winsock::Gethostbyname);
        Hook("getaddrinfo", (void *)Winsock::Getaddrinfo);
        Hook("connect", (void *)Winsock::Connectsocket);
        Hook("recvfrom", (void *)Winsock::Receivefrom);
        Hook("socket", (void *)Winsock::Createsocket);
        Hook("sendto", (void *)Winsock::Sendto);
        Hook("recv", (void *)Winsock::Receive);
        Hook("send", (void *)Winsock::Send);
        Hook("bind", (void *)Winsock::Bind);
    }
}
