/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#include "Localnetworking.hpp"
#include "Stdinclude.hpp"
#include <WinSock2.h>
#include <Ws2tcpip.h>

namespace Winsock
{
    #define Callhook(Name, ...) WSGuards[#Name].lock(); WSHooks[#Name].Removehook(); __VA_ARGS__; WSHooks[#Name].Installhook(); WSGuards[#Name].unlock();
    robin_hood::unordered_flat_map<std::string_view, Simplehook::Stomphook> WSHooks;
    // Maximum length should be a IPv6; so let's pretend it's a union of v4 and v6.
    robin_hood::unordered_flat_map<std::string, sockaddr_in6> Proxyhosts;
    std::unordered_map<std::string_view, std::mutex> WSGuards;
    std::vector<size_t> Nonblockingsockets;

    // Utility functionality.
    inline uint16_t getPort(const sockaddr *Sockaddr)
    {
        if (Sockaddr->sa_family == AF_INET6) return ntohs(((sockaddr_in6 *)Sockaddr)->sin6_port);
        else return ntohs(((sockaddr_in *)Sockaddr)->sin_port);
    }
    inline uint16_t getPort(const size_t Socket)
    {
        int Clientsize{ sizeof(SOCKADDR_IN) };
        SOCKADDR_IN Client{ AF_INET };

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
        auto Address = std::make_unique<char[]>(INET6_ADDRSTRLEN);
        if (Sockaddr->sa_family == AF_INET6) inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)Sockaddr)->sin6_addr), Address.get(), INET6_ADDRSTRLEN);
        else inet_ntop(AF_INET, &(((struct sockaddr_in *)Sockaddr)->sin_addr), Address.get(), INET6_ADDRSTRLEN);
        return std::string(Address.get());
    }

    // Only relevant while debugging.
    #if !defined (NDEBUG)
    size_t __stdcall Createsocket(int Family, int Type, int Protocol)
    {
        Callhook(socket, const auto Result = socket(Family, Type, Protocol));

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
    #endif

    // Datagram sockets need to remember the host-information.
    int __stdcall Controlsocket(size_t Socket, unsigned long Command, unsigned long *Argument)
    {
        if(Command == FIONBIO && *Argument == 1)
            Nonblockingsockets.push_back(Socket);

        Callhook(ioctlsocket, const auto Result = ioctlsocket(Socket, Command, Argument));
        return Result;
    }
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, sockaddr *To, int Tolength)
    {
        const auto Address = getAddress(To);
        if(Localnetworking::isProxiedhost(Address))
        {
            // Associate the client-port.
            const auto Port = getPort(Socket);
            Localnetworking::Associateport(Address, Port);

            // Save the host-info for later.
            const auto Readable = va("%s:%u", Address.c_str(), getPort(To));
            auto Entry = &Proxyhosts[Readable];
            std::memcpy(Entry, To, Tolength);

            // Replace the target with our IPv4 proxy.
            ((SOCKADDR_IN *)To)->sin_family = AF_INET;
            ((SOCKADDR_IN *)To)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ((SOCKADDR_IN *)To)->sin_port = htons(Localnetworking::BackendUDPport);
        }

        Callhook(sendto, const auto Result = sendto(Socket, Buffer, Length, Flags, To, Tolength));
        return Result;
    }
    int __stdcall Receivefrom(size_t Socket, char *Buffer, int Length, int Flags, sockaddr *From, int *Fromlength)
    {
        int Result{-1};
        unsigned long CMDFlags{1};

        // NOTE(tcn): Our implementation only supports non-blocking sockets, so hackery is needed.
        if(Nonblockingsockets.end() == std::find(Nonblockingsockets.begin(), Nonblockingsockets.end(), Socket))
        {
            // Put the socket into a non-blocking state.
            Callhook(ioctlsocket, ioctlsocket(Socket, FIONBIO, &CMDFlags));

            // Poll until we get data.
            while(true)
            {
                Callhook(recvfrom, Result = recvfrom(Socket, Buffer, Length, Flags, From, Fromlength));
                if(Result == -1 && WSAGetLastError() != WSAEWOULDBLOCK) break;
                std::this_thread::yield();
            }

            // Restore the state.
            CMDFlags = uint32_t();
            Callhook(ioctlsocket, ioctlsocket(Socket, FIONBIO, &CMDFlags));
        }
        else
        {
            Callhook(recvfrom, Result = recvfrom(Socket, Buffer, Length, Flags, From, Fromlength));
        }

        // Quit early on errors.
        if (!From || !Fromlength || Result < 0) return Result;

        // Check if the sender is our backend.
        if(((sockaddr_in *)From)->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
        {
            const auto Senderport = getPort(Socket);
            // But not via the main socket as proxied packets have their own.
            if(Senderport != Localnetworking::BackendUDPport)
            {
                const auto Address = Localnetworking::getAddress(getPort(From));
                const auto Readable = va("%s:%u", Address.data(), Senderport);

                // Replace with the real host-information.
                const auto Hostinfo = Proxyhosts.find(Readable);
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
        const auto Port = getPort(Name);
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

        // NOTE(tcn): Our backend is non-blocking, as such we need to hack.
        Callhook(connect, auto Result = connect(Socket, Name, Namelength));
        if (Result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
        {
            unsigned long Blocky = 0, Noblocky = 1;
            ioctlsocket(Socket, FIONBIO, &Blocky);
            Callhook(connect, Result = connect(Socket, Name, Namelength));
            if (Result == SOCKET_ERROR && WSAGetLastError() == WSAEISCONN) Result = 0;
            ioctlsocket(Socket, FIONBIO, &Noblocky);
        }

        Debugprint(va("Connecting (0x%X) to %s:%u - %s", Socket, Readable.c_str(), Port, Result ? "FAILED" : "SUCCESS"));
        return Result;
    }

    // Create a proxy address if needed.
    hostent *__stdcall Gethostbyname(const char *Hostname)
    {
        Debugprint(va("Performing hostname lookup for \"%s\"", Hostname));
        Callhook(gethostbyname, auto Result = gethostbyname(Hostname));

        // If the host is interesting, give it a fake IP to work offline.
        if (Localnetworking::isProxiedhost(Hostname))
        {
            // Resolve to a known host and replace the address.
            Callhook(gethostbyname, Result = gethostbyname("localhost"));
            Result->h_name = const_cast<char *>(Hostname);
            Result->h_addrtype = AF_INET;
            *Result->h_aliases = NULL;
            Result->h_length = 4;

            ((in_addr *)Result->h_addr_list[0])->s_addr = inet_addr(Localnetworking::getAddress(Hostname).data());
            Result->h_addr_list[1] = NULL;
        }

        return Result;
    }

    #if 0
    struct hostent *__stdcall Gethostbyaddr(const char *Address, int Addresslength, int Addresstype)
    {
        return nullptr;
    }
    int __stdcall Getaddrinfo(const char *Nodename, const char *Servicename, ADDRINFOA *Hints, ADDRINFOA **Result)
    {
        return 0;
    }
    #endif
}

// Hook the front-facing APIs.
void InstallWinsock()
{
    auto Getexport = [](std::string_view Name) -> void *
    {
        auto Address = GetProcAddress(GetModuleHandleA("wsock32.dll"), Name.data());
        if (!Address) Address = GetProcAddress(GetModuleHandleA("ws2_32.dll"), Name.data());
        return Address;
    };
    auto Hook = [&](std::string_view Name, void *Target)
    {
        auto Address = Getexport(Name);
        if(Address) Winsock::WSHooks[Name].Installhook(Address, Target);
    };

    // Proxy Winsocks exports with our own information.
    Hook("gethostbyname", Winsock::Gethostbyname);
    Hook("ioctlsocket", Winsock::Controlsocket);
    Hook("connect", Winsock::Connectsocket);
    Hook("recvfrom", Winsock::Receivefrom);
    Hook("sendto", Winsock::Sendto);

    // For debugging only.
    #if !defined(NDEBUG)
    Hook("socket", Winsock::Createsocket);
    #endif
}
