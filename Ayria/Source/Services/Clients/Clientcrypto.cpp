/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-27
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Clientinfo
{
    Hashmap<uint32_t, LZString_t> Clientkeys;
    std::string HardwareID;

    // If another clients crypto-key is needed, we request it.
    void Requestcryptokeys(std::vector<uint32_t> UserIDs)
    {
        const auto Object = JSON::Object_t({
            { "Senderkey", Base64::Encode(PK_RSA::getPublickey(Global.Cryptokeys)) },
            { "SenderID", Global.UserID },
            { "WantedIDs", UserIDs }
        });

        if (Global.Stateflags.isOnline)
        {
            // TODO(tcn): Forward request to some remote server.
        }

        Backend::Network::Transmitmessage("Keyshare", Object);
    }
    std::string getCryptokey(uint32_t UserID)
    {
        if (!Clientkeys.contains(UserID)) return {};
        else return Clientkeys[UserID];
    }

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
        static_assert(false, "NIX function is not yet implemented.");
        HardwareID = va("NIX#%u#%s#%s", 0, "", "");
        #endif

        return HardwareID;
    }

    // Handle local clients requests.
    static void __cdecl Keysharehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Senderkey = Request.value<std::string>("B64Senderkey");
        const auto WantedIDs = Request.value<std::vector<uint32_t> >("WantedIDs");

        // If the other client is secretive, ignore them.
        if (B64Senderkey.empty()) return;

        // Update the clients key if they are online.
        if (const auto Client = Clientinfo::getLANClient(NodeID))
        {
            Clientkeys.emplace(Client->UserID, B64Senderkey);

            // Developer information.
            Debugprint(va("Updated shared key for clientID %08X", Client->UserID));
        }

        // Check if they want our key back.
        for (const auto &ClientID : WantedIDs)
        {
            if (ClientID == Global.UserID)
            {
                // Reuse the function with no wanted keys.
                Requestcryptokeys({});
                break;
            }
        }
    }

    // In the future, allow a persistant key.
    void Initializecrypto()
    {
        // Create a weak random key for this session.
        Global.Cryptokeys = PK_RSA::Createkeypair(512);

        // Listen for local clients sharing their keys.
        Backend::Network::Registerhandler("Keyshare", Keysharehandler);
    }
}
