/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Backend/Backend.hpp"
#include "Services/Services.hpp"
#include "Subsystems/Subsystems.hpp"

// Global system information.
struct Globalstate_t
{
    union
    {
        uint64_t AccountID;
        struct
        {
            uint32_t UserID;
            uint8_t Stateflags;
            uint8_t Accountflags;
            uint16_t Creationdate;
        };
    };

    RSA *Cryptokeys;
    char8_t Username[24], Locale[10];

    struct
    {
        uint8_t
            isOnline : 1,
            useIAThooks : 1,
            enableExternalconsole : 1;
    };
};
extern Globalstate_t Global;
