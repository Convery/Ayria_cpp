/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-23
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Steam/Datatypes/Datatypes.hpp"

// A lot of interfaces will have unreferenced parameters, so ignore warnings.
#pragma warning(disable: 4100)

namespace Steam
{
    // Keep global data together, but remember to keep natural alignment for pointers!
    #pragma pack(push, 1)
    struct Globalstate_t
    {
        // Helper to initialize the pointers in the same region.
        static inline std::pmr::monotonic_buffer_resource Internal{ 256 };
        template <typename T, typename ...Args> static auto Allocate(Args&& ...va)
        { auto Buffer = Internal.allocate(sizeof(T)); return new (Buffer) T(std::forward<Args>(va)...); }

        SteamID_t XUID { 0x1100001DEADC0DEULL };
        uint32_t AppID{}, ModID{};

        std::unique_ptr<std::pmr::u8string> Installpath{ Allocate<std::pmr::u8string>(&Internal) };
        std::unique_ptr<std::pmr::u8string> Username{ Allocate<std::pmr::u8string>(&Internal) };
        std::unique_ptr<std::pmr::string> Locale{ Allocate<std::pmr::string>(&Internal) };
        std::unique_ptr<std::pmr::string> LongID{ Allocate<std::pmr::string>(&Internal) };

        // Just copy Ayrias settings, might be useful later.
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

        // Simplify access.
        std::string getLongID() const
        {
            return LongID->c_str();
        }
    };
    #pragma pack(pop)

    // Let's ensure that modifications do not extend the global state too much.
    static_assert(sizeof(Globalstate_t) <= 64, "Do not cross cache lines with Globalstate_t!");
    extern Globalstate_t Global;

    // Steam uses 32-bit IDs for the account, so we need to do some conversions.
    SteamID_t toSteamID(const std::string &LongID);
    bool compareSteamID(SteamID_t A, SteamID_t B);
    std::string fromSteamID(SteamID_t AccountID);

    // A Steam interface is a class that proxies calls to their backend.
    // As such we can create a generic interface with just callbacks.
    template <size_t N = 1> struct Interface_t { void *VTABLE[N]; };

    // The types of interfaces provided as of writing.
    enum class Interfacetype_t
    {
        UGC,
        APPS,
        USER,
        HTTP,
        MUSIC,
        UTILS,
        VIDEO,
        CLIENT,
        FRIENDS,
        APPLIST,
        INVENTORY,
        USERSTATS,
        CONTROLLER,
        GAMESERVER,
        GAMESEARCH,
        NETWORKING,
        HTMLSURFACE,
        MUSICREMOTE,
        SCREENSHOTS,
        MATCHMAKING,
        REMOTESTORAGE,
        CONTENTSERVER,
        UNIFIEDMESSAGES,
        GAMESERVERSTATS,
        MATCHMAKINGSERVERS,
        MASTERSERVERUPDATER,
        MAX,
        INVALID
    };

    // Return a specific version of the interface by name or the latest by their category / type.
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t<> *Interface);
    Interface_t<> **Fetchinterface(std::string_view Name);
    Interface_t<> **Fetchinterface(Interfacetype_t Type);
    bool Scanforinterfaces(std::string_view Filename);
    size_t Getinterfaceversion(Interfacetype_t Type);
    void Initializeinterfaces();

    // Block and wait for Steams IPC initialization event as some games need it.
    // Also redirect module lookups for legacy compatibility.
    DWORD __stdcall InitializeIPC(void *);
    void Redirectmodulehandle();

    // Unified creation and initialization for the SteamDB.
    sqlite::Database_t Database();

    // Asynchronous tasks.
    namespace Tasks
    {
        void Completerequest(uint64_t RequestID, ECallbackType Callbacktype, void *Databuffer);
        void Completerequest(uint64_t RequestID, int32_t Callbacktype, void *Databuffer);
        void Registercallback(void *Callback, int32_t Callbacktype);
        uint64_t Createrequest();
        void Runcallbacks();
    }

    // Server interaction, contained in SteamGameserver.cpp
    namespace Gameserver
    {
        bool Start(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t unServerFlags, const char *pchGameDir, const char *pchVersion);
        bool Start(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort, uint16_t usQueryPort, uint32_t unServerFlags, AppID_t nAppID, const char *pchVersion);
        void Initialize();
        bool Terminate();
        bool isActive();
    }
}
