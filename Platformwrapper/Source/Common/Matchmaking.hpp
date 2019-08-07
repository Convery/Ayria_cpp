/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-08-04
    License: MIT

    LAN Matchmaking-broadcasts over UDP.
*/

#pragma once
#include <Stdinclude.hpp>

namespace Matchmaking
{
    struct Server_t
    {
        uint64_t SessionID;
        uint64_t Lastmessage;
        std::string Hostname;
        struct
        {
            uint32_t Terminated : 1;
            uint32_t Publicgame : 1;
            uint32_t Fullserver : 1;
        } Hostflags;
        nlohmann::json Gamedata{};

        template<typename T> void Set(std::string &&Property, T Value) { Gamedata[Property] = Value; }
        template<typename T> T Get(std::string &&Property, T Defaultvalue) { return Gamedata.value(Property, Defaultvalue); }
    };

    // Fetches the local server, or creates a new one.
    std::shared_ptr<Server_t> Localserver();

    // A list of all currently active servers on the net.
    std::vector<std::shared_ptr<Server_t>> Externalservers();

    // Listen for new broadcasted messages.
    std::thread Startlistening(uint32_t GameID);

    // Notify about our properties changing.
    void Broadcastupdate();
}
