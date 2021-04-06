/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#pragma once
#include "Stdinclude.hpp"
#include "Datatypes/Datatypes.hpp"

namespace Steam
{
    #pragma pack(push, 1)
    struct Globalstate_t
    {
        SteamID_t XUID { 0x1100001DEADC0DEULL };
        uint32_t ApplicationID{};
        char8_t Username[20];

        uint32_t Startuptime{ uint32_t(uint32_t(time(NULL))) };
        std::unique_ptr<std::string> Installpath;
        std::unique_ptr<std::string> Locale;
        std::unique_ptr<std::string> Clan;
    };
    extern Globalstate_t Global;
    #pragma pack(pop)

    // A Steam interface is a class that proxies calls to their backend.
    // As such we can create a generic interface with just callbacks.
    using Interface_t = struct { void *VTABLE[80]; };

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
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t *Interface);
    Interface_t **Fetchinterface(std::string_view Name);
    Interface_t **Fetchinterface(Interfacetype_t Type);
    bool Scanforinterfaces(std::string_view Filename);
    size_t Getinterfaceversion(Interfacetype_t Type);
    void Initializeinterfaces();

    // Block and wait for Steams IPC initialization event as some games need it.
    // Also redirect module lookups for legacy compatibility.
    DWORD __stdcall InitializeIPC(void *);
    void Redirectmodulehandle();

    // Asynchronous tasks.
    namespace Callbacks
    {
        void Completerequest(uint64_t RequestID, int32_t Callbacktype, void *Databuffer);
        void Registercallback(void *Callback, int32_t Callbacktype);
        uint64_t Createrequest();
        void Runcallbacks();
    }

    using namespace Callbacks;
}
