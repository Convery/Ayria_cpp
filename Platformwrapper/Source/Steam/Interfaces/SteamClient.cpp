/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    using SteamAPI_PostAPIResultInProcess_t = void(*)(SteamAPICall_t callHandle, void *, uint32_t unCallbackSize, int iCallbackNum);
    using SteamAPI_CheckCallbackRegistered_t = uint32_t(*)( int iCallbackNum );
    using SteamAPIWarningMessageHook_t = void(__cdecl *)(int, const char *);

    // User innterface for GetIVAC if anyone wants to implement it.
    struct IVAC
    {
        virtual bool BVACCreateProcess(void *lpVACBlob, unsigned int cbBlobSize, const char *lpApplicationName,
            char *lpCommandLine, uint32_t dwCreationFlags, void *lpEnvironment, char *lpCurrentDirectory, uint32_t nGameID) = 0;

        virtual void KillAllVAC() = 0;

        virtual uint8_t *PbLoadVacBlob(int *pcbVacBlob) = 0;
        virtual void FreeVacBlob(uint8_t *pbVacBlob) = 0;

        virtual void RealHandleVACChallenge(int nClientGameID, uint8_t *pubChallenge, int cubChallenge) = 0;
    };

    struct SteamClient
    {
        HSteamPipe CreateSteamPipe()
        {
            Traceprint();
            return 1;
        }
        HSteamUser CreateGlobalUser(HSteamPipe hSteamPipe)
        {
            Traceprint();
            return 1;
        }
        HSteamUser ConnectToGlobalUser(HSteamPipe hSteamPipe)
        {
            Traceprint();
            return 1;
        }
        HSteamUser CreateLocalUser0(HSteamPipe *phSteamPipe)
        {
            Traceprint();
            return 1;
        }
        HSteamUser CreateLocalUser1(HSteamPipe *phSteamPipe, uint32_t eAccountType)
        {
            Traceprint();
            return 1;
        }
        bool BReleaseSteamPipe(HSteamPipe hSteamPipe)
        {
            return true;
        }
        bool BShutdownIfAllPipesClosed()
        {
            // TODO(tcn): Investigate if we should call std::exit here.
            return false;
        }
        void ReleaseUser(HSteamPipe hSteamPipe, HSteamUser hUser) {}
        void Remove_SteamAPI_CPostAPIResultInProcess(SteamAPI_PostAPIResultInProcess_t func) {}
        void RunFrame() {}
        void SetLocalIPBinding(uint32_t unIP, uint16_t usPort) { Traceprint(); }
        void SetWarningMessageHook(SteamAPIWarningMessageHook_t pFunction) { }
        void Set_SteamAPI_CCheckCallbackRegisteredInProcess(SteamAPI_CheckCallbackRegistered_t func) { }
        void Set_SteamAPI_CPostAPIResultInProcess(SteamAPI_PostAPIResultInProcess_t func) { }
        void DestroyAllInterfaces() { }

        void *GetISteamGenericInterface(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface(pchVersion);
        }
        const char *GetUniverseName(uint32_t eUniverse)
        {
            switch (eUniverse)
            {
                case 0: return "Invalid";
                case 1: return "Public";
                case 2: return "Beta";
                case 3: return "Internal";
                case 4: return "Dev";
                case 5: return "RC";
            }

            return "Public";
        }
        IVAC *GetIVAC(int32_t hSteamUser)
        {
            Traceprint();
            return nullptr;
        }
        uint32_t GetIPCCallCount()
        {
            // TODO(tcn): Investigate if this is used in release-builds.
            return 42;
        }

        void *GetISteamAppList(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMAPPLIST_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamApps(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMAPPS_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamBilling(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamBilling"s + pchVersion);
        }
        void *GetISteamContentServer(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamContentServer"s + pchVersion);
        }
        void *GetISteamController(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamController"s + pchVersion);
        }
        void *GetISteamFriends(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamFriends"s + pchVersion);
        }
        void *GetISteamGameSearch(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamMatchGameSearch"s + pchVersion);
        }
        void *GetISteamGameServer(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamGameServer"s + pchVersion);
        }
        void *GetISteamGameServerStats(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamGameServerStats"s + pchVersion);
        }
        void *GetISteamHTMLSurface(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMHTMLSURFACE_INTERFACE_VERSION_"s + pchVersion);
        }
        void *GetISteamHTTP(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMHTTP_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamInput(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamInput"s + pchVersion);
        }
        void *GetISteamInventory(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMINVENTORY_INTERFACE_V"s + pchVersion);
        }
        void *GetISteamMasterServerUpdater(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamMasterServerUpdater"s + pchVersion);
        }
        void *GetISteamMatchmaking(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamMatchMaking"s + pchVersion);
        }
        void *GetISteamMatchmakingServers(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamMatchMakingServers"s + pchVersion);
        }
        void *GetISteamMusic(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMMUSIC_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamMusicRemote(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMMUSICREMOTE_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamNetworking(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamNetworking"s + pchVersion);
        }
        void *GetISteamPS3OverlayRender()
        {
            return Fetchinterface("SteamPS3Overlay");
        }
        void *GetISteamParentalSettings(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMPARENTALSETTINGS_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamParties(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamParties"s + pchVersion);
        }
        void *GetISteamRemotePlay(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMREMOTEPLAY_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamRemoteStorage(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMREMOTESTORAGE_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamScreenshots(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMSCREENSHOTS_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamUGC(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMUGC_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamUnifiedMessages(HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMUNIFIEDMESSAGES_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamUser(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamUser"s + pchVersion);
        }
        void *GetISteamUserStats(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMUSERSTATS_INTERFACE_VERSION"s + pchVersion);
        }
        void *GetISteamUtils(HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("SteamUtils"s + pchVersion);
        }
        void *GetISteamVideo(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
        {
            return Fetchinterface("STEAMVIDEO_INTERFACE_V"s + pchVersion);
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamClient001 : Interface_t<>
    {
        SteamClient001()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient002 : Interface_t<>
    {
        SteamClient002()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient003 : Interface_t<>
    {
        SteamClient003()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient004 : Interface_t<>
    {
        SteamClient004()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient005 : Interface_t<>
    {
        SteamClient005()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient006 : Interface_t<21>
    {
        SteamClient006()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, CreateGlobalUser);
            Createmethod(3, SteamClient, ConnectToGlobalUser);
            Createmethod(4, SteamClient, CreateLocalUser0);
            Createmethod(5, SteamClient, ReleaseUser);
            Createmethod(6, SteamClient, GetISteamUser);
            Createmethod(7, SteamClient, GetIVAC);
            Createmethod(8, SteamClient, GetISteamGameServer);
            Createmethod(9, SteamClient, SetLocalIPBinding);
            Createmethod(10, SteamClient, GetUniverseName);
            Createmethod(11, SteamClient, GetISteamFriends);
            Createmethod(12, SteamClient, GetISteamUtils);
            Createmethod(13, SteamClient, GetISteamBilling);
            Createmethod(14, SteamClient, GetISteamMatchmaking);
            Createmethod(15, SteamClient, GetISteamContentServer);
            Createmethod(16, SteamClient, GetISteamApps);
            Createmethod(17, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(18, SteamClient, GetISteamMatchmakingServers);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
        };
    };
    struct SteamClient007 : Interface_t<22>
    {
        SteamClient007()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser0);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamContentServer);
            Createmethod(12, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(13, SteamClient, GetISteamMatchmakingServers);
            Createmethod(14, SteamClient, GetISteamGenericInterface);
            Createmethod(15, SteamClient, RunFrame);
            Createmethod(16, SteamClient, GetIPCCallCount);
            Createmethod(17, SteamClient, GetISteamUserStats);
            Createmethod(18, SteamClient, GetISteamApps);
            Createmethod(19, SteamClient, GetISteamNetworking);
            Createmethod(20, SteamClient, SetWarningMessageHook);
            Createmethod(21, SteamClient, GetISteamRemoteStorage);
        };
    };
    struct SteamClient008 : Interface_t<21>
    {
        SteamClient008()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(12, SteamClient, GetISteamMatchmakingServers);
            Createmethod(13, SteamClient, GetISteamGenericInterface);
            Createmethod(14, SteamClient, GetISteamUserStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, RunFrame);
            Createmethod(19, SteamClient, GetIPCCallCount);
            Createmethod(20, SteamClient, SetWarningMessageHook);
        };
    };
    struct SteamClient009 : Interface_t<22>
    {
        SteamClient009()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(12, SteamClient, GetISteamMatchmakingServers);
            Createmethod(13, SteamClient, GetISteamGenericInterface);
            Createmethod(14, SteamClient, GetISteamUserStats);
            Createmethod(15, SteamClient, GetISteamGameServerStats);
            Createmethod(16, SteamClient, GetISteamApps);
            Createmethod(17, SteamClient, GetISteamNetworking);
            Createmethod(18, SteamClient, GetISteamRemoteStorage);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
        };
    };
    struct SteamClient010 : Interface_t<24>
    {
        SteamClient010()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(12, SteamClient, GetISteamMatchmakingServers);
            Createmethod(13, SteamClient, GetISteamGenericInterface);
            Createmethod(14, SteamClient, GetISteamUserStats);
            Createmethod(15, SteamClient, GetISteamGameServerStats);
            Createmethod(16, SteamClient, GetISteamApps);
            Createmethod(17, SteamClient, GetISteamNetworking);
            Createmethod(18, SteamClient, GetISteamRemoteStorage);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
        };
    };
    struct SteamClient011 : Interface_t<25>
    {
        SteamClient011()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMasterServerUpdater);
            Createmethod(12, SteamClient, GetISteamMatchmakingServers);
            Createmethod(13, SteamClient, GetISteamGenericInterface);
            Createmethod(14, SteamClient, GetISteamUserStats);
            Createmethod(15, SteamClient, GetISteamGameServerStats);
            Createmethod(16, SteamClient, GetISteamApps);
            Createmethod(17, SteamClient, GetISteamNetworking);
            Createmethod(18, SteamClient, GetISteamRemoteStorage);
            Createmethod(19, SteamClient, GetISteamScreenshots);
            Createmethod(20, SteamClient, RunFrame);
            Createmethod(21, SteamClient, GetIPCCallCount);
            Createmethod(22, SteamClient, SetWarningMessageHook);
            Createmethod(23, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(24, SteamClient, GetISteamHTTP);
        };
    };
    struct SteamClient012 : Interface_t<26>
    {
        SteamClient012()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
        };
    };
    struct SteamClient013 : Interface_t<30>
    {
        SteamClient013()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
            Createmethod(26, SteamClient, GetISteamUGC);
            Createmethod(27, SteamClient, GetISteamInventory);
            Createmethod(28, SteamClient, GetISteamVideo);
            Createmethod(29, SteamClient, GetISteamAppList);
        };
    };
    struct SteamClient014 : Interface_t<31>
    {
        SteamClient014()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
            Createmethod(26, SteamClient, GetISteamUGC);
            Createmethod(27, SteamClient, GetISteamInventory);
            Createmethod(28, SteamClient, GetISteamVideo);
            Createmethod(29, SteamClient, GetISteamAppList);
            Createmethod(30, SteamClient, GetISteamMusic);
        };
    };
    struct SteamClient015 : Interface_t<32>
    {
        SteamClient015()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
            Createmethod(26, SteamClient, GetISteamUGC);
            Createmethod(27, SteamClient, GetISteamInventory);
            Createmethod(28, SteamClient, GetISteamVideo);
            Createmethod(29, SteamClient, GetISteamAppList);
            Createmethod(30, SteamClient, GetISteamMusic);
            Createmethod(31, SteamClient, GetISteamMusicRemote);
        };
    };
    struct SteamClient016 : Interface_t<36>
    {
        SteamClient016()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
            Createmethod(26, SteamClient, GetISteamUGC);
            Createmethod(27, SteamClient, GetISteamInventory);
            Createmethod(28, SteamClient, GetISteamVideo);
            Createmethod(29, SteamClient, GetISteamAppList);
            Createmethod(30, SteamClient, GetISteamMusic);
            Createmethod(31, SteamClient, GetISteamMusicRemote);
            Createmethod(32, SteamClient, GetISteamHTMLSurface);
            Createmethod(33, SteamClient, Set_SteamAPI_CPostAPIResultInProcess);
            Createmethod(34, SteamClient, Remove_SteamAPI_CPostAPIResultInProcess);
            Createmethod(35, SteamClient, Set_SteamAPI_CCheckCallbackRegisteredInProcess);
        };
    };
    struct SteamClient017 : Interface_t<36>
    {
        SteamClient017()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, RunFrame);
            Createmethod(20, SteamClient, GetIPCCallCount);
            Createmethod(21, SteamClient, SetWarningMessageHook);
            Createmethod(22, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(23, SteamClient, GetISteamHTTP);
            Createmethod(24, SteamClient, GetISteamUnifiedMessages);
            Createmethod(25, SteamClient, GetISteamController);
            Createmethod(26, SteamClient, GetISteamUGC);
            Createmethod(27, SteamClient, GetISteamAppList);
            Createmethod(28, SteamClient, GetISteamMusic);
            Createmethod(29, SteamClient, GetISteamMusicRemote);
            Createmethod(30, SteamClient, GetISteamHTMLSurface);
            Createmethod(31, SteamClient, Set_SteamAPI_CPostAPIResultInProcess);
            Createmethod(32, SteamClient, Remove_SteamAPI_CPostAPIResultInProcess);
            Createmethod(33, SteamClient, Set_SteamAPI_CCheckCallbackRegisteredInProcess);
            Createmethod(34, SteamClient, GetISteamInventory);
            Createmethod(35, SteamClient, GetISteamVideo);
        };
    };
    struct SteamClient018 : Interface_t<40>
    {
        SteamClient018()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, GetISteamGameSearch);
            Createmethod(20, SteamClient, RunFrame);
            Createmethod(21, SteamClient, GetIPCCallCount);
            Createmethod(22, SteamClient, SetWarningMessageHook);
            Createmethod(23, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(24, SteamClient, GetISteamHTTP);
            Createmethod(25, SteamClient, GetISteamUnifiedMessages);
            Createmethod(26, SteamClient, GetISteamController);
            Createmethod(27, SteamClient, GetISteamUGC);
            Createmethod(28, SteamClient, GetISteamAppList);
            Createmethod(29, SteamClient, GetISteamMusic);
            Createmethod(30, SteamClient, GetISteamMusicRemote);
            Createmethod(31, SteamClient, GetISteamHTMLSurface);
            Createmethod(32, SteamClient, Set_SteamAPI_CPostAPIResultInProcess);
            Createmethod(33, SteamClient, Remove_SteamAPI_CPostAPIResultInProcess);
            Createmethod(34, SteamClient, Set_SteamAPI_CCheckCallbackRegisteredInProcess);
            Createmethod(35, SteamClient, GetISteamInventory);
            Createmethod(36, SteamClient, GetISteamVideo);
            Createmethod(37, SteamClient, GetISteamParentalSettings);
            Createmethod(38, SteamClient, GetISteamInput);
            Createmethod(39, SteamClient, GetISteamParties);

        };
    };
    struct SteamClient019 : Interface_t<41>
    {
        SteamClient019()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, GetISteamGameSearch);
            Createmethod(20, SteamClient, RunFrame);
            Createmethod(21, SteamClient, GetIPCCallCount);
            Createmethod(22, SteamClient, SetWarningMessageHook);
            Createmethod(23, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(24, SteamClient, GetISteamHTTP);
            Createmethod(25, SteamClient, GetISteamUnifiedMessages);
            Createmethod(26, SteamClient, GetISteamController);
            Createmethod(27, SteamClient, GetISteamUGC);
            Createmethod(28, SteamClient, GetISteamAppList);
            Createmethod(29, SteamClient, GetISteamMusic);
            Createmethod(30, SteamClient, GetISteamMusicRemote);
            Createmethod(31, SteamClient, GetISteamHTMLSurface);
            Createmethod(32, SteamClient, Set_SteamAPI_CPostAPIResultInProcess);
            Createmethod(33, SteamClient, Remove_SteamAPI_CPostAPIResultInProcess);
            Createmethod(34, SteamClient, Set_SteamAPI_CCheckCallbackRegisteredInProcess);
            Createmethod(35, SteamClient, GetISteamInventory);
            Createmethod(36, SteamClient, GetISteamVideo);
            Createmethod(37, SteamClient, GetISteamParentalSettings);
            Createmethod(38, SteamClient, GetISteamInput);
            Createmethod(39, SteamClient, GetISteamParties);
            Createmethod(40, SteamClient, GetISteamRemotePlay);
        };
    };
    struct SteamClient020 : Interface_t<42>
    {
        SteamClient020()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser1);
            Createmethod(4, SteamClient, ReleaseUser);
            Createmethod(5, SteamClient, GetISteamUser);
            Createmethod(6, SteamClient, GetISteamGameServer);
            Createmethod(7, SteamClient, SetLocalIPBinding);
            Createmethod(8, SteamClient, GetISteamFriends);
            Createmethod(9, SteamClient, GetISteamUtils);
            Createmethod(10, SteamClient, GetISteamMatchmaking);
            Createmethod(11, SteamClient, GetISteamMatchmakingServers);
            Createmethod(12, SteamClient, GetISteamGenericInterface);
            Createmethod(13, SteamClient, GetISteamUserStats);
            Createmethod(14, SteamClient, GetISteamGameServerStats);
            Createmethod(15, SteamClient, GetISteamApps);
            Createmethod(16, SteamClient, GetISteamNetworking);
            Createmethod(17, SteamClient, GetISteamRemoteStorage);
            Createmethod(18, SteamClient, GetISteamScreenshots);
            Createmethod(19, SteamClient, GetISteamGameSearch);
            Createmethod(20, SteamClient, RunFrame);
            Createmethod(21, SteamClient, GetIPCCallCount);
            Createmethod(22, SteamClient, SetWarningMessageHook);
            Createmethod(23, SteamClient, BShutdownIfAllPipesClosed);
            Createmethod(24, SteamClient, GetISteamHTTP);
            Createmethod(25, SteamClient, GetISteamUnifiedMessages);
            Createmethod(26, SteamClient, GetISteamController);
            Createmethod(27, SteamClient, GetISteamUGC);
            Createmethod(28, SteamClient, GetISteamAppList);
            Createmethod(29, SteamClient, GetISteamMusic);
            Createmethod(30, SteamClient, GetISteamMusicRemote);
            Createmethod(31, SteamClient, GetISteamHTMLSurface);
            Createmethod(32, SteamClient, Set_SteamAPI_CPostAPIResultInProcess);
            Createmethod(33, SteamClient, Remove_SteamAPI_CPostAPIResultInProcess);
            Createmethod(34, SteamClient, Set_SteamAPI_CCheckCallbackRegisteredInProcess);
            Createmethod(35, SteamClient, GetISteamInventory);
            Createmethod(36, SteamClient, GetISteamVideo);
            Createmethod(37, SteamClient, GetISteamParentalSettings);
            Createmethod(38, SteamClient, GetISteamInput);
            Createmethod(39, SteamClient, GetISteamParties);
            Createmethod(40, SteamClient, GetISteamRemotePlay);
            Createmethod(41, SteamClient, DestroyAllInterfaces);
        };
    };

    struct Steamclientloader
    {
        Steamclientloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::CLIENT, "SteamClient001", SteamClient001);
            Register(Interfacetype_t::CLIENT, "SteamClient002", SteamClient002);
            Register(Interfacetype_t::CLIENT, "SteamClient003", SteamClient003);
            Register(Interfacetype_t::CLIENT, "SteamClient004", SteamClient004);
            Register(Interfacetype_t::CLIENT, "SteamClient005", SteamClient005);
            Register(Interfacetype_t::CLIENT, "SteamClient006", SteamClient006);
            Register(Interfacetype_t::CLIENT, "SteamClient007", SteamClient007);
            Register(Interfacetype_t::CLIENT, "SteamClient008", SteamClient008);
            Register(Interfacetype_t::CLIENT, "SteamClient009", SteamClient009);
            Register(Interfacetype_t::CLIENT, "SteamClient010", SteamClient010);
            Register(Interfacetype_t::CLIENT, "SteamClient011", SteamClient011);
            Register(Interfacetype_t::CLIENT, "SteamClient012", SteamClient012);
            Register(Interfacetype_t::CLIENT, "SteamClient013", SteamClient013);
            Register(Interfacetype_t::CLIENT, "SteamClient014", SteamClient014);
            Register(Interfacetype_t::CLIENT, "SteamClient015", SteamClient015);
            Register(Interfacetype_t::CLIENT, "SteamClient016", SteamClient016);
            Register(Interfacetype_t::CLIENT, "SteamClient017", SteamClient017);
            Register(Interfacetype_t::CLIENT, "SteamClient018", SteamClient018);
            Register(Interfacetype_t::CLIENT, "SteamClient019", SteamClient019);
            Register(Interfacetype_t::CLIENT, "SteamClient020", SteamClient020);
        }
    };
    static Steamclientloader Interfaceloader{};
}