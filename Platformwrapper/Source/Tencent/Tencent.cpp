/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-13
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common/Common.hpp"

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
        { 8, "auth3.qq.com:3074;lsg.qq.com:3074;1004" },   // Authentication-info, last ID being appID.
        { 22, "1100001DEADC0DE" },  // PlatformID of sorts.
        { 24, "Tencentticket" },    //
        { 25, "OpenIDKey" },        // OpenID's standardized implementation.
        { 26, "7" },                // Jump-signature length.
        { 27, "Jumpsig" },          // Seems to be anti-cheat related.
    };
    std::unordered_map<uint32_t, int32_t> Integers{};
    std::unordered_map<uint32_t, uint32_t> DWORDs
    {
        { 1, 42 },          // Unknown, seems unused.
        { 5, 0x7F000001 },  // IP-address.
        { 7, 38 },          // Authentication-info size.
        { 18, 1337 },       // UserID?
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
            Traceprint();

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
        virtual BOOL getBytestring_(uint32_t Tag, uint8_t *Buffer, uint32_t *Size, uint32_t UserID)
        {
            auto Result = Bytestrings.find(Tag);
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));
            if(Result == Bytestrings.end()) return FALSE;

            // If a buffer is provided, fill it.
            if(Buffer) std::memcpy(Buffer, Result->second.c_str(), std::min(size_t(*Size), Result->second.size()));

            // Always set the total size.
            *Size = Result->second.size();

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), UserID != 0);

            return TRUE;
        }
        virtual BOOL getWidestring_(uint32_t Tag, wchar_t *Buffer, uint32_t *Size, uint32_t UserID)
        {
            auto Result = Widestrings.find(Tag);
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));
            if(Result == Widestrings.end()) return FALSE;

            // If a buffer is provided, fill it.
            if(Buffer) std::wmemcpy(Buffer, Result->second.c_str(), std::min(size_t(*Size >> 1), Result->second.size()));

            // Always set the total size.
            *Size = Result->second.size() * sizeof(wchar_t);

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), UserID != 0);

            return TRUE;
        }
        virtual BOOL getDWORD_(uint32_t Tag, uint32_t *Buffer, uint32_t UserID)
        {
            Debugprint(va("%s(%u)", __FUNCTION__, Tag));

            auto Result = DWORDs.find(Tag);
            if(Buffer && Result != DWORDs.end())
                *Buffer = Result->second;

            // Notify listeners.
            Sendmessage(1 | (Tag << 0x10), UserID != 0);

            return Result != DWORDs.end();
        }
    };
}

// Temporary implementation of the Tensafe interface, TODO(tcn): Give it; it's own module?
namespace Tensafe
{
    struct Cryptoblock
    {
        uint32_t Command;
        uint8_t *Inputbuffer;
        uint32_t Inputlength;
        uint8_t *Outputbuffer;
        uint32_t Outputlength;
    };
    struct Packetheader
    {
        uint32_t Unknown;
        uint32_t SequenceID;
        uint32_t CRC32Hash;
        uint32_t Payloadlength;
        uint32_t Packettype;
    };
    struct Interface
    {
        uint32_t UserID;
        uint32_t Uin;
        uint32_t Gameversion;
        uint64_t pSink;
        time_t Startuptime;
        uint32_t Configneedsreload;
        char Outgoingpacket[512];
        char Incomingpacket[512];
        uint32_t Packetssent;
        uint32_t Unk5;
        uint32_t Unk6;
        uint32_t Unk7;
        uint32_t SequenceID;
        uint32_t Unk8;
        char Config[1030];

        virtual size_t setInit(void *Info)
        {
            Traceprint();
            Uin = ((uint32_t *)Info)[0];
            Gameversion = ((uint32_t *)Info)[1];
            pSink = *(size_t *)Info + sizeof(uint64_t);

            return 1;
        }
        virtual size_t onLogin()
        {
            Traceprint();
            time(&Startuptime);
            srand(Startuptime & 0xFFFFFFFF);
            UserID = rand();

            // TODO: Send to pSink.

            return 1;
        }
        virtual size_t Encrypt(Cryptoblock *Block)
        {
            Traceprint();

            /*
                Too tired to implement.
            */

            return 1;
        }
        virtual size_t isCheatpacket(uint32_t Command)
        {
            Traceprint();

            /*
                Incomingpacket =
                {
                    uint16_t Header;
                    uint16_t Commandcount;
                    uint32_t Commands[];
                    ...
                };

                for c in Commands => return c == Command;
            */

            return 1;
        }
        virtual size_t Decrypt(Cryptoblock *Block)
        {
            Traceprint();

            /*
                Too tired to implement.
            */

            return 1;
        }
        virtual size_t Release()
        {
            Traceprint();

            /*
                Call Dtor internally.
            */

            return 1;
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
extern "C" EXPORT_ATTR void *CreateObj(int Type)
{
    Traceprint();
    return new Tensafe::Interface();
}

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
