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
struct Tencentcore
{
    virtual void Ctor() { Traceprint(); }
    virtual void Initialize() { Traceprint(); }
    virtual void Unknown0() { Traceprint(); }
    virtual void Unknown1() { Traceprint(); }
    virtual void Unknown2() { Traceprint(); }
    virtual int Shutdown_notify0() { Traceprint(); return 1; }
    virtual int Shutdown_notify1() { Traceprint(); return 1; }
    virtual void Shutdown_notify2() { Traceprint(); }
    virtual void Unknown3() { Traceprint(); }
    virtual bool getInteger(uint32_t Index, uint32_t *Buffer)
    {
        *Buffer = Integerstore[Index];

        Debugprint(va("Tencent_INT[%u] => %u", Index, *Buffer));
        return true;
    }
    virtual bool getString(uint32_t Index, char *Buffer, uint32_t *Size)
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
    virtual void Unknown4() { Traceprint(); }
    virtual bool getArray(uint32_t Index, char *Buffer, uint32_t *Size, uint32_t UserID)
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
    virtual void Unknown5() { Traceprint(); }
    virtual bool getArraysize(uint32_t Index, uint32_t *Buffer, uint32_t UserID)
    {
        // Off by one in the SDK?
        return getArray(Index + 1, nullptr, Buffer, UserID);
    }
};

// Temporary implementation of the Tensafe interface, TODO(tcn): Give it; it's own module?
struct TencentCryptoblock
{
    uint32_t Command;
    uint8_t *Inputbuffer;
    uint32_t Inputlength;
    uint8_t *Outputbuffer;
    uint32_t Outputlength;
};
struct TencentPacketheader
{
    uint32_t Unknown;
    uint32_t SequenceID;
    uint32_t CRC32Hash;
    uint32_t Payloadlength;
    uint32_t Packettype;
};
struct TencentAnticheat
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
        time(&Startuptime);
        srand(Startuptime & 0xFFFFFFFF);
        UserID = rand();

        // TODO: Send to server.

        return 1;
    }
    virtual size_t Encrypt(TencentCryptoblock *Block)
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
    virtual size_t Decrypt(TencentCryptoblock *Block)
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

extern "C"
{
    // Tencent anti-cheat initialization override.
    EXPORT_ATTR void *CreateObj(int Type)
    {
        return new TencentAnticheat();
    }

    // Tencent OpenID resolving override.
    EXPORT_ATTR int InitCrossContextByOpenID(void *)
    {
        Traceprint();
        return 0;
    }

    // Create the interface with the required version.
    EXPORT_ATTR void Invoke(uint32_t GameID, void **Interface)
    {
        Debugprint(va("Initializing Tencent for game %u", GameID));
        *Interface = new Tencentcore();

        // TODO(tcn): Name the indexes.
        Arraystore[8][2] = va("%u", GameID);
        Integerstore[18] = GameID;
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
