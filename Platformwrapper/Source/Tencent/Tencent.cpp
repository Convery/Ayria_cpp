/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-13
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common/Common.hpp"

// Tencent variables, should be fetched in the background.
auto Integerstore = []() { std::array<uint32_t, 64> ${}; $[1] = 42; $[18] = 1004; return $; }();
auto Stringstore = []()
{
    std::array<std::string, 64> ${};
    $[7] = "Appticket";
    $[22] = "PlatformID";
    $[24] = "Tencentticket";
    $[25] = "OpenIDLoginKey";
    $[27] = "Unknown";
    return $;
}();
auto Arraystore = []()
{
    std::array<std::vector<std::string>, 64> ${};
    $[8] = { "auth3.qq.com:3074", "lsg.qq.com:3074", "1004" };
    return $;
}();

// Helper for fake classes.
static std::any Hackery;
#define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

struct Tencentcore
{
    void Ctor() { Traceprint(); }
    void Initialize() { Traceprint(); }
    void Unknown0() { Traceprint(); }
    void Unknown1() { Traceprint(); }
    void Unknown2() { Traceprint(); }
    int Shutdown_notify0() { Traceprint(); return 1; }
    int Shutdown_notify1() { Traceprint(); return 1; }
    void Shutdown_notify2() { Traceprint(); }
    void Unknown3() { Traceprint(); }
    bool getInteger(uint32_t Index, uint32_t *Buffer)
    {
        *Buffer = Integerstore[Index];

        Debugprint(va("Tencent_INT[%u] => %u", Index, *Buffer));
        return true;
    }
    bool getString(uint32_t Index, char *Buffer, uint32_t *Size)
    {
        if(Buffer) // If a buffer is provided, fill it.
        {
            std::memcpy(Buffer, Stringstore[Index].c_str(), std::min(*Size, (uint32_t)Stringstore[Index].size()));
            Debugprint(va("Tencent_STRING[%u] => %s", Index, Buffer));
        }

        // Always set the total size.
        *Size = Stringstore[Index].size();

        return true;
    }
    void Unknown4() { Traceprint(); }
    bool getArray(uint32_t Index, char *Buffer, uint32_t *Size, uint32_t UserID)
    {
        std::string Readable{};
        for(const auto &Item : Arraystore[Index])
        {
            Readable += Item;
            Readable += ";";
        }
        // Remove the last ';'
        if(!Readable.empty()) Readable.pop_back();

        if(Buffer) // If a buffer is provided, fill it.
        {
            std::memcpy(Buffer, Readable.c_str(), std::min(*Size, (uint32_t)Readable.size()));
            Debugprint(va("Tencent_ARRAY[%u] => %s", Index, Buffer));
        }

        // Always set the total size.
        *Size = Readable.size();

        return true;
    }
    void Unknown5() { Traceprint(); }
    bool getArraysize(uint32_t Index, uint32_t *Buffer, uint32_t UserID)
    {
        // Off by one in the SDK?
        return getArray(Index + 1, nullptr, Buffer, UserID);
    }
};
struct Tencent : Ayria::Fakeclass_t
{
    Tencent()
    {
        Createmethod(0, Tencentcore, Ctor);
        Createmethod(1, Tencentcore, Initialize);
        Createmethod(2, Tencentcore, Unknown0);
        Createmethod(3, Tencentcore, Unknown1);
        Createmethod(4, Tencentcore, Unknown2);
        Createmethod(5, Tencentcore, Shutdown_notify0);
        Createmethod(6, Tencentcore, Shutdown_notify1);
        Createmethod(7, Tencentcore, Shutdown_notify2);
        Createmethod(8, Tencentcore, Unknown3);
        Createmethod(9, Tencentcore, getInteger);
        Createmethod(10, Tencentcore, getString);
        Createmethod(11, Tencentcore, Unknown4);
        Createmethod(12, Tencentcore, getArray);
        Createmethod(13, Tencentcore, Unknown5);
        Createmethod(14, Tencentcore, getArraysize);
    }
};

// Temporary implementation of the Tensafe interface.
int Verysafe0(int Index) { Debugprint(va("%s: 0x%X", __func__, Index)); return 1; }
int Verysafe4(int Index) { Debugprint(va("%s: 0x%X", __func__, Index)); return 1; }
int Verysafe8(int Index) { Debugprint(va("%s: 0x%X", __func__, Index)); return 1; }
int Verysafe12(int Index) { Debugprint(va("%s: 0x%X", __func__, Index)); return 1; }
int Verysafe16(int Index) { Debugprint(va("%s: 0x%X", __func__, Index)); return 1; }
struct Tensafe : Ayria::Fakeclass_t
{
    Tensafe()
    {
        Hackery = &Verysafe0; VTABLE[0] = *(void **)&Hackery;
        Hackery = &Verysafe4; VTABLE[1] = *(void **)&Hackery;
        Hackery = &Verysafe8; VTABLE[2] = *(void **)&Hackery;
        Hackery = &Verysafe12; VTABLE[3] = *(void **)&Hackery;
        Hackery = &Verysafe16; VTABLE[4] = *(void **)&Hackery;
    }
};

extern "C"
{
    // Tencent anti-cheat initialization override.
    EXPORT_ATTR void *CreateObj(int Type)
    {
        static auto Debug = new Tensafe();
        return &Debug;
    }

    // Tencent OpenID resolving override.
    EXPORT_ATTR int InitCrossContextByOpenID(void *)
    {
        return 0;
    }

    // Create the interface with the required version.
    EXPORT_ATTR void Invoke(uint32_t GameID, void **Interface)
    {
        Integerstore[18] = GameID;
        Debugprint(va("Initializing Tencent for game %u", GameID));
        static auto Localinterface = new Tencent();
        *Interface = &Localinterface;
    }
}

// Initialize Tencent as a plugin.
void Tencent_init()
{
    void *Address{};

    // Override the original interface generation.
    Address = GetProcAddress(LoadLibraryA("TenProxy.dll"), "Invoke");
    if(Address) if(!Mhook_SetHook((void **)&Address, Invoke)) Simplehook::Stomphook().Installhook(Address, Invoke);

    // Override the anti-cheat initialization.
    Address = GetProcAddress(LoadLibraryA("TerSafe.dll"), "CreateObj");
    if(Address) if(!Mhook_SetHook((void **)&Address, CreateObj)) Simplehook::Stomphook().Installhook(Address, CreateObj);

    // Override the OpenID resolving.
    Address = GetProcAddress(LoadLibraryA("./Cross/CrossShell.dll"), "InitCrossContextByOpenID");
    if(Address) if(!Mhook_SetHook((void **)&Address, InitCrossContextByOpenID)) Simplehook::Stomphook().Installhook(Address, InitCrossContextByOpenID);
}
