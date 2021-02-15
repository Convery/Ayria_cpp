/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-15
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Relations
{
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
        const auto Entries = List();
        if (Entries.empty()) [[unlikely]]
        {
            std::remove("./Ayria/Socialrelations.json");
            return;
        }

        JSON::Array_t Array;
        Array.reserve(Entries.size());

        for (const auto &Entry : Entries)
        {
            Array.emplace_back(JSON::Object_t({
                { "UserID", Entry->UserID },
                { "Flags", Entry->Flags.Raw },
                { "Username", Entry->Username }
            }));
        }

        FS::Writefile(L"./Ayria/Socialrelations.json", JSON::Dump(Array));
    }

    // Notify another user of our friendslist being updated.
    static void onRelationchange(uint32_t UserID)
    {
        if (!Relationships.contains(UserID)) return;
        if (!Clientinfo::isClientonline(UserID)) return;

        const auto Relation = Relationships[UserID].Flags;
        const auto Request = JSON::Object_t({
            { "Target", UserID },
            { "isFriend", Relation.isFriend }
            // We do not notify if the user is blocked.
        });

        Backend::Network::Sendmessage("Relationshipupdate", JSON::Dump(Request));
    }
    static void __cdecl Relationchangehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Client = Clientinfo::getLocalclient(NodeID);
        if (!Client) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        if (Global.UserID != Request.value<uint32_t>("Target")) [[likely]] return;

        if (Relationships.contains(Client->UserID))
        {
            auto Relation = &Relationships[Client->UserID].Flags;
            if (Relation->isFriend && Request.value<bool>("isFriend"))
                Relation->isPending = false;
        }
    }

    // Modify the relations stored to disk.
    void Insert(uint32_t UserID, std::u8string_view Username, Relationflags_t Flags)
    {
        isModified = true;
        Flags.isPending = true;
        Relationships[UserID] = { UserID, std::u8string(Username.data(), Username.size()), Flags };
        onRelationchange(UserID);
    }
    void Remove(uint32_t UserID)
    {
        Relationships.erase(UserID);
        onRelationchange(UserID);
        isModified = true;
    }

    // List all known relations.
    std::vector<const Userrelation_t *> List()
    {
        std::vector<const Userrelation_t *> Result;
        Result.reserve(Relationships.size());

        for (const auto &[Key, Value] : Relationships)
            Result.emplace_back(&Value);

        return Result;
    }

    // JSON interface for the plugins.
    namespace API
    {
        static std::string __cdecl Insert(const char *JSONString)
        {
            const auto Request = JSON::Parse(JSONString);
            const auto Flags = Request.value<uint8_t>("Flags");
            const auto UserID = Request.value<uint32_t>("UserID");
            const auto Username = Request.value<std::u8string>("Username");

            Social::Relations::Insert(UserID, Username, { Flags });
            return "{}";
        }
        static std::string __cdecl Remove(const char *JSONString)
        {
            const auto Request = JSON::Parse(JSONString);
            const auto UserID = Request.value<uint32_t>("UserID");

            Social::Relations::Remove(UserID);
            return "{}";
        }
        static std::string __cdecl List(const char *)
        {
            const auto Entries = Social::Relations::List();

            JSON::Array_t Array;
            Array.reserve(Entries.size());

            for (const auto &Entry : Entries)
            {
                Array.emplace_back(JSON::Object_t({
                    { "UserID", Entry->UserID },
                    { "Flags", Entry->Flags.Raw },
                    { "Username", Entry->Username }
                }));
            }

            return JSON::Dump(Array);
        }
    }

    // Load the relations from disk.
    void Initialize()
    {
        Loadrelations();
        std::atexit([]() { if (isModified) Saverelations(); });

        Backend::API::addHandler("Social::Relations::List", API::List);
        Backend::API::addHandler("Social::Relations::Remove", API::Remove);
        Backend::API::addHandler("Social::Relations::Insert", API::Insert);
        Backend::Network::addHandler("Relationshipupdate", Relationchangehandler);
    }
}
