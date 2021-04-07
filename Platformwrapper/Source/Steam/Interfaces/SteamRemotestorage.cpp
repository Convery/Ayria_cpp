/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    enum ERemoteStoragePublishedFileVisibility
    {
        k_ERemoteStoragePublishedFileVisibilityPublic = 0,
        k_ERemoteStoragePublishedFileVisibilityFriendsOnly = 1,
        k_ERemoteStoragePublishedFileVisibilityPrivate = 2,
        k_ERemoteStoragePublishedFileVisibilityUnlisted = 3,
    };
    struct RemoteStorageUpdatePublishedFileRequest_t
    {
        PublishedFileId_t m_unPublishedFileId;
        const char *m_pchFile;
        const char *m_pchPreviewFile;
        const char *m_pchTitle;
        const char *m_pchDescription;
        ERemoteStoragePublishedFileVisibility m_eVisibility;
        SteamParamStringArray_t *m_pTags;
        bool m_bUpdateFile;
        bool m_bUpdatePreviewFile;
        bool m_bUpdateTitle;
        bool m_bUpdateDescription;
        bool m_bUpdateVisibility;
        bool m_bUpdateTags;

        RemoteStorageUpdatePublishedFileRequest_t()
        {
            Initialize(0xFFFFFFFFFFFFFFFFULL);
        }
        RemoteStorageUpdatePublishedFileRequest_t(PublishedFileId_t unPublishedFileId)
        {
            Initialize(unPublishedFileId);
        }

        PublishedFileId_t GetPublishedFileId() { return m_unPublishedFileId; }

        void SetFile(const char *pchFile)
        {
            m_pchFile = pchFile;
            m_bUpdateFile = true;
        }

        const char *GetFile() { return m_pchFile; }
        bool BUpdateFile() { return m_bUpdateFile; }

        void SetPreviewFile(const char *pchPreviewFile)
        {
            m_pchPreviewFile = pchPreviewFile;
            m_bUpdatePreviewFile = true;
        }

        const char *GetPreviewFile() { return m_pchPreviewFile; }
        bool BUpdatePreviewFile() { return m_bUpdatePreviewFile; }

        void SetTitle(const char *pchTitle)
        {
            m_pchTitle = pchTitle;
            m_bUpdateTitle = true;
        }

        const char *GetTitle() { return m_pchTitle; }
        bool BUpdateTitle() { return m_bUpdateTitle; }

        void SetDescription(const char *pchDescription)
        {
            m_pchDescription = pchDescription;
            m_bUpdateDescription = true;
        }

        const char *GetDescription() { return m_pchDescription; }
        bool BUpdateDescription() { return m_bUpdateDescription; }

        void SetVisibility(ERemoteStoragePublishedFileVisibility eVisibility)
        {
            m_eVisibility = eVisibility;
            m_bUpdateVisibility = true;
        }

        const ERemoteStoragePublishedFileVisibility GetVisibility() { return m_eVisibility; }
        bool BUpdateVisibility() { return m_bUpdateVisibility; }

        void SetTags(SteamParamStringArray_t *pTags)
        {
            m_pTags = pTags;
            m_bUpdateTags = true;
        }

        SteamParamStringArray_t *GetTags() { return m_pTags; }
        bool BUpdateTags() { return m_bUpdateTags; }
        SteamParamStringArray_t **GetTagsPointer() { return &m_pTags; }

        void Initialize(PublishedFileId_t unPublishedFileId)
        {
            m_unPublishedFileId = unPublishedFileId;
            m_pchFile = 0;
            m_pchPreviewFile = 0;
            m_pchTitle = 0;
            m_pchDescription = 0;
            m_pTags = 0;

            m_bUpdateFile = false;
            m_bUpdatePreviewFile = false;
            m_bUpdateTitle = false;
            m_bUpdateDescription = false;
            m_bUpdateTags = false;
            m_bUpdateVisibility = false;
        }
    };

    // Per-user storage directory.
    static std::string Storagepath(std::string_view Path)
    {
        static std::string Basepath{ []()
        {
            const auto Tmp = va("./Ayria/Storage/Steam/%16X/", Global.XUID.FullID);
            std::filesystem::create_directories(Tmp.c_str());
            return Tmp;
        }() };

        while (!Path.empty())
        {
            const auto Char = Path.front();
            if (Char == '.' || Char == '/' || Char == '\\')
                Path.remove_prefix(1);
            else
                break;
        }

        return Basepath + std::string(Path);
    }

    struct SteamRemotestorage
    {
        enum EWorkshopEnumerationType
        {
            k_EWorkshopEnumerationTypeRankedByVote = 0,
            k_EWorkshopEnumerationTypeRecent = 1,
            k_EWorkshopEnumerationTypeTrending = 2,
            k_EWorkshopEnumerationTypeFavoritesOfFriends = 3,
            k_EWorkshopEnumerationTypeVotedByFriends = 4,
            k_EWorkshopEnumerationTypeContentByFriends = 5,
            k_EWorkshopEnumerationTypeRecentFromFollowedUsers = 6,
        };
        enum EWorkshopVideoProvider
        {
            k_EWorkshopVideoProviderNone = 0,
            k_EWorkshopVideoProviderYoutube = 1
        };
        enum ERemoteStoragePlatform
        {
            k_ERemoteStoragePlatformNone = 0,
            k_ERemoteStoragePlatformWindows = (1 << 0),
            k_ERemoteStoragePlatformOSX = (1 << 1),
            k_ERemoteStoragePlatformPS3 = (1 << 2),
            k_ERemoteStoragePlatformLinux = (1 << 3),
            k_ERemoteStoragePlatformSwitch = (1 << 4),
            k_ERemoteStoragePlatformAndroid = (1 << 5),
            k_ERemoteStoragePlatformIOS = (1 << 6),
            // NB we get one more before we need to widen some things

            k_ERemoteStoragePlatformAll = 0xffffffff
        };
        enum EWorkshopFileAction
        {
            k_EWorkshopFileActionPlayed = 0,
            k_EWorkshopFileActionCompleted = 1,
        };
        enum EWorkshopFileType
        {
            k_EWorkshopFileTypeFirst = 0,

            k_EWorkshopFileTypeCommunity			  = 0,		// normal Workshop item that can be subscribed to
            k_EWorkshopFileTypeMicrotransaction		  = 1,		// Workshop item that is meant to be voted on for the purpose of selling in-game
            k_EWorkshopFileTypeCollection			  = 2,		// a collection of Workshop or Greenlight items
            k_EWorkshopFileTypeArt					  = 3,		// artwork
            k_EWorkshopFileTypeVideo				  = 4,		// external video
            k_EWorkshopFileTypeScreenshot			  = 5,		// screenshot
            k_EWorkshopFileTypeGame					  = 6,		// Greenlight game entry
            k_EWorkshopFileTypeSoftware				  = 7,		// Greenlight software entry
            k_EWorkshopFileTypeConcept				  = 8,		// Greenlight concept
            k_EWorkshopFileTypeWebGuide				  = 9,		// Steam web guide
            k_EWorkshopFileTypeIntegratedGuide		  = 10,		// application integrated guide
            k_EWorkshopFileTypeMerch				  = 11,		// Workshop merchandise meant to be voted on for the purpose of being sold
            k_EWorkshopFileTypeControllerBinding	  = 12,		// Steam Controller bindings
            k_EWorkshopFileTypeSteamworksAccessInvite = 13,		// internal
            k_EWorkshopFileTypeSteamVideo			  = 14,		// Steam video
            k_EWorkshopFileTypeGameManagedItem		  = 15,		// managed completely by the game, not the user, and not shown on the web

            // Update k_EWorkshopFileTypeMax if you add values.
            k_EWorkshopFileTypeMax = 16
        };
        enum EUGCReadAction
        {
            // Keeps the file handle open unless the last byte is read.  You can use this when reading large files (over 100MB) in sequential chunks.
            // If the last byte is read, this will behave the same as k_EUGCRead_Close.  Otherwise, it behaves the same as k_EUGCRead_ContinueReading.
            // This value maintains the same behavior as before the EUGCReadAction parameter was introduced.
            k_EUGCRead_ContinueReadingUntilFinished = 0,

            // Keeps the file handle open.  Use this when using UGCRead to seek to different parts of the file.
            // When you are done seeking around the file, make a final call with k_EUGCRead_Close to close it.
            k_EUGCRead_ContinueReading = 1,

            // Frees the file handle.  Use this when you're done reading the content.
            // To read the file from Steam again you will need to call UGCDownload again.
            k_EUGCRead_Close = 2,
        };
        enum EWorkshopVote
        {
            k_EWorkshopVoteUnvoted = 0,
            k_EWorkshopVoteFor = 1,
            k_EWorkshopVoteAgainst = 2,
            k_EWorkshopVoteLater = 3,
        };

        ERemoteStoragePlatform GetSyncPlatforms(const char *pchFile)
        {
            return k_ERemoteStoragePlatformAll;
        }
        PublishedFileUpdateHandle_t CreatePublishedFileUpdateRequest(PublishedFileId_t unPublishedFileId);

        SteamAPICall_t CommitPublishedFileUpdate(PublishedFileUpdateHandle_t updateHandle);
        SteamAPICall_t DeletePublishedFile(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t EnumeratePublishedFilesByUserAction(EWorkshopFileAction eAction, uint32_t unStartIndex);
        SteamAPICall_t EnumeratePublishedWorkshopFiles(EWorkshopEnumerationType eEnumerationType, uint32_t unStartIndex, uint32_t unCount, uint32_t unDays, SteamParamStringArray_t *pTags, SteamParamStringArray_t *pUserTags);
        SteamAPICall_t EnumerateUserPublishedFiles(uint32_t unStartIndex);
        SteamAPICall_t EnumerateUserSharedWorkshopFiles(SteamID_t steamId, uint32_t unStartIndex, SteamParamStringArray_t *pRequiredTags, SteamParamStringArray_t *pExcludedTags);
        SteamAPICall_t EnumerateUserSubscribedFiles(uint32_t unStartIndex);
        SteamAPICall_t FileReadAsync(const char *pchFile, uint32_t nOffset, uint32_t cubToRead);
        SteamAPICall_t FileShare(const char *pchFile);
        SteamAPICall_t FileWriteAsync(const char *pchFile, const void *pvData, uint32_t cubData);
        SteamAPICall_t GetPublishedFileDetails0(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t GetPublishedFileDetails1(PublishedFileId_t unPublishedFileId, uint32_t unMaxSecondsOld);
        SteamAPICall_t GetPublishedItemVoteDetails(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t GetUserPublishedItemVoteDetails(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t PublishFile(const char *pchFile, const char *pchPreviewFile, AppID_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags);
        SteamAPICall_t PublishVideo0(const char *pchVideoURL, const char *pchPreviewFile, AppID_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags);
        SteamAPICall_t PublishVideo1(EWorkshopVideoProvider eVideoProvider, const char *pchVideoAccount, const char *pchVideoIdentifier, const char *pchPreviewFile, AppID_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags);
        SteamAPICall_t PublishWorkshopFile0(const char *pchFile, const char *pchPreviewFile, AppID_t nConsumerAppId, const char *pchTitle, const char *pchDescription, SteamParamStringArray_t *pTags);
        SteamAPICall_t PublishWorkshopFile1(const char *pchFile, const char *pchPreviewFile, AppID_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags, EWorkshopFileType eWorkshopFileType);
        SteamAPICall_t SetUserPublishedFileAction(PublishedFileId_t unPublishedFileId, EWorkshopFileAction eAction);
        SteamAPICall_t SubscribePublishedFile(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t UGCDownload0(UGCHandle_t hContent);
        SteamAPICall_t UGCDownload1(UGCHandle_t hContent, uint32_t unPriority);
        SteamAPICall_t UGCDownloadToLocation(UGCHandle_t hContent, const char *pchLocation, uint32_t unPriority);
        SteamAPICall_t UnsubscribePublishedFile(PublishedFileId_t unPublishedFileId);
        SteamAPICall_t UpdatePublishedFile(RemoteStorageUpdatePublishedFileRequest_t updatePublishedFileRequest);
        SteamAPICall_t UpdateUserPublishedItemVote(PublishedFileId_t unPublishedFileId, bool bVoteUp);

        UGCFileWriteStreamHandle_t FileWriteStreamOpen(const char *pchFile)
        {
            return (UGCFileWriteStreamHandle_t)std::fopen(Storagepath(pchFile).c_str(), "RB");
        }
        UGCHandle_t GetCachedUGCHandle(int32_t iCachedContent) { return 0xFFFFFFFFFFFFFFFFULL; }

        bool FileDelete(const char *pchFile)
        {
            return !!std::remove(Storagepath(pchFile).c_str());
        }
        bool FileExists(const char *pchFile)
        {
            return FS::Fileexists(Storagepath(pchFile));
        }
        bool FileFetch(const char *pchFile)
        {
            return FileExists(pchFile);
        }
        bool FileForget(const char *pchFile)
        {
            return FileExists(pchFile);
        }
        bool FilePersist(const char *pchFile)
        {
            return FileExists(pchFile);
        }
        bool FilePersisted(const char *pchFile)
        {
            // Not available in cloud-storage, only locally.
            return false;
        }
        bool FileReadAsyncComplete(SteamAPICall_t hReadCall, void *pvBuffer, uint32_t cubToRead);
        bool FileWrite(const char *pchFile, const void *pvData, int32_t cubData)
        {
            return FS::Writefile(Storagepath(pchFile), std::string_view((const char *)pvData, cubData));
        }
        bool FileWriteStreamCancel(UGCFileWriteStreamHandle_t writeHandle)
        {
            std::fclose((std::FILE *)writeHandle);
            return true;
        }
        bool FileWriteStreamClose(UGCFileWriteStreamHandle_t writeHandle)
        {
            std::fclose((std::FILE *)writeHandle);
            return true;
        }
        bool FileWriteStreamWriteChunk(UGCFileWriteStreamHandle_t writeHandle, const void *pvData, int32_t cubData);
        bool GetQuota0(int32_t *pnTotalBytes, int32_t *puAvailableBytes)
        {
            *pnTotalBytes = 0;
            *puAvailableBytes = INT32_MAX - 1;
            return true;
        }
        bool GetQuota1(uint64_t *pnTotalBytes, uint64_t *puAvailableBytes)
        {
            *pnTotalBytes = 0;
            *puAvailableBytes = 20ULL * INT32_MAX - 1ULL;
            return true;
        }
        bool GetUGCDetails(UGCHandle_t hContent, AppID_t *pnAppID, char **ppchName, int32_t *pnFileSizeInBytes, SteamID_t *pSteamIDOwner)
        {
            if (hContent == NULL) return false;
            static std::string Filename;
            char Buffer[FILENAME_MAX]{};
            Filename.clear();

            // OS specific hackery to get the filename from stdio.
            #if defined (_WIN32)
                const auto FD = _fileno((FILE *)hContent);
                const auto Handle = _get_osfhandle(FD);

                GetFileInformationByHandleEx((HANDLE)Handle, FileNameInfo, &Buffer, FILENAME_MAX);
                const auto Info = *(FILE_NAME_INFO *)Buffer;
                Filename = Encoding::toNarrow(std::wstring_view{ Info.FileName, Info.FileNameLength });
            #else
                readlink(va("/proc/self/fd/%ull", hContent).c_str(), Buffer, FILENAME_MAX);
                Filename = Buffer;
            #endif

            *pSteamIDOwner = Global.XUID;
            *ppchName = (char *)Filename.c_str();
            *pnAppID = Global.ApplicationID;

            std::fseek((FILE *)hContent, 0, SEEK_END);
            *pnFileSizeInBytes = std::ftell((FILE *)hContent);
            std::rewind((FILE *)hContent);

            return !Filename.empty();
        }
        bool GetUGCDownloadProgress(UGCHandle_t hContent, uint32_t *puDownloadedBytes, uint32_t *puTotalBytes)
        {
            *puDownloadedBytes = *puTotalBytes = 100;
            return true;
        }
        bool IsCloudEnabledForAccount()
        { return false; }
        bool IsCloudEnabledForApp()
        { return false; }
        bool ResetFileRequestState()
        { return true; }
        bool SetSyncPlatforms(const char *pchFile, ERemoteStoragePlatform eRemoteStoragePlatform)
        {
            return true;
        }
        bool SynchronizeToClient()
        { return true; }
        bool SynchronizeToServer()
        { return true; }
        bool UpdatePublishedFileDescription(PublishedFileUpdateHandle_t updateHandle, const char *pchDescription);
        bool UpdatePublishedFileFile(PublishedFileUpdateHandle_t updateHandle, const char *pchFile);
        bool UpdatePublishedFilePreviewFile(PublishedFileUpdateHandle_t updateHandle, const char *pchPreviewFile);
        bool UpdatePublishedFileSetChangeDescription(PublishedFileUpdateHandle_t updateHandle, const char *pchChangeDescription);
        bool UpdatePublishedFileTags(PublishedFileUpdateHandle_t updateHandle, SteamParamStringArray_t *pTags);
        bool UpdatePublishedFileTitle(PublishedFileUpdateHandle_t updateHandle, const char *pchTitle);
        bool UpdatePublishedFileVisibility(PublishedFileUpdateHandle_t updateHandle, ERemoteStoragePublishedFileVisibility eVisibility);

        const char *GetFileNameAndSize(int iFile, int32_t *pnFileSizeInBytes)
        {
            const auto Files = FS::Findfiles(Storagepath(""), "");
            if (iFile >= Files.size()) return "";

            *pnFileSizeInBytes = GetFileSize(Files[iFile].c_str());

            static std::string Cache;
            Cache = Files[iFile];
            return Cache.c_str();
        }

        int32_t FileRead(const char *pchFile, void *pvData, int32_t cubDataToRead)
        {
            const auto Filebuffer = FS::Readfile(Storagepath(pchFile));
            const auto Min = std::min(int32_t(Filebuffer.size()), cubDataToRead);

            std::memcpy(pvData, Filebuffer.data(), Min);
            return Min;
        }
        int32_t GetCachedUGCCount()
        {
            return 0;
        }
        int32_t GetFileCount()
        {
            const auto Files = FS::Findfiles(Storagepath(""), "");
            return Files.size();
        }
        int32_t GetFileSize(const char *pchFile)
        {
            return FS::Filesize(Storagepath(pchFile));
        }
        int32_t UGCRead0(UGCHandle_t hContent, void *pvData, int32_t cubDataToRead)
        {
            return UGCRead2(hContent, pvData, cubDataToRead, 0, k_EUGCRead_ContinueReadingUntilFinished);
        }
        int32_t UGCRead1(UGCHandle_t hContent, void *pvData, int32_t cubDataToRead, uint32_t cOffset)
        {
            return UGCRead2(hContent, pvData, cubDataToRead, cOffset, k_EUGCRead_ContinueReadingUntilFinished);
        }
        int32_t UGCRead2(UGCHandle_t hContent, void *pvData, int32_t cubDataToRead, uint32_t cOffset, EUGCReadAction eAction)
        {
            const auto Handle = (std::FILE *)hContent;
            std::fseek(Handle, cOffset, SEEK_SET);

            const auto Result = std::fread(pvData, 1, cubDataToRead, Handle);
            if (eAction == k_EUGCRead_Close || (eAction == k_EUGCRead_ContinueReadingUntilFinished && Result < cubDataToRead))
                std::fclose(Handle);
            return Result;
        }

        int64_t GetFileTimestamp(const char *pchFile)
        {
            return FS::Filestats(Storagepath(pchFile)).Modified;
        }

        void GetFileListFromServer()
        {}
        void SetCloudEnabledForApp(bool bEnabled)
        {}
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamRemotestorage001 : Interface_t
    {
        SteamRemotestorage001()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, GetFileSize);
            Createmethod(2, SteamRemotestorage, FileRead);
            Createmethod(3, SteamRemotestorage, FileExists);
            Createmethod(4, SteamRemotestorage, FileDelete);
            Createmethod(5, SteamRemotestorage, GetFileCount);
            Createmethod(6, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(7, SteamRemotestorage, GetQuota0);
        };
    };
    struct SteamRemotestorage002 : Interface_t
    {
        SteamRemotestorage002()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, GetFileSize);
            Createmethod(2, SteamRemotestorage, FileRead);
            Createmethod(3, SteamRemotestorage, FileExists);
            Createmethod(4, SteamRemotestorage, GetFileCount);
            Createmethod(5, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(6, SteamRemotestorage, GetQuota0);
        };
    };
    struct SteamRemotestorage003 : Interface_t
    {
        SteamRemotestorage003()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, FileExists);
            Createmethod(6, SteamRemotestorage, FilePersisted);
            Createmethod(7, SteamRemotestorage, GetFileSize);
            Createmethod(8, SteamRemotestorage, GetFileTimestamp);
            Createmethod(9, SteamRemotestorage, GetFileCount);
            Createmethod(10, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(11, SteamRemotestorage, GetQuota0);
            Createmethod(12, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(13, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(14, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(15, SteamRemotestorage, UGCDownload0);
            Createmethod(16, SteamRemotestorage, GetUGCDetails);
            Createmethod(17, SteamRemotestorage, UGCRead0);
            Createmethod(18, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(19, SteamRemotestorage, GetCachedUGCHandle);
        };
    };
    struct SteamRemotestorage004 : Interface_t
    {
        SteamRemotestorage004()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileExists);
            Createmethod(7, SteamRemotestorage, FilePersisted);
            Createmethod(8, SteamRemotestorage, GetFileSize);
            Createmethod(9, SteamRemotestorage, GetFileTimestamp);
            Createmethod(10, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(11, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(12, SteamRemotestorage, GetQuota0);
            Createmethod(13, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(14, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(15, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(16, SteamRemotestorage, UGCDownload0);
            Createmethod(17, SteamRemotestorage, GetUGCDetails);
            Createmethod(18, SteamRemotestorage, UGCRead0);
            Createmethod(19, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(20, SteamRemotestorage, GetCachedUGCHandle);
        };
    };
    struct SteamRemotestorage005 : Interface_t
    {
        SteamRemotestorage005()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileExists);
            Createmethod(7, SteamRemotestorage, FilePersisted);
            Createmethod(8, SteamRemotestorage, GetFileSize);
            Createmethod(9, SteamRemotestorage, GetFileTimestamp);
            Createmethod(10, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(11, SteamRemotestorage, GetFileCount);
            Createmethod(12, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(13, SteamRemotestorage, GetQuota0);
            Createmethod(14, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(15, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(16, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(17, SteamRemotestorage, UGCDownload0);
            Createmethod(18, SteamRemotestorage, GetUGCDetails);
            Createmethod(19, SteamRemotestorage, UGCRead0);
            Createmethod(20, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(21, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(22, SteamRemotestorage, GetFileListFromServer);
            Createmethod(23, SteamRemotestorage, FileFetch);
            Createmethod(24, SteamRemotestorage, FilePersist);
            Createmethod(25, SteamRemotestorage, SynchronizeToClient);
            Createmethod(26, SteamRemotestorage, SynchronizeToServer);
            Createmethod(27, SteamRemotestorage, ResetFileRequestState);
            Createmethod(28, SteamRemotestorage, PublishFile);
            Createmethod(29, SteamRemotestorage, PublishWorkshopFile0);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFile);
            Createmethod(31, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(32, SteamRemotestorage, DeletePublishedFile);
            Createmethod(33, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(34, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(35, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(36, SteamRemotestorage, UnsubscribePublishedFile);
        };
    };
    struct SteamRemotestorage006 : Interface_t
    {
        SteamRemotestorage006()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileExists);
            Createmethod(7, SteamRemotestorage, FilePersisted);
            Createmethod(8, SteamRemotestorage, GetFileSize);
            Createmethod(9, SteamRemotestorage, GetFileTimestamp);
            Createmethod(10, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(11, SteamRemotestorage, GetFileCount);
            Createmethod(12, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(13, SteamRemotestorage, GetQuota0);
            Createmethod(14, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(15, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(16, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(17, SteamRemotestorage, UGCDownload0);
            Createmethod(18, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(19, SteamRemotestorage, GetUGCDetails);
            Createmethod(20, SteamRemotestorage, UGCRead0);
            Createmethod(21, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(22, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(23, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(24, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(25, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(26, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(27, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(28, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(31, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(32, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(33, SteamRemotestorage, DeletePublishedFile);
            Createmethod(34, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(35, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(36, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(37, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(38, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(39, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(40, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(41, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(42, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(43, SteamRemotestorage, PublishVideo0);
            Createmethod(44, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(45, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(46, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
        };
    };
    struct SteamRemotestorage007 : Interface_t
    {
        SteamRemotestorage007()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileExists);
            Createmethod(7, SteamRemotestorage, FilePersisted);
            Createmethod(8, SteamRemotestorage, GetFileSize);
            Createmethod(9, SteamRemotestorage, GetFileTimestamp);
            Createmethod(10, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(11, SteamRemotestorage, GetFileCount);
            Createmethod(12, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(13, SteamRemotestorage, GetQuota0);
            Createmethod(14, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(15, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(16, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(17, SteamRemotestorage, UGCDownload0);
            Createmethod(18, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(19, SteamRemotestorage, GetUGCDetails);
            Createmethod(20, SteamRemotestorage, UGCRead0);
            Createmethod(21, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(22, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(23, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(24, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(25, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(26, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(27, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(28, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(31, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(32, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(33, SteamRemotestorage, DeletePublishedFile);
            Createmethod(34, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(35, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(36, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(37, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(38, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(39, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(40, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(41, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(42, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(43, SteamRemotestorage, PublishVideo1);
            Createmethod(44, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(45, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(46, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
        };
    };
    struct SteamRemotestorage008 : Interface_t
    {
        SteamRemotestorage008()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(7, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(8, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(9, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(10, SteamRemotestorage, FileExists);
            Createmethod(11, SteamRemotestorage, FilePersisted);
            Createmethod(12, SteamRemotestorage, GetFileSize);
            Createmethod(13, SteamRemotestorage, GetFileTimestamp);
            Createmethod(14, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(15, SteamRemotestorage, GetFileCount);
            Createmethod(16, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(17, SteamRemotestorage, GetQuota0);
            Createmethod(18, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(19, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(20, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(21, SteamRemotestorage, UGCDownload0);
            Createmethod(22, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(23, SteamRemotestorage, GetUGCDetails);
            Createmethod(24, SteamRemotestorage, UGCRead0);
            Createmethod(25, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(26, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(27, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(28, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(31, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(32, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(33, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(34, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(35, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(36, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(37, SteamRemotestorage, DeletePublishedFile);
            Createmethod(38, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(39, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(40, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(41, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(43, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(44, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(45, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(46, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(47, SteamRemotestorage, PublishVideo1);
            Createmethod(48, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(49, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(50, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
        };
    };
    struct SteamRemotestorage009 : Interface_t
    {
        SteamRemotestorage009()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(7, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(8, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(9, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(10, SteamRemotestorage, FileExists);
            Createmethod(11, SteamRemotestorage, FilePersisted);
            Createmethod(12, SteamRemotestorage, GetFileSize);
            Createmethod(13, SteamRemotestorage, GetFileTimestamp);
            Createmethod(14, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(15, SteamRemotestorage, GetFileCount);
            Createmethod(16, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(17, SteamRemotestorage, GetQuota0);
            Createmethod(18, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(19, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(20, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(21, SteamRemotestorage, UGCDownload0);
            Createmethod(22, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(23, SteamRemotestorage, GetUGCDetails);
            Createmethod(24, SteamRemotestorage, UGCRead1);
            Createmethod(25, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(26, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(27, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(28, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(31, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(32, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(33, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(34, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(35, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(36, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(37, SteamRemotestorage, DeletePublishedFile);
            Createmethod(38, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(39, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(40, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(41, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(43, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(44, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(45, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(46, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(47, SteamRemotestorage, PublishVideo1);
            Createmethod(48, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(49, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(50, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
        };
    };
    struct SteamRemotestorage010 : Interface_t
    {
        SteamRemotestorage010()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(7, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(8, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(9, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(10, SteamRemotestorage, FileExists);
            Createmethod(11, SteamRemotestorage, FilePersisted);
            Createmethod(12, SteamRemotestorage, GetFileSize);
            Createmethod(13, SteamRemotestorage, GetFileTimestamp);
            Createmethod(14, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(15, SteamRemotestorage, GetFileCount);
            Createmethod(16, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(17, SteamRemotestorage, GetQuota0);
            Createmethod(18, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(19, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(20, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(21, SteamRemotestorage, UGCDownload1);
            Createmethod(22, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(23, SteamRemotestorage, GetUGCDetails);
            Createmethod(24, SteamRemotestorage, UGCRead1);
            Createmethod(25, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(26, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(27, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(28, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(31, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(32, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(33, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(34, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(35, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(36, SteamRemotestorage, GetPublishedFileDetails0);
            Createmethod(37, SteamRemotestorage, DeletePublishedFile);
            Createmethod(38, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(39, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(40, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(41, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(43, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(44, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(45, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(46, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(47, SteamRemotestorage, PublishVideo1);
            Createmethod(48, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(49, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(50, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
            Createmethod(51, SteamRemotestorage, UGCDownloadToLocation);
        };
    };
    struct SteamRemotestorage011 : Interface_t
    {
        SteamRemotestorage011()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(7, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(8, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(9, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(10, SteamRemotestorage, FileExists);
            Createmethod(11, SteamRemotestorage, FilePersisted);
            Createmethod(12, SteamRemotestorage, GetFileSize);
            Createmethod(13, SteamRemotestorage, GetFileTimestamp);
            Createmethod(14, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(15, SteamRemotestorage, GetFileCount);
            Createmethod(16, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(17, SteamRemotestorage, GetQuota0);
            Createmethod(18, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(19, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(20, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(21, SteamRemotestorage, UGCDownload1);
            Createmethod(22, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(23, SteamRemotestorage, GetUGCDetails);
            Createmethod(24, SteamRemotestorage, UGCRead1);
            Createmethod(25, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(26, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(27, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(28, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(29, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(30, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(31, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(32, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(33, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(34, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(35, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(36, SteamRemotestorage, GetPublishedFileDetails1);
            Createmethod(37, SteamRemotestorage, DeletePublishedFile);
            Createmethod(38, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(39, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(40, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(41, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(43, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(44, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(45, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(46, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(47, SteamRemotestorage, PublishVideo1);
            Createmethod(48, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(49, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(50, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
            Createmethod(51, SteamRemotestorage, UGCDownloadToLocation);
        };
    };
    struct SteamRemotestorage012 : Interface_t
    {
        SteamRemotestorage012()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileForget);
            Createmethod(3, SteamRemotestorage, FileDelete);
            Createmethod(4, SteamRemotestorage, FileShare);
            Createmethod(5, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(6, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(7, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(8, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(9, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(10, SteamRemotestorage, FileExists);
            Createmethod(11, SteamRemotestorage, FilePersisted);
            Createmethod(12, SteamRemotestorage, GetFileSize);
            Createmethod(13, SteamRemotestorage, GetFileTimestamp);
            Createmethod(14, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(15, SteamRemotestorage, GetFileCount);
            Createmethod(16, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(17, SteamRemotestorage, GetQuota0);
            Createmethod(18, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(19, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(20, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(21, SteamRemotestorage, UGCDownload1);
            Createmethod(22, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(23, SteamRemotestorage, GetUGCDetails);
            Createmethod(24, SteamRemotestorage, UGCRead2);
            Createmethod(25, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(26, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(27, SteamRemotestorage, GetFileListFromServer);
            Createmethod(28, SteamRemotestorage, FileFetch);
            Createmethod(29, SteamRemotestorage, FilePersist);
            Createmethod(30, SteamRemotestorage, SynchronizeToClient);
            Createmethod(31, SteamRemotestorage, SynchronizeToServer);
            Createmethod(32, SteamRemotestorage, ResetFileRequestState);
            Createmethod(33, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(34, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(35, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(36, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(37, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(38, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(39, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(40, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(41, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(42, SteamRemotestorage, GetPublishedFileDetails1);
            Createmethod(43, SteamRemotestorage, DeletePublishedFile);
            Createmethod(44, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(45, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(46, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(47, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(48, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(49, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(50, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(51, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(52, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(53, SteamRemotestorage, PublishVideo1);
            Createmethod(54, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(55, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(56, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
            Createmethod(57, SteamRemotestorage, UGCDownloadToLocation);
        };
    };
    struct SteamRemotestorage013 : Interface_t
    {
        SteamRemotestorage013()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileWriteAsync);
            Createmethod(3, SteamRemotestorage, FileReadAsync);
            Createmethod(4, SteamRemotestorage, FileReadAsyncComplete);
            Createmethod(5, SteamRemotestorage, FileForget);
            Createmethod(6, SteamRemotestorage, FileDelete);
            Createmethod(7, SteamRemotestorage, FileShare);
            Createmethod(8, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(9, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(10, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(11, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(12, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(13, SteamRemotestorage, FileExists);
            Createmethod(14, SteamRemotestorage, FilePersisted);
            Createmethod(15, SteamRemotestorage, GetFileSize);
            Createmethod(16, SteamRemotestorage, GetFileTimestamp);
            Createmethod(17, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(18, SteamRemotestorage, GetFileCount);
            Createmethod(19, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(20, SteamRemotestorage, GetQuota0);
            Createmethod(21, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(22, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(23, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(24, SteamRemotestorage, UGCDownload1);
            Createmethod(25, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(26, SteamRemotestorage, GetUGCDetails);
            Createmethod(27, SteamRemotestorage, UGCRead2);
            Createmethod(28, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(29, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(30, SteamRemotestorage, GetFileListFromServer);
            Createmethod(31, SteamRemotestorage, FileFetch);
            Createmethod(32, SteamRemotestorage, FilePersist);
            Createmethod(33, SteamRemotestorage, SynchronizeToClient);
            Createmethod(34, SteamRemotestorage, SynchronizeToServer);
            Createmethod(35, SteamRemotestorage, ResetFileRequestState);
            Createmethod(36, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(37, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(38, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(39, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(40, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(41, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(43, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(44, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(45, SteamRemotestorage, GetPublishedFileDetails1);
            Createmethod(46, SteamRemotestorage, DeletePublishedFile);
            Createmethod(47, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(48, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(49, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(50, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(51, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(52, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(53, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(54, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(55, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(56, SteamRemotestorage, PublishVideo1);
            Createmethod(57, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(58, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(59, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
            Createmethod(60, SteamRemotestorage, UGCDownloadToLocation);
        };
    };
    struct SteamRemotestorage014 : Interface_t
    {
        SteamRemotestorage014()
        {
            Createmethod(0, SteamRemotestorage, FileWrite);
            Createmethod(1, SteamRemotestorage, FileRead);
            Createmethod(2, SteamRemotestorage, FileWriteAsync);
            Createmethod(3, SteamRemotestorage, FileReadAsync);
            Createmethod(4, SteamRemotestorage, FileReadAsyncComplete);
            Createmethod(5, SteamRemotestorage, FileForget);
            Createmethod(6, SteamRemotestorage, FileDelete);
            Createmethod(7, SteamRemotestorage, FileShare);
            Createmethod(8, SteamRemotestorage, SetSyncPlatforms);
            Createmethod(9, SteamRemotestorage, FileWriteStreamOpen);
            Createmethod(10, SteamRemotestorage, FileWriteStreamWriteChunk);
            Createmethod(11, SteamRemotestorage, FileWriteStreamClose);
            Createmethod(12, SteamRemotestorage, FileWriteStreamCancel);
            Createmethod(13, SteamRemotestorage, FileExists);
            Createmethod(14, SteamRemotestorage, FilePersisted);
            Createmethod(15, SteamRemotestorage, GetFileSize);
            Createmethod(16, SteamRemotestorage, GetFileTimestamp);
            Createmethod(17, SteamRemotestorage, GetSyncPlatforms);
            Createmethod(18, SteamRemotestorage, GetFileCount);
            Createmethod(19, SteamRemotestorage, GetFileNameAndSize);
            Createmethod(20, SteamRemotestorage, GetQuota1);
            Createmethod(21, SteamRemotestorage, IsCloudEnabledForAccount);
            Createmethod(22, SteamRemotestorage, IsCloudEnabledForApp);
            Createmethod(23, SteamRemotestorage, SetCloudEnabledForApp);
            Createmethod(24, SteamRemotestorage, UGCDownload1);
            Createmethod(25, SteamRemotestorage, GetUGCDownloadProgress);
            Createmethod(26, SteamRemotestorage, GetUGCDetails);
            Createmethod(27, SteamRemotestorage, UGCRead2);
            Createmethod(28, SteamRemotestorage, GetCachedUGCCount);
            Createmethod(29, SteamRemotestorage, GetCachedUGCHandle);
            Createmethod(30, SteamRemotestorage, GetFileListFromServer);
            Createmethod(31, SteamRemotestorage, FileFetch);
            Createmethod(32, SteamRemotestorage, FilePersist);
            Createmethod(33, SteamRemotestorage, SynchronizeToClient);
            Createmethod(34, SteamRemotestorage, SynchronizeToServer);
            Createmethod(35, SteamRemotestorage, ResetFileRequestState);
            Createmethod(36, SteamRemotestorage, PublishWorkshopFile1);
            Createmethod(37, SteamRemotestorage, CreatePublishedFileUpdateRequest);
            Createmethod(38, SteamRemotestorage, UpdatePublishedFileFile);
            Createmethod(39, SteamRemotestorage, UpdatePublishedFilePreviewFile);
            Createmethod(40, SteamRemotestorage, UpdatePublishedFileTitle);
            Createmethod(41, SteamRemotestorage, UpdatePublishedFileDescription);
            Createmethod(42, SteamRemotestorage, UpdatePublishedFileVisibility);
            Createmethod(43, SteamRemotestorage, UpdatePublishedFileTags);
            Createmethod(44, SteamRemotestorage, CommitPublishedFileUpdate);
            Createmethod(45, SteamRemotestorage, GetPublishedFileDetails1);
            Createmethod(46, SteamRemotestorage, DeletePublishedFile);
            Createmethod(47, SteamRemotestorage, EnumerateUserPublishedFiles);
            Createmethod(48, SteamRemotestorage, SubscribePublishedFile);
            Createmethod(49, SteamRemotestorage, EnumerateUserSubscribedFiles);
            Createmethod(50, SteamRemotestorage, UnsubscribePublishedFile);
            Createmethod(51, SteamRemotestorage, UpdatePublishedFileSetChangeDescription);
            Createmethod(52, SteamRemotestorage, GetPublishedItemVoteDetails);
            Createmethod(53, SteamRemotestorage, UpdateUserPublishedItemVote);
            Createmethod(54, SteamRemotestorage, GetUserPublishedItemVoteDetails);
            Createmethod(55, SteamRemotestorage, EnumerateUserSharedWorkshopFiles);
            Createmethod(56, SteamRemotestorage, PublishVideo1);
            Createmethod(57, SteamRemotestorage, SetUserPublishedFileAction);
            Createmethod(58, SteamRemotestorage, EnumeratePublishedFilesByUserAction);
            Createmethod(59, SteamRemotestorage, EnumeratePublishedWorkshopFiles);
            Createmethod(60, SteamRemotestorage, UGCDownloadToLocation);
        };
    };

    struct Steamremotestorageloader
    {
        Steamremotestorageloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage001", SteamRemotestorage001);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage002", SteamRemotestorage002);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage003", SteamRemotestorage003);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage004", SteamRemotestorage004);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage005", SteamRemotestorage005);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage006", SteamRemotestorage006);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage007", SteamRemotestorage007);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage008", SteamRemotestorage008);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage009", SteamRemotestorage009);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage010", SteamRemotestorage010);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage011", SteamRemotestorage011);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage012", SteamRemotestorage012);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage013", SteamRemotestorage013);
            Register(Interfacetype_t::REMOTESTORAGE, "SteamRemotestorage014", SteamRemotestorage014);
        }
    };
    static Steamremotestorageloader Interfaceloader{};
}
