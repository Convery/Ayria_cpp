/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

namespace Config
{
    // Save the configuration to disk.
    static void Saveconfig()
    {
        JSON::Object_t Object{};

        // Need to cast the bits to bool as otherwise they'd be uint16.
        Object["enableExternalconsole"] = (bool)Global.Settings.enableExternalconsole;
        Object["enableIATHooking"] = (bool)Global.Settings.enableIATHooking;
        Object["enableFileshare"] = (bool)Global.Settings.enableFileshare;
        Object["enableRouting"] = (bool)Global.Settings.enableRouting;
        Object["pruneDB"] = (bool)Global.Settings.pruneDB;
        Object["Username"] = *Global.Username;

        FS::Writefile(L"./Ayria/Settings.json", JSON::Dump(Object));
    }

    // Load (or reload) the configuration file.
    void Initialize()
    {
        // Load the last configuration from disk.
        const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
        Global.Settings.enableExternalconsole = Config.value<bool>("enableExternalconsole");
        Global.Settings.enableIATHooking = Config.value<bool>("enableIATHooking");
        Global.Settings.enableFileshare = Config.value<bool>("enableFileshare");
        Global.Settings.enableRouting = Config.value<bool>("enableRouting");
        Global.Settings.pruneDB = Config.value<bool>("pruneDB");
        *Global.Username = Config.value("Username", u8"AYRIA"s);

        // Select a source for crypto..
        if (std::strstr(GetCommandLineA(), "--randID")) setKey_RANDOM();
        else setKey_HWID();

        // Notify the user about the current settings.
        Infoprint("Loaded account:");
        Infoprint(va("ShortID: 0x%08X", Global.getShortID()));
        Infoprint(va("LongID: %s", Global.getLongID().c_str()));
        Infoprint(va("Username: %s", *Global.Username));

        // If there was no config, force-save one for the user instantly.
        std::atexit([]() { if (Global.Settings.modifiedConfig) Saveconfig(); });
        if (Config.empty()) Saveconfig();
    }

    // Generate an identifier for the current hardware.
    inline std::pair<std::string, std::string> GenerateHWID()
    {
        #if defined (_WIN32)
        std::string MOBOSerial{};
        std::string SystemUUID{};

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
                /*const auto Handle =*/ (void)Consume(uint16_t);

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

        return { MOBOSerial, SystemUUID };
        #else
        static_assert(false, "NIX function is not yet implemented.");
        return {};
        #endif
    }

    // Re-configure the client's public key (ID).
    void setKey_RANDOM()
    {
        uint8_t Seed[32]{};
        RAND_bytes(Seed, 32);
        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(std::to_array(Seed));
    }
    void setKey_HWID()
    {
        const auto [MOBO, UUID] = GenerateHWID();
        const auto Seed = Hash::SHA256(Hash::SHA256(MOBO) + Hash::SHA256(UUID));
        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(Seed);
    }
}
