/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-23
    License: MIT
*/

#if !defined(AYRIAMODULE_INCLUDE)
#define AYRIAMODULE_INCLUDE

#if defined(__cplusplus)
#include <Stdinclude.hpp>
#else
#include <stdint.h>
#include <assert.h>
#endif

// Helper to access Ayria.dll exports, no ABI stability implied.
struct Ayriamodule_t
{
    // Run a periodic task on the systems background thread.
    void(__cdecl *createPeriodictask)(unsigned int PeriodMS, void(__cdecl *Callback)(void));

    // Call the exported JSON functions, pass NULL as name to list all. Result-string freed after 8 calls.
    const char *(__cdecl *JSONRequest)(const char *Function, const char *JSONString);

    // UTF8 escaped ASCII strings are used for console functions. Colour as ARGB.
    void(__cdecl *addConsolemessage)(const char *String, unsigned int Colour);
    void(__cdecl *addConsolecommand)(const char *Name, void(__cdecl *Callback)(int Argc, const char **Argv));

    // Communication with the network, use with extreme care.
    void(__cdecl *subscribeMessage)(const char *Identifier, bool (__cdecl * Callback)(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length));
    void(__cdecl *publishMessage)(const char *Identifier, const char *Message, unsigned int Length);
    void(__cdecl *connectUser)(const char *IPv4, const char *Port);

    // Listen and publish notifications to other plugins, e.g. new chat-message.
    void(__cdecl *unsubscribeNotification)(const char *Identifier, void(__cdecl *Callback)(const char *JSONString));
    void(__cdecl *subscribeNotification)(const char *Identifier, void(__cdecl *Callback)(const char *JSONString));
    void(__cdecl *publishNotification)(const char *Identifier, const char *JSONString);

    // Internal, notify other plugins the application is fully initialized.
    void(__cdecl *onInitialized)(bool);

    // Hot-reloading for plugin developers in debug-mode.
    #if !defined(NDEBUG)
    void(__cdecl *unloadPlugin)(const char *Pluginname);
    void(__cdecl *loadPlugins)();
    #endif

    // Helpers for C++, C users will have to create their own methods.
    #if defined(__cplusplus)

    JSON::Value_t doRequest(const std::string &Endpoint, const JSON::Value_t &Payload) const
    {
        return JSON::Parse(JSONRequest(Endpoint.c_str(), JSON::Dump(Payload).c_str()));
    }
    JSON::Value_t doRequest(const std::string &Endpoint, JSON::Value_t &&Payload) const
    {
        return doRequest(Endpoint, Payload);
    }

    // Trigger asserts instead of having to check the pointers validity.
    static const char *__cdecl AYA_Nullsub1(...) { assert(false); return ""; }
    static void __cdecl AYA_Nullsub2(...) { assert(false); }

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
            Import(addConsolemessage);
            Import(addConsolecommand);

            Import(subscribeMessage);
            Import(publishMessage);
            Import(connectUser);

            Import(createPeriodictask);
            Import(onInitialized);
            Import(JSONRequest);

            Import(unsubscribeNotification);
            Import(subscribeNotification);
            Import(publishNotification);

            #if !defined(NDEBUG)
            Import(unloadPlugin);
            Import(loadPlugins);
            #endif
            #undef Import
        }
        else
        {
            addConsolemessage = decltype(addConsolemessage)(AYA_Nullsub2);
            addConsolecommand = decltype(addConsolecommand)(AYA_Nullsub2);

            subscribeMessage = decltype(subscribeMessage)(AYA_Nullsub2);
            publishMessage = decltype(publishMessage)(AYA_Nullsub2);
            connectUser = decltype(connectUser)(AYA_Nullsub2);

            createPeriodictask = decltype(createPeriodictask)(AYA_Nullsub2);
            onInitialized = decltype(onInitialized)(AYA_Nullsub2);
            JSONRequest = decltype(JSONRequest)(AYA_Nullsub1);

            unsubscribeNotification = decltype(unsubscribeNotification)(AYA_Nullsub2);
            subscribeNotification = decltype(subscribeNotification)(AYA_Nullsub2);
            publishNotification = decltype(publishNotification)(AYA_Nullsub2);

            #if !defined(NDEBUG)
            unloadPlugin = decltype(unloadPlugin)(AYA_Nullsub2);
            loadPlugins = decltype(loadPlugins)(AYA_Nullsub2);
            #endif
        }
    }
    #endif
};
extern Ayriamodule_t Ayria;
#endif
