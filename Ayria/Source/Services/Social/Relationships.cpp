/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-15
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Relations
{
    using Relationflags_t = union { uint8_t Raw; struct { uint8_t isMututal : 1, isFriend : 1, isBlocked : 1; }; };
    using Userrelation_t = struct { uint32_t UserID; std::u8string Username; Relationflags_t Flags; };
    static Hashmap<uint32_t, Userrelation_t> Relationships{};
    static bool isModified{};

    // Load and save to disk.
    static void Loadrelations()
    {
        const JSON::Array_t Array = JSON::Parse(FS::Readfile<char>(L"./Ayria/Socialrelations.json"));
        for (const auto Object : Array)
        {
            const auto Username = Object.value<std::u8string>("Username");
            const auto UserID = Object.value<uint32_t>("UserID");
            const auto Flags = Object.value<uint8_t>("Flags");

            // Warn the user about bad data.
            if (!UserID || !Flags)
            {
                Errorprint(va("Socialrelations.json contains an invalid entry at ID %u", Relationships.size()));
                continue;
            }

            Relationships[UserID] = { UserID, Username, Relationflags_t(Flags) };
        }
    }
    static void Saverelations()
    {
        if (Relationships.empty()) [[unlikely]]
        {
            std::remove("./Ayria/Socialrelations.json");
            return;
        }

        JSON::Array_t Array;
        Array.reserve(Relationships.size());

        for (const auto &[ID, Entry] : Relationships)
        {
            Array.emplace_back(JSON::Object_t({
                { "Username", Entry.Username },
                { "Flags", Entry.Flags.Raw },
                { "UserID", Entry.UserID }
            }));
        }

        FS::Writefile(L"./Ayria/Socialrelations.json", JSON::Dump(Array));
    }

    // Fetch information about clients.
    bool isBlocked(uint32_t UserID)
    {
        return Relationships.contains(UserID) && Relationships[UserID].Flags.isBlocked;
    }

    // Notify another user of our friendslist being updated.
    static void onRelationchange(uint32_t UserID)
    {
        if (Global.Stateflags.isPrivate) [[unlikely]] return;
        if (!Clientinfo::isClientonline(UserID)) return;
        if (!Relationships.contains(UserID)) return;

        const auto Relation = Relationships[UserID].Flags;
        const auto Request = JSON::Object_t({
            { "UserID", UserID },
            { "isFriend", Relation.isFriend }
            // We do not notify if the user is blocked.
        });

        Backend::Network::Transmitmessage("Relationshipupdate", Request);
    }
    static void __cdecl Relationchangehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Client = Clientinfo::getLANClient(NodeID);
        if (!Client) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        if (Global.UserID != Request.value<uint32_t>("UserID")) [[likely]] return;

        if (Relationships.contains(Client->UserID))
        {
            auto Relation = &Relationships[Client->UserID].Flags;
            if (Relation->isFriend && Request.value<bool>("isFriend"))
            {
                Relation->isMututal = true;
                isModified = true;
            }
        }
    }

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl Insert(JSON::Value_t &&Request)
        {
            Relationflags_t Flags{};

            const auto Username = Request.value<std::u8string>("Username");
            const auto UserID = Request.value<uint32_t>("UserID");
            Flags.isBlocked = Request.value<bool>("isBlocked");
            Flags.isFriend = Request.value<bool>("isFriend");

            // We really need an online ID..
            if (!UserID || !Clientinfo::isClientonline(UserID)) return {};
            if (!Flags.Raw) return {};

            Relationships[UserID] = { UserID, Username, Flags };
            onRelationchange(UserID);
            isModified = true;
            return "{}";
        }
        static std::string __cdecl Remove(JSON::Value_t &&Request)
        {
            const auto UserID = Request.value<uint32_t>("UserID");
            if (!UserID || !Clientinfo::isClientonline(UserID)) return {};

            Relationships.erase(UserID);
            onRelationchange(UserID);
            isModified = true;
            return "{}";
        }
        static std::string __cdecl List(JSON::Value_t &&)
        {
            JSON::Array_t Result;
            Result.reserve(Relationships.size());

            for (const auto &[ID, Entry] : Relationships)
            {
                Result.emplace_back(JSON::Object_t({
                    { "isMututal", (bool)Entry.Flags.isMututal },
                    { "isBlocked", (bool)Entry.Flags.isBlocked },
                    { "isFriend", (bool)Entry.Flags.isFriend },
                    { "Username", Entry.Username },
                    { "UserID", Entry.UserID }
                }));
            }

            return JSON::Dump(Result);
        }
    }

    // Load the relations from disk.
    void Initialize()
    {
        Loadrelations();
        std::atexit([]() { if (isModified) Saverelations(); });

        Backend::Network::Registerhandler("Relationshipupdate", Relationchangehandler);

        Backend::API::addEndpoint("Social::Relations::List", API::List);
        Backend::API::addEndpoint("Social::Relations::Remove", API::Remove, R"({ "UserID" : 1234 })");
        Backend::API::addEndpoint("Social::Relations::Insert", API::Insert, R"({ "UserID" : 1234, "isFriend" : false, "isBlocked" : true })");
    }
}
