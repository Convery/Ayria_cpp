/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-07
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

#if defined (_WIN32)
// Get motherboard-ID, system UUID, and volume-ID.
static inline std::string getHWID()
{
    std::string MOBOSerial, SystemUUID;
    DWORD VolumeID{};

    // Volume information.
    GetVolumeInformationA("C:\\", nullptr, 0, &VolumeID, nullptr, nullptr, nullptr, NULL);

    // SMBIOS information.
    {
        const auto Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
        const auto Buffer = std::make_unique<char8_t[]>(Size);
        GetSystemFirmwareTable('RSMB', 0, Buffer.get(), Size);

        const auto Version_major = *(uint8_t *)(Buffer.get() + 1);
        const auto Tablelength = *(DWORD *)(Buffer.get() + 4);
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
                        SystemUUID.append(va("%02X", Table[4 + i]));

                    Table.remove_prefix(Length);
                }
                else if (Type == 2)
                {
                    const auto Index = *(uint8_t *)(Table.data() + 3);
                    Table.remove_prefix(Length);

                    for (uint8_t i = 1; i < Index; ++i)
                        Table.remove_prefix(std::strlen((char *)Table.data()) + 1);

                    MOBOSerial = std::string((char *)Table.data());
                }
                else Table.remove_prefix(Length);

                // Have all the information we want.
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

    return va("WIN32:%u:%s:%s", VolumeID, MOBOSerial.c_str(), SystemUUID.c_str());
}
#else
static inline std::string getHWID() { return "NIX:"; }
#endif

namespace Userinfo
{
    Account_t Account{};
    std::string Sharedkey{};
    std::string Privatekey{};
    std::string HardwareID{};

    // User information.
    Account_t *getAccount() { return &Account; };
    std::string_view getB64Sharedkey() { return Sharedkey; }
    std::string_view getB64Privatekey() { return Privatekey; }
    std::string_view getB64HardwareID() { return HardwareID; }

    // API export.
    std::string __cdecl Accountinfo(const char *)
    {
        JSON::Object_t Object;
        Object["Privatekey"] = Privatekey;
        Object["Locale"] = Account.Locale;
        Object["UserID"] = Account.ID.Raw;
        Object["Username"] = Account.Username;

        return JSON::Dump(Object);
    }

    // Announce our presence once in a while.
    static void __cdecl Sendclientinfo()
    {
        JSON::Object_t Object;
        Object["Sharedkey"] = Sharedkey;
        Object["Locale"] = Account.Locale;
        Object["UserID"] = Account.ID.Raw;
        Object["Username"] = Account.Username;

        Backend::Sendmessage(Hash::FNV1_32("Clientdiscovery"), JSON::Dump(Object));
    }

    void Initialize()
    {
        HardwareID = Base64::Encode(getHWID());
        const char *Existingkey{};

        #if defined (_WIN32)
        char Buffer[512]{};
        DWORD Size{ 512 };
        HKEY Registrykey;

        if (ERROR_SUCCESS == RegOpenKeyA(HKEY_CURRENT_USER, "Environment", &Registrykey))
        {
            if (ERROR_SUCCESS == RegQueryValueExA(Registrykey, "AYRIA_CLIENTPK", nullptr, nullptr, (LPBYTE)Buffer, &Size))
                Existingkey = Buffer;
            RegCloseKey(Registrykey);
        }
        #else
        Existingkey = std::getenv("AYRIA_CLIENTPK");
        #endif

        if (Existingkey)
        {
            Privatekey = Existingkey;
            Sharedkey = Base64::Encode(PK_RSA::getPublickey(PK_RSA::Createkeypair(Base64::Decode(Privatekey))));
        }
        else
        {
            // Weak key as it's just for basic privacy.
            const auto Keypair = PK_RSA::Createkeypair(512);
            Sharedkey = Base64::Encode(PK_RSA::getPublickey(Keypair));
            Privatekey = Base64::Encode(PK_RSA::getPrivatekey(Keypair));

            // We do not want to wait for the system to finish.
            std::thread([]()
            {
                #if defined (_WIN32)
                system(("SETX AYRIA_CLIENTPK "s + Privatekey).c_str());
                #else
                system(("SETENV AYRIA_CLIENTPK "s + Privatekey).c_str());
                #endif
            }).detach();
        }

        // Set the account from disk-data for offline sessions.
        const auto Object = JSON::Parse(FS::Readfile<char>("./Ayria/Clientinfo.json"));
        Account.ID.AccountID = Object.value("UserID", static_cast<uint32_t>(0xDEADC0DE));
        Account.Username = Object.value("Username", u8"Ayria"s);
        Account.Locale = Object.value("Locale", u8"English"s);

        // TODO(tcn): If we are logged in, get global account-data.

        // Register a background event.
        Backend::Enqueuetask(5000, Sendclientinfo);
    }
}
