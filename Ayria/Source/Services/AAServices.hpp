/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-13
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Services
{
    namespace Fileshare { void Initialize(); }
    namespace Usergroups { void Initialize(); }
    namespace Matchmaking { void Initialize(); }
    namespace Socialpresence { void Initialize(); }
    namespace Groupmessaging { void Initialize(); }
    namespace Clientmessaging { void Initialize(); }
    namespace Clientrelations { void Initialize(); }

    namespace Clientinfo
    {
        struct Client_t
        {
            uint32_t GameID, ModID;
            std::u8string Username;
            std::array<uint8_t, 32> Signingkey;
            std::array<uint8_t, 32> Encryptionkey;
        };

        // Add the handlers and tasks.
        void Initialize();
    }

    inline void Initialize()
    {
        Clientinfo::Initialize();
    }
}
