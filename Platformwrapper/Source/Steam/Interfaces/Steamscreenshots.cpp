/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-12-28
    License: MIT
*/

#include <Steam.hpp>

namespace Steam
{
    static bool isHooked = false;

    struct SteamScreenshots
    {
        enum EVRScreenshotType
        {
            k_EVRScreenshotType_None = 0,
            k_EVRScreenshotType_Mono = 1,
            k_EVRScreenshotType_Stereo = 2,
            k_EVRScreenshotType_MonoCubemap = 3,
            k_EVRScreenshotType_MonoPanorama = 4,
            k_EVRScreenshotType_StereoPanorama = 5
        };

        ScreenshotHandle AddScreenshotToLibrary(const char *pchFilename, const char *pchThumbnailFilename, int nWidth, int nHeight)
        {
            Traceprint();
            return 0;
        }
        ScreenshotHandle AddVRScreenshotToLibrary(EVRScreenshotType eType, const char *pchFilename, const char *pchVRFilename)
        {
            Traceprint();
            return 0;
        }
        ScreenshotHandle WriteScreenshot(void *pubRGB, uint32_t cubRGB, int nWidth, int nHeight)
        {
            // TODO(tcn): Process screenshots?
            Infoprint(va("Client wants to save a screenshot(%ix%i), which is not implemented.", nWidth, nHeight));

            const auto Request = std::shared_ptr<Tasks::ScreenshotReady_t >();
            const auto TaskID = Tasks::Createrequest();
            Request->m_eResult = EResult::k_EResultIOFailure;
            Request->m_hLocal = 0;

            Tasks::Completerequest(TaskID, Tasks::ECallbackType::ScreenshotReady_t, Request);
            return Request->m_hLocal;
        }

        bool TagPublishedFile(ScreenshotHandle hScreenshot, PublishedFileId_t unPublishedFileId)
        {
            Traceprint();
            return true;
        }
        bool SetLocation(ScreenshotHandle hScreenshot, const char *pchLocation)
        {
            Traceprint();
            return true;
        }
        bool TagUser(ScreenshotHandle hScreenshot, SteamID_t steamID)
        {
            Traceprint();
            return true;
        }
        bool IsScreenshotsHooked()
        {
            return isHooked;
        }

        void HookScreenshots(bool bHook)
        {
            isHooked = bHook;
        }
        void TriggerScreenshot()
        {
            if (isHooked)
            {
                const auto TaskID = Tasks::Createrequest();
                const auto Request = std::shared_ptr<Tasks::ScreenshotRequested_t>();


                Tasks::Completerequest(TaskID, Tasks::ECallbackType::ScreenshotRequested_t, Request);
            }
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamScreenshots001 : Interface_t<6>
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
    struct SteamScreenshots002 : Interface_t<7>
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

    struct SteamScreenshots003 : Interface_t<9>
    {
        SteamScreenshots003()
        {
            Createmethod(0, SteamScreenshots, WriteScreenshot);
            Createmethod(1, SteamScreenshots, AddScreenshotToLibrary);
            Createmethod(2, SteamScreenshots, TriggerScreenshot);
            Createmethod(3, SteamScreenshots, HookScreenshots);
            Createmethod(4, SteamScreenshots, SetLocation);
            Createmethod(5, SteamScreenshots, TagUser);
            Createmethod(6, SteamScreenshots, TagPublishedFile);
            Createmethod(7, SteamScreenshots, IsScreenshotsHooked);
            Createmethod(8, SteamScreenshots, AddVRScreenshotToLibrary);
        };
    };

    struct Steamscreenshotsloader
    {
        Steamscreenshotsloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::SCREENSHOTS, "SteamScreenshots001", SteamScreenshots001);
            Register(Interfacetype_t::SCREENSHOTS, "SteamScreenshots002", SteamScreenshots002);
            Register(Interfacetype_t::SCREENSHOTS, "SteamScreenshots003", SteamScreenshots003);
        }
    };
    static Steamscreenshotsloader Interfaceloader{};
}
