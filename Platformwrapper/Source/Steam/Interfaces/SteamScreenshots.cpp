/*
    Initial author: Convery (tcn@ayria.se)
    Started: 10-04-2019
    License: MIT
*/

#include "../Steam.hpp"

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
            Registerinterface(Interfacetype_t::SCREENSHOTS, "SteamScreenshots001", new SteamScreenshots001());
            Registerinterface(Interfacetype_t::SCREENSHOTS, "SteamScreenshots002", new SteamScreenshots002());
        }
    };
    static Steamscreenshotsloader Interfaceloader{};
}
