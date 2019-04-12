/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#include "../Stdinclude.hpp"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std::string_literals;
#include <WinSock2.h>
#include <Ws2tcpip.h>

namespace Winsock
{
    #define Callhook(Name, ...) WSHooks[#Name].Removehook(); __VA_ARGS__; WSHooks[#Name].Installhook();
    phmap::flat_hash_map<std::string_view, Simplehook::Stomphook> WSHooks;

    // Utility.
    std::string Plainaddress(const struct sockaddr *Sockaddr)
    {
        auto Address = std::make_unique<char[]>(INET6_ADDRSTRLEN);
        if (Sockaddr->sa_family == AF_INET6) inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)Sockaddr)->sin6_addr), Address.get(), INET6_ADDRSTRLEN);
        else inet_ntop(AF_INET, &(((struct sockaddr_in *)Sockaddr)->sin_addr), Address.get(), INET6_ADDRSTRLEN);
        return std::string(Address.get());
    }
    uint16_t WSPort(const struct sockaddr *Sockaddr)
    {
        if (Sockaddr->sa_family == AF_INET6) return ntohs(((struct sockaddr_in6 *)Sockaddr)->sin6_port);
        else return ntohs(((struct sockaddr_in *)Sockaddr)->sin_port);
    }

    // Shims
    size_t __stdcall Createsocket(int Family, int Type, int Protocol)
    {
        size_t Result{}; Callhook(socket, Result = socket(Family, Type, Protocol));

        Debugprint(va("Creating a %s socket for %s over %s protocol (0x%X)",
        [&]() { if (Family == AF_INET) return "IPv4"s; if (Family == AF_INET6) return "IPv6"s; if (Family == AF_IPX) return "IPX "s; return va("%u", Family); }().c_str(),
        [&]() { if (Type == SOCK_STREAM) return "streaming"s; if (Type == SOCK_DGRAM) return "datagrams"s; return va("%u", Type); }().c_str(),
        [&]()
        {
            switch (Protocol)
            {
                case IPPROTO_IP: return "Any"s; case IPPROTO_ICMP: return "ICMP"s; case IPPROTO_TCP: return "TCP"s; case IPPROTO_UDP: return "UDP"s;
                case /*NSPROTO_IPX*/ 1000: return "IPX"s; case /*NSPROTO_SPX*/ 1256: return "SPX"s; case /*NSPROTO_SPXII*/ 1257: return "SPX2"s;
                default: return va("%u", Protocol);
            }
        }().c_str(),
        Result));

        return Result;
    }
    int __stdcall Connectsocket(size_t Socket, struct sockaddr *Name, int Namelength)
    {
        int Result{};
        const auto Readable = Plainaddress(Name);
        if (!Localnetworking::isAddressproxied(Readable))
        {
            Callhook(connect, Result = connect(Socket, Name, Namelength));
            Debugprint(va("Connecting (0x%X) to %s:%u - %s", Socket, Readable.c_str(), WSPort(Name), Result ? "FAILED" : "SUCCESS"));
        }
        else
        {
            const auto Proxyport = Localnetworking::getProxyport(Readable);

            SOCKADDR_IN Client{};
            Client.sin_family = AF_INET;
            Client.sin_port = htons(Proxyport);
            Client.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
            bind(Socket, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));

            SOCKADDR_IN Server{};
            Server.sin_family = AF_INET;
            Server.sin_port = htons(4200);
            Server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

            Debugprint(va("Proxying connect (0x%X) to %s:%u", Socket, "TODO", WSPort(Name)));
            Callhook(connect, Result = connect(Socket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)));
        }

        return Result;
    }
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, struct sockaddr *To, int Tolength)
    {
        int Result{};
        const auto Readable = Plainaddress(To);
        if (!Localnetworking::isAddressproxied(Readable))
        {
            Callhook(sendto, Result = sendto(Socket, Buffer, Length, Flags, To, Tolength));
        }
        else
        {
            SOCKADDR_IN Client{};
            int Clientsize{ sizeof(SOCKADDR_IN) };
            getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);
            Localnetworking::Associateport(Readable, Client.sin_port);

            SOCKADDR_IN Server{};
            Server.sin_family = AF_INET;
            Server.sin_port = htons(4201);
            Server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

            Debugprint(va("(0x%X)Sending a packet from proxyport %u", Socket, ntohs(Client.sin_port)));
            Callhook(sendto, Result = sendto(Socket, Buffer, Length, Flags, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)));
        }

        return Result;
    }
    int __stdcall Getaddrinfo(const char *Nodename, const char *Servicename, ADDRINFOA *Hints, ADDRINFOA **Result)
    {
        return 0;
    }
    int __stdcall Closesocket(size_t Socket)
    {
        return 0;
    }

    struct hostent *__stdcall Gethostbyname(const char *Hostname)
    {
        Debugprint(va("Performing lookup for \"%s\"", Hostname));
        hostent *Result{};

        if (!Localnetworking::isHostproxied(Hostname))
        {
            Callhook(gethostbyname, Result = gethostbyname(Hostname));
        }
        else
        {
            const auto Proxy = Localnetworking::getProxyaddress(Hostname);
            Callhook(gethostbyname, Result = gethostbyname("localhost"));
            Result->h_name = const_cast<char *>(Hostname);
            Result->h_addrtype = AF_INET;
            *Result->h_aliases = NULL;
            Result->h_length = 4;

            ((in_addr *)Result->h_addr_list[0])->S_un.S_addr = inet_addr(Proxy.c_str());
            Result->h_addr_list[1] = NULL;
        }

        return Result;
    }
    struct hostent *__stdcall Gethostbyaddr(const char *Address, int Addresslength, int Addresstype)
    {
        return nullptr;
    }
}

// Hook the front-facing APIs.
void InstallWinsock()
{
    auto Getexport = [](std::string_view Name) -> void *
    {
        auto Address = GetProcAddress(GetModuleHandleA("wsock32.dll"), Name.data());
        if (!Address) GetProcAddress(GetModuleHandleA("WS2_32.dll"), Name.data());
        return Address;
    };
    auto Hook = [&](std::string_view Name, void *Target)
    {
        Winsock::WSHooks[Name].Installhook(Getexport(Name), Target);
    };

    Hook("gethostbyname", Winsock::Gethostbyname);
    Hook("connect", Winsock::Connectsocket);
    Hook("socket", Winsock::Createsocket);
    Hook("sendto", Winsock::Sendto);
}
