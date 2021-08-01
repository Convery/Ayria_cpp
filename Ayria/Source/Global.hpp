/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-24
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Keep global data together, but remember to keep natural alignment for pointers!
#pragma pack(push, 1)
struct AccountID_t
{
    uint32_t AyriaID{}; // Official ID for identification.
    uint32_t UserID{};  // WW32 hash of the public signing key.

    AccountID_t() = default;
    AccountID_t(uint64_t Raw)
    {
        AyriaID = (Raw >> 32) & 0xFFFFFFFF;
        UserID = Raw & 0xFFFFFFFF;
    }
    operator uint64_t () const
    {
        return { uint64_t(AyriaID) << 32 | UserID };
    }
};
struct Globalstate_t
{
    uint32_t InternalIP{}, ExternalIP{};    // From LANNetworking and STUN.
    uint32_t GameID{}, ModID{};             // Set through Platform-wrapper.
    AccountID_t AccountID{};
    // 24 bytes.

    // Rarely used, but good to have.
    std::unique_ptr<std::u8string> Username{};
    // 28 / 32 bytes.

    // 25519-curve cryptography.
    std::unique_ptr<std::array<uint8_t, 32>> SigningkeyPublic{};    // Shared static key.
    std::unique_ptr<std::array<uint8_t, 64>> SigningkeyPrivate{};   // Derived from authentication.
    std::unique_ptr<std::array<uint8_t, 32>> EncryptionkeyPrivate{};// Transient session-key.
    // 40 / 56 bytes.

    // Internal settings, need packing or we'll get 6 bytes of padding.
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

            // 7 bits available.
            END : 1;
    } Settings{};
};
#pragma pack(pop)

// Let's ensure that modifications do not extend the global state too much.
static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
extern Globalstate_t Global;

// Project includes.
#include "Backend\AABackend.hpp"
