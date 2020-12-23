/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    /*
        Appinfo_%u.json layout:
        {
            "DLC" : [ { "Friendlyname": "", "Filename": "", "ID": 0 } ],
            "Appdata": [ { "Key" : "Value" } ],
            "Languages": [ "", "" ]

        }
    */
    static nlohmann::json *pAppdata{}, sAppdata{};
    static nlohmann::json *getAppdata()
    {
        if (pAppdata) return pAppdata;

        const auto Filename = va(L"./Ayria/Assets/Steam/Appinfo_%u.json", Steam.ApplicationID);
        const auto Filebuffer = FS::Readfile<char>(Filename);
        if(!Filebuffer.empty()) sAppdata = ParseJSON(Filebuffer);
        pAppdata = &sAppdata;
        return pAppdata;
    }

    struct SteamApps
    {
        int GetAppData(uint32_t nAppID, const char *pchKey, char *pchValue, int cchValueMax)
        {
            const auto Object = getAppdata();
            const auto Appdata = Object->value("Appdata", nlohmann::json::array());
            const auto Value = Appdata.value(pchKey, std::string());

            if (Value.empty())
            {
                Debugprint(va("%s: %s : %s", __FUNCTION__, pchKey, "MISSING"));
                return 0;
            }
            else
            {
                Debugprint(va("%s: %s : %s", __FUNCTION__, pchKey, Value.c_str()));
                std::strncpy(pchValue, Value.c_str(), cchValueMax);
                return (int)std::strlen(pchValue);
            }
        }
        bool BIsSubscribed()
        {
            return true;
        }
        bool BIsLowViolence()
        {
            Traceprint();
            return false;
        }
        bool BIsCybercafe()
        {
            Traceprint();
            return false;
        }
        bool BIsVACBanned()
        {
            return false;
        }
        const char *GetCurrentGameLanguage()
        {
            Traceprint();
            static auto Locale = Steam.Steamlocale.asUTF8();
            return (char *)Locale.c_str();
        }
        const char *GetAvailableGameLanguages()
        {
            static std::u8string Result{};
            if (!Result.empty()) return (char *)Result.c_str();

            const auto Object = getAppdata();
            auto Languages = Object->value("Languages", std::unordered_set<std::u8string>());
            Languages.insert(Steam.Steamlocale.asUTF8());

            for (const auto &Language : Languages)
            {
                Result += Language;
                Result += u8',';
            }
            Result.pop_back();

            return (char *)Result.c_str();
        }
        bool BIsSubscribedApp(uint32_t nAppID)
        {
            return true;
        }
        bool BIsDlcInstalled(uint32_t nAppID)
        {
            const auto Object = getAppdata();
            const auto DLC = Object->value("DLC", nlohmann::json::array());

            for (const auto &Item : DLC)
            {
                if (Item.value("ID", uint32_t()) == nAppID)
                {
                    return FS::Fileexists(Item.value("Filename", "./Ayria/DEV_DLC"));
                }
            }

            return FS::Fileexists(L"./Ayria/DEV_DLC");
        }
        uint32_t GetEarliestPurchaseUnixTime(uint32_t nAppID)
        {
            return 0;
        }
        bool BIsSubscribedFromFreeWeekend()
        {
            Traceprint();
            return false;
        }
        int GetDLCCount()
        {
            const auto Object = getAppdata();
            const auto DLC = Object->value("DLC", nlohmann::json::array());

            return (int)DLC.size();
        }
        bool BGetDLCDataByIndex(int iDLC, uint32_t *pAppID, bool *pbAvailable, char *pchName, int cchNameBufferSize)
        {
            const auto Object = getAppdata();
            const auto DLC = Object->value("DLC", nlohmann::json::array());

            for (const auto &Item : DLC)
            {
                if (iDLC != 0) iDLC--;
                else
                {
                    *pbAvailable = true;
                    *pAppID = Item.value("ID", 0);
                    std::strncpy(pchName, Item.value("Friendlyname", "").c_str(), cchNameBufferSize);
                    return true;
                }
            }

            return false;
        }
        void InstallDLC(uint32_t nAppID) const
        {
            Infoprint(va("Want to install DLC %u..", nAppID));
        }
        void UninstallDLC(uint32_t nAppID) const
        {
            Infoprint(va("Want to uninstall DLC %u..", nAppID));
        }
        void RequestAppProofOfPurchaseKey(uint32_t nAppID)
        {
            // Deprecated.
            Traceprint();
            return;
        }
        bool GetCurrentBetaName(char *pchName, int cchNameBufferSize)
        {
            return false;
        }
        bool MarkContentCorrupt(bool bMissingFilesOnly)
        {
            Traceprint();
            return false;
        }
        uint32_t GetInstalledDepots0(uint32_t *pvecDepots, uint32_t cMaxDepots)
        {
            Traceprint();
            return 0;
        }
        uint32_t GetAppInstallDir(uint32_t appID, char *pchFolder, uint32_t cchFolderBufferSize)
        {
            Debugprint(va("%s: %u", __FUNCTION__, appID));
            return GetCurrentDirectoryA(cchFolderBufferSize, pchFolder);
        }
        bool BIsAppInstalled(uint32_t appID)
        {
            return true;
        }
        uint32_t GetInstalledDepots1(uint32_t appID, uint32_t *pvecDepots, uint32_t cMaxDepots)
        {
            Traceprint();
            return 0;
        }
        CSteamID GetAppOwner()
        {
            return Steam.XUID;
        }
        const char *GetLaunchQueryParam(const char *pchKey)
        {
            auto Offset = std::strstr(GetCommandLineA(), pchKey);
            if (!Offset) return "";

            Offset += strlen(pchKey) + 1;
            return Offset;
        }
        bool GetDlcDownloadProgress(uint32_t nAppID, uint64_t *punBytesDownloaded, uint64_t *punBytesTotal)
        {
            Traceprint();
            return false;
        }
        int GetAppBuildId()
        {
            Traceprint();
            return 0;
        }
        uint64_t RegisterActivationCode(const char *pchActivationCode)
        {
            // Steam internal.
            Traceprint();
            return 0;
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
        }
    };
    static Steamappsloader Interfaceloader{};
}
