/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamScreenshots
    {
        uint32_t WriteScreenshot(void *pubRGB, uint32_t cubRGB, int nWidth, int nHeight)
        {
            Traceprint();
            return 0;
        }
        uint32_t AddScreenshotToLibrary(const char *pchJpegOrTGAFilename, const char *pchJpegOrTGAThumbFilename, int nWidth, int nHeight)
        {
            Traceprint();
            return 0;
        }
        void TriggerScreenshot()
        {
            Traceprint();
        }
        void HookScreenshots(bool bHook)
        {
            Traceprint();
        }
        bool SetLocation(uint32_t hScreenshot, const char *pchLocation)
        {
            Traceprint();
            return false;
        }
        bool TagUser(uint32_t hScreenshot, CSteamID steamID)
        {
            Traceprint();
            return false;
        }
        bool TagPublishedFile(uint32_t hScreenshot, uint64_t unPublishedFileId)
        {
            Traceprint();
            return false;
        }
    };

    struct SteamScreenshots001 : Interface_t
    {
        SteamScreenshots001()
        {
            Createmethod(0, SteamScreenshots, WriteScreenshot);
            Createmethod(1, SteamScreenshots, AddScreenshotToLibrary);
            Createmethod(2, SteamScreenshots, TriggerScreenshot);
            Createmethod(3, SteamScreenshots, HookScreenshots);
            Createmethod(4, SteamScreenshots, SetLocation);
            Createmethod(5, SteamScreenshots, TagUser);
        };
    };
    struct SteamScreenshots002 : Interface_t
    {
        SteamScreenshots002()
        {
            Createmethod(0, SteamScreenshots, WriteScreenshot);
            Createmethod(1, SteamScreenshots, AddScreenshotToLibrary);
            Createmethod(2, SteamScreenshots, TriggerScreenshot);
            Createmethod(3, SteamScreenshots, HookScreenshots);
            Createmethod(4, SteamScreenshots, SetLocation);
            Createmethod(5, SteamScreenshots, TagUser);
            Createmethod(6, SteamScreenshots, TagPublishedFile);
        };
    };

    struct Steamscreenshotsloader
    {
        Steamscreenshotsloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::SCREENSHOTS, "SteamScreenshots001", SteamScreenshots001);
            Register(Interfacetype_t::SCREENSHOTS, "SteamScreenshots002", SteamScreenshots002);
        }
    };
    static Steamscreenshotsloader Interfaceloader{};
}
