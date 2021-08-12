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
    uint32_t KeyID{};   // WW32 hash of the public signing key.

    AccountID_t() = default;
    AccountID_t(uint64_t Raw)
    {
        AyriaID = (Raw >> 32) & 0xFFFFFFFF;
        KeyID = Raw & 0xFFFFFFFF;
    }
    operator uint64_t () const
    {
        return { uint64_t(AyriaID) << 32 | KeyID };
    }
};
struct alignas(64) Globalstate_t
{
    // Helper to initialize the pointers in the same region.
    static inline std::pmr::monotonic_buffer_resource Internal{ 196 };
    template <typename T, typename ...Args> static auto Allocate(Args&& ...va)
    { auto Buffer = Internal.allocate(sizeof(T)); return new (Buffer) T(std::forward<Args>(va)...); }

    uint32_t InternalIP{}, ExternalIP{};    // From LANNetworking and STUN.
    uint32_t GameID{}, ModID{};             // Set through Platform-wrapper.
    AccountID_t AccountID{};
    // 24 bytes.

    // 25519-curve cryptography.
    std::unique_ptr<std::array<uint8_t, 32>> SigningkeyPublic{ Allocate<std::array<uint8_t, 32>>() };     // Shared static key.
    std::unique_ptr<std::array<uint8_t, 64>> SigningkeyPrivate{ Allocate<std::array<uint8_t, 64>>() };    // Derived from authentication.
    std::unique_ptr<std::array<uint8_t, 32>> EncryptionkeyPrivate{ Allocate<std::array<uint8_t, 32>>() }; // Transient session-key.
    // 36 / 48 bytes.

    // Rarely used (for now), but good to have in the future.
    std::unique_ptr<std::pmr::u8string> Username{ Allocate<std::pmr::u8string>(&Internal) };
    // 40 / 56 bytes.

    // Internal settings, need packing (line 11) or we'll get 6 bytes of padding.
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
    // 42 / 58 bytes.

    // ************************************
    // 22 / 6 bytes available for use here.
    // ************************************

    // Struct alignment causes intentional padding.
    #pragma warning(suppress : 4324)
};
#pragma pack(pop)

// Let's ensure that modifications do not extend the global state too much.
static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
extern Globalstate_t Global;

// Project includes.
#include "Backend\AABackend.hpp"
