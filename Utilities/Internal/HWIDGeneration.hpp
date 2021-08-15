/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-13
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Generate an identifier for the current hardware.
inline std::string GenerateHWID()
{
    // Get motherboard-ID, system UUID, and volume-ID.
    #if defined (_WIN32)
    DWORD VolumeID{};
    std::string MOBOSerial{};
    std::string SystemUUID{};

    // Volume information.
    GetVolumeInformationA("C:\\", nullptr, 0, &VolumeID, nullptr, nullptr, nullptr, NULL);

    // SMBIOS information.
    {
        const auto Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
        const auto Buffer = std::make_unique<char8_t[]>(Size);
        GetSystemFirmwareTable('RSMB', 0, Buffer.get(), Size);

        const auto Tablelength = *(DWORD *)(Buffer.get() + 4);
        const auto Version_major = *(uint8_t *)(Buffer.get() + 1);
        auto Table = std::u8string_view(Buffer.get() + 8, Tablelength);

        #define Consume(x) *(x *)Table.data(); Table.remove_prefix(sizeof(x));
        if (Version_major == 0 || Version_major >= 2)
        {
            while (!Table.empty())
            {
                const auto Type = Consume(uint8_t);
                const auto Length = Consume(uint8_t);
                const auto Handle = Consume(uint16_t);

                if (Type == 1)
                {
                    SystemUUID.reserve(16);
                    for (size_t i = 0; i < 16; ++i)
                        SystemUUID.append(std::format("{:02X}", (uint8_t)Table[4 + i]));

                    Table.remove_prefix(Length);
                }
                else if (Type == 2)
                {
                    const auto Index = *(const uint8_t *)(Table.data() + 3);
                    Table.remove_prefix(Length);

                    for (uint8_t i = 1; i < Index; ++i)
                        Table.remove_prefix(std::strlen((char *)Table.data()) + 1);

                    MOBOSerial = std::string((char *)Table.data());
                }
                else Table.remove_prefix(Length);

                // Have all the information we want?
                if (!MOBOSerial.empty() && !SystemUUID.empty()) break;

                // Skip to next entry.
                while (!Table.empty())
                {
                    const auto Value = Consume(uint16_t);
                    if (Value == 0) break;
                }
            }
        }
        #undef Consume
    }

    return std::format("WIN#{}#{}#{}", VolumeID, MOBOSerial, SystemUUID);
    #else
    static_assert(false, "NIX function is not yet implemented.");
    return std::format("NIX#{}#{}#{}", 0, "", "");
    #endif
}
