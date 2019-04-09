/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    struct SteamClient
    {
        // Version 6.
        int32_t CreateSteamPipe()
        {
            Traceprint();
            return 0;
        }
        bool BReleaseSteamPipe(int32_t hSteamPipe)
        {
            Traceprint();
            return false;
        }
        int32_t CreateGlobalUser(int32_t *phSteamPipe)
        {
            Traceprint();
            return 0;
        }
        int32_t ConnectToGlobalUser(int32_t hSteamPipe)
        {
            Traceprint();
            return 0;
        }
        int32_t CreateLocalUser(int32_t *phSteamPipe)
        {
            Traceprint();
            return 0;
        }
        void ReleaseUser(int32_t hSteamPipe, int32_t hUser)
        {
            Traceprint();
            return;
        }
        void *GetISteamUser(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamUser();
        }
        void *GetIVAC(int32_t hSteamUser)
        {
            Traceprint();
            return nullptr;
        }
        void *GetISteamGameServer(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamGameServer();
        }
        void SetLocalIPBinding(uint32_t unIP, uint16_t usPort)
        {
            Traceprint();
            return;
        }
        const char *GetUniverseName(uint32_t eUniverse)
        {
            Traceprint();
            return "Public";
        }
        void *GetISteamFriends(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamFriends();
        }
        void *GetISteamUtils(int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamUtils();
        }
        void *GetISteamBilling(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return nullptr;
        }
        void *GetISteamMatchmaking(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamMatchmaking();
        }
        void *GetISteamContentServer(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return nullptr;
        }
        void *GetISteamApps(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamApps();
        }
        void *GetISteamMasterServerUpdater(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return nullptr;
        }
        void *GetISteamMatchmakingServers(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamMatchmakingServers();
        }
        void RunFrame()
        {
        }
        uint32_t GetIPCCallCount()
        {
            Traceprint();
            return 20;
        }

        // Version 7.
        void *GetISteamGenericInterface(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamInternal_CreateInterface(pchVersion);
        }
        void *GetISteamUserStats(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamUserStats();
        }
        void *GetISteamNetworking(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamNetworking();
        }
        void SetWarningMessageHook(void *pFunction)
        {
            Traceprint();
            return;
        }
        void *GetISteamRemoteStorage(int32_t hSteamuser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamRemoteStorage();
        }

        // Version 9.
        void *GetISteamGameServerStats(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamGameServerStats();
        }

        // Version 10.
        bool BShutdownIfAllPipesClosed()
        {
            Traceprint();
            return false;
        }
        void *GetISteamHTTP(int32_t hSteamuser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamHTTP();
        }

        // Version 11.
        void *GetISteamScreenshots(int32_t hSteamuser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamScreenshots();
        }

        // Version 12.
        void *GetISteamUnifiedMessages(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamUnifiedMessages();
        }
        void *GetISteamController(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamController();
        }

        // Version 13.
        void *GetISteamUGC(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamUGC();
        }
        void *GetISteamInventory(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamInventory();
        }
        void *GetISteamVideo(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamVideo();
        }
        void *GetISteamAppList(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamAppList();
        }

        // Version 14.
        void *GetISteamMusic(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamMusic();
        }

        // Version 15.
        void *GetISteamMusicRemote(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamMusicRemote();
        }

        // Version 16.
        void *GetISteamHTMLSurface(int32_t hSteamUser, int32_t hSteamPipe, const char *pchVersion)
        {
            Traceprint();
            return SteamHTMLSurface();
        }
        void Set_SteamAPI_CPostAPIResultInProcess(void(*)(uint64_t callHandle, void *, uint32_t unCallbackSize, int iCallbackNum))
        {
            Traceprint();
            return;
        }
        void Remove_SteamAPI_CPostAPIResultInProcess(void(*)(uint64_t callHandle, void *, uint32_t unCallbackSize, int iCallbackNum))
        {
            Traceprint();
            return;
        }
        void Set_SteamAPI_CCheckCallbackRegisteredInProcess(unsigned int(*)(int iCallbackNum))
        {
            Traceprint();
            return;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamClient001 : Interface_t
    {
        SteamClient001()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient002 : Interface_t
    {
        SteamClient002()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient003 : Interface_t
    {
        SteamClient003()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient004 : Interface_t
    {
        SteamClient004()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient005 : Interface_t
    {
        SteamClient005()
        {
            /*
                Missing SDK information.
            */
        };
    };
    struct SteamClient006 : Interface_t
    {
        SteamClient006()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, CreateGlobalUser);
            Createmethod(3, SteamClient, ConnectToGlobalUser);
            Createmethod(4, SteamClient, CreateLocalUser);
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
    struct SteamClient007 : Interface_t
    {
        SteamClient007()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient008 : Interface_t
    {
        SteamClient008()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient009 : Interface_t
    {
        SteamClient009()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient010 : Interface_t
    {
        SteamClient010()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient011 : Interface_t
    {
        SteamClient011()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient012 : Interface_t
    {
        SteamClient012()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient013 : Interface_t
    {
        SteamClient013()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient014 : Interface_t
    {
        SteamClient014()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient015 : Interface_t
    {
        SteamClient015()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient016 : Interface_t
    {
        SteamClient016()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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
    struct SteamClient017 : Interface_t
    {
        SteamClient017()
        {
            Createmethod(0, SteamClient, CreateSteamPipe);
            Createmethod(1, SteamClient, BReleaseSteamPipe);
            Createmethod(2, SteamClient, ConnectToGlobalUser);
            Createmethod(3, SteamClient, CreateLocalUser);
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

    struct Steamclientloader
    {
        Steamclientloader()
        {
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient001", new SteamClient001());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient002", new SteamClient002());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient003", new SteamClient003());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient004", new SteamClient004());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient005", new SteamClient005());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient006", new SteamClient006());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient007", new SteamClient007());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient008", new SteamClient008());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient009", new SteamClient009());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient010", new SteamClient010());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient011", new SteamClient011());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient012", new SteamClient012());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient013", new SteamClient013());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient014", new SteamClient014());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient015", new SteamClient015());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient016", new SteamClient016());
            Registerinterface(Interfacetype_t::CLIENT, "SteamClient017", new SteamClient017());
        }
    };
    static Steamclientloader Interfaceloader{};
}