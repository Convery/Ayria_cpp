/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-30
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Social::Relations
{
    Hashmap<uint32_t, Userrelation_t> Relationmap{};
    bool isModified{};

    // Modify the relations stored to disk.
    void Insert(uint32_t UserID, std::u8string_view Username, Relationflags_t Flags)
    {
        auto &Entry = Relationmap[UserID];
        if (!Username.empty()) Entry.Username = Username;
        Entry.Flags.Raw |= Flags.Raw;
        Entry.UserID = UserID;
        isModified = true;
    }
    std::vector<const Userrelation_t *> List()
    {
        std::vector<const Userrelation_t *>  Result;
        Result.reserve(Relationmap.size());

        for (const auto &[ID, Relation] : Relationmap)
            Result.emplace_back(&Relation);

        return Result;
    }
    void Remove(uint32_t UserID)
    {
        Relationmap.erase(UserID);
        isModified = true;
    }

    // Load the relations from disk.
    void Initialize()
    {
        // Load existing relations on startup.
        const JSON::Array_t Array = JSON::Parse(FS::Readfile<char>(L"./Ayria/Socialrelations.json"));
        for (const auto Object : Array)
        {
            const auto Username = Object.value<std::u8string>("Username");
            const auto UserID = Object.value<uint32_t>("UserID");
            const auto Flags = Object.value<uint8_t>("Flags");

            // Warn the user about bad data.
            if (!UserID || !Flags)
            {
                Errorprint(va("Socialrelations.json contains an invalid entry at ID %u", Relationmap.size()));
                continue;
            }

            Relationmap[UserID] = { UserID, Username, Relationflags_t(Flags) };
        }

        // Save the relations if they have been modified.
        std::atexit([]()
        {
            if (isModified)
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
                    Array.emplace_back(JSON::Object_t(
                        {
                            { "UserID", Entry->UserID},
                            { "Flags", Entry->Flags.Raw},
                            { "Username", Entry->Username}
                        }));
                }

                FS::Writefile(L"./Ayria/Socialrelations.json", JSON::Dump(Array));
            }
        });
    }
}
