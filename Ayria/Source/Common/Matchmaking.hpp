/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Matchmaking
{
    struct Session_t
    {
        // Internal use.
        uint32_t Lastmessage;

        nlohmann::json Hostinfo;
        nlohmann::json Gameinfo;
        nlohmann::json Playerdata;
        nlohmann::json Sessiondata;
    };
    struct Localsession_t : Session_t
    {
        bool Active;
    };

    // Fetch all known sessions on the LAN.
    std::vector<std::shared_ptr<Session_t>> getNetworksessions();
    Localsession_t *getLocalsession();

    // Register handlers and set up session.
    void Initialize();

    // Announce the clients info if active.
    void doFrame();

    // Add API handlers.
    inline std::string __cdecl getLocalsession(const char *)
    {
        auto Object = nlohmann::json::object();
        auto Session = getLocalsession();
        static std::string Result;

        Object["Hostinfo"] = Session->Hostinfo;
        Object["Gameinfo"] = Session->Gameinfo;
        Object["Playerdata"] = Session->Playerdata;
        Object["Sessiondata"] = Session->Sessiondata;

        const auto Plaintext = Object.dump(4);
        if (Result != Plaintext) Result = Plaintext;
        return Result.c_str();
    }
    inline std::string __cdecl getNetworksessions(const char *)
    {
        auto Array = nlohmann::json::array();
        static std::string Result;

        for (const auto &Session : getNetworksessions())
        {
            auto Object = nlohmann::json::object();
            Object["Hostinfo"] = Session->Hostinfo;
            Object["Gameinfo"] = Session->Gameinfo;
            Object["Playerdata"] = Session->Playerdata;
            Object["Sessiondata"] = Session->Sessiondata;

            Array += Object;
        }

        Result = Array.dump(4);
        return Result.c_str();
    }
    inline std::string __cdecl Hostupdate(const char *JSONString)
    {
        auto Session = getLocalsession();
        Session->Hostinfo.update(ParseJSON(JSONString));
        return getLocalsession(nullptr);
    }
    inline std::string __cdecl Gameupdate(const char *JSONString)
    {
        auto Session = getLocalsession();
        Session->Gameinfo.update(ParseJSON(JSONString));
        return getLocalsession(nullptr);
    }
    inline std::string __cdecl Playerupdate(const char *JSONString)
    {
        auto Session = getLocalsession();
        Session->Playerdata.update(ParseJSON(JSONString));
        return getLocalsession(nullptr);
    }
    inline std::string __cdecl Sessionupdate(const char *JSONString)
    {
        auto Session = getLocalsession();
        Session->Sessiondata.update(ParseJSON(JSONString));
        return getLocalsession(nullptr);
    }

    inline std::string __cdecl Terminatesession(const char *)
    {
        getLocalsession()->Active = false;
        return "{}";
    }
    inline std::string __cdecl Createsession(const char *JSONString)
    {
        const auto Config = ParseJSON(JSONString);
        auto Session = getLocalsession();

        Session->Sessiondata = Config.value("Sessiondata", nlohmann::json::object());
        Session->Playerdata = Config.value("Playerdata", nlohmann::json::array());
        Session->Gameinfo = Config.value("Gameinfo", nlohmann::json::object());
        Session->Hostinfo = Config.value("Hostinfo", nlohmann::json::object());
        Session->Active = true;

        return getLocalsession(nullptr);
    }

    inline void API_Initialize()
    {
        Initialize();

        API::Registerhandler_Matchmake("Hostupdate", Hostupdate);
        API::Registerhandler_Matchmake("Gameupdate", Gameupdate);
        API::Registerhandler_Matchmake("Playerupdate", Playerupdate);
        API::Registerhandler_Matchmake("Sessionupdate", Sessionupdate);
        API::Registerhandler_Matchmake("getLocalsession", getLocalsession);
        API::Registerhandler_Matchmake("getNetworksessions", getNetworksessions);

        API::Registerhandler_Matchmake("Createsession", Createsession);
        API::Registerhandler_Matchmake("Terminatesession", Terminatesession);
    }
}
