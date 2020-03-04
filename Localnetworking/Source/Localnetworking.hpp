/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-03
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Localnetworking2
{
    extern uint16_t Backendport;

    // Whether or not a plugin claims ownership of this host.
    bool isProxiedhost(std::string_view Hostname);

    // Notify the backend about a client-port that will connect to us.
    void Associateport(std::string_view Hostname, uint16_t Port);

    // A datagram socket, which internally is sent over TCP.
    size_t Createsocket(IServer::Address_t Serveraddress, std::string_view Hostname);

    // Initialize the server backends, only TCP and UDP for now.
    void Createbackend(uint16_t Serverport);

    // Hook the various frontends.
    void Initializewinsock();
}

namespace Localnetworking2
{
    struct Serverinfo_t
    {
        // 240.0.x.y
        uint32_t Proxyaddress;
        std::string Hostname;
        IServer *Instance;
    };
    extern uint16_t Backendport;

    // Resolve the hostname and try to proxy it.
    Serverinfo_t *getProxyserver(std::string_view Hostname);
    Serverinfo_t *getProxyserver(uint32_t Address);

    // Initialize the server backend.
    void Createbackend(uint16_t Serverport);

    // Hook the various frontends.
    void Initializewinsock();

    using Connection_t = std::pair<size_t, size_t>;
    Connection_t Connectserver(size_t Clientsocket, IServer *Serverinstance);
    Connection_t Connectserver(IServer *Serverinstance);
}

namespace Localnetworking
{
    struct Proxyserver_t
    {
        std::string Hostname;
        sockaddr_in Address;
        IServer *Instance;
    };

    // Resolve the hostname and return a unique address if proxied.
    Proxyserver_t *getProxyserver(std::string_view Hostname);
    Proxyserver_t *getProxyserver(sockaddr_in *Hostname);

    // Associate a socket with a server.
    void Connectserver(size_t Clientsocket, IServer *Serverinstance);

    // Hook the various frontends.
    void Initializewinsock();
}
