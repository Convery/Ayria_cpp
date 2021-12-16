/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-12-16
    License: MIT
*/

#include <Steam.hpp>

// We use for loops for results that return 0 or 1 values, ignore warnings.
#pragma warning(disable: 4702)

namespace Steam
{
    using SteamAPIWarningMessageHook_t = void(__cdecl *)(int, const char *);

    struct SteamUtilities
    {
        enum ESteamIPv6ConnectivityProtocol
        {
            k_ESteamIPv6ConnectivityProtocol_Invalid = 0,
            k_ESteamIPv6ConnectivityProtocol_HTTP = 1,		// because a proxy may make this different than other protocols
            k_ESteamIPv6ConnectivityProtocol_UDP = 2,		// test UDP connectivity. Uses a port that is commonly needed for other Steam stuff. If UDP works, TCP probably works.
        };
        enum ESteamIPv6ConnectivityState
        {
            k_ESteamIPv6ConnectivityState_Unknown = 0,	// We haven't run a test yet
            k_ESteamIPv6ConnectivityState_Good = 1,		// We have recently been able to make a request on ipv6 for the given protocol
            k_ESteamIPv6ConnectivityState_Bad = 2,		// We failed to make a request, either because this machine has no ipv6 address assigned, or it has no upstream connectivity
        };
        enum EGamepadTextInputLineMode
        {
            k_EGamepadTextInputLineModeSingleLine = 0,
            k_EGamepadTextInputLineModeMultipleLines = 1
        };
        enum EGamepadTextInputMode
        {
            k_EGamepadTextInputModeNormal = 0,
            k_EGamepadTextInputModePassword = 1
        };
        enum ENotificationPosition
        {
            k_EPositionTopLeft = 0,
            k_EPositionTopRight = 1,
            k_EPositionBottomLeft = 2,
            k_EPositionBottomRight = 3,
        };
        enum ETextFilteringContext
        {
            k_ETextFilteringContextUnknown = 0,	    // Unknown context
            k_ETextFilteringContextGameContent = 1,	// Game content, only legally required filtering is performed
            k_ETextFilteringContextChat = 2,	    // Chat from another player
            k_ETextFilteringContextName = 3,	    // Character or item name
        };
        enum ESteamAPICallFailure
        {
            k_ESteamAPICallFailureNone = -1,			    // no failure
            k_ESteamAPICallFailureSteamGone = 0,		    // the local Steam process has gone away
            k_ESteamAPICallFailureNetworkFailure = 1,	    // the network connection to Steam has been broken, or was already broken
                                                            // SteamServersDisconnected_t callback will be sent around the same time
                                                            // SteamServersConnected_t will be sent when the client is able to talk to the Steam servers again
            k_ESteamAPICallFailureInvalidHandle = 2,	    // the SteamAPICall_t handle passed in no longer exists
            k_ESteamAPICallFailureMismatchedCallback = 3,   // GetAPICallResult() was called with the wrong callback type for this API call
        };
        enum ECheckFileSignature : uint32_t
        {
            k_ECheckFileSignatureInvalidSignature = 0,
            k_ECheckFileSignatureValidSignature = 1,
            k_ECheckFileSignatureFileNotFound = 2,
            k_ECheckFileSignatureNoSignaturesFoundForThisApp = 3,
            k_ECheckFileSignatureNoSignaturesFoundForThisFile = 4,
        };

        ESteamAPICallFailure GetAPICallFailureReason(SteamAPICall_t hSteamAPICall)
        {
            return k_ESteamAPICallFailureNone;
        }
        ESteamIPv6ConnectivityState GetIPv6ConnectivityState(ESteamIPv6ConnectivityProtocol eProtocol)
        {
            // If the hardware doesn't support IPv6 in 2021, I don't even..
            return k_ESteamIPv6ConnectivityState_Good;
        }
        uint8_t GetConnectedUniverse()
        {
            return (uint8_t)Global.XUID.Universe;
        }

        SteamAPICall_t CheckFileSignature(const char *szFileName)
        {
            const auto RequestID = Tasks::Createrequest();
            const auto Request = std::shared_ptr<Tasks::CheckFileSignature_t>();
            Request->m_eCheckFileSignature = k_ECheckFileSignatureValidSignature;

            Tasks::Completerequest(RequestID, Tasks::ECallbackType::CheckFileSignature_t, Request);
            return RequestID;
        }

