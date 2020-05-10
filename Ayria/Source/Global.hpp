/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-14
    License: MIT
*/

#pragma once
#define NK_INCLUDE_COMMAND_USERDATA
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
    void addFunction(std::string_view Name, std::function<void(int, const char **)> Callback);
    void addMessage(std::string_view Message, uint32_t Colour = 0xD6B749);

    void onStartup();
    void onFrame();
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
    bool Createsurface(std::string_view Classname, std::function<bool(struct nk_context *)> onRender);
    void setVisibility(std::string_view Classname, bool Visible);
    void onFrame();

    // Helpers for Nuklear.
    struct Font_t
    {
        HDC Devicecontext;
        HFONT Fonthandle;
        LONG Fontheight;
    };
    namespace Fonts
    {
        struct nk_user_font Createfont(const char *Name, int32_t Fontsize);
        struct nk_user_font Createfont(const char *Name, int32_t Fontsize, void *Fontdata, uint32_t Datasize);
    }
    namespace Images
    {
        template <bool isRGB = true> // NOTE(tcn): Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
        std::unique_ptr<struct nk_image> Createimage(const uint8_t *Pixelbuffer, struct nk_vec2i Size);
        void Deleteimage(std::unique_ptr<struct nk_image> &&Image);
    }
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
