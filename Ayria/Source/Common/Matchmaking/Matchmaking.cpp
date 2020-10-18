/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Matchmaking
{
    std::unordered_map<uint32_t, std::shared_ptr<Session_t>> Networksessions;
    Session_t Localsession{};

    // Fetch all known sessions on the LAN.
    std::vector<std::shared_ptr<Session_t>> getNetworksessions()
    {
        std::vector<std::shared_ptr<Session_t>> Result;
        Result.reserve(Networksessions.size());

        for (const auto &[Key, Value] : Networksessions)
            Result.emplace_back(Value);

        return Result;
    }
    Session_t *getLocalsession()
    {
        return &Localsession;
    }

    // Internal helpers.
    static void Sendserverinfo()
    {
        const auto String = getLocalsession(nullptr);
        Backend::Sendmessage(Hash::FNV1_32("Serverupdate"), String);
    }
    static void __cdecl Updatehandler(uint32_t NodeID, const char *JSONString)
    {
        if (!Networksessions.contains(NodeID))
            Networksessions[NodeID] = std::make_shared<Session_t>(Session_t());

        // Session->Sessiondata.update(ParseJSON(JSONString));

        const auto Object = ParseJSON(JSONString);
        auto Session = Networksessions[NodeID];

        Session->Sessiondata.update(Object.value("Sessiondata", nlohmann::json::object()));
        Session->Playerdata.update(Object.value("Playerdata", nlohmann::json::array()));
        Session->Gameinfo.update(Object.value("Gameinfo", nlohmann::json::object()));
        Session->Hostinfo.update(Object.value("Hostinfo", nlohmann::json::object()));
        Session->HostID = Object.value("HostID", 0);

        Session->Lastmessage = GetTickCount();
    }

    // Register handlers and set up session.
    void Initialize()
    {
        // Listen for servers.
        Backend::Registermessagehandler(Hash::FNV1_32("Serverupdate"), Updatehandler);
    }

    // Announce the clients info if active.
    static uint32_t Lastupdate{};
    void doFrame()
    {
        const auto Currenttime = GetTickCount();
        if (Currenttime > (Lastupdate + 5000))
        {
            // Clear out old sessions.
            std::erase_if(Networksessions, [&](const auto &Item)
                { return (Currenttime - Item.second->Lastmessage) > 10000; });

            // Announce our presence.
            if (Localsession.HostID) [[unlikely]]
                Sendserverinfo();

            Lastupdate = Currenttime;
        }
    }
}
