/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Clientinfo
{
    #pragma pack(push, 1)
    struct Networkclient
    {
        uint32_t ClientID;
        uint32_t IPAddress;
        std::wstring Username;
    };
    struct Networkpool
    {
        uint16_t Groupport;
        uint16_t Maxmembers;
        uint32_t Groupaddress;
        std::vector<Networkclient> Members;
    };
    struct Matchmakingsession : Networkpool
    {
        union
        {
            uint8_t Flags;
            struct
            {
                uint8_t
                    isHost : 1,
                    isPublic : 1,
                    isServer : 1,
                    isRunning : 1,
                    Compressedinfo : 1,
                    RESERVED1 : 1,
                    RESERVED2 : 1,
                    RESERVED3 : 1;
            };
        };
        uint32_t Platform;
        std::wstring Servername;
        std::string Sessiondata;
    };

    struct Ayriaclient
    {
        uint32_t ClientID;
        std::wstring Locale;
        std::wstring Username;
    };
    #pragma pack(pop)

    // For modularity.
    namespace Internal
    {
        void UpdateLocalclient(float Deltatime);
        void InitLocalclient();
    }

    // Wrappers for the subsystems.
    inline void doFrame(float Deltatime)
    {
        Internal::UpdateLocalclient(Deltatime);
    }
    inline void Initialize()
    {
        Internal::InitLocalclient();
    }

    // Exports in JSON.
    namespace API
    {
        extern "C" EXPORT_ATTR void __cdecl getLocalclient(const char** JSONString);
    }
}
