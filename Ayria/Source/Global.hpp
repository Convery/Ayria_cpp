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
    std::unique_ptr<std::wstring> Username, Locale;
    RSA *Cryptokeys;
    bool isOnline;

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
};
extern Globalstate_t Global;
