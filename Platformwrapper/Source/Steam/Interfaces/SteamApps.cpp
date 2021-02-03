/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct SteamApps
    {
        enum ERegisterActivationCodeResult : uint32_t
        {
            k_ERegisterActivationCodeResultOK = 0,
            k_ERegisterActivationCodeResultFail = 1,
            k_ERegisterActivationCodeResultAlreadyRegistered = 2,
            k_ERegisterActivationCodeResultTimeout = 3,
            k_ERegisterActivationCodeAlreadyOwned = 4,
        };

        bool BGetDLCDataByIndex(int32_t iDLC, AppID_t *pAppID, bool *pbAvailable, char *pchName, int32_t cchNameBufferSize)
        {
            static const auto DLCInfo = []
            {
                const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Storage/SteamApps.json"));
                const auto App = Config.value(va("%u", Global.ApplicationID));
                return App.value<JSON::Array_t>("DLCInfo");
            }();

            // Off-by-one error in the game?
            if (DLCInfo.size() <= iDLC) return false;

            for (const auto &DLC : DLCInfo)
            {
                if (iDLC--) continue;
                if (!DLC.contains("ID") || !DLC.contains("Name") || !DLC.contains("Checkfile"))
                {
                    Errorprint(va("Malformed DLC data in SteamApps.json for app %u", Global.ApplicationID));
                    return false;
                }

                const auto Name = DLC.value<std::string>("Name");
                std::memcpy(pchName, Name.c_str(), std::min(int32_t(Name.size()), cchNameBufferSize));
                *pbAvailable = FS::Fileexists(DLC.value<std::string>("Checkfile"));
                *pAppID = DLC.value<uint32_t>("ID");
                break;
            }

            return true;
        }
        bool GetDlcDownloadProgress(AppID_t nAppID, uint64_t *punBytesDownloaded, uint64_t *punBytesTotal)
        {
            if (BIsDlcInstalled(nAppID))
            {
                *punBytesDownloaded = 100;
                *punBytesTotal = 100;
                return true;
            }

            return false;
        }
        SteamAPICall_t GetFileDetails(const char *pszFileName)
        {
            Debugprint(va("%s: %s", __FUNCTION__, pszFileName));

            const auto Filebuffer = FS::Readfile(pszFileName);
            const auto SHA = Hash::SHA1(Filebuffer.data(), Filebuffer.size());

            const auto Request = new Callbacks::FileDetailsResult_t();
            Request->m_eResult = Filebuffer.empty() ? EResult::k_EResultFileNotFound : EResult::k_EResultOK;
            std::memcpy(Request->m_FileSHA, SHA.data(), std::min(size_t(20), SHA.size()));
            Request->m_ulFileSize = Filebuffer.size();

            const auto RequestID = Callbacks::Createrequest();
            Callbacks::Completerequest(RequestID, Callbacks::Types::FileDetailsResult_t, Request);

            return RequestID;
        }
        bool BIsDlcInstalled(AppID_t appID)
        {
            static const auto DLCInfo = []
            {
                const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Storage/SteamApps.json"));
                const auto App = Config.value(va("%u", Global.ApplicationID));
                return App.value<JSON::Array_t>("DLCInfo");
            }();

            for (const auto &DLC : DLCInfo)
            {
                if (appID == DLC.value<uint32_t>("ID"))
                {
                    return FS::Fileexists(DLC.value<std::string>("Checkfile"));
                }
            }

            return FS::Fileexists(L"./Ayria/DEV_DLC");
        }
        bool BIsAppInstalled(AppID_t appID)
        {
            // Naturally the current app is installed.
            if (appID == Global.ApplicationID)
                return true;

            do
            {
                HKEY Registrykey;
                if (RegOpenKeyW(HKEY_LOCAL_MACHINE, va(L"Software\\Valve\\Steam\\Apps\\%u", Global.ApplicationID).c_str(), &Registrykey))
                    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, va(L"Software\\Wow6432Node\\Valve\\Steam\\Apps\\%u", Global.ApplicationID).c_str(), &Registrykey))
                        break;

                DWORD Boolean{}; DWORD Size{ sizeof(DWORD) };
                if (ERROR_SUCCESS == RegQueryValueExW(Registrykey, L"Installed", nullptr, nullptr, (LPBYTE)&Boolean, &Size))
                    return !!Boolean;

            } while (false);

            return false;
        }
        void UninstallDLC(AppID_t nAppID)
        {
            Infoprint(va("The game wants to uninstall DLC ID %u", nAppID));
        }
        void InstallDLC(AppID_t nAppID)
        {
            Infoprint(va("The game wants to install DLC ID %u", nAppID));
        }
        int32_t GetDLCCount()
        {
            static const auto DLCInfo = []
            {
                const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Storage/SteamApps.json"));
                const auto App = Config.value(va("%u", Global.ApplicationID));
                return App.value<JSON::Array_t>("DLCInfo");
            }();

            return int32_t(DLCInfo.size());
        }

        int32_t GetAppData(AppID_t nAppID, const char *pchKey, char *pchValue, int32_t cchValueMax)
        {
            const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Storage/SteamApps.json"));
            const auto Appdata = Config.value(va("%u", nAppID)).value("Appdata");
            const auto Value = Appdata.value(pchKey, ""s);

            std::memcpy(pchValue, Value.c_str(), std::min(int32_t(Value.size()), cchValueMax));
            return std::min(int32_t(Value.size()), cchValueMax);
        }
        uint32_t GetAppInstallDir(AppID_t appID, char *pchFolder, uint32_t cchFolderBufferSize)
        {
            // Some developers are a bit silly.
            if (appID == Global.ApplicationID)
            {
                return GetCurrentDirectoryA(cchFolderBufferSize, pchFolder);
            }

            // We need to contact Steam for this information, maybe in a future Ayria version.
            Traceprint();
            return 0;
        }
        uint32_t GetInstalledDepots1(AppID_t appID, DepotID_t *pvecDepots, uint32_t cMaxDepots)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetInstalledDepots0(DepotID_t *pvecDepots, uint32_t cMaxDepots)
        {
            return GetInstalledDepots1(Global.ApplicationID, pvecDepots, cMaxDepots);
        }
        int32_t GetLaunchCommandLine(char *pszCommandLine, int32_t cubCommandLine)
        {
            std::string Line = GetCommandLineA();
            std::memcpy(pszCommandLine, Line.c_str(), std::min(int32_t(Line.size()), cubCommandLine));
            return std::min(int32_t(Line.size()), cubCommandLine);
        }
        const char *GetLaunchQueryParam(const char *pchKey)
        {
            const auto Line = std::strstr(GetCommandLineA(), "?");
            if (!Line) return "";

            const auto Tokens = Tokenizestring_s(Line + 1, "=;");
            if (Tokens.size() & 1)
            {
                Errorprint(va("%s: Malformed commandline.", __FUNCTION__));
                return "";
            }

            for (int i = 0; i < Tokens.size(); i += 2)
            {
                // Values beginning with @ should be ignored.
                if (0 == std::strcmp(pchKey, Tokens[i].c_str()) && Tokens[i + 1][0] != '@')
                {
                    static std::string Heapstring;
                    Heapstring = Tokens[i + 1];
                    return Heapstring.c_str();
                }
            }

            return "";
        }
        int32_t GetAppBuildId()
        {
            // This would need to be synced with Steam.
            return 0;
        }
        SteamID_t GetAppOwner()
        {
            // For using family-sharing.
            return Global.XUID;
        }

        bool BIsTimedTrial(uint32_t *punSecondsAllowed, uint32_t *punSecondsPlayed)
        {
            return false;
        }
        bool BIsSubscribedFromFamilySharing()
        {
            return false;
        }
        bool BIsSubscribedApp(AppID_t appID)
        {
            return BIsAppInstalled(appID);
        }
        bool BIsSubscribedFromFreeWeekend()
        {
            return false;
        }
        bool BIsLowViolence()
        {
            return false;
        }
        bool BIsSubscribed()
        {
            return true;
        }
        bool BIsCybercafe()
        {
            return false;
        }
        bool BIsVACBanned()
        {
            return false;
        }

        bool GetCurrentBetaName(char *pchName, int32_t cchNameBufferSize)
        {
            return false;
        }
        const char *GetAvailableGameLanguages()
        {
            static const auto Appinfo = []
            {
                const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Storage/SteamApps.json"));
                return Config.value(va("%u", Global.ApplicationID));
            }();

            std::string List{ *Global.Locale };
            for (const std::string Item : Appinfo.value<JSON::Array_t>("Languages"))
            {
                List += ",";
                List += Item;
            }

            static std::string Heapresult;
            Heapresult = std::move(List);
            return Heapresult.c_str();
        }
        const char *GetCurrentGameLanguage()
        {
            return Global.Locale->c_str();
        }

        SteamAPICall_t RegisterActivationCode(const char *pchActivationCode)
        {
            // Activation seems to only be relevant on consoles.
            Debugprint(va("%s: %s", __FUNCTION__, pchActivationCode));

            const auto Request = new Callbacks::RegisterActivationCodeResponse_t();
            Request->m_eResult = k_ERegisterActivationCodeResultFail;

            const auto RequestID = Callbacks::Createrequest();
            Callbacks::Completerequest(RequestID, Callbacks::Types::RegisterActivationCodeResponse_t, Request);

            return RequestID;
        }
        uint32_t GetEarliestPurchaseUnixTime(AppID_t nAppID)
        {
            return 1000000000; // 2001-09-09
        }
        void RequestAppProofOfPurchaseKey(AppID_t nAppID)
        {
            Warningprint("The game wants a proof of purchase token, inform the developer.");

            const auto Request = new Callbacks::AppProofOfPurchaseKeyResponse_t();
            Request->m_eResult = EResult::k_EResultOK;
            Request->m_cchKeyLength = 0;
            Request->m_nAppID = nAppID;

            Callbacks::Completerequest(Callbacks::Createrequest(), Callbacks::Types::AppProofOfPurchaseKeyResponse_t, Request);
        }
        void RequestAllProofOfPurchaseKeys() {}

        bool MarkContentCorrupt(bool bMissingFilesOnly)
        {
            Errorprint("The game thinks that this install is corrupted.");
            return true;
        }

    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamApps001 : Interface_t
    {
        SteamApps001()
        {
            Createmethod(0, SteamApps, GetAppData);
        }
    };
    struct SteamApps002 : Interface_t
    {
        SteamApps002()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
        };
    };
    struct SteamApps003 : Interface_t
    {
        SteamApps003()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
        };
    };
    struct SteamApps004 : Interface_t
    {
        SteamApps004()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
            Createmethod(8, SteamApps, GetEarliestPurchaseUnixTime);
            Createmethod(9, SteamApps, BIsSubscribedFromFreeWeekend);
            Createmethod(10, SteamApps, GetDLCCount);
            Createmethod(11, SteamApps, BGetDLCDataByIndex);
            Createmethod(12, SteamApps, InstallDLC);
            Createmethod(13, SteamApps, UninstallDLC);
        };
    };
    struct SteamApps005 : Interface_t
    {
        SteamApps005()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
            Createmethod(8, SteamApps, GetEarliestPurchaseUnixTime);
            Createmethod(9, SteamApps, BIsSubscribedFromFreeWeekend);
            Createmethod(10, SteamApps, GetDLCCount);
            Createmethod(11, SteamApps, BGetDLCDataByIndex);
            Createmethod(12, SteamApps, InstallDLC);
            Createmethod(13, SteamApps, UninstallDLC);
            Createmethod(14, SteamApps, RequestAppProofOfPurchaseKey);
            Createmethod(15, SteamApps, GetCurrentBetaName);
            Createmethod(16, SteamApps, MarkContentCorrupt);
            Createmethod(17, SteamApps, GetInstalledDepots0);
            Createmethod(18, SteamApps, GetAppInstallDir);
            Createmethod(19, SteamApps, BIsAppInstalled);
        };
    };
    struct SteamApps006 : Interface_t
    {
        SteamApps006()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
            Createmethod(8, SteamApps, GetEarliestPurchaseUnixTime);
            Createmethod(9, SteamApps, BIsSubscribedFromFreeWeekend);
            Createmethod(10, SteamApps, GetDLCCount);
            Createmethod(11, SteamApps, BGetDLCDataByIndex);
            Createmethod(12, SteamApps, InstallDLC);
            Createmethod(13, SteamApps, UninstallDLC);
            Createmethod(14, SteamApps, RequestAppProofOfPurchaseKey);
            Createmethod(15, SteamApps, GetCurrentBetaName);
            Createmethod(16, SteamApps, MarkContentCorrupt);
            Createmethod(17, SteamApps, GetInstalledDepots1);
            Createmethod(18, SteamApps, GetAppInstallDir);
            Createmethod(19, SteamApps, BIsAppInstalled);
            Createmethod(20, SteamApps, GetAppOwner);
            Createmethod(21, SteamApps, GetLaunchQueryParam);
        };
    };
    struct SteamApps007 : Interface_t
    {
        SteamApps007()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
            Createmethod(8, SteamApps, GetEarliestPurchaseUnixTime);
            Createmethod(9, SteamApps, BIsSubscribedFromFreeWeekend);
            Createmethod(10, SteamApps, GetDLCCount);
            Createmethod(11, SteamApps, BGetDLCDataByIndex);
            Createmethod(12, SteamApps, InstallDLC);
            Createmethod(13, SteamApps, UninstallDLC);
            Createmethod(14, SteamApps, RequestAppProofOfPurchaseKey);
            Createmethod(15, SteamApps, GetCurrentBetaName);
            Createmethod(16, SteamApps, MarkContentCorrupt);
            Createmethod(17, SteamApps, GetInstalledDepots1);
            Createmethod(18, SteamApps, GetAppInstallDir);
            Createmethod(19, SteamApps, BIsAppInstalled);
            Createmethod(20, SteamApps, GetAppOwner);
            Createmethod(21, SteamApps, GetLaunchQueryParam);
            Createmethod(22, SteamApps, GetDlcDownloadProgress);
            Createmethod(23, SteamApps, GetAppBuildId);
            Createmethod(24, SteamApps, RegisterActivationCode);
        };
    };
    struct SteamApps008 : Interface_t
    {
        SteamApps008()
        {
            Createmethod(0, SteamApps, BIsSubscribed);
            Createmethod(1, SteamApps, BIsLowViolence);
            Createmethod(2, SteamApps, BIsCybercafe);
            Createmethod(3, SteamApps, BIsVACBanned);
            Createmethod(4, SteamApps, GetCurrentGameLanguage);
            Createmethod(5, SteamApps, GetAvailableGameLanguages);
            Createmethod(6, SteamApps, BIsSubscribedApp);
            Createmethod(7, SteamApps, BIsDlcInstalled);
            Createmethod(8, SteamApps, GetEarliestPurchaseUnixTime);
            Createmethod(9, SteamApps, BIsSubscribedFromFreeWeekend);
            Createmethod(10, SteamApps, GetDLCCount);
            Createmethod(11, SteamApps, BGetDLCDataByIndex);
            Createmethod(12, SteamApps, InstallDLC);
            Createmethod(13, SteamApps, UninstallDLC);
            Createmethod(14, SteamApps, RequestAppProofOfPurchaseKey);
            Createmethod(15, SteamApps, GetCurrentBetaName);
            Createmethod(16, SteamApps, MarkContentCorrupt);
            Createmethod(17, SteamApps, GetInstalledDepots1);
            Createmethod(18, SteamApps, GetAppInstallDir);
            Createmethod(19, SteamApps, BIsAppInstalled);
            Createmethod(20, SteamApps, GetAppOwner);
            Createmethod(21, SteamApps, GetLaunchQueryParam);
            Createmethod(22, SteamApps, GetDlcDownloadProgress);
            Createmethod(23, SteamApps, GetAppBuildId);
            Createmethod(24, SteamApps, RequestAllProofOfPurchaseKeys);
            Createmethod(25, SteamApps, GetFileDetails);
            Createmethod(26, SteamApps, GetLaunchCommandLine);
            Createmethod(27, SteamApps, BIsSubscribedFromFamilySharing);
            Createmethod(28, SteamApps, BIsTimedTrial);
        };
    };

    struct Steamappsloader
    {
        Steamappsloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::APPS, "SteamApps001", SteamApps001);
            Register(Interfacetype_t::APPS, "SteamApps002", SteamApps002);
            Register(Interfacetype_t::APPS, "SteamApps003", SteamApps003);
            Register(Interfacetype_t::APPS, "SteamApps004", SteamApps004);
            Register(Interfacetype_t::APPS, "SteamApps005", SteamApps005);
            Register(Interfacetype_t::APPS, "SteamApps006", SteamApps006);
            Register(Interfacetype_t::APPS, "SteamApps007", SteamApps007);
            Register(Interfacetype_t::APPS, "SteamApps008", SteamApps008);
        }
    };
    static Steamappsloader Interfaceloader{};
}
