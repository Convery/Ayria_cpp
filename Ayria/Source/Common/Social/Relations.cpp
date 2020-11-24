/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social::Relationships
{
    static std::unordered_set<Userrelation_t, decltype(WW::Hash), decltype(WW::Equal)> Relations;

    void Remove(uint32_t UserID, std::u8string_view Username)
    {
        std::erase_if(Relations, [&](const Userrelation_t &Relation)
        {
            return Relation.UserID == UserID || (!Username.empty() && Relation.Username == Username);
        });
    }
    std::vector<const Userrelation_t *> All()
    {
        std::vector<const Userrelation_t *> Result;
        Result.reserve(Relations.size());

        for (auto it = Relations.cbegin(); it != Relations.cend(); ++it)
            Result.push_back(&*it);

        return Result;
    }
    void Add(Userrelation_t &Addition)
    {
        Relations.insert(Addition);
    }

    // Load the relations from disk.
    void Initialize()
    {
        // Get the relations from disk.
        if (const auto Filebuffer = FS::Readfile<char>(L"./Ayria/Relations.json"); !Filebuffer.empty())
        {
            const auto Result = JSON::Parse(Filebuffer);

            // Malformed arrays get rejected.
            if (Result.Type == JSON::Array)
            {
                const auto Array = Result.get<JSON::Array_t>();
                Relations.reserve(Array.size());

                for (const auto &Object : Array)
                {
                    Userrelation_t Relation{};
                    Relation.Username = Object.value("Username", std::u8string());
                    Relation.Flags.Raw = Object.value("Flags", uint32_t());
                    Relation.UserID = Object.value("UserID", uint32_t());
                    Relations.insert(Relation);
                }
            }
            else
            {
                Errorprint("Relations.json is expected to be in the form of [ { UserID, Username, Flags }, .. ]");
            }
        }

        // TODO(tcn): Fetch relations from some remote server.

        // Save the relations when app is closed.
        std::atexit([]()
        {
            JSON::Array_t Output;
            for (const auto &Relation : Relations)
            {
                Output.push_back(JSON::Object_t({
                    { "UserID", Relation.UserID },
                    { "Flags", Relation.Flags.Raw },
                    { "Username", Relation.Username }
                    }));
            }
            FS::Writefile(L"./Ayria/Relations.json", JSON::Dump(Output));
        });
    }
}
