/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"
#include <winioctl.h>

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

    // Just two random sources for somewhat static data.
    inline cmp::Vector_t<uint8_t, 32> bySMBIOS()
    {
        uint8_t Version_major{}, Version_minor{};
        std::u8string_view Table;

        #if defined (_WIN32)
        const auto Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
        const auto Buffer = std::make_unique<char8_t[]>(Size);
        GetSystemFirmwareTable('RSMB', 0, Buffer.get(), Size);

        Version_major = *(uint8_t *)(Buffer.get() + 1);
        Version_minor = *(uint8_t *)(Buffer.get() + 2);
        const auto Tablelength = *(uint32_t *)(Buffer.get() + 4);
        Table = std::u8string_view(Buffer.get() + 8, Tablelength);

        #else // Linux assumed.
        const auto File = FS::Readfile<char8_t>("/sys/firmware/dmi/tables/smbios_entry_point");

        // SMBIOS
        if (*(uint32_t *)File.data() == 0x5F534D5F)
        {
            Version_major = File[6]; Version_minor = File[7];

            const auto Offset = *(uint32_t *)(File.data() + 0x18);
            const auto Tablelength = *(uint32_t *)(File.data() + 0x16);
            Table = std::u8string_view(File.data() + Offset, Tablelength);
        }

        // SMBIOS3
        if (*(uint32_t *)File.data() == 0x5F534D33)
        {
            Version_major = File[7]; Version_minor = File[8];

            const auto Offset = *(uint64_t *)(File.data() + 0x10);
            const auto Tablelength = *(uint32_t *)(File.data() + 0x0C);
            Table = std::u8string_view(File.data() + Offset, Tablelength);
        }
        #endif

        // Fixup for silly OEMs..
        if (Version_minor == 0x1F) Version_minor = 3;
        if (Version_minor == 0x21) Version_minor = 3;
        if (Version_minor == 0x33) Version_minor = 6;

        // Sometimes 2.x is reported as "default" AKA 0.
        if (Version_major == 0 || Version_major >= 2)
        {
            std::string Serial1{}, Serial2{}, Serial3{};

            while (!Table.empty())
            {
                // Helper to skip trailing strings.
                const auto Structsize = [](const char8_t *Start) -> size_t
                {
                    auto End = Start;
                    End += Start[1];

                    if (!*End) End++;
                    while (*(End++)) while (*(End++)) {};
                    return End - Start;
                } (Table.data());

                auto Entry = Table.substr(0, Structsize);
                const auto Headerlength = Entry[1];
                const auto Type = Entry[0];

                if (Version_minor >= 3)
                {
                    //
                    if (Type == 17)
                    {
                        const auto Stringindex = Entry[0x18];
                        Entry.remove_prefix(Headerlength);

                        for (uint8_t i = 1; i < Stringindex; ++i)
                            Entry.remove_prefix(std::strlen((char *)Entry.data()) + 1);

                        if (Serial1.empty()) Serial1 = std::string((char *)Entry.data());
                        else if (Serial2.empty()) Serial2 = std::string((char *)Entry.data());
                        else Serial3 = std::string((char *)Entry.data());
                    }
                }
                else
                {
                    // Sometimes messed up if using modded BIOS, i.e. 02000300040005000006000700080009
                    if (Type == 1)
                    {
                        std::string Serial; Serial.reserve(16);

                        for (size_t i = 0; i < 16; ++i) Serial.append(std::format("{:02X}", (uint8_t)Entry[8 + i]));

                        if (Serial1.empty()) Serial1 = Serial;
                        else if (Serial2.empty()) Serial2 = Serial;
                        else Serial3 = Serial;
                    }

                    // Sometimes not actually filled..
                    if (Type == 2)
                    {
                        const auto Stringindex = Entry[0x03];
                        Entry.remove_prefix(Headerlength);

                        for (uint8_t i = 1; i < Stringindex; ++i)
                            Entry.remove_prefix(std::strlen((char *)Entry.data()) + 1);

                        if (Serial1.empty()) Serial1 = std::string((char *)Entry.data());
                        else if (Serial2.empty()) Serial2 = std::string((char *)Entry.data());
                        else Serial3 = std::string((char *)Entry.data());
                    }

                    // Not as unique as we want..
                    if (Type == 4)
                    {
                        const auto Serial = std::format("{}", *(uint64_t *)&Entry[8]);
                        if (Serial1.empty()) Serial1 = Serial;
                        else if (Serial2.empty()) Serial2 = Serial;
                        else Serial3 = Serial;
                    }
                }

                if (!Serial1.empty() && !Serial2.empty() && !Serial3.empty()) break;
                Table.remove_prefix(Structsize);
            }

            const auto Maxsize = std::max(Serial1.size(), std::max(Serial2.size(), Serial3.size()));
            const auto TMP = (uint8_t *)alloca(Maxsize);
            std::memset(TMP, 0, Maxsize);

            // In-case we want to verify something.
            if constexpr (Build::isDebug)
            {
                Infoprint(va("Serial1: %s", Serial1));
                Infoprint(va("Serial2: %s", Serial2));
                Infoprint(va("Serial3: %s", Serial3));
            }

            // Mix so that the ordering is no longer relevant.
            for (size_t i = 0; i < Serial1.size(); ++i) TMP[i] ^= Serial1[i];
            for (size_t i = 0; i < Serial2.size(); ++i) TMP[i] ^= Serial2[i];
            for (size_t i = 0; i < Serial3.size(); ++i) TMP[i] ^= Serial3[i];

            return Hash::SHA256(std::span(TMP, Maxsize));
        }

        return {};
    }
    inline cmp::Vector_t<uint8_t, 32> byDisk()
    {
        constexpr auto getNVME = []() -> std::string
        {
            std::array<uint32_t, 1036> Query = { 49, 0, 3, 1, 1, 0, 40, 4096 };

            const HANDLE Handle = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE,
                                                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

            if (!DeviceIoControl(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query), &Query, sizeof(Query), NULL, NULL))
            {
                CloseHandle(Handle);
                return {};
            }
            else
            {
                // Some manufacturers don't care about endian..
                const auto Buffer = std::span((char *)&Query[13], 20);
                const auto Spaces = std::ranges::count(Buffer, ' ');
                if (Buffer[Buffer.size() - Spaces - 1] == ' ')
                {
                    for (uint8_t i = 0; i < 20; i += 2)
                    {
                        std::swap(Buffer[i], Buffer[i + 1]);
                    }
                }

                CloseHandle(Handle);
                return std::string((char *)&Query[13], 20);
            }
        };
        constexpr auto getSATA = []() -> std::string
        {
            std::array<uint8_t, 36 + 512> Query{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0xA0, 0xEC }; *(uint32_t *)&Query[0] = 512;

            const HANDLE Handle = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE,
                                                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

            if (!DeviceIoControl(Handle, 0x0007C088, &Query, sizeof(Query), &Query, sizeof(Query), NULL, NULL))
            {
                CloseHandle(Handle);
                return {};
            }
            else
            {
                // Some manufacturers don't care about endian..
                const auto Buffer = std::span((char *)&Query[36], 20);
                const auto Spaces = std::ranges::count(Buffer, ' ');
                if (Buffer[Buffer.size() - Spaces - 1] == ' ')
                {
                    for (uint8_t i = 0; i < 20; i += 2)
                    {
                        std::swap(Buffer[i], Buffer[i + 1]);
                    }
                }

                CloseHandle(Handle);
                return std::string((char *)&Query[36], 20);
            }
        };

        const auto Serial1 = getNVME();
        if constexpr (Build::isDebug) Infoprint(va("NVME: %s", Serial1));
        if (!Serial1.empty()) return Hash::SHA256(Serial1);

        const auto Serial2 = getSATA();
        if constexpr (Build::isDebug) Infoprint(va("SATA: %s", Serial2));
        if (!Serial2.empty()) return Hash::SHA256(Serial2);

        return {};
    }

    // Re-configure the client's public key (ID).
    void setKey_RANDOM()
    {
        std::array<uint32_t, 4> Seed;
        rand_s(&Seed[0]); rand_s(&Seed[1]);
        rand_s(&Seed[2]); rand_s(&Seed[3]);

        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(std::as_bytes(std::span(Seed)));
    }
    void setKey_HWID()
    {
        const auto Seed = Hash::SHA256(bySMBIOS() + byDisk());
        std::tie(*Global.Publickey, *Global.Privatekey) = qDSA::Createkeypair(Seed);
    }
}
