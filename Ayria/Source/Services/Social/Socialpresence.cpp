/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Presence
{
    static Hashmap<uint32_t, LZString_t> Remotepresence;
    static LZString_t Localpresence;
    static uint32_t Lastupdate{};
    static bool isDirty{};

    void setPresence(std::string Key, std::string Value)
    {
        const std::string Presence{ Localpresence.size() ? std::string(Localpresence) : "{}" };
        JSON::Object_t Object = JSON::Parse(Presence);

        Object[Key] = Value;
        Localpresence = JSON::Dump(Object);
        Remotepresence[Global.UserID] = Localpresence;
        isDirty = true;
    }
    std::string getPresence(uint32_t UserID)
    {
        if (Remotepresence.contains(UserID))
            return Remotepresence[UserID];
        else
            return "{}";
    }

    static void Sendpresence()
    {
        if (Localpresence.size())
        {
            if (isDirty || (GetTickCount() - Lastupdate) > 10'000)
            {
                isDirty = false;
                Lastupdate = GetTickCount();

                const auto Request = JSON::Object_t({ { "Presence", Base64::Encode(std::string(Localpresence)) } });
                Backend::Network::Sendmessage("Socialpresence", JSON::Dump(Request));
            }
        }
    }
    static void __cdecl Presencehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Sender = Clientinfo::getLocalclient(NodeID);
        if (!Sender) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Presence = Request.value<std::string>("Presence");
        const auto Decoded = Base64::Decode(Presence);

        if (!Decoded.empty())
            Remotepresence[Sender->UserID] = Decoded;
    }

    void Initialize()
    {
        Backend::Enqueuetask(1000, Sendpresence);
        Backend::Network::addHandler("Presenceupdate", Presencehandler);
    }
}
