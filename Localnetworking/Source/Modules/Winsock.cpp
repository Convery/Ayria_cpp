/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#include "Localnetworking.hpp"
#include <Stdinclude.hpp>
#include <WinSock2.h>
#include <Ws2tcpip.h>

namespace Winsock
{
    #define Calloriginal(Name) ((decltype(Name) *)Originalfunctions[#Name])
    robin_hood::unordered_flat_map<std::string_view, void *> Originalfunctions;
    robin_hood::unordered_flat_map<std::string, sockaddr_in6> Proxyhosts;

    // Utility functionality.
    inline uint16_t getPort(const sockaddr *Sockaddr)
    {
        if (Sockaddr->sa_family == AF_INET6) return ntohs(((sockaddr_in6 *)Sockaddr)->sin6_port);
        return ntohs(((sockaddr_in *)Sockaddr)->sin_port);
    }
    inline uint16_t getPort(const size_t Socket)
    {
        int Clientsize{ sizeof(SOCKADDR_IN) };
        SOCKADDR_IN Client{ AF_INET, 0 };

        // Ensure that the socket is bound somewhere.
        Client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        bind(Socket, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));

        // Query the port assigned to the socket.
        getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);

        // Return with the hosts endian.
        return getPort((SOCKADDR *)&Client);
    }
    inline std::string getAddress(const sockaddr *Sockaddr)
    {
        const auto Address = std::make_unique<char[]>(INET6_ADDRSTRLEN);
        if (Sockaddr->sa_family == AF_INET6) inet_ntop(AF_INET6, &((struct sockaddr_in6 *)Sockaddr)->sin6_addr, Address.get(), INET6_ADDRSTRLEN);
        else inet_ntop(AF_INET, &((struct sockaddr_in *)Sockaddr)->sin_addr, Address.get(), INET6_ADDRSTRLEN);
        return std::string(Address.get());
    }

    // Only relevant while debugging.
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
                // NOTE(tcn): Case 0 (IPPROTO_IP) is actually the same as IPPROTO_HOPOPTS, but everyone uses it as 'any' protocol.
                case IPPROTO_IP: return "Any"s; case IPPROTO_ICMP: return "ICMP"s; case IPPROTO_TCP: return "TCP"s; case IPPROTO_UDP: return "UDP"s;
                case /*NSPROTO_IPX*/ 1000: return "IPX"s; case /*NSPROTO_SPX*/ 1256: return "SPX"s; case /*NSPROTO_SPXII*/ 1257: return "SPX2"s;
                default: return va("%u", Protocol);
            }
        }().c_str(), Result));

        /*
            TODO(tcn):
            Given that Windows Vista+ doesn't support the IPX protocol anymore,
            we should probably create a IPX<->UDP solution for such sockets.
            It was a very popular protocol in older games.
        */

        return Result;
    }

    // Datagram sockets need to remember the host-information.
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, sockaddr *To, int Tolength)
    {
        const auto Address = getAddress(To);
        if(Localnetworking::isProxiedhost(Address))
        {
            // Associate the client-port.
            const auto Port = getPort(Socket);
            Localnetworking::Associateport(Address, Port);

            // Save the host-info for later.
            const auto Entry = &Proxyhosts[Address.c_str()];
            std::memcpy(Entry, To, Tolength);

            // Replace the target with our IPv4 proxy.
            ((SOCKADDR_IN *)To)->sin_family = AF_INET;
            ((SOCKADDR_IN *)To)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ((SOCKADDR_IN *)To)->sin_port = htons(Localnetworking::BackendUDPport);
        }

        //Debugprint(va("Sending to %s:%u", Address.c_str(), getPort(To)));
        return Calloriginal(sendto)(Socket, Buffer, Length, Flags, To, Tolength);
    }
    int __stdcall Receivefrom(size_t Socket, char *Buffer, int Length, int Flags, sockaddr *From, int *Fromlength)
    {
        const auto Result = Calloriginal(recvfrom)(Socket, Buffer, Length, Flags, From, Fromlength);
        if (!From || !Fromlength || Result < 0) return Result;
        //Debugprint(va("Recv from %s:%u", getAddress(From).c_str(), getPort(From)));

        // Check if the sender is our backend.
        if(((sockaddr_in *)From)->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
        {
            // Check if we have an address associated with this port in the backend.
            if(const auto Address = Localnetworking::getAddress(getPort(Socket)); !Address.empty())
            {
                // Replace with the real host-information if available.
                const auto Hostinfo = Proxyhosts.find(Address.data());
                if(Hostinfo != Proxyhosts.end())
                {
                    std::memcpy(From, &Hostinfo->second, *Fromlength);
                }
            }
        }

        return Result;
    }

    // Streamed connections just proxies the port.
    int __stdcall Connectsocket(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        const auto Readable = getAddress(Name);
        if (Localnetworking::isProxiedhost(Readable))
        {
            // Associate the host-info.
            Localnetworking::Associateport(Readable, getPort(Socket));

            // Replace the host with our IPv4 proxy.
            ((SOCKADDR_IN *)Name)->sin_family = AF_INET;
            ((SOCKADDR_IN *)Name)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ((SOCKADDR_IN *)Name)->sin_port = htons(Localnetworking::BackendTCPport);
        }

        const auto Result = Calloriginal(connect)(Socket, Name, Namelength);
        Debugprint(va("Connecting (0x%X) to %s:%u - %s", Socket, Readable.c_str(), getPort(Name), Result ? "FAILED" : "SUCCESS"));
        if(Result == -1) WSASetLastError(WSAEWOULDBLOCK);
        return Result;
    }

    // Create a proxy address if needed.
    hostent *__stdcall Gethostbyname(const char *Hostname)
    {
        Debugprint(va("Performing hostname lookup for \"%s\"", Hostname));
        auto Result = Calloriginal(gethostbyname)(Hostname);

        // If the host is interesting, give it a fake IP to work offline.
        if (Localnetworking::isProxiedhost(Hostname))
        {
            // Resolve to a known host and replace the address.
            Result = Calloriginal(gethostbyname)("localhost");
            Result->h_name = const_cast<char *>(Hostname);
            Result->h_addrtype = AF_INET;
            *Result->h_aliases = nullptr;
            Result->h_length = 4;

            ((in_addr *)Result->h_addr_list[0])->s_addr = inet_addr(Localnetworking::getAddress(Hostname).data());
            Result->h_addr_list[1] = nullptr;
        }

        return Result;
    }
    int __stdcall Getaddrinfo(const char *Nodename, const char *Servicename, ADDRINFOA *Hints, ADDRINFOA **Result)
    {
        int lResult{};
        if (Hints) Hints->ai_family = PF_INET;
        Debugprint(va("Performing hostname lookup for \"%s\"", Nodename));

        // If the host is interesting, give it a fake IP to work offline.
        if (Localnetworking::isProxiedhost(Nodename))
        {
            // Resolve to a known host and replace the address.
            lResult = Calloriginal(getaddrinfo)("localhost", Servicename, Hints, Result);

            // Set the IP for all records.
            for (ADDRINFOA *ptr = *Result; ptr != NULL; ptr = ptr->ai_next)
            {
                ((sockaddr_in *)ptr->ai_addr)->sin_addr.S_un.S_addr = inet_addr(Localnetworking::getAddress(Nodename).data());
            }
        }
        else
        {
            lResult = Calloriginal(getaddrinfo)(Nodename, Servicename, Hints, Result);
        }

        return lResult;
    }

    #if 0
    struct hostent *__stdcall Gethostbyaddr(const char *Address, int Addresslength, int Addresstype)
    {
        return nullptr;
    }

    #endif
}

// Hook the front-facing APIs.
void InstallWinsock()
{
    // Proxy Winsocks exports with our own information.
    const auto Hook = [&](std::string_view Name, void *Target)
    {
        if (auto Address = GetProcAddress(GetModuleHandleA("ws2_32.dll"), Name.data()))
        {
            Winsock::Originalfunctions[Name] = (void *)Address;
            Mhook_SetHook(&Winsock::Originalfunctions[Name], Target);
            return;
        }
        if (auto Address = GetProcAddress(GetModuleHandleA("wsock32.dll"), Name.data()))
        {
            Winsock::Originalfunctions[Name] = (void *)Address;
            Mhook_SetHook(&Winsock::Originalfunctions[Name], Target);
            return;
        }
    };

    Hook("gethostbyname", (void *)Winsock::Gethostbyname);
    Hook("getaddrinfo", (void *)Winsock::Getaddrinfo);
    Hook("connect", (void *)Winsock::Connectsocket);
    Hook("recvfrom", (void *)Winsock::Receivefrom);
    Hook("sendto", (void *)Winsock::Sendto);

    // For debugging.
    #if !defined(NDEBUG)
    Hook("socket", Winsock::Createsocket);
    #endif
}
