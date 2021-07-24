/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-24
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Backend\AABackend.hpp"

// Keep global data together.
struct Globalstate_t
{
    uint32_t AccountID{ 0xDEADC0DE };
    uint32_t GameID{}, ModID{};
    uint32_t Databasekey{};
    uint32_t InternalIP{};
    uint32_t ExternalIP{};
    // 24 bytes.

    // Rarely used, but good to have.
    std::unique_ptr<std::u8string> Username{};
    // 28 / 32 bytes.

    // 25519-curve cryptography.
    std::unique_ptr<std::array<uint8_t, 32>> SigningkeyPublic{};
    std::unique_ptr<std::array<uint8_t, 64>> SigningkeyPrivate{};
    std::unique_ptr<std::array<uint8_t, 32>> EncryptionkeyPrivate{};
    // 40 / 56 bytes.

    // Internal settings.
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
    } Settings{};
};

// Let's ensure that modifications do not extend the global state too much.
static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
extern Globalstate_t Global;
