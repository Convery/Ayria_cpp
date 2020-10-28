/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-25
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

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

    return va("%u:%s:%s", VolumeID, MOBOSerial.c_str(), SystemUUID.c_str());
}

namespace Clientinfo
{
    std::unordered_map<uint32_t, std::string> Publickeys;
    std::string Hardwarekey{};
    RSA *Sessionkey{};

    // Requests key from Clients, shares own.
    void Syncpublickeys(std::vector<uint32_t> Clients)
    {
        // No need to request keys we already have.
        std::erase_if(Clients, [](const auto &ID) { return Publickeys.contains(ID); });

        auto Object = nlohmann::json::object();
        Object["Publickey"] = Base64::Encode(PK_RSA::getPublickey(getSessionkey()));
        Object["Wantedkeys"] = Clients;

        Backend::Sendmessage(Hash::FNV1_32("Syncpublickeys"), Object.dump());
    }
    void __cdecl Keysharehandler(uint32_t NodeID, const char *JSONString)
    {
        const auto Client = getNetworkclient(NodeID);
        if (!Client || Client->AccountID.Raw == 0) [[unlikely]]
            return; // WTF?

        const auto Request = ParseJSON(JSONString);
        const auto Publickey = Request.value("Publickey", std::string());
        const auto Wantedclients = Request.value("Wantedkeys", std::vector<uint32_t>());

        if (Publickey.empty()) [[unlikely]] return; // WTF?
        Publickeys[Client->AccountID.AccountID] = Request["Publickey"];

        const auto Localclient = Clientinfo::getLocalclient();
        for (const auto &wClient : Wantedclients)
        {
            if (wClient == Localclient->ID.AccountID)
            {
                Syncpublickeys({});
                break;
            }
        }
    }

    // Client crypto information.
    std::string_view getPublickey(uint32_t ClientID)
    {
        if (!Publickeys.contains(ClientID))
        {
            Syncpublickeys({ ClientID });
            return {};
        }
        else return Publickeys[ClientID];
    }
    std::string_view getHardwarekey()
    {
        if (Hardwarekey.empty()) [[unlikely]]
        {
            const auto HWID = getHWID();
            Hardwarekey = Hash::Tiger192(HWID.data(), HWID.size());
        }

        return Hardwarekey;
    }
    RSA *getSessionkey()
    {
        // Weak key, just for basic privacy.
        if (!Sessionkey) [[unlikely]]
        {
            Sessionkey = PK_RSA::Createkeypair(512);
        }

        return Sessionkey;
    }

    // Initialize the subsystems.
    void Initialize_crypto()
    {
        getHardwarekey(); getSessionkey();
        Backend::Registermessagehandler(Hash::FNV1_32("Syncpublickeys"), Keysharehandler);
    }
}
