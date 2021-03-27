/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-25
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// Fetch information about the system.
static std::string GenerateHWID();

namespace Services::Clientinfo
{
    using Client_t = struct { uint32_t NodeID; LZString_t Sharedkey; LZString_t Username; bool Authenticated; };
    static Hashmap<uint32_t /* ClientID */, Client_t> Onlineclients;
    static std::string ClienthardwareID;

    // Helper functionallity.
    std::string_view getHardwareID()
    {
        if (ClienthardwareID.empty()) [[unlikely]]
            ClienthardwareID = GenerateHWID();

        return ClienthardwareID;
    }
    uint32_t getClientID(uint32_t NodeID)
    {
        for (const auto &[Key, Value] : Onlineclients)
            if (Value.NodeID == NodeID)
                return Key;
        return 0;
    }
    std::string getSharedkey(uint32_t ClientID)
    {
        if (!Onlineclients.contains(ClientID)) return {};
        return Onlineclients[ClientID].Sharedkey;
    }

    // Handle local client-requests.
    static void __cdecl Discoveryhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Sharedkey = Request.value<std::string>("B64Sharedkey");
        const auto B64Ticket = Request.value<std::string>("B64Ticket");
        const auto Username = Request.value<std::u8string>("Username");
        const auto ClientID = Request.value<uint32_t>("ClientID");

        // If the client can't even format a simple request, block them.
        if (!ClientID || Username.empty()) [[unlikely]]
        {
            Infoprint(va("Blocking client %08X due to invalid discovery request.", ClientID));
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // Insert the new client if neeeded.
        auto Client = &Onlineclients[ClientID];
        if (!Client->NodeID)
        {
            *Client = { NodeID, B64Sharedkey, Username };

            // Notify developers about the progress.
            Debugprint(va("Found a new LAN client with ID %08X and name %s", ClientID, Encoding::toNarrow(Username).c_str()));

            // If the user doesn't want to be associated, block them.
            if (Relationships::isBlocked(Global.ClientID, ClientID)) [[unlikely]]
            {
                Infoprint(va("Blocking client %08X due to relationship status.", ClientID));
                Backend::Network::Blockclient(NodeID);
                Onlineclients.erase(ClientID);
                return;
            }
        }

        // Verify authentication ticket if provided.
        if (!Client->Authenticated && !B64Ticket.empty())
        {
            // TODO(tcn): Implement some online service.
            assert(false);
        }

        // Update the clients database entry.
        Backend::Database() << "REPLACE INTO Onlineclients ( Authenticated, ClientID, Lastupdate, B64Sharedkey, Username ) VALUES ( ?, ?, ?, ?, ? );"
                            << Client->Authenticated << ClientID << uint32_t(time(NULL)) << Client->Sharedkey << Client->Username;
    }
    static void __cdecl Terminationhandler(unsigned int NodeID, const char *, unsigned int)
    {
        const auto ClientID = getClientID(NodeID);

        // Remove the clients database entry.
        if (Onlineclients[ClientID].Authenticated)
            Backend::Database() << "DELETE FROM Onlineclients WHERE ClientID = ?;" << ClientID;
        else
            Backend::Database() << "DELETE FROM Onlineclients WHERE ClientID = ? AND Authenticated = false;" << ClientID;

        Onlineclients.erase(ClientID);
    }

    // Send requests to local clients.
    static void Senddiscovery()
    {
        // If the client is AFK, don't send any updates.
        if (Global.Stateflags.isAway) return;

        static const auto B64Sharedkey = Base64::Encode(PK_RSA::getPublickey(Global.Cryptokeys));
        auto Request = JSON::Object_t({
            { "Username", std::u8string(Global.Username) },
            { "B64Sharedkey", B64Sharedkey },
            { "ClientID", Global.ClientID }
        });

        // Only if we have authenticated.
        if (Global.B64Authticket) Request["B64Ticket"] = *Global.B64Authticket;

        Backend::Network::Transmitmessage("Clientinfo::Discovery", Request);
    }
    static void Sendterminate()
    {
        Backend::Network::Transmitmessage("Clientinfo::Terminate", {});
    }

    // JSON API access for the plugins.
    static std::string __cdecl Accountinfo(JSON::Value_t &&)
    {
        auto Response = JSON::Object_t({
            { "Username", std::u8string(Global.Username) },
            { "Locale", std::u8string(Global.Locale) },
            { "AccountID", Global.ClientID }
        });

        // Only if we have authenticated.
        if (Global.B64Authticket) Response["B64Ticket"] = *Global.B64Authticket;

        return JSON::Dump(Response);
    }

    // Set up the service.
    void Initialize()
    {
        // Create a weak random key for this session.
        Global.Cryptokeys = PK_RSA::Createkeypair(512);

        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Clientinfo::Terminate", Terminationhandler);
        Backend::Network::Registerhandler("Clientinfo::Discovery", Discoveryhandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("getAccountinfo", Accountinfo);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, Senddiscovery);
        std::atexit(Sendterminate);
    }
}

// Fetch information about the system.
static std::string GenerateHWID()
{
    // Get motherboard-ID, system UUID, and volume-ID.
    #if defined (_WIN32)
    std::string MOBOSerial;
    std::string SystemUUID;
    DWORD VolumeID{};

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

    return va("WIN#%u#%s#%s", VolumeID, MOBOSerial.c_str(), SystemUUID.c_str());

    #else
    static_assert(false, "NIX function is not yet implemented.");
    return va("NIX#%u#%s#%s", 0, "", "");
    #endif
}
