/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Presence
{
    static Hashmap<uint32_t, LZString_t> Presencemap{};
    static uint32_t Lastupdate{};
    static bool isDirty;

    // Notify the world about our presence.
    static void Sendpresence()
    {
        if (Global.Stateflags.isPrivate) return;

        if (Presencemap.contains(Global.UserID))
        {
            if (isDirty || (GetTickCount() - Lastupdate) > 10'000)
            {
                isDirty = false;
                Lastupdate = GetTickCount();
                Backend::Network::Transmitmessage("Presence::Update", JSON::Parse((std::string)Presencemap[Global.UserID]));

                if (Global.Stateflags.isOnline)
                {
                    // TODO(tcn): Notify some server.
                }
            }
        }
    }
    static void __cdecl Presencehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLANClient(NodeID);
        if (!Sender) [[unlikely]] return;

        // Parse and dump as validation.
        Presencemap[Sender->UserID] = JSON::Dump(JSON::Parse(std::string_view(Message, Length)));
    }

    namespace API
    {
        static std::string __cdecl Set(JSON::Value_t &&Request)
        {
            const auto Storage = &Presencemap[Global.UserID];
            auto Object = JSON::Parse(std::string(*Storage));

            Request.update(Object);
            *Storage = JSON::Dump(Request);
            return "{}";
        }
        static std::string __cdecl Get(JSON::Value_t &&Request)
        {
            const auto UserID = Request.value<uint32_t>("UserID");
            if (!Presencemap.contains(UserID)) return "{}";
            return Presencemap[UserID];
        }
    }

    void Initialize()
    {
        // Send presence if outdated.
        Backend::Enqueuetask(5000, Sendpresence);

        // Register the network handlers.
        Backend::Network::Registerhandler("Presence::Update", Presencehandler);

        // JSON endpoints.
        Backend::API::addEndpoint("Presence::Set", API::Set, R"([ "Key" : value })");
        Backend::API::addEndpoint("Presence::Get", API::Get, R"({ "UserID" : 1234 })");
    }
}
