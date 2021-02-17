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
#pragma pack(push, 1)
struct Globalstate_t    // 51 bytes.
{
    uint32_t UserID;
    char8_t Locale[12];
    char8_t Username[24];

    RSA *Cryptokeys;

    union
    {
        uint8_t Full;
        struct
        {
            uint8_t
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
                isAdmin : 1,
                isModerator : 1;
        };
    } Privilegeflags;
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
};
extern Globalstate_t Global;
#pragma pack(pop)
