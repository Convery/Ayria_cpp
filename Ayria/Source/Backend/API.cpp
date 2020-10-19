/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-14
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// C-exports, JSON in and JSON out.
namespace API
{
    #define Storage(x)                                                                                  \
        std::unordered_map<uint32_t, std::string> Functionnames_ ##x;                                   \
        std::unordered_map<uint32_t, std::string> Functionresults_ ##x;                                 \
        std::unordered_map<uint32_t, Functionhandler> Functionhandlers_ ##x;                            \

    #define Register(x)                                                                                 \
        void Registerhandler_ ##x(std::string_view Function, Functionhandler Handler)                   \
        { const auto FunctionID = Hash::FNV1_32(Function);                                              \
        Functionnames_ ##x[FunctionID] = Function; Functionhandlers_ ##x[FunctionID] = Handler; }       \

    #define Export(x)                                                                                   \
        extern "C" EXPORT_ATTR const char *__cdecl API_ ##x(uint32_t FunctionID, const char *JSONString)\
        { if (FunctionID == 0 || !Functionhandlers_ ##x.contains(FunctionID))                           \
            { static std::string Result; auto Array = nlohmann::json::array();                          \
              for (const auto &[ID, Name] : Functionnames_ ##x)                                         \
                    Array += { { "FunctionID", ID }, { "Functionname", Name } };                        \
              Result = Array.dump(4); return Result.c_str(); }                                          \
          Functionresults_ ##x[FunctionID] = Functionhandlers_ ##x[FunctionID](JSONString);             \
          return Functionresults_ ##x[FunctionID].c_str(); }                                            \

    Storage(Client); Register(Client); Export(Client);
    Storage(Social); Register(Social); Export(Social);
    Storage(Network); Register(Network); Export(Network);
    Storage(Matchmake); Register(Matchmake); Export(Matchmake);
    Storage(Fileshare); Register(Fileshare); Export(Fileshare);

    #undef Export
    #undef Storage
    #undef Register
}
