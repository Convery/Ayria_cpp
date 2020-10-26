/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Helper to create 'classes' when needed.
using Fakeclass_t = struct { void *VTABLE[70]; };

// Exports as struct for easier plugin initialization.
struct Ayriamodule_t
{
    HMODULE Modulehandle{};

    // Create a functionID from the name of the service.
    static uint32_t toFunctionID(const char *Name) { return Hash::FNV1_32(Name); };

    // using Callback_t = void(__cdecl *)(int Argc, wchar_t **Argv);
    void(__cdecl *addConsolemessage)(const wchar_t *String, unsigned int Colour);
    void(__cdecl *addConsolecommand)(const wchar_t *Name, const void *Callback);
    void(__cdecl *execCommandline)(const wchar_t *String);

    // using Messagecallback_t = void(__cdecl *)(const char *JSONString);
    void(__cdecl *addNetworklistener)(uint32_t MessageID, void *Callback);

    // FunctionID = FNV1_32("Service name"); ID 0 / Invalid = List all available.
    const char *(__cdecl *API_Client)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Social)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Network)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Matchmake)(uint32_t FunctionID, const char *JSONString);
    const char *(__cdecl *API_Fileshare)(uint32_t FunctionID, const char *JSONString);

    Ayriamodule_t()
    {
        #if defined(NDEBUG)
        Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll");
        #else
        Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64d.dll" : "./Ayria/Ayria32d.dll");
        #endif
        assert(Modulehandle);

        #define Import(x) x = (decltype(x))GetProcAddress(Modulehandle, #x);
        Import(addConsolemessage);
        Import(addConsolecommand);
        Import(execCommandline);
        Import(addNetworklistener);
        Import(API_Client);
        Import(API_Social);
        Import(API_Network);
        Import(API_Matchmake);
        Import(API_Fileshare);
        #undef Import
    }
};
extern Ayriamodule_t Ayria;

// Common functionality.
namespace Matchmaking
{
    struct Session_t { uint32_t HostID; nlohmann::json Hostinfo, Gameinfo, Playerdata, Sessiondata; };
    std::vector<Session_t> *getNetworkservers();
    Session_t *getLocalsession();

    void doFrame();
    void Update();
}
