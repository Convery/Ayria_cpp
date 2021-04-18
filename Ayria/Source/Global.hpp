/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#pragma once
#include "Services.hpp"
#include "Backend/Backend.hpp"
#include "Subsystems/Subsystems.hpp"

// Global system information, 56 bytes on x64.
struct Globalstate_t
{
    uint32_t GameID;
    uint32_t ClientID;
    uint32_t Publicaddress;
    char8_t Username[22];

    union
    {
        uint8_t Full;
        struct
        {
            uint8_t
                isAway : 1,
                isOnline : 1,
                isIngame : 1,
                isPrivate : 1,
                isHosting : 1;
        };
    } Stateflags;
    union
    {
        uint8_t Full;
        struct
        {
            uint8_t
                modifiedConfig : 1,
                enableIATHooking : 1,
                enableExternalconsole : 1;
        };
    } Applicationsettings;

    RSA *Cryptokeys;
    std::string *B64Authticket;
};

// Let's not cross a cache-line.
static_assert(sizeof(Globalstate_t) <= 64);
extern Globalstate_t Global;
