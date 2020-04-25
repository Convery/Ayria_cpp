/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-25
    License: MIT
*/

#include "../../Common.hpp"

namespace Matchmaking
{
    std::unordered_map<uint64_t, Server_t> Serverlist;
    Server_t Self{};

    std::vector<Server_t> getServers()
    {
        std::vector<Server_t> Servers;
        Servers.reserve(Serverlist.size());
        for (const auto &[ID, Item] : Serverlist)
            if(uint32_t(time(NULL)) - Item.Lastmodified <= 30)
                Servers.emplace_back(Item);

        return Servers;
    }
    void Announceupdate(Server_t Delta)
    {
        if (!Delta.Port) Self.Port = Delta.Port;
        if (!Delta.Address) Self.Address = Delta.Address;
        if (!Delta.ServerID) Self.ServerID = Delta.ServerID;
        if (!Delta.Session.empty()) Self.Session = Delta.Session;
        if (!Delta.Players.empty()) Self.Players = Delta.Players;
        if (!Delta.Maxplayers) Self.Maxplayers = Delta.Maxplayers;
        if (!Delta.Gametags.empty()) Self.Gametags = Delta.Gametags;

        Communication::Broadcastmessage("Serverupdate", 0, Bytebuffer(bbSerialize(Self)));
    }
    void Announcequit()
    {
        Communication::Broadcastmessage("Serverterminate", 0, Bytebuffer());
    }
    Server_t *Localserver()
    {
        return &Self;
    }

    // Callbacks from the Ayria network-layer.
    void onUpdate(uint64_t, uint64_t SenderID, std::string_view, Bytebuffer &&Data)
    {
        Serverlist[SenderID].Deserialize(Data);
        Serverlist[SenderID].Lastmodified = uint32_t(time(NULL));
    }
    void onTerminate(uint64_t, uint64_t SenderID, std::string_view, Bytebuffer &&)
    {
        Serverlist.erase(SenderID);
    }

    // Add the callbacks on startup.
    namespace { struct Startup { Startup()
    {
        Communication::addMessagecallback("Serverupdate", onUpdate);
        Communication::addMessagecallback("Serverterminate", onTerminate);
    } }; static Startup Loader{}; }
}
