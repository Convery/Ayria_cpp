/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-24
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Keep global data together, but remember to keep natural alignment for pointers!
#pragma pack(push, 1)

class Globalstate_t
{
    // Helper to initialize the pointers in the same region.
    static inline std::pmr::monotonic_buffer_resource Internal{ 196 };
    template <typename T, typename ...Args> static auto Allocate(Args&& ...va)
    { auto Buffer = Internal.allocate(sizeof(T)); return new (Buffer) T(std::forward<Args>(va)...); }

    public:
    uint32_t InternalIP{}, ExternalIP{};    // From LANNetworking and STUN, big endian.
    uint32_t GameID{}, ModID{};             // Set through Platform-wrapper.
    // 16 bytes.

    // Cryptokeys are random or derived from authentication. Primary user identifier.
    std::unique_ptr<qDSA::Key_t> Privatekey{ Allocate<qDSA::Key_t>() };
    std::unique_ptr<qDSA::Key_t> Publickey{ Allocate<qDSA::Key_t>() };
    // 24 / 32 bytes.

    // Rarely used (for now), but good to have in the future.
    std::unique_ptr<std::pmr::u8string> Username{ Allocate<std::pmr::u8string>(&Internal) };
    // 28 / 40 bytes.

    // Internal settings, need packing (line 11) or we'll get 6 bytes of padding.
    union
    {
        uint16_t Raw;
        struct
        {
            uint16_t
                // Application internal.
                enableExternalconsole : 1,
                enableIATHooking : 1,
                enableFileshare : 1,
                modifiedConfig : 1,
                noNetworking : 1,
                pruneDB : 1,

                // Social state.
                isPrivate : 1,
                isAway : 1,

                // Matchmaking state.
                isHosting : 1,
                isIngame : 1,

                // 6 bits available.
                PLACEHOLDER : 6;
        };
    } Settings{};
    // 30 / 42 bytes.

    // ************************************
    // 34 / 22 bytes available for use here.
    // ************************************

    // Helpers for access to the members.
    uint64_t getShortID() const { return Hash::WW64(getLongID()); }
    const std::string &getLongID() const
    {
        static auto LongID = Base58::Encode(*Publickey);
        static auto Lasthash = Hash::WW32(*Publickey);

        const auto Currenthash = Hash::WW32(*Publickey);
        if (Currenthash != Lasthash) [[unlikely]]
        {
            LongID = Base58::Encode(*Publickey);
            Lasthash = Currenthash;
        }

        return LongID;
    }
};
#pragma pack(pop)

// Let's ensure that modifications do not extend the global state too much.
static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
extern Globalstate_t Global;

// Project includes.
#include "Backend\AABackend.hpp"
#include "Services\AAServices.hpp"