        bool BIsPSNOnline()
        {
            return true;
        }
        bool BIsReadyToShutdown()
        {
            return true;
        }
        bool BOverlayNeedsPresent()
        {
            return false;
        }
        bool GetAPICallResult(SteamAPICall_t hSteamAPICall, void *pCallback, int cubCallback, int iCallbackExpected, bool *pbFailed)
        {
            // Please use the Steam_API export like a normal person..
            Warningprint(va("The client is trying to call %s directly, notify the developers.", __FUNCTION__));
            return false;
        }
        bool GetCSERIPPort(uint32_t *unIP, uint16_t *usPort)
        {
            // As we don't connect to steam directly, this is not needed.
            return false;
        }
        bool GetEnteredGamepadTextInput(char *pchValue, uint32_t cchValueMax)
        {
            // TODO(tcn): Gamepad support?
            return false;
        }
        bool GetImageRGBA(int iImage, uint8_t *pubDest, int nDestBufferSize)
        {
            // TODO(tcn): Contact Ayria CDN or something.
            return false;
        }
        bool GetImageSize(int iImage, uint32_t *pnWidth, uint32_t *pnHeight)
        {
            // TODO(tcn): Contact Ayria CDN or something.
            return false;
        }
        bool InitFilterText0()
        {
            return false;
        }
        bool InitFilterText1(uint32_t unFilterOptions)
        {
            return false;
        }
        bool IsAPICallCompleted(SteamAPICall_t hSteamAPICall, bool *pbFailed)
        {
            return false;
        }
        bool IsOverlayEnabled()
        {
            return !!std::strstr(GetCommandLineA(), "-overlay");
        }
        bool IsSteamChinaLauncher()
        {
            return false;
        }
        bool IsSteamInBigPictureMode()
        {
            return false;
        }
        bool IsSteamRunningInVR()
        {
            return false;
        }
        bool IsVRHeadsetStreamingEnabled()
        {
            return false;
        }
        bool ShowGamepadTextInput0(EGamepadTextInputMode eInputMode, EGamepadTextInputLineMode eInputLineMode, const char *szText, uint32_t uMaxLength)
        {
            // TODO(tcn): Gamepad support?
            return false;
        }
        bool ShowGamepadTextInput1(EGamepadTextInputMode eInputMode, EGamepadTextInputLineMode eInputLineMode, const char *pchDescription, uint32_t unCharMax, const char * pchExistingText)
        {
            // TODO(tcn): Gamepad support?
            return false;
        }

        const char *GetIPCountry()
        {
            // TODO(tcn): Get some real info.
            return "SE";
        }
        const char *GetSteamUILanguage()
        {
            return Global.Locale->c_str();
        }

        // Much censorship.
        int FilterText0(char *pchOutFilteredText, uint32_t nByteSizeOutFilteredText, const char *pchInputMessage, bool bLegalOnly)
        {
            const auto Input = std::string_view(pchInputMessage);
            const auto Length = std::min(nByteSizeOutFilteredText, (uint32_t)Input.size());

            std::memcpy(pchOutFilteredText, Input.data(), Length);
            return Length;
        }
        int FilterText1(ETextFilteringContext eContext, SteamID_t sourceSteamID, const char *pchInputMessage, char *pchOutFilteredText, uint32_t nByteSizeOutFilteredText)
        {
            return FilterText0(pchOutFilteredText, nByteSizeOutFilteredText, pchInputMessage, true);
        }

        uint32_t GetAppID()
        {
            return Global.AppID;
        }
        uint32_t GetEnteredGamepadTextLength()
        {
            // TODO(tcn): Gamepad support?
            return 0;
        }
        uint32_t GetIPCCallCount()
        {
            Warningprint(va("%s should not be called in release mode, notify a developer.", __FUNCTION__));
            return 42;
        }
        uint32_t GetSecondsSinceAppActive()
        {
            return uint32_t((GetTickCount64() - Global.Startuptime) / 1000);
        }
        uint32_t GetSecondsSinceComputerActive()
        {
            return uint32_t(GetTickCount64() / 1000);
        }
        uint32_t GetServerRealTime()
        {
            return uint32_t(uint32_t(time(NULL)));
        }
        uint8_t GetCurrentBatteryPower()
        {
            // We are always on AC power.
            return 255;
        }

