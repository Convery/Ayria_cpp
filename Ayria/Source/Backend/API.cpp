/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// JSON Interface.
namespace Backend::API
{
    using Handler_t = struct { LZString_t Name; Callback_t Handler; LZString_t Usage; };
    static Hashmap<uint32_t, Handler_t> Requesthandlers{};
    static Ringbuffer_t<std::string, 8> Results{};
    static std::string Failurestring{};

    // Rather than adding generic results to the buffer.
    static const auto Generichash = Hash::WW32("{}");
    static const char *Genericresult = "{}";

    // static std::string __cdecl Callback(JSON::Value_t &&Request);
    // using Callback_t = std::string (__cdecl *)(JSON::Value_t &&Request);
    void addEndpoint(std::string_view Functionname, Callback_t Callback, std::string_view Usagestring)
    {
        const auto ID = Hash::WW32(Functionname);
        Requesthandlers[ID] = { Functionname, Callback, Usagestring };
    }

    // For internal use.
    std::vector<JSON::Value_t> listEndpoints()
    {
        std::vector<JSON::Value_t> Result;
        Result.reserve(Requesthandlers.size());

        for (const auto &[ID, Handler] : Requesthandlers)
        {
            Result.emplace_back(JSON::Object_t({
                { "Endpointname", (std::u8string)Handler.Name },
                { "Usage", (std::u8string)Handler.Usage },
                { "EndpointID", ID}
            }));
        }

        return Result;
    }
    const char *callEndpoint(std::string_view Functionname, JSON::Value_t &&Request)
    {
        static Debugmutex Threadsafe{};
        std::scoped_lock Lock(Threadsafe);

        const auto ID = Hash::WW32(Functionname);
        if (!Requesthandlers.contains(ID))
        {
            const auto Failure = JSON::Object_t({
                { "Error", va("There's no JSON API with name %*s or ID %08X", Functionname.size(), Functionname.data(), ID) },
                { "Endpoints", listEndpoints() }
            });

            Failurestring = JSON::Dump(Failure);
            return Failurestring.c_str();
        }

        // Call the handler with the object.
        auto Handlerinfo = &Requesthandlers[ID];
        const auto Result = Handlerinfo->Handler(std::move(Request));

        // Functions that return no data use a simple "{}".
        if (Result.empty())
        {
            const auto Failure = JSON::Object_t({
                { "Error", "The function returned an error, verify your input."s },
                { "Usage", (std::u8string)Handlerinfo->Usage }
            });

            Failurestring = JSON::Dump(Failure);
            return Failurestring.c_str();
        }

        // Is it a generic result?
        if (Hash::WW32(Result) == Generichash)
            return Genericresult;

        // Save the result on the heap for 8 calls.
        return Results.push_back(Result)->c_str();
    }

    // Export the JSON interface to the plugins.
    namespace API
    {
        extern "C" EXPORT_ATTR const char *__cdecl JSONRequest(const char *Function, const char *JSONString)
        {
            std::string_view Functionname = Function ? Function : "";
            return callEndpoint(Functionname, JSON::Parse(JSONString));
        }
    }
}
