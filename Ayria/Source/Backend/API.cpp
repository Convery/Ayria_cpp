/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend::API
{
    static Hashmap<uint32_t, std::string> Functionresults{};
    static Hashmap<uint32_t, Callback_t> Functionhandlers{};
    static Hashmap<uint32_t, std::string> Functionnames{};

    void addHandler(const char *Function, Callback_t Callback)
    {
        assert(Function);  assert(Callback);
        const auto Hash = Hash::WW32(Function);
        Functionnames.emplace(Hash, Function);
        Functionhandlers.emplace(Hash, Callback);
    }
    const char *callHandler(const char *Function, const char *JSONString)
    {
        assert(Function); assert(JSONString);
        const auto Hash = Hash::WW32(Function);

        const auto Result = Functionhandlers.at(Hash)(JSONString);
        if (Hash::WW32(Result) != Hash::WW32(Functionresults.at(Hash)))
            Functionresults.emplace(Hash, Result);

        return Functionresults.at(Hash).c_str();
    }
    extern "C" EXPORT_ATTR const char *__cdecl JSONAPI(const char *Function, const char *JSONString)
    {
        assert(Function); assert(JSONString);
        const auto Hash = Hash::WW32(Function);

        if (!JSONString) [[unlikely]] JSONString = "";
        if (!Function || !Functionhandlers.contains(Hash)) [[unlikely]]
        {
            // Return a list of the available functions.
            JSON::Array_t Array; Array.reserve(Functionnames.size());
            for (const auto &[Key, Value] : Functionnames)
                Array.push_back(Value);

            Functionresults.emplace(0, JSON::Dump(Array));
            return Functionresults.at(0).c_str();
        }

        const auto Result = Functionhandlers.at(Hash)(JSONString);
        if (Hash::WW32(Result) != Hash::WW32(Functionresults.at(Hash)))
            Functionresults.emplace(Hash, Result);

        return Functionresults.at(Hash).c_str();
    }
}