        void PostPS3SysutilCallback(uint64_t status, uint64_t param, void* userdata) {}
        void RunFrame() { }
        void SetOverlayNotificationInset(int nHorizontalInset, int nVerticalInset) {}
        void SetOverlayNotificationPosition(ENotificationPosition eNotificationPosition) {}
        void SetPSNGameBootInviteStrings(const char *pchSubject, const char *pchBody) {}
        void SetVRHeadsetStreamingEnabled(bool bEnabled) {}
        void SetWarningMessageHook(SteamAPIWarningMessageHook_t pFunction) {}
        void StartVRDashboard() {}
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamUtilities001 : Interface_t<4>
    {
        SteamUtilities001()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
        }
    };
    struct SteamUtilities002 : Interface_t<9>
    {
        SteamUtilities002()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
        }
    };
    struct SteamUtilities003 : Interface_t<17>
    {
        SteamUtilities003()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
        }
    };
    struct SteamUtilities004 : Interface_t<18>
    {
        SteamUtilities004()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
        }
    };
    struct SteamUtilities005 : Interface_t<23>
    {
        SteamUtilities005()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput0);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
        };
    };
    struct SteamUtilities006 : Interface_t<25>
    {
        SteamUtilities006()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput0);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtilities, GetSteamUILanguage);
            Createmethod(24, SteamUtilities, IsSteamRunningInVR);
        };
    };
    struct SteamUtilities007 : Interface_t<25>
    {
        SteamUtilities007()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput1);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtilities, GetSteamUILanguage);
            Createmethod(24, SteamUtilities, IsSteamRunningInVR);
        };
    };
    struct SteamUtilities008 : Interface_t<27>
    {
        SteamUtilities008()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput1);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtilities, GetSteamUILanguage);
            Createmethod(24, SteamUtilities, SetOverlayNotificationInset);
            Createmethod(25, SteamUtilities, IsSteamInBigPictureMode);
            Createmethod(26, SteamUtilities, StartVRDashboard);
        };
    };
    struct SteamUtilities009 : Interface_t<33>
    {
        SteamUtilities009()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput1);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtilities, GetSteamUILanguage);
            Createmethod(24, SteamUtilities, SetOverlayNotificationInset);
            Createmethod(25, SteamUtilities, IsSteamInBigPictureMode);
            Createmethod(26, SteamUtilities, StartVRDashboard);
            Createmethod(27, SteamUtilities, IsVRHeadsetStreamingEnabled);
            Createmethod(28, SteamUtilities, SetVRHeadsetStreamingEnabled);
            Createmethod(29, SteamUtilities, IsSteamChinaLauncher);
            Createmethod(30, SteamUtilities, InitFilterText0);
            Createmethod(31, SteamUtilities, FilterText0);
            Createmethod(32, SteamUtilities, GetIPv6ConnectivityState);
        };
    };
    struct SteamUtilities010 : Interface_t<33>
    {
        SteamUtilities010()
        {
            Createmethod(0, SteamUtilities, GetSecondsSinceAppActive);
            Createmethod(1, SteamUtilities, GetSecondsSinceComputerActive);
            Createmethod(2, SteamUtilities, GetConnectedUniverse);
            Createmethod(3, SteamUtilities, GetServerRealTime);
            Createmethod(4, SteamUtilities, GetIPCountry);
            Createmethod(5, SteamUtilities, GetImageSize);
            Createmethod(6, SteamUtilities, GetImageRGBA);
            Createmethod(7, SteamUtilities, GetCSERIPPort);
            Createmethod(8, SteamUtilities, GetCurrentBatteryPower);
            Createmethod(9, SteamUtilities, GetAppID);
            Createmethod(10, SteamUtilities, SetOverlayNotificationPosition);
            Createmethod(11, SteamUtilities, IsAPICallCompleted);
            Createmethod(12, SteamUtilities, GetAPICallFailureReason);
            Createmethod(13, SteamUtilities, GetAPICallResult);
            Createmethod(14, SteamUtilities, RunFrame);
            Createmethod(15, SteamUtilities, GetIPCCallCount);
            Createmethod(16, SteamUtilities, SetWarningMessageHook);
            Createmethod(17, SteamUtilities, IsOverlayEnabled);
            Createmethod(18, SteamUtilities, BOverlayNeedsPresent);
            Createmethod(19, SteamUtilities, CheckFileSignature);
            Createmethod(20, SteamUtilities, ShowGamepadTextInput1);
            Createmethod(21, SteamUtilities, GetEnteredGamepadTextLength);
            Createmethod(22, SteamUtilities, GetEnteredGamepadTextInput);
            Createmethod(23, SteamUtilities, GetSteamUILanguage);
            Createmethod(24, SteamUtilities, SetOverlayNotificationInset);
            Createmethod(25, SteamUtilities, IsSteamInBigPictureMode);
            Createmethod(26, SteamUtilities, StartVRDashboard);
            Createmethod(27, SteamUtilities, IsVRHeadsetStreamingEnabled);
            Createmethod(28, SteamUtilities, SetVRHeadsetStreamingEnabled);
            Createmethod(29, SteamUtilities, IsSteamChinaLauncher);
            Createmethod(30, SteamUtilities, InitFilterText1);
            Createmethod(31, SteamUtilities, FilterText1);
            Createmethod(32, SteamUtilities, GetIPv6ConnectivityState);
        };
    };

    struct Steamutilitiesloader
    {
        Steamutilitiesloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::UTILS, "SteamUtilities001", SteamUtilities001);
            Register(Interfacetype_t::UTILS, "SteamUtilities002", SteamUtilities002);
            Register(Interfacetype_t::UTILS, "SteamUtilities003", SteamUtilities003);
            Register(Interfacetype_t::UTILS, "SteamUtilities004", SteamUtilities004);
            Register(Interfacetype_t::UTILS, "SteamUtilities005", SteamUtilities005);
            Register(Interfacetype_t::UTILS, "SteamUtilities006", SteamUtilities006);
            Register(Interfacetype_t::UTILS, "SteamUtilities007", SteamUtilities007);
            Register(Interfacetype_t::UTILS, "SteamUtilities008", SteamUtilities008);
            Register(Interfacetype_t::UTILS, "SteamUtilities009", SteamUtilities009);
            Register(Interfacetype_t::UTILS, "SteamUtilities010", SteamUtilities010);
        }
    };
    static Steamutilitiesloader Interfaceloader{};
}
