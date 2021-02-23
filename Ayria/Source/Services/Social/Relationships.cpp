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

        // Send to the local network only, for now.
        const auto Relation = &Relationships[UserID];
        const auto Request = JSON::Object_t({
            { "UserID", UserID },
            { "isFriend", Relation->Flags.isFriend }
            // We do not notify if the user is blocked.
        });
        Backend::Network::Transmitmessage("Relationshipupdate", Request);

        // Save the change for later.
        Backend::Database() << "insert or replace into Socialrelations (ClientID, Uername, Flags) values (?, ?, ?);"
                            << Relation->UserID << Encoding::toNarrow(Relation->Username) << Relation->Flags.Raw;
    }
    static void __cdecl Relationchangehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Client = Clientinfo::getLANClient(NodeID);
        if (!Client) [[unlikely]] return;

        const auto Request = JSON::Parse(std::string_view(Message, Length));
        if (Global.UserID != Request.value<uint32_t>("UserID")) [[likely]] return;

        if (Relationships.contains(Client->UserID))
        {
            auto Relation = &Relationships[Client->UserID];
            if (Relation->Flags.isFriend && Request.value<bool>("isFriend"))
            {
                Relation->Flags.isMututal = true;

                // Save the change for later.
                Backend::Database() << "insert or replace into Socialrelations (ClientID, Uername, Flags) values (?, ?, ?);"
                                    << Relation->UserID << Encoding::toNarrow(Relation->Username) << Relation->Flags.Raw;
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
            return "{}";
        }
        static std::string __cdecl Remove(JSON::Value_t &&Request)
        {
            const auto UserID = Request.value<uint32_t>("UserID");
            if (!UserID || !Clientinfo::isClientonline(UserID)) return {};

            Relationships.erase(UserID);
            onRelationchange(UserID);
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
        // Persistent database entry for the friendslist.
        Backend::Database() << "create table if not exists Socialrelations ("
                               "ClientID integer primary key unique not null, "
                               "Username text, "
                               "Flags integer);";

        // Load existing relations.
        Backend::Database() << "select * from Socialrelations;" >> [](uint32_t ID, std::string Name, uint8_t Flags)
        {
            const Userrelation_t Relation{ ID, Encoding::toUTF8(Name), {Flags} };
            Relationships.emplace(ID, Relation);
        };

        // Register the network handlers.
        Backend::Network::Registerhandler("Relationshipupdate", Relationchangehandler);

        // JSON endpoints.
        Backend::API::addEndpoint("Social::Relations::List", API::List);
        Backend::API::addEndpoint("Social::Relations::Remove", API::Remove, R"({ "UserID" : 1234 })");
        Backend::API::addEndpoint("Social::Relations::Insert", API::Insert, R"({ "UserID" : 1234, "isFriend" : false, "isBlocked" : true })");
    }
}
