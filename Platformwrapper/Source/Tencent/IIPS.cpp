/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT

    Work in progress.
*/

#include <winternl.h>
#include <Stdinclude.hpp>
#include "../Common/Common.hpp"
#pragma warning(disable : 4100)

namespace IIPS
{
    struct Interface5 *Original;
    struct Interface5
    {
        #define Method(Type, Name, ArgsIN, ArgsOUT) virtual Type Name ArgsIN { Traceprint(); return Original->Name ArgsOUT; }

        Method(int, Constructor, (bool a1), (a1));
        Method(bool, Initialize, (char *a1), (a1));
        Method(bool, Uninitialize, (), ());
        Method(int, GetFileNameByFileIndex, (int a1), (a1));
        Method(int, GetFileIndexByFileName1, (char *a1), (a1));
        Method(int, GetFileIndexByFileName2, (char *a1), (a1));
        Method(int, GetFileIndexByArchiveName, (char *a1, int a2), (a1, a2));
        Method(int, GetFileCount, (), ());
        Method(bool, ReadDataByFileIndex, (int a1, int a2, int a3, int *a4), (a1, a2, a3, a4));
        Method(bool, GetFileSize, (int a1, int a2), (a1, a2));
        Method(bool, GetFileCompressSize, (int a1, int a2), (a1, a2));
        Method(bool, GetFileMD5, (int a1, char *a2), (a1, a2));
        Method(uint8_t, IsFileReady, (int a1), (a1));
        Method(bool, IsDirectory, (int a1), (a1));
        Method(uint32_t, Sync, (int a1, int a2, int a3, int a4, uint8_t a5), (a1, a2, a3, a4, a5));
        Method(void, CancelSync, (int a1), (a1));
        Method(bool, GetSyncInfo1, (int a1, char *a2), (a1, a2));
        Method(bool, SetSyncPriority, (int a1, int a2, bool a3), (a1, a2, a3));
        Method(int, LoadConfiguration, (int a1), (a1));
        Method(int, Method019, (int a1, char *a2, wchar_t *a3, int a4), (a1, a2, a3, a4));
        Method(bool, EnableDownload, (bool a1), (a1));
        Method(int, Method021, (), ());
        Method(int, Method022, (), ());
        Method(bool, RestoreFile, (int a1, int a2, int a3), (a1, a2, a3));
        Method(bool, SetUIN, (int a1), (a1));
        Method(int, Method025, (), ());
        Method(int, Method026, (int a1), (a1));
        Method(bool, IsDirectoryReady, (int a1), (a1));
        Method(bool, RestoreDirectory, (int a1, int a2, int a3), (a1, a2, a3));
        Method(int, IIPSFindFirstFile, (int a1, int a2), (a1, a2));
        Method(int, IIPSFindNextFile, (void *a1), (a1));
        Method(bool, IIPSFindClose, (int a1), (a1));
        Method(int, GetPackagesInfo, (int a1, int a2, int a3), (a1, a2, a3));
        Method(void, SetDownloadSpeedLimit1, (int a1), (a1));
        Method(void, SetDownloadSpeedLimit2, (int a1), (a1));
        Method(bool, SetDLAttribute, (int a1, int a2), (a1, a2));
        Method(bool, SetStatistic, (int a1, int a2), (a1, a2));
        Method(bool, SetHostAction1, (int a1, int a2, uint8_t a3), (a1, a2, a3));
        Method(bool, SetHostAction2, (int a1, uint8_t *a2), (a1, a2));
        Method(int, IFSPackageFindFirstFile, (int a1, void *a2, int a3), (a1, a2, a3));
        Method(int, IFSPackageFindNextFile, (bool a1, void *a2), (a1, a2));
        Method(bool, IFSPackageFindClose, (int a1), (a1));
        Method(int, GetFileIndexInSingleArchiveByName, (int a1, void *a2, int a3), (a1, a2, a3));
        Method(bool, GetSyncInfo2, (int a1, int *a2), (a1, a2));
        Method(bool, UNK_Method044, (), ());
        Method(bool, UNK_Method045, (), ());
        Method(bool, UNK_Method046, (), ());
        Method(bool, UNK_Method047, (), ());
        Method(bool, UNK_Method048, (), ());
        Method(bool, UNK_Method049, (), ());
        Method(bool, UNK_Method050, (), ());
        Method(bool, UNK_Method051, (), ());
        Method(bool, UNK_Method052, (), ());
        Method(bool, UNK_Method053, (), ());
        Method(bool, UNK_Method054, (), ());
        Method(bool, UNK_Method055, (), ());
        Method(bool, UNK_Method056, (), ());
        Method(bool, UNK_Method057, (), ());
        Method(bool, UNK_Method058, (), ());
        Method(bool, UNK_Method059, (), ());
        Method(bool, UNK_Method060, (), ());
        Method(bool, UNK_Method061, (), ());
        Method(bool, UNK_Method062, (), ());
        Method(bool, UNK_Method063, (), ());
        Method(bool, UNK_Method064, (), ());
        Method(bool, UNK_Method065, (), ());
        Method(bool, UNK_Method066, (), ());
        Method(bool, UNK_Method067, (), ());
        Method(bool, UNK_Method068, (), ());
        Method(bool, UNK_Method069, (), ());
    };

}

void *Originalresource;
extern "C" EXPORT_ATTR void *GetResourceManager()
{
    Traceprint();

    IIPS::Original = (IIPS::Interface5 *)((decltype(GetResourceManager) *)Originalresource)();
    return new IIPS::Interface5();
}
