/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-25
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// Get motherboard-ID or volume-ID as fallback.
static inline std::string getSerial()
{
    const auto Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    const auto Buffer = std::make_unique<char[]>(Size);
    GetSystemFirmwareTable('RSMB', 0, Buffer.get(), Size);

    const auto Version = *(WORD *)(Buffer.get() + 1);
    const auto Tablelength = *(DWORD *)(Buffer.get() + 4);
    auto Table = std::string_view(Buffer.get() + 8, Tablelength);

    if (Version == 0 || Version >= 0x0200)
    {
        #define Consume(x) *(x *)Table.data(); Table.remove_prefix(sizeof(x));
        while (!Table.empty())
        {
            const auto Type = Consume(uint8_t);
            const auto Length = Consume(uint8_t);
            const auto Handle = Consume(uint16_t);

            // EOF marker.
            if (Type == 127) break;

            // Only interested in the motherboard.
            if (Type != 2)
            {
                Table.remove_prefix(Length);
                while (!Table.empty())
                {
                    const auto Value = Consume(uint16_t);
                    if (Value == 0) break;
                }
            }
            else
            {
                const auto Index = *(uint8_t *)(Table.data() + 3);
                Table.remove_prefix(Length);

                for (uint8_t i = 1; i < Index; ++i)
                    Table.remove_prefix(std::strlen(Table.data()) + 1);

                return std::string(Table.data());
            }
        }
        #undef Consume
    }

    DWORD Serialnumber{};
    GetVolumeInformationA("C:\\", nullptr, 0, &Serialnumber, nullptr, nullptr, nullptr, NULL);
    return Hash::Tiger192(&Serialnumber, sizeof(DWORD));
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
            const auto Serial = getSerial();
            Hardwarekey = Hash::Tiger192(Serial.data(), Serial.size());
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
