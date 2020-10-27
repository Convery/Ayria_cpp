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
    void(__cdecl *addConsolemessage)(const void *String, unsigned int Length, unsigned int Colour);
    void(__cdecl *addConsolecommand)(const void *Name, unsigned int Length, const void *Callback);
    void(__cdecl *execCommandline)(const void *String, unsigned int Length);

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
    #define Parse(x) x = Object.value(#x, decltype(x)());
    #define Dump(x) Object[#x] = x;
    struct Playerpart_t
    {
        uint32_t PlayerID;
        std::u8string Playername;
        nlohmann::json::object_t Gamedata;

        Playerpart_t() = default;
        Playerpart_t(const nlohmann::json &Object)
        {
            Parse(PlayerID); Parse(Playername); Parse(Gamedata);
        }
        Playerpart_t(std::string_view JSONString) : Playerpart_t(ParseJSON(JSONString)) {}

        nlohmann::json toJSON()
        {
            auto Object = nlohmann::json::object();
            Dump(PlayerID); Dump(Playername); Dump(Gamedata);
            return Object;
        }
    };
    struct Steampart_t
    {
        uint16_t Gameport, Queryport, Spectatorport;

        uint32_t ApplicationID, IPAddress, Protocol;
        uint32_t Versionint, Serverflags, Spawncount;
        uint32_t Currentplayers, Botplayers, Maxplayers;

        std::u8string Productname, Productdesc;
        std::u8string Servername, Spectatorname;
        std::u8string Region, Versionstring, Infostring;
        std::u8string Mapname, Gametype, Gametags, Gamemod;

        bool isLAN, isPrivate, isDedicated;

        Steampart_t() = default;
        Steampart_t(const nlohmann::json &Object)
        {
            Parse(Gameport); Parse(Queryport); Parse(Spectatorport);

            Parse(ApplicationID); Parse(IPAddress); Parse(Protocol);
            Parse(Versionint); Parse(Serverflags); Parse(Spawncount);
            Parse(Currentplayers); Parse(Botplayers); Parse(Maxplayers);

            Parse(Productname); Parse(Productdesc);
            Parse(Servername); Parse(Spectatorname);
            Parse(Region); Parse(Versionstring); Parse(Infostring);
            Parse(Mapname); Parse(Gametype); Parse(Gametags); Parse(Gamemod);

            Parse(isLAN); Parse(isPrivate); Parse(isDedicated);
        }
        Steampart_t(std::string_view JSONString) : Steampart_t(ParseJSON(JSONString)) {}

        nlohmann::json toJSON()
        {
            auto Object = nlohmann::json::object();
            Dump(Gameport); Dump(Queryport); Dump(Spectatorport);

            Dump(ApplicationID); Dump(IPAddress); Dump(Protocol);
            Dump(Versionint); Dump(Serverflags); Dump(Spawncount);
            Dump(Currentplayers); Dump(Botplayers); Dump(Maxplayers);

            Dump(Productname); Dump(Productdesc);
            Dump(Servername); Dump(Spectatorname);
            Dump(Region); Dump(Versionstring); Dump(Infostring);
            Dump(Mapname); Dump(Gametype); Dump(Gametags); Dump(Gamemod);

            Dump(isLAN); Dump(isPrivate); Dump(isDedicated);
            return Object;
        }
    };
    struct Session_t
    {
        uint64_t HostID;
        Steampart_t Steam;
        std::u8string Hostname;
        std::u8string Hostlocale;
        std::vector<Playerpart_t> Players;

        Session_t() = default;
        Session_t(const nlohmann::json &Object)
        {
            Parse(Hostlocale); Parse(Hostname); Parse(HostID);

            const auto Sessiondata = ParseJSON(Object.value("Sessiondata", std::string()));
            Steam = Steampart_t(Sessiondata.value("Steam", nlohmann::json::object()));
            for (const auto &Player : Sessiondata.value("Players", nlohmann::json::array()))
                Players.push_back(Playerpart_t(Player));
        }
        Session_t(std::string_view JSONString) : Session_t(ParseJSON(JSONString)) {}

        nlohmann::json toJSON()
        {
            auto Object = nlohmann::json::object();
            Object["Steam"] = Steam.toJSON();
            for (auto &Player : Players)
                Object["Players"] += Player.toJSON();

            return Object;
        }
    };
    #undef Parse
    #undef Dump

    // Manage the sessions we know of, updates in the background.
    std::vector<Session_t *> getLANSessions();
    std::vector<Session_t *> getWANSessions();
    Session_t *getLocalsession();

    // Notify the backend about our session changing.
    void Invalidatesession();
    void doFrame();
}
