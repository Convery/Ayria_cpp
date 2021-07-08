/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#pragma once
#include "Assets/Assets.hpp"
#include "Backend/Backend.hpp"
#include "Services/AAServices.hpp"
#include "Subsystems/Subsystems.hpp"

// Global system information, should be <= 64 bytes on x64.
struct Globalstate_t
{
    uint32_t InternalIP{}, ExternalIP{};
    uint32_t AccountID{ 0xDEADC0DE };
    uint32_t GameID{}, ModID{};
    uint32_t Sharedkeyhash{};
    // 24 bytes.

    char8_t Username[22]{ u8"Unknown" };
    union
    {
        uint16_t Raw;
        struct
        {
            uint16_t
                // Application.
                enableExternalconsole : 1,
                enableIATHooking : 1,
                enableFileshare : 1,
                modifiedConfig : 1,
                noNetworking : 1,

                // Social state.
                isPrivate : 1,
                isAway : 1,

                // Matchmaking state.
                isHosting : 1,
                isIngame : 1,

                END : 1;
        };
    } Settings;
    // 48 bytes.

    std::array<uint8_t, 32> *Publickey{};
    std::array<uint8_t, 32> *Privatekey{};
    // 56 / 64 bytes.
};

// Let's not cross a cache-line.
static_assert(sizeof(Globalstate_t) <= 64);
extern Globalstate_t Global;
