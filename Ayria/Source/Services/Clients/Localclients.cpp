/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-23
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Clientinfo
{
    std::vector<Client_t> Localclients;
    Hashset<std::wstring> Clientnames;
    std::string HardwareID;

    // Primarily used for cryptography.
    std::string_view getHardwareID()
    {
        if (!HardwareID.empty()) return HardwareID;

        // Get motherboard-ID, system UUID, and volume-ID.
        #if defined (_WIN32)
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

        HardwareID = va("WIN#%u#%s#%s", VolumeID, MOBOSerial.c_str(), SystemUUID.c_str());

        #else
        HardwareID = va("NIX#%u#%s#%s", 0, "", "");
        #endif

        return HardwareID;
    }

    // Clients are split into LAN and WAN, networkIDs are for LAN.
    Client_t *getClientbyID(uint32_t NetworkID)
    {
        for (auto &Localclient : Localclients)
            if (Localclient.NetworkID == NetworkID)
                return &Localclient;
        return nullptr;
    }
    std::vector<Client_t *> getLocalclients()
    {
        std::vector<Client_t *> Result; Result.reserve(Localclients.size());
        for (auto &Localclient : Localclients)
            Result.push_back(&Localclient);
        return Result;
    }

    // Announce our presence once in a while.
    static void __cdecl Sendclientinfo()
    {
        const auto Object = JSON::Object_t({
            { "Username", std::u8string_view(Global.Username) },
            { "AccountID", Global.AccountID }
        });

        Backend::Network::Sendmessage("Clientdiscovery", JSON::Dump(Object));
    }
    static void __cdecl Discoveryhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Username = Request.value<std::wstring>("Username");
        const auto AccountID = Request.value<uint64_t>("AccountID");

        // Can't be arsed with clients that can't format requests.
        if (!AccountID || Username.empty())
        {
            Backend::Network::Blockclient(NodeID);
            return;
        }

        if (Localclients.end() != std::ranges::find_if(Localclients, [=](const auto &Client) { return Client.NetworkID == NodeID; }))
        {
            Client_t New{};
            New.NetworkID = NodeID;
            New.AccountID = AccountID;
            New.Username = &*Clientnames.insert(Username).first;
            Localclients.push_back(New);
        }
    }

    // Load client-info from disk.
    void Initialize()
    {
        #if defined (_WIN32)
        const char *Existingkey{};
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
            Global.Cryptokeys = PK_RSA::Createkeypair(Base64::Decode(Existingkey));
        }
        else
        {
            // Weak key as it's just for basic privacy.
            Global.Cryptokeys = PK_RSA::Createkeypair(512);

            // We do not want to wait for the system to finish.
            std::thread([]()
            {
                const auto Privatekey = Base64::Encode(PK_RSA::getPrivatekey(Global.Cryptokeys));

                #if defined (_WIN32)
                system(("SETX AYRIA_CLIENTPK "s + Privatekey).c_str());
                #else
                system(("SETENV AYRIA_CLIENTPK "s + Privatekey).c_str());
                #endif
            }).detach();
        }

        Backend::Enqueuetask(5000, Sendclientinfo);
        Backend::Enqueuetask(30000, Updateremoteclients);
        Backend::Network::addHandler("Clientdiscovery", Discoveryhandler);
    }
}
