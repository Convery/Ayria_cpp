/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct SteamUtils
    {
        uint32_t GetSecondsSinceAppActive()
        {
            Traceprint();
            return uint32_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() - Ayria::Global.Startuptimestamp);
        }
        uint32_t GetSecondsSinceComputerActive()
        {
            Traceprint();
            return uint32_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        }
        uint32_t GetConnectedUniverse()
        {
            Traceprint();

            // k_EUniversePublic
            return 1;
        }
        uint32_t GetServerRealTime()
        {
            Traceprint();
            return uint32_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        }

        const char *GetIPCountry()
        {
            Traceprint();
            /*
                TODO(Convery):
                Connect to the client dll and ask.
            */

            return "SE";
        }
        bool GetImageSize(int iImage, uint32_t *pnWidth, uint32_t *pnHeight)
        {
            Traceprint();

            // We do not handle any image requests.
            return false;
        }
        bool GetImageRGBA(int iImage, uint8_t *pubDest, int nDestBufferSize)
        {
            Traceprint();

            // We do not handle any image requests.
            return false;
        }
        bool GetCSERIPPort(uint32_t *unIP, uint16_t *usPort)
        {
            Traceprint();

            // As we don't connect to steam directly, this is not needed.
            return false;
        }
        uint8_t GetCurrentBatteryPower()
        {
            Traceprint();

            // We are always on AC power.
            return 255;
        }

        uint32_t GetAppID()
        {
            return Global.ApplicationID;
        }
        void SetOverlayNotificationPosition(uint32_t eNotificationPosition)
        {
            Traceprint();
        }
        bool IsAPICallCompleted(uint64_t hSteamAPICall, bool *pbFailed)
        {
            return false;
        }
        uint32_t GetAPICallFailureReason(uint64_t hSteamAPICall)
        {
            return 0;
        }
        bool GetAPICallResult(uint64_t hSteamAPICall, void *pCallback, int cubCallback, int iCallbackExpected, bool *pbFailed)
        {
            Traceprint();
            return false;
        }
        void RunFrame()
        {
        }
        uint32_t GetIPCCallCount()
        {
            Traceprint();

            // Debug information.
            return 100;
        }
        void SetWarningMessageHook(size_t pFunction)
        {
            Traceprint();
        }

        bool IsOverlayEnabled()
        {
            return false;
        }

        bool BOverlayNeedsPresent()
        {
            return false;
        }
        uint64_t CheckFileSignature(const char *szFileName)
        {
            Traceprint();
            return 0;
        }
        bool ShowGamepadTextInput(uint32_t eInputMode, uint32_t eInputLineMode, const char *szText, uint32_t uMaxLength)
        {
            Traceprint();
            return false;
        }
        uint32_t GetEnteredGamepadTextLength()
        {
            Traceprint();
            return 0;
        }
        bool GetEnteredGamepadTextInput(char *pchValue, uint32_t cchValueMax)
        {
            Traceprint();
            return false;
        }

        const char *GetSteamUILanguage()
        {
            Traceprint();
            return Global.Language.c_str();
        }
        bool IsSteamRunningInVR()
        {
            Traceprint();
            return false;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamUtilities001 : Interface_t
    {
        SteamUtilities001()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
        }
    };
    struct SteamUtilities002 : Interface_t
    {
        SteamUtilities002()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
        }
    };
    struct SteamUtilities003 : Interface_t
    {
        SteamUtilities003()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
            Createmethod(9, SteamUtils, GetAppID);
            Createmethod(10, SteamUtils, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtils, IsAPICallCompleted);
            Createmethod(12, SteamUtils, GetAPICallFailureReason);
            Createmethod(13, SteamUtils, GetAPICallResult);
            Createmethod(14, SteamUtils, RunFrame);
            Createmethod(15, SteamUtils, GetIPCCallCount);
            Createmethod(16, SteamUtils, SetWarningMessageHook);
        }
    };
    struct SteamUtilities004 : Interface_t
    {
        SteamUtilities004()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
            Createmethod(9, SteamUtils, GetAppID);
            Createmethod(10, SteamUtils, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtils, IsAPICallCompleted);
            Createmethod(12, SteamUtils, GetAPICallFailureReason);
            Createmethod(13, SteamUtils, GetAPICallResult);
            Createmethod(14, SteamUtils, RunFrame);
            Createmethod(15, SteamUtils, GetIPCCallCount);
            Createmethod(16, SteamUtils, SetWarningMessageHook);
            Createmethod(17, SteamUtils, IsOverlayEnabled);
        }
    };
    struct SteamUtilities005 : Interface_t
    {
        SteamUtilities005()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
            Createmethod(9, SteamUtils, GetAppID);
            Createmethod(10, SteamUtils, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtils, IsAPICallCompleted);
            Createmethod(12, SteamUtils, GetAPICallFailureReason);
            Createmethod(13, SteamUtils, GetAPICallResult);
            Createmethod(14, SteamUtils, RunFrame);
            Createmethod(15, SteamUtils, GetIPCCallCount);
            Createmethod(16, SteamUtils, SetWarningMessageHook);
            Createmethod(17, SteamUtils, IsOverlayEnabled);
            Createmethod(18, SteamUtils, BOverlayNeedsPresent);
            Createmethod(19, SteamUtils, CheckFileSignature);
            Createmethod(20, SteamUtils, ShowGamepadTextInput);
            Createmethod(21, SteamUtils, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtils, GetEnteredGamepadTextInput);
        };
    };
    struct SteamUtilities006 : Interface_t
    {
        SteamUtilities006()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
            Createmethod(9, SteamUtils, GetAppID);
            Createmethod(10, SteamUtils, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtils, IsAPICallCompleted);
            Createmethod(12, SteamUtils, GetAPICallFailureReason);
            Createmethod(13, SteamUtils, GetAPICallResult);
            Createmethod(14, SteamUtils, RunFrame);
            Createmethod(15, SteamUtils, GetIPCCallCount);
            Createmethod(16, SteamUtils, SetWarningMessageHook);
            Createmethod(17, SteamUtils, IsOverlayEnabled);
            Createmethod(18, SteamUtils, BOverlayNeedsPresent);
            Createmethod(19, SteamUtils, CheckFileSignature);
            Createmethod(20, SteamUtils, ShowGamepadTextInput);
            Createmethod(21, SteamUtils, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtils, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtils, GetSteamUILanguage);
            Createmethod(24, SteamUtils, IsSteamRunningInVR);
        };
    };
    struct SteamUtilities007 : Interface_t
    {
        SteamUtilities007()
        {
            Createmethod(0, SteamUtils, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtils, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtils, GetConnectedUniverse);
            Createmethod(3, SteamUtils, GetServerRealTime);
            Createmethod(4, SteamUtils, GetIPCountry);
            Createmethod(5, SteamUtils, GetImageSize);
            Createmethod(6, SteamUtils, GetImageRGBA);
            Createmethod(7, SteamUtils, GetCSERIPPort);
            Createmethod(8, SteamUtils, GetCurrentBatteryPower);
            Createmethod(9, SteamUtils, GetAppID);
            Createmethod(10, SteamUtils, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtils, IsAPICallCompleted);
            Createmethod(12, SteamUtils, GetAPICallFailureReason);
            Createmethod(13, SteamUtils, GetAPICallResult);
            Createmethod(14, SteamUtils, RunFrame);
            Createmethod(15, SteamUtils, GetIPCCallCount);
            Createmethod(16, SteamUtils, SetWarningMessageHook);
            Createmethod(17, SteamUtils, IsOverlayEnabled);
            Createmethod(18, SteamUtils, BOverlayNeedsPresent);
            Createmethod(19, SteamUtils, CheckFileSignature);
            Createmethod(20, SteamUtils, ShowGamepadTextInput);
            Createmethod(21, SteamUtils, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtils, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtils, GetSteamUILanguage);
            Createmethod(24, SteamUtils, IsSteamRunningInVR);
        };
    };

    struct Steamutilitiesloader
    {
        Steamutilitiesloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::UTILS, "SteamUtilities001", SteamUtilities001);
            Register(Interfacetype_t::UTILS, "SteamUtilities002", SteamUtilities002);
            Register(Interfacetype_t::UTILS, "SteamUtilities003", SteamUtilities003);
            Register(Interfacetype_t::UTILS, "SteamUtilities004", SteamUtilities004);
            Register(Interfacetype_t::UTILS, "SteamUtilities005", SteamUtilities005);
            Register(Interfacetype_t::UTILS, "SteamUtilities006", SteamUtilities006);
            Register(Interfacetype_t::UTILS, "SteamUtilities007", SteamUtilities007);
        }
    };
    static Steamutilitiesloader Interfaceloader{};
}
