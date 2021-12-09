/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Backend::JSONAPI
{
    static Hashmap<std::string, Callback_t> Requesthandlers{};
    static Ringbuffer_t<std::string, 16> Results{};

    // Rather than adding generic results to the buffer.
    constexpr auto Generichash = Hash::WW32("{}");
    constexpr auto Genericresult = "{}";

    using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
    // static std::string __cdecl Callback(JSON::Value_t &&Request);

    // Listen for requests to this functionname.
    void addEndpoint(std::string_view Functionname, Callback_t Callback)
    {
        if (Callback) Requesthandlers[std::string(Functionname)] = Callback;
    }

    // For internal use.
    const char *callEndpoint(std::string_view Functionname, JSON::Value_t &&Request)
    {
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

        const auto Result = Requesthandlers[std::string(Functionname)](std::move(Request));
        if (Result.empty() || Hash::WW32(Result) == Generichash) [[likely]] return Genericresult;

        // Save the result on the heap for 16 calls.
        return Results.emplace_back(Result).c_str();
    }

    // Access from the plugins.
    extern "C" EXPORT_ATTR const char *__cdecl JSONRequest(const char *Function, const char *JSONString)
    {
        const std::string_view Functionname = Function ? Function : "";
        return callEndpoint(Functionname, JSON::Parse(JSONString));
    }
}
