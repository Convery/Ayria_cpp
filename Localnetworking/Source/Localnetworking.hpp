/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-03
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

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
