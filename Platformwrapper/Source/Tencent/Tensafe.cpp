/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-10
    License: MIT

    Tensafe / Tersafe is a dll/driver pair for anti-cheating.
    Work in progress.
*/

#include <winternl.h>
#include <Stdinclude.hpp>
#include "../Common.hpp"
#pragma warning(disable : 4100)

#pragma region Datatypes
enum class Packettypes_t : uint8_t
{
    onLogin = 1,
    onSYNReq = 2,
    onSYNAck = 3,
    onConfigupdate = 4,
    encrypteddata = 5,
};

#pragma pack(push, 1)
struct Packet_t
{
    uint32_t Totalsize;
    uint32_t SequenceID;
    uint32_t CRC32Checksum;
    uint32_t Payloadlength;
    uint8_t Packettype;

    // Data-marker.
    uint8_t Payload;
};
struct IPCPacket_t
{
    uint8_t *Dataptr;
    uint32_t Dataused;
    uint32_t Maxsize;
};
struct Encryptedpackage_t
{
    uint32_t Command;
    uint8_t *Gamepackage;
    uint32_t Gamelength;
    uint8_t *Encryptedpackage;
    uint32_t Encryptedlength;
};
#pragma pack(pop)
#pragma endregion

// Simple bit-rotate.
template<typename T> T ROL(T Value, int32_t Count)
{
    const auto Bits = sizeof(T) * 8;
    const auto isSigned = T(-1) < 0;

    if (Count)
    {
        Count %= Bits;

        T High = Value >> (Bits - Count);
        if(isSigned) High &= ~((T(-1) << Count));

        Value <<= Count;
        Value |= High;
    }
    else
    {
        Count = -Count % Bits;
        T Low = Value << (Bits - Count);

        Value >>= Count;
        Value |= Low;
    }

    return Value;
}
template<typename T> T ROR(T Value, int32_t Count)
{
    return ROL(Value, -Count);
}

struct Callbackinterface000
{
    virtual int onPacket(IPCPacket_t *Packet);
};
void Sendtoserver(Packettypes_t Packettype, Blob &&Data, struct Tensafe *Interface);
template <typename T> bool PackIPC(IPCPacket_t *Packet, T Value)
{
    if (Packet->Maxsize - Packet->Dataused < sizeof(T))
        return false;

    if constexpr (sizeof(T) == 1)
    {
        *(uint8_t *)&Packet->Dataptr[Packet->Dataused] = Value;
        Packet->Dataused++;
    }
    if constexpr (sizeof(T) == 2)
    {
        *(uint16_t *)&Packet->Dataptr[Packet->Dataused] = ROL(Value, 8);
        Packet->Dataused += 2;
    }
    if constexpr (sizeof(T) == 4)
    {
        *(uint32_t *)&Packet->Dataptr[Packet->Dataused] = (((Value << 16) | Value & 0xFF00) << 8) | (((Value >> 16) | Value & 0xFF0000) >> 8);
        Packet->Dataused += 4;
    }

    return true;
}

// NOTE(tcn): Very WIP.
struct Tensafe
{
    uint32_t UserID{};
    uint32_t Interfaceversion{};
    uint32_t Gameversion{};
    void *Callback{};
    time_t Startuptime{};
    uint32_t Configneedsreload{};
    char Outgoingpacket[512]{};
    char Incomingpacket[512]{};
    uint32_t Unknown{};
    uint32_t SequenceID{};
    uint32_t Packetsrecieved{};
    uint32_t Unknown2{};
    uint32_t Encryptedpacketsrecieved{};
    uint32_t Unknown3{};
    uint32_t Clientsent{};
    uint32_t Clientrecvd{};
    char Config[1030]{};

