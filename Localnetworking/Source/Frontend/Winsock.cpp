/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#include "../Stdinclude.hpp"
#include <WinSock2.h>
using namespace std::string_literals;

namespace Winsock
{
    #define Callhook(Name, ...) WSHooks[#Name].Removehook(); __VA_ARGS__; WSHooks[#Name].Installhook();
    struct Socketinfo_t { size_t Nativesocket; int Family, Type, Protocol; };
    phmap::flat_hash_map<std::string_view, Simplehook::Stomphook> WSHooks;
    std::vector<Socketinfo_t> Sockets;

    size_t __stdcall Createsocket(int Family, int Type, int Protocol)
    {
        auto Familystring = [&]() { if (Family == AF_INET) return "IPv4"s; if (Family == AF_INET6) return "IPv6"s; if (Family == AF_IPX) return "IPX "s; return va("%u", Family); }();
        auto Typestring = [&]() { if (Type == SOCK_STREAM) return "Stream"s; if (Type == SOCK_DGRAM) return "Datagram"s; return va("%u", Type); }();
        auto Protocolstring = [&]()
        {
            switch (Protocol)
            {
                case IPPROTO_IP: return "Any"s; case IPPROTO_ICMP: return "ICMP"s; case IPPROTO_TCP: return "TCP"s; case IPPROTO_UDP: return "UDP"s;
                case /*NSPROTO_IPX*/ 1000: return "IPX"s; case /*NSPROTO_SPX*/ 1256: return "SPX"s; case /*NSPROTO_SPXII*/ 1257: return "SPX2"s;
                default: return va("%u", Protocol);
            }
        }();

        Debugprint(va("Creating %s-socket over %s for protocol %s", Typestring.c_str(), Familystring.c_str(), Protocolstring.c_str()));
        size_t Result{}; Callhook(socket, Result = socket(Family, Type, Protocol));
        if (Result) Sockets.push_back({ Result, Family, Type, Protocol });
        return Result;
    }
    int __stdcall Connectsocket(size_t Socket, const struct sockaddr *Name, int Namelength)
    {
        // Check with the backend if this socket or name is claimed.
        Traceprint();
        return 0;
    }
    int __stdcall Receivefrom(size_t Socket, char *Buffer, int Length, int Flags, struct sockaddr *From, int *Fromlength)
    {
        return 0;
    }
    int __stdcall Sendto(size_t Socket, const char *Buffer, int Length, int Flags, const struct sockaddr *To, int Tolength)
    {
        return 0;
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
        return nullptr;
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

    Hook("connect", Winsock::Connectsocket);
    Hook("socket", Winsock::Createsocket);
}


