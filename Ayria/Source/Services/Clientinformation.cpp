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
    static Hashmap<uint32_t /* NodeID */, uint32_t /* ClientID */> Onlineclients;
    static Hashset<uint64_t> Verifiedtickets;
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
        return Onlineclients[NodeID];
    }
    std::string getSharedkey(uint32_t ClientID)
    {
        try
        {
            std::string Sharedkey;
            Backend::Database() << "SELECT B64Sharedkey FROM Clientinfo WHERE ClientID = ?;" << ClientID
                                >> Sharedkey;
            return Base64::Decode(Sharedkey);
        }
        catch (...) {}

        return {};
    }

    // Handle local client-requests.
    static void __cdecl Discoveryhandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto B64Authticket = Request.value<std::string>("B64Authticket");
        const auto B64Sharedkey = Request.value<std::string>("B64Sharedkey");
        const auto Username = Request.value<std::u8string>("Username");
        const auto PublicIP = Request.value<uint32_t>("PublicIP");
        const auto ClientID = Request.value<uint32_t>("ClientID");
        const auto GameID = Request.value<uint32_t>("GameID");

        // The sender didn't include essential information, so we block them for this session.
        if (!ClientID || Username.empty()) [[unlikely]]
        {
            Debugprint(va("Blocking client %08X for this session due to an invalid discovery request.", ClientID));
            Backend::Network::Blockclient(NodeID);
            return;
        }

        // Is this a new client?
        if (!Onlineclients.contains(NodeID)) [[unlikely]]
        {
            // Notify developers about the progress.
            Infoprint(va("Found a new client with ID %08X and name %s.", ClientID, Encoding::toNarrow(Username).c_str()));

            // If the client has blocked the other, we should not waste time processing updates.
            if (Relationships::isBlocked(Global.ClientID, ClientID)) [[unlikely]]
            {
                Debugprint(va("Blocking client %08X due to relationship status.", ClientID));
                Backend::Network::Blockclient(NodeID);
                return;
            }

            // Add the client to the list.
            Onlineclients[NodeID] = ClientID;
        }

        // Insert or replace into the database any new/changed values.
        try { Backend::Database() << "REPLACE INTO Clientinfo (ClientID, Lastupdate, B64Authticket, B64Sharedkey, "
                                     "PublicIP, GameID, Username, isLocal) VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
                                  << ClientID << time(NULL) << B64Authticket << B64Sharedkey << PublicIP
                                  << GameID << Encoding::toNarrow(Username) << true;
        } catch (...) {}
    }
    static void __cdecl Terminationhandler(unsigned int NodeID, const char *, unsigned int)
    {
        if constexpr (Build::isDebug)
        {
            try
            {
                std::string Username; const auto ClientID = getClientID(NodeID);
                Backend::Database() << "SELECT Username FROM Clientinfo WHERE ClientID = ?;" << ClientID >> Username;

                Debugprint(va("Client %08X \"%s\" has terminated.", ClientID, Username.c_str()));
            }
            catch (...) {}
        }

        Onlineclients.erase(NodeID);
    }

    // Send requests to local clients.
    static void Senddiscovery()
    {
        // If the client is private, don't send updates.
        if (Global.Stateflags.isPrivate) [[unlikely]] return;

        // If the client is AFK, don't send any updates.
        if (Global.Stateflags.isAway) [[unlikely]] return;

        // The key needs to be Base64 else it'll be encoded as a massive array.
        static const auto B64Sharedkey = Base64::Encode(PK_RSA::getPublickey(Global.Cryptokeys));

        // Auth-ticket is only provided if we are authenticated.
        auto Request = JSON::Object_t({
            { "B64Authticket", Global.B64Authticket ? *Global.B64Authticket : ""s },
            { "Username", std::u8string(Global.Username) },
            { "PublicIP", Global.Publicaddress },
            { "B64Sharedkey", B64Sharedkey },
            { "ClientID", Global.ClientID },
            { "GameID", Global.GameID },
        });

        // On startup, also insert our own info for easier lookups.
        static bool doOnce = [&](){
            const auto Message = JSON::Dump(Request);
            Discoveryhandler(0, Message.c_str(), (unsigned int)Message.size());
            return true;
        }();

        // Notify all LAN clients.
        Backend::Network::Transmitmessage("Clientinfo::Discovery", Request);
    }
    static void Sendterminate()
    {
        Backend::Network::Transmitmessage("Clientinfo::Terminate", {});
    }

    // Check for new states once in a while.
    static void Updatestate()
    {
        const auto Changes = Backend::getDatabasechanges("Clientinfo");
        for (const auto &Item : Changes)
        {
            try
            {
                Backend::Database() << "SELECT ClientID, B64Authticket, GameID from Clientinfo WHERE rowid = ?;"
                                    << Item >> [](uint32_t ClientID, std::string B64Authticket, uint32_t GameID)
                {
                    if (ClientID == Global.ClientID) Global.GameID = GameID;
                    else
                    {
                        const auto Tickethash = Hash::WW64(B64Authticket);
                        if (!Verifiedtickets.contains(Tickethash)) [[unlikely]]
                        {
                            // TODO(tcn): Validate the ticket.
                        }
                    }
                };
            }
            catch (...) {};
        }

        // Notify the other clients that we are alive.
        Senddiscovery();
    }

    // Set up the service.
    void Initialize()
    {
        // Create a weak random key for this session.
        Global.Cryptokeys = PK_RSA::Createkeypair(512);

        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Clientinfo::Terminate", Terminationhandler);
        Backend::Network::Registerhandler("Clientinfo::Discovery", Discoveryhandler);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, Updatestate);
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
