/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-11
    License: MIT
*/

#pragma once
#include "Stdinclude.hpp"

namespace Localnetworking
{
    extern uint16_t BackendTCPport, BackendUDPport;

    // Initialize the server backends, only TCP and UDP for now.
    void Createbackend(uint16_t TCPPort = 4200, uint16_t UDPPort = 4201);

    // Resolve the address and associated ports, or create a new address if needed.
    void Associateport(std::string_view Address, uint16_t Port);
    std::string getAddress(std::string_view Hostname);
    std::string_view getAddress(uint16_t Senderport);
    bool isProxiedhost(std::string_view Hostname);
}
