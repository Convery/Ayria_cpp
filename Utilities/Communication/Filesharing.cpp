/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-07
    License: MIT
*/

#include "Filesharing.hpp"

namespace Filesharing
{
    int __cdecl Read(const char *Sharename, void *Buffer, int Size);
    int __cdecl Write(const char *Sharename, void *Buffer, int Size);
    struct Implementation_t
    {
        int(__cdecl *Read)(const char *Sharename, void *Buffer, int Size);
        int(__cdecl *Write)(const char *Sharename, void *Buffer, int Size);
    };
}

namespace Filesharing
{
    Implementation_t This{ Read, Write };

    bool Write(std::string_view Sharename, Blob &&Data)
    {
        const auto Interfacename = va("%u_Filesharing", GetCurrentProcessId());
        const auto Interface = (Implementation_t *)Singleinstance::Create(Interfacename, &This);
        return !!Interface->Write(Sharename.data(), Data.data(), Data.size());
    }
    Blob Read(std::string_view Sharename, uint32_t Maxsize)
    {
        const auto Interfacename = va("%u_Filesharing", GetCurrentProcessId());
        const auto Interface = (Implementation_t *)Singleinstance::Create(Interfacename, &This);

        const auto Buffer = std::make_unique<uint8_t[]>(Maxsize);
        const auto Count = Interface->Read(Sharename.data(), Buffer.get(), Maxsize);
        return Blob(Buffer.get(), Count);
    }
    bool Write(std::string_view Sharename, std::string &&Data)
    {
        const auto Interfacename = va("%u_Filesharing", GetCurrentProcessId());
        const auto Interface = (Implementation_t *)Singleinstance::Create(Interfacename, &This);
        return !!Interface->Write(Sharename.data(), Data.data(), Data.size());
    }
}

namespace Filesharing
{
    std::mutex Threadguard;
    std::unordered_map<std::string, Blob> Sharedfiles;

    int __cdecl Read(const char *Sharename, void *Buffer, int Size)
    {
        const auto Share = Sharedfiles.find(Sharename);
        if (Share == Sharedfiles.end()) return 0;

        const auto Clamped = std::clamp(Size, 0, (int)Share->second.size());
        std::memcpy(Buffer, Share->second.data(), Clamped);
        return Clamped;
    }
    int __cdecl Write(const char *Sharename, void *Buffer, int Size)
    {
        Sharedfiles[Sharename] = Blob((uint8_t *)Buffer, Size);
        return Size;
    }
}
