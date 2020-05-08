/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#pragma warning(push, 0)
#include <nuklear.h>
#pragma warning(pop)

// API exports.
struct Ayriamodule_t
{
    void *Modulehandle;

    // Client information.
    void(__cdecl *getClientID)(unsigned int *ClientID);
    void(__cdecl *getClientname)(const char **Clientname);
    void(__cdecl *getClientticket)(const char **Clientticket);

    // Console interaction, callback = void f(int argc, const char **argv);
    void(__cdecl *addConsolemessage)(const char *String, unsigned int Colour);
    void(__cdecl *addConsolefunction)(const char *Name, void *Callback);

    // Network interaction, callback = void f(const char *Content);
    void(__cdecl *addNetworklistener)(const char *Subject, void *Callback);
    void(__cdecl *addNetworkbroadcast)(const char *Subject, const char *Message);
};

namespace Console
{
    void addFunction(std::string_view Name, std::function<void(int, char **)> Callback);
    void addMessage(std::string_view Message, uint32_t Colour = 0xD6B749);

    void onStartup();
}

namespace Network
{
    void addHandler(std::string_view Subject, std::function<void(const char *)> Callback);
    void addBroadcast(std::string_view Subject, std::string_view Content);
    void addGreeting(std::string_view Subject, std::string_view Content);

    void onStartup();
    void onFrame();
}

namespace Client
{
    struct Client_t
    {
        std::string Clientname{};
        std::string Publickey{};
        uint32_t ClientID{};
    };

    std::vector<Client_t> getClients();
    void onStartup();
}

namespace Graphics
{
    void Registerwindow(std::function<void(struct nk_context *)> Callback);
    RECT Getgamewindow();
    void onStartup();
    void onFrame();
}

namespace Pluginloader
{
    void Loadplugins();

    bool InstallTLSCallback();
    void RestoreTLSCallback();

    bool InstallEPCallback();
    void RestoreEPCallback();
}

// Forward declarations for Appmain.
void onStartup();
void onFrame();