    using Initinfo = struct { uint32_t Interfaceversion, Gameversion; void *Callback; };
    virtual int32_t Initialize(Initinfo *Info)
    {
        this->Callback = Info->Callback;
        this->Gameversion = Info->Gameversion;
        this->Interfaceversion = Info->Interfaceversion;
        Debugprint(va("%s: In: %u Gameversion: %u, Callback: %p", __FUNCTION__, Interfaceversion, Gameversion, Callback));

        // C-style bool.
        return 1;
    }
    virtual int32_t onLogin()
    {
        Bytebuffer Writer{};
        time(&Startuptime);
        srand(Startuptime & 0xFFFFFFFF);

        // NOTE(tcn): No real ID for western clients.
        UserID = rand();

        // Ask the client nicely to encrypt and forward the info to the server.
        Writer.Write(Interfaceversion, false);
        Writer.Write(UserID, false);
        Writer.Write(Gameversion, false);
        Writer.Write(uint32_t(0), false);
        Sendtoserver(Packettypes_t::onLogin, Writer.asBlob(), this);

        return 1;
    }
    virtual size_t Encrypt(Encryptedpackage_t *Packet)
    {
        Traceprint();
        if (!Packet) { Debugprint("Encrypt with invalid Packet-pointer"); return 2; }
        auto k = Memprotect::Makewriteable(Packet, sizeof(*Packet));

        std::string Buffer = "Testing";
        Debugprint(Buffer);

        Buffer = va("Encrypt Command %u, Enclen %u, Gamelen %u\n", Packet->Command, Packet->Encryptedlength, Packet->Gamelength);
        Debugprint(Buffer);

        const auto Ptr = Packet->Gamepackage;
        for (size_t i = 0; i < Packet->Gamelength; ++i)
        {
            if (i % 16 == 0) Buffer += "\n\t\t";
            Buffer += va("%02x ", Ptr[i]);
        }

        Debugprint(Buffer);

        // Return that we don't need to encrypt.
        return 1;
    }
    virtual size_t isCheatpacket(uint32_t Command)
    {
        Debugprint(va("Packet ID: %u", Command));

        /*
            NOTE(tcn): Commands seem to be:

            1 - Startup
            2 - Server_ack
            3 - Client_ack
            4 - Server statistics / heartbeat
            5 - Cheat packet.
        */

        return 0;
    }
    virtual size_t Decrypt(Encryptedpackage_t *Packet)
    {
        Traceprint();
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

// Create and pack a request.
void Sendtoserver(const Packettypes_t Packettype, Blob &&Data, struct Tensafe *Interface)
{
    constexpr size_t Headersize = 17;
    assert(Interface != nullptr);
    Traceprint();

    // Network format of the packet.
    const auto Packetbuffer = std::make_unique<uint8_t[]>(Headersize + Data.size());
    auto Packet = (Packet_t *)Packetbuffer.get();
    Packet->SequenceID = uint32_t(++Interface->SequenceID);
    Packet->Totalsize = uint32_t(Headersize + Data.size());
    Packet->Payloadlength = uint32_t(Data.size());
    Packet->Packettype = (uint8_t)Packettype;
    Packet->CRC32Checksum = 0;

    // Inline packing wrapper thingy because Tencent.
    IPCPacket_t IPC{ &Packet->Payload, 0, Packet->Payloadlength };
    PackIPC(&IPC, Packet->Totalsize);
    PackIPC(&IPC, Packet->SequenceID);
    PackIPC(&IPC, Packet->CRC32Checksum);
    PackIPC(&IPC, Packet->Payloadlength);
    PackIPC(&IPC, Packet->Packettype);

    // Versioning is hard.
    if (Interface->Callback && Interface->Interfaceversion == 0)
        ((Callbackinterface000 *)Interface->Callback)->onPacket(&IPC);
}

namespace Driverpart
{
    /*
        NOTE(tcn):
        IOCTRL codes when interacting with the TenSafe driver:

        0x22C400 - Format as struct?
        0x22C404 - Apply?
        0x22E4A0 - 4 bytes in
        0x22E484
        0x22E488
        0x22E4A4
        0x22E544
        0x22E490 - Returns essential data
        0x22E4AC
        0x22E4B4
        0x22E4A8 - Version info
    */
    void *OriginalIO;
    BOOL __stdcall IOCTRL(HANDLE hDevice, DWORD dwIoControlCode, uint8_t *lpInBuffer, DWORD nInBufferSize, uint8_t *lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
    {
        Debugprint(va("IOCTRL: 0x%X from %p", dwIoControlCode, _ReturnAddress()));
        return ((decltype(DeviceIoControl) *)OriginalIO)(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

        #if 0
        auto XORFunc1 = [](uint32_t a1, bool Encode, uint8_t *Input, uint32_t Length)
        {
            for (int i = 0; i < Length; ++i)
            {
                if (Encode) Input[i] ^= i;
                Input[i] = ((i & 7) + a1) ^ Input[i];
                if (!Encode) Input[i] ^= i;
            }
        };

        switch (dwIoControlCode)
        {
        case 0x22E484:
        {
            // Decode input data.
            XORFunc1(NULL, false, lpInBuffer, nInBufferSize - 8);

            // Nothing returned.
            return 1;
        }
        case 0x22E488:
        {
            // Decode input data.
            XORFunc1(NULL, false, lpInBuffer, nInBufferSize - 8);

            // Nothing returned.
            return 1;
        }
        case 0x22E5DC:
        {
            // Decode input data.
            XORFunc1(NULL, false, lpInBuffer, nInBufferSize - 8);

            // Nothing returned.
            return 1;
        }
        case 0x22E4A8:
        {
            std::memset(lpOutBuffer, 0, 16);
            *(DWORD *)lpOutBuffer = 1377;
            *lpBytesReturned = 16;

            // Obfuscate.
            for (int i = 0; i < 8; ++i)
            {
                lpOutBuffer[i] ^= i ^ lpOutBuffer[(i & 7) + 8];
            }
            return 1;
        }
        case 0x22E490:
        {
            std::memset(lpOutBuffer, 0, 16);
            *(DWORD *)lpOutBuffer = 1377;

            XORFunc1(0, 1, lpOutBuffer, 4);
            return 1;
        }



        default:
        {
            return ((decltype(DeviceIoControl) *)OriginalIO)(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
        }
        }

        #endif
    }

    //OriginalIO = GetProcAddress(LoadLibraryA("kernel32.dll"), "DeviceIoControl");
    //Mhook_SetHook(&OriginalIO, MyDeviceIoControl);
}

extern "C" EXPORT_ATTR void* CreateObj(int Type)
{
    Debugprint(va("Create anti-cheat version %u", Type));

    if (Type == 0) return new Tensafe();

    return nullptr;
}
