/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#pragma once
#include "../Stdinclude.hpp"

namespace Steam
{
    // Keep the global state together.
    struct Globalstate_t
    {
        uint64_t UserID;
        std::string Path;
        std::string Username;
        std::string Language;
        uint32_t ApplicationID;
        uint64_t Startuptimestamp;
    };
    extern Globalstate_t Global;

    // A Steam interface is a class that proxies calls to their backend.
    // As such we can create a generic interface with just callbacks.
    using Interface_t = struct { void *VTABLE[70]; };

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

    // Block and wait for Steams IPC initialization event as some games need it.
    // Also redirect module lookups for legacy compatibility.
    void Redirectmodulehandle();
    void InitializeIPC();
}
