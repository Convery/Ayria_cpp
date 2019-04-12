/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#pragma once
#include <cstdint>
#include <string_view>

namespace Localnetworking
{
    // Initialize the server backends on port 4200 and 4201 respectively.
    void Createbackend(uint16_t TCPPort = 4200, uint16_t UDPPort = 4201);

    // Check if we are going to proxy any connection-property.
    bool isAddressproxied(std::string_view Address);
    bool isHostproxied(std::string_view Hostname);

    // Create or fetch a unique property for the connection.
    std::string getProxyaddress(std::string_view Hostname);
    uint16_t getProxyport(std::string_view Address);

    // Associate a port with an address.
    void Associateport(std::string_view Address, uint16_t Port);
    std::string Addressfromport(uint16_t Port);
}
