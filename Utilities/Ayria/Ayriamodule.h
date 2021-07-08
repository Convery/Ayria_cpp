/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-23
    License: MIT
*/

#if !defined(AYRIAMODULE_INCLUDE)
#define AYRIAMODULE_INCLUDE

#if defined(__cplusplus)
#include <Stdinclude.hpp>
#endif

// Helper to access Ayria.dll exports, no ABI stability implied.
static const char *__cdecl Nullsub1(...) { return ""; }
static void __cdecl Nullsub2(...) {}
struct Ayriamodule_t
{
    // Call the exported JSON functions, pass NULL as name to list all. Result-string freed after 8 calls.
    const char *(__cdecl *doJSONRequest)(const char *Endpointname, const char *JSONString);

    // UTF8 escaped ASCII strings are used for console functions. Colour as ARGB.
    void(__cdecl *addConsolemessage)(const char *String, unsigned int Colour);
    void(__cdecl *addConsolecommand)(const char *Name, void(__cdecl *Callback)(int Argc, const char **Argv));

    // Local area network communication.
    void(__cdecl *publishLANPacket)(const char *Messageidentifier, const char *JSONPayload);
    void(__cdecl *registerLANCallback)(const char *Messageidentifier,
                                       void(__cdecl *Callback)(unsigned int AccountID, const char *Message, unsigned int Length));

    // Run a periodic task on the systems background thread.
    void(__cdecl *createPeriodictask)(unsigned int PeriodMS, void(__cdecl *Callback)(void));

    // Internal, notify other plugins the application is fully initialized.
    void(__cdecl *onInitialized)(bool);

    // Helpers for C++, C users will have to create their own methods.
    #if defined(__cplusplus)
    JSON::Value_t doRequest(const std::string &Endpoint, const JSON::Value_t &Payload)
    {
        return JSON::Parse(doJSONRequest(Endpoint.c_str(), JSON::Dump(Payload).c_str()));
    }
    JSON::Value_t doRequest(const std::string &Endpoint, JSON::Value_t &&Payload)
    {
        return doRequest(Endpoint, Payload);
    }

    Ayriamodule_t()
    {
        #if defined(NDEBUG)
        const auto Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll");
        #else
        const auto Modulehandle = LoadLibraryA(Build::is64bit ? "./Ayria/Ayria64d.dll" : "./Ayria/Ayria32d.dll");
        #endif

        if (Modulehandle) [[likely]]
        {
            #define Import(x) x = (decltype(x))GetProcAddress(Modulehandle, #x)
            Import(doJSONRequest);
            Import(addConsolemessage);
            Import(addConsolecommand);
            Import(publishLANPacket);
            Import(registerLANCallback);
            Import(createPeriodictask);
            Import(onInitialized);
            #undef Import
        }
        else
        {
            doJSONRequest = (decltype(doJSONRequest))Nullsub1;
            addConsolemessage = (decltype(addConsolemessage))Nullsub2;
            addConsolecommand = (decltype(addConsolecommand))Nullsub2;
            publishLANPacket = (decltype(publishLANPacket))Nullsub2;
            registerLANCallback = (decltype(registerLANCallback))Nullsub2;
            createPeriodictask = (decltype(createPeriodictask))Nullsub2;
            onInitialized = (decltype(onInitialized))Nullsub2;
        }
    }
    #endif
};
extern Ayriamodule_t Ayria;
#endif
