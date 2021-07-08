/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-28
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Clientpresence
{



    // Access from the plugins.
    static std::string __cdecl Setpresence(JSON::Value_t &&Request)
    {
        JSON::Array_t DBInsert{};

        if (Request.Type == JSON::Type_t::Array)
        {
            const auto &Array = Request.get<JSON::Array_t>();
            DBInsert.resize(Array.size());

            for (const auto &Item : Array)
            {
                DBInsert.emplace_back(JSON::Object_t({
                    { "Value", Item.value<std::string>("Value") },
                    { "Key", Item.value<std::string>("Key") },
                    { "AyriaID", Global.Sharedkeyhash }
                }));
            }
        }
        else
        {
            DBInsert = { JSON::Object_t({
                { "Value", Request.value<std::string>("Value") },
                { "Key", Request.value<std::string>("Key") },
                { "AyriaID", Global.Sharedkeyhash }
            }) };
        }

        // Send

        if (DB::setClientpresence(DBInsert)) return "{}";
        return R"({ "Error" : "Could not add all values." })";
    }

    // Set up the service.
    void Initialize()
    {

    }
}
