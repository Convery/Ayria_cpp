/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Helper to create 'classes' when needed.
using Fakeclass_t = struct { void *VTABLE[70]; };

// Common functionality for interfacing with Ayria.
#define Parse(x) x = Object.value(#x, decltype(x)());
#define Dump(x) Object[#x] = x;
namespace Matchmaking
{
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
        nlohmann::json::object_t Keyvalues;

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
            Parse(Keyvalues);
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
            Dump(Keyvalues);
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

    // Manage the sessions we know of, updates in the background.
    std::vector<Session_t *> getLANSessions();
    std::vector<Session_t *> getWANSessions();
    Session_t *getLocalsession();

    // Notify the backend about our session changing.
    void Invalidatesession();
    void doFrame();
}
namespace Social
{
    using Relationflags_t = union
    {
        uint32_t Raw;
        struct
        {
            uint32_t
                isFriend : 1,
                isBlocked : 1,
                isFollower : 1,
                isGroupmember : 1,
                AYA_RESERVED1 : 1,
                AYA_RESERVED2 : 1,
                AYA_RESERVED3 : 1;
        };
    };
    struct Relation_t
    {
        uint32_t UserID;
        Relationflags_t Flags;
        std::u8string Username;

        Relation_t() = default;
        Relation_t(const nlohmann::json &Object)
        {
            Parse(UserID); Parse(Username);
            Flags.Raw = Object.value("Flags", uint32_t());
        }
        Relation_t(std::string_view JSONString) : Relation_t(ParseJSON(JSONString)) {}
    };

    // Manage the relations we know of, updates in the background.
    bool addRelation(uint32_t UserID, std::u8string Username = {}, Relationflags_t Flags = {});
    void removeRelation(uint32_t UserID, const std::u8string &Username = u8"");
    void blockUser(uint32_t UserID, const std::u8string &Username = u8"");

    using Userinfo_t = std::pair<uint32_t, std::u8string>;
    Userinfo_t getUser(uint32_t UserID, std::u8string Username = {});
    std::vector<Relation_t *> getFriends();
    void doFrame();
}
#undef Parse
#undef Dump
