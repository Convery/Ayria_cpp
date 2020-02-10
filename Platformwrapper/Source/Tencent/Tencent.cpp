/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-13
    License: MIT
*/

#include <winternl.h>
#include <Stdinclude.hpp>
#include "../Common/Common.hpp"
#pragma warning(disable : 4100)

namespace Tencent
{
    // These should probably be fetched in the background.
    std::unordered_map<uint32_t, std::wstring> Widestrings
    {
        { 14, L"Appname" },     // Probably the main process.
        { 15, L"Windowname" },  // Used for Sendmessage as well.
        { 16, L"PASS:-q" },     // Arguments passed to command-line, "-q %u -k %s -s %s -j %s -t %d"
    };
    std::unordered_map<uint32_t, std::string> Bytestrings
    {
        { 3, "7" },                 // Game-signature length.
        { 4, "Gamesig" },           // Also used for QQ stuff.
        { 5, "Loginkey" },          // Seems to be more of a SessionID.
        { 7, "Appticket" },         // For authentication.
        { 8, "auth3.qq.com:3074;"   // Authentication address.
             "lsg.qq.com:3074;"     // Service address.
             "1004" },              // App-ID for auth.
        { 22, "1100001DEADC0DE" },  // PlatformID of sorts.
        { 24, "Tencentticket" },    //
        { 25, "OpenIDKey" },        // OpenID's standardized implementation.
        { 26, "7" },                // Jump-signature length.
        { 27, "Jumpsig" },          // Seems to be anti-cheat related.
    };
    std::unordered_map<uint32_t, int32_t> Integers{};
    std::unordered_map<uint32_t, uint32_t> DWORDs
    {
        { 1, 42 },          // Game-version.
        { 5, 83886082 },    // Zone-ID.
        { 7, 38 },          // Authentication-info size.
        { 18, 1 },          // Server-ID.
    };

    void Sendmessage(WPARAM wParam, LPARAM lParam)
    {
        auto Result = FindWindowW(L"static", NULL);
        while(Result)
        {
            PostMessageW(Result, 0xBD2u, wParam, lParam);
            Result = FindWindowExW(NULL, Result, L"static", NULL);
        }
    }
    struct Interface
    {
        // TODO(tcn): Implement these when needed.
        virtual BOOL getLogininfo(void *Loginstruct)
        {
            Traceprint();

            // Notify listeners.
            Sendmessage(1 | 0x650000, 0);

            return true;
        }
        virtual BOOL Initialize()
        {
            // Other components may expect us to have a mapping with at least 64 'entries'. See the unordered_maps above.
            auto Handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, va("TCLS_SHAREDMEMEMORY%u", GetCurrentProcessId()).c_str());
            auto Pointer = MapViewOfFile(Handle, 0xF001F, 0, 0, 0);
            *(DWORD *)Pointer = 64;
            Traceprint();

            // TODO(tcn): Investigate what else needs to be initialized.

            // Notify listeners.
            Sendmessage(1 | 0x640000, 0);

            return true;
        }
        virtual BOOL Restart() { Traceprint(); return true; }
        virtual BOOL Uninitialize() { Traceprint(); return true; }
        virtual BOOL setEventhandler(void *Eventhandler) { Traceprint(); return true; }
        virtual void ReleaseTENIO() { Traceprint(); }
        virtual BOOL Notifyshutdown() { Traceprint(); return true; }
        virtual void *Closehandle() { Traceprint(); return this; }

        // Global info.
        virtual BOOL getInteger(uint32_t Tag, int32_t *Buffer)
        {
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));

            auto Result = Integers.find(Tag);
            if(Buffer && Result != Integers.end())
                *Buffer = Result->second;

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), 0);

            return Result != Integers.end();
        }
        virtual BOOL getDWORD(uint32_t Tag, uint32_t *Buffer)
        {
            return getDWORD_(Tag, Buffer, 0);
        }
        virtual BOOL getBytestring(uint32_t Tag, uint8_t *Buffer, uint32_t *Size)
        {
            return getBytestring_(Tag, Buffer, Size, 0);
        }
        virtual BOOL getWidestring(uint32_t Tag, wchar_t *Buffer, uint32_t *Size)
        {
            return getWidestring_(Tag, Buffer, Size, 0);
        }

        // User info. NOTE(tcn): '_' because it needs to have a different name or CL changes the layout.
        virtual BOOL getBytestring_(uint32_t Tag, uint8_t *Buffer, uint32_t *Size, uint32_t ServerID)
        {
            auto Result = Bytestrings.find(Tag);
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));
            if(Result == Bytestrings.end()) return FALSE;

            // If a buffer is provided, fill it.
            if(Buffer) std::memcpy(Buffer, Result->second.c_str(), std::min(size_t(*Size), Result->second.size()));

            // Always set the total size.
            *Size = Result->second.size();

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), ServerID != 0);

            return TRUE;
        }
        virtual BOOL getWidestring_(uint32_t Tag, wchar_t *Buffer, uint32_t *Size, uint32_t ServerID)
        {
            auto Result = Widestrings.find(Tag);
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));
            if(Result == Widestrings.end()) return FALSE;

            // If a buffer is provided, fill it.
            if(Buffer) std::wmemcpy(Buffer, Result->second.c_str(), std::min(size_t(*Size >> 1), Result->second.size()));

            // Always set the total size.
            *Size = Result->second.size() * sizeof(wchar_t);

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), ServerID != 0);

            return TRUE;
        }
        virtual BOOL getDWORD_(uint32_t Tag, uint32_t *Buffer, uint32_t ServerID)
        {
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));

            auto Result = DWORDs.find(Tag);
            if(Buffer && Result != DWORDs.end())
                *Buffer = Result->second;

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), ServerID != 0);

            return Result != DWORDs.end();
        }
    };
}


// Create the interface with the required version.
extern "C" EXPORT_ATTR void Invoke(uint32_t GameID, void **Interface)
{
    Debugprint(va("Initializing Tencent for game %u", GameID));
    *Interface = new Tencent::Interface();

    // Notify the plugins that we are initialized.
    #if defined(_WIN32)
    auto Bootstrapper = GetModuleHandleA("Localbootstrap.dll");
    if (!Bootstrapper) Bootstrapper = GetModuleHandleA(va("Bootstrapper%d.dll", Build::is64bit ? 64 : 32).c_str());
    if (!Bootstrapper) Bootstrapper = GetModuleHandleA(va("Bootstrapper%dd.dll", Build::is64bit ? 64 : 32).c_str());
    if (Bootstrapper)
    {
        auto Callback = GetProcAddress(Bootstrapper, "onInitializationdone");
        if (Callback) (reinterpret_cast<void (*)()>(Callback))();
    }
    #endif
}

// Tencent anti-cheat initialization override.
extern "C" EXPORT_ATTR void* CreateObj(int Type);

// Tencent OpenID resolving override.
extern "C" EXPORT_ATTR int InitCrossContextByOpenID(void *)
{
    Traceprint();
    return 0;
}

// Initialize Tencent as a plugin.
void Tencent_init()
{
    #define Hook(x, y, z) { void *Address = GetProcAddress(LoadLibraryA(x), y); \
    if(Address && Address != z && !Mhook_SetHook((void **)&Address, z)) assert(false);  }

    // Override the original interface generation.
    Hook("TenProxy.dll", "Invoke", Invoke);

    // Override the anti-cheat initialization.
    Hook("TerSafe.dll", "CreateObj", CreateObj);

    // Override the cross-platform system.
    Hook("./Cross/CrossShell.dll", "InitCrossContextByOpenID", InitCrossContextByOpenID);
}
