/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-24
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Keep global data together, but remember to keep natural alignment for pointers!
#pragma pack(push, 1)
struct alignas(64) Globalstate_t
{
    // Helper to initialize the pointers in the same region.
    static inline std::pmr::monotonic_buffer_resource Internal{ 196 };
    template <typename T, typename ...Args> static auto Allocate(Args&& ...va)
    { auto Buffer = Internal.allocate(sizeof(T)); return new (Buffer) T(std::forward<Args>(va)...); }

    uint32_t InternalIP{}, ExternalIP{};    // From LANNetworking and STUN.
    uint32_t GameID{}, ModID{};             // Set through Platform-wrapper.
    // 16 bytes.

    // 25519-curve cryptography, random or derived from authentication. Primary account identifier.
    std::unique_ptr<std::array<uint8_t, 32>> SigningkeyPublic{ Allocate<std::array<uint8_t, 32>>() };
    std::unique_ptr<std::array<uint8_t, 64>> SigningkeyPrivate{ Allocate<std::array<uint8_t, 64>>() };
    std::unique_ptr<std::array<uint8_t, 32>> EncryptionkeyPrivate{ Allocate<std::array<uint8_t, 32>>() };
    // 28 / 40 bytes.

    // Rarely used (for now), but good to have in the future.
    std::unique_ptr<std::pmr::u8string> Username{ Allocate<std::pmr::u8string>(&Internal) };
    // 32 / 48 bytes.

    // Internal settings, need packing (line 11) or we'll get 6 bytes of padding.
    struct
    {
        uint16_t
            // Application.
            enableExternalconsole : 1,
            enableIATHooking : 1,
            enableFileshare : 1,
            isAuthenticated : 1,
            modifiedConfig : 1,
            noNetworking : 1,

            // Social state.
            isPrivate : 1,
            isAway : 1,

            // Matchmaking state.
            isHosting : 1,
            isIngame : 1,

            // 6 bits available.
            END : 1;
    } Settings{};
    // 34 / 50 bytes.

    // ************************************
    // 30 / 14 bytes available for use here.
    // ************************************

    // Helpers for access to the members.
    std::string getLongID() const { return Base58::Encode(*SigningkeyPublic); }
    uint32_t getShortID() const { return Hash::WW32(*SigningkeyPublic); }

    // Struct alignment causes intentional padding.
    #pragma warning(suppress : 4324)
};
#pragma pack(pop)

// Let's ensure that modifications do not extend the global state too much.
static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
extern Globalstate_t Global;

// Project includes.
#include "Backend\AABackend.hpp"
#include "Services\AAServices.hpp"
