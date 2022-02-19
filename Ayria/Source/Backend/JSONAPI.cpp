/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

namespace JSONAPI
{
    static Hashmap<std::string, Callback_t> Requesthandlers{};
    static Ringbuffer_t<std::string, 16> Results{};

    // Rather than adding generic results to the buffer.
    constexpr auto Generichash = Hash::WW32("{}");
    constexpr auto Genericresult = "{}";

    // Listen for requests to this functionname.
    void addEndpoint(std::string_view Functionname, Callback_t Callback)
    {
        if (Callback) Requesthandlers[std::string(Functionname)] = Callback;
    }

    // Access from the plugins.
    extern "C" EXPORT_ATTR const char *__cdecl JSONRequest(const char *Function, const char *JSONString)
    {
        const std::string_view Functionname = Function ? Function : "";

        if (!Requesthandlers.contains(Functionname)) [[unlikely]]
        {
            static std::string Failurestring = va(R"({ "Error" : "No endpoint with name %*s available.", \n"Endpoints" : [\n)",
            Functionname.size(), Functionname.data());

            for (const auto &Name : Requesthandlers | std::views::keys)
            {
                Failurestring.append(Name);
                Failurestring.append(",");
            }

            Failurestring.pop_back();
            Failurestring.append(R"(] })");
            return Failurestring.c_str();
        }

        const auto Result = Requesthandlers[std::string(Functionname)](JSON::Parse(JSONString));
        if (Result.empty() || Hash::WW32(Result) == Generichash) [[likely]] return Genericresult;

        // Save the result on the heap for 16 calls.
        return Results.emplace_back(Result).c_str();
    }
}
