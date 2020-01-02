/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-03
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Localnetworking
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
