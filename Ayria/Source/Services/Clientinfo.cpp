/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-23
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

#include <openssl/curve25519.h>

namespace Services::Clientinfo
{
    // Keep a cache of the active LAN clients.
    static Hashmap<uint32_t /* SessionID */, uint32_t /* AccountID */> Activeclients{};
    static Hashmap<uint32_t /* AccountID */, std::array<uint8_t, 32>> Sharedkeys{};
    static uint64_t Lastmessage{};

    // Fetch the AccountID from a LAN clients SessionID.
    uint32_t getAccountID(uint32_t SessionID)
    {
        if (!Activeclients.contains(SessionID)) [[unlikely]] return {};
        return Activeclients[SessionID];
    }
    uint32_t getSessionID(uint32_t AccountID)
    {
        for (const auto &[Session, Account] : Activeclients)
            if (Account == AccountID)
                return Session;
        return {};
    }

    // Broadcast to the LAN clients.
    static void Sendping() { Network::LAN::Publish("AYRIA::Clientping"); }
    static void Sendgoodbye() { Network::LAN::Publish("AYRIA::Clientgoodbye"); }
    static void Sendhello(bool isHeartbeat = false, JSON::Value_t &&Deltavalues = {})
    {
        const auto Currenttime = GetTickCount64();

        if (!Deltavalues.isNull()) [[unlikely]]
        {
            Network::LAN::Publish("AYRIA::Clienthello", JSON::Dump(Deltavalues));
        }
        else
        {
            if (isHeartbeat) [[likely]]
            {
                // Let's not spam.
                if ((Currenttime - Lastmessage) < 5000) return;
                Network::LAN::Publish("AYRIA::Clienthello");
            }
            else
            {
                // Format the global state.
                const auto Request = JSON::Object_t({
                    { "Sharedkey", Base64::Encode(*Global.Publickey) },
                    { "Username", std::u8string(Global.Username) },
                    { "InternalIP", Global.InternalIP },
                    { "ExternalIP", Global.ExternalIP },
                    { "AccountID", Global.AccountID },
                    { "GameID", Global.GameID },
                    { "ModID", Global.ModID }
                });

                Network::LAN::Publish("AYRIA::Clienthello", JSON::Dump(Request));
            }
        }

        // Update the heartbeat timeout.
        Lastmessage = Currenttime;
    }

    // Callbacks for the LAN messages.
    static void __cdecl Clienthello(unsigned int SessionID, const char *Message, unsigned int Length)
    {
        auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto Sharedkey = Request.value<std::string>("Sharedkey");
        const auto Username = Request.value<std::string>("Username");
        const auto AccountID = Request.value<uint32_t>("AccountID");
        const auto GameID = Request.value<uint32_t>("GameID");
        const auto ModID = Request.value<uint32_t>("ModID");
        const auto Sharedkeyhash = Hash::WW32(Sharedkey);

        // Have we seen this client before?
        if (!Activeclients.contains(SessionID)) [[likely]]
        {
            // We need to know the key to continue.
            if (Sharedkey.empty())
            {
                Sendping();
                return;
            }

            // Update the cache.
            Activeclients[SessionID] = AccountID;
            std::memcpy(Sharedkeys[AccountID].data(), Base64::Decode(Sharedkey).data(), 32);

            // Add the client to the DB.
            DB::setKnownclients(JSON::Object_t({
                { "Sharedkeyhash", Sharedkeyhash },
                { "AccountID", AccountID },
                { "Username", Username }
            }));
        }

        // If no changes are sent, we just update the timestamp.
        if (!Request.empty())
        {
            // Insert any new info into the DB,
            Request["InternalIP"sv] = Network::LAN::getPeer(SessionID);
            Request["AyriaID"sv] = DB::getKeyhash(AccountID);
            DB::setClientinfo(std::move(Request));

            // Notify subscribers about this event.
            const auto Info = JSON::Dump(JSON::Object_t({
                { "AccountID", AccountID },
                { "Username", Username },
                { "GameID", GameID },
                { "ModID", ModID }
            }));
            Postevent("Client::Info", Info.c_str(), Info.size());
        }

        DB::Touchclient(DB::getKeyhash(AccountID));
    }
    static void __cdecl Clientping(unsigned int, const char *, unsigned int) { Sendhello(); }
    static void __cdecl Clientgoodbye(unsigned int AccountID, const char *, unsigned int)
    {
        // Only notify the plugins if we've seen this client before.
        const auto SessionID = getSessionID(AccountID);
        if (NULL == SessionID) [[unlikely]] return;

        // Fetch clientinfo.
        const auto Info = JSON::Dump(DB::getClientinfo(DB::getKeyhash(AccountID)));

        // Notify subscribers about this event.
        Postevent("Client::Quit", Info.c_str(), Info.size());
        Activeclients.erase(SessionID);
        Sharedkeys.erase(AccountID);
    }

    // JSON access to the global state.
    static std::string __cdecl setSocialstate(JSON::Value_t &&Request)
    {
        if (Request.contains("isPrivate")) Global.Settings.isPrivate = Request.value<bool>("isPrivate");
        if (Request.contains("isAway")) Global.Settings.isAway = Request.value<bool>("isAway");
        return "{}";
    }
    static std::string __cdecl setGamestate(JSON::Value_t &&Request)
    {
        if (Request.contains("isHosting")) Global.Settings.isHosting = Request.value<bool>("isHosting");
        if (Request.contains("isIngame")) Global.Settings.isIngame = Request.value<bool>("isIngame");
        if (Request.contains("GameID")) Global.GameID = Request.value<uint32_t>("GameID");
        if (Request.contains("ModID")) Global.ModID = Request.value<uint32_t>("ModID");
        return "{}";
    }
    static std::string __cdecl getClientinfo(JSON::Value_t &&)
    {
        const auto Object = JSON::Object_t({
            { "Sharedkey", Base64::Encode(*Global.Publickey) },
            { "Username", std::u8string(Global.Username) },
            { "Sharedkeyhash", Global.Sharedkeyhash },
            { "InternalIP", Global.InternalIP },
            { "ExternalIP", Global.ExternalIP },
            { "AccountID", Global.AccountID },
            { "GameID", Global.GameID },
            { "ModID", Global.ModID }
        });

        return JSON::Dump(Object);
    }

    // Create a proper key for server authentication, returns the email-hash.
    std::string Createkey(std::string_view Email, std::string_view Password)
    {
        constexpr auto Emailsalt = Base64::Encode("This is a silly email");
        uint8_t Hash[64]{}, Seed[32]{};

        const auto OK1 = PKCS5_PBKDF2_HMAC(Email.data(), Email.size(), (uint8_t *)Emailsalt.data(), Emailsalt.size(), 1429, EVP_sha512(), 64, Hash);
        const auto OK2 = PKCS5_PBKDF2_HMAC(Password.data(), Password.size(), Hash, 64, 9241, EVP_sha512(), 32, Seed);
        if (!OK1 || !OK2) return {};

        // Fixup for RFC7748
        Seed[0] |= ~248;
        Seed[31] &= ~64;
        Seed[31] |= ~127;

        // Initialize the crypto state from the previous hash.
        if (!Global.Privatekey) Global.Privatekey = new std::array<uint8_t, 32>();
        if (!Global.Publickey) Global.Publickey = new std::array<uint8_t, 32>();
        X25519_public_from_private(Global.Publickey->data(), Seed);
        std::memcpy(Global.Privatekey->data(), Seed, 32);

        Global.Sharedkeyhash = Hash::WW32(Base64::Encode(*Global.Publickey));
        return Base64::Encode(std::string_view((char *)Hash, 64));
    }

    // Derive a key for encryption.
    std::string Derivecryptokey(uint32_t AccountID)
    {
        // While we could check the DB, we can't do anything if the client is offline.
        if (!Sharedkeys.contains(AccountID)) return {};

        uint8_t Secret[32]{};
        if (NULL == X25519(Secret, Global.Privatekey->data(), Sharedkeys[AccountID].data()))
            return {};

        return Hash::Tiger192(Secret, 32);
    }

    // Set up the service.
    void Initialize()
    {
        // Create a random key for this session.
        Global.Publickey = new std::array<uint8_t, 32>();
        Global.Privatekey = new std::array<uint8_t, 32>();
        X25519_keypair(Global.Publickey->data(), Global.Privatekey->data());
        Global.Sharedkeyhash = Hash::WW32(Base64::Encode(*Global.Publickey));

        // Register the LAN callbacks.
        Network::LAN::Register("AYRIA::Clientgoodbye", Clientgoodbye);
        Network::LAN::Register("AYRIA::Clienthello", Clienthello);
        Network::LAN::Register("AYRIA::Clientping", Clientping);

        // Register the JSON endpoints.
        Backend::API::addEndpoint("setSocialstate", setSocialstate);
        Backend::API::addEndpoint("getClientinfo", getClientinfo);
        Backend::API::addEndpoint("setGamestate", setGamestate);

        // Insert our session into the DB incase the plugins need it.
        DB::setKnownclients(JSON::Object_t({
            { "Username", std::u8string(Global.Username) },
            { "Sharedkeyhash", Global.Sharedkeyhash },
            { "AccountID", Global.AccountID }
        }));
        DB::setClientinfo(JSON::Object_t({
            { "Sharedkey", Base64::Encode(*Global.Publickey) },
            { "Username", std::u8string(Global.Username) },
            { "InternalIP", Global.InternalIP },
            { "ExternalIP", Global.ExternalIP },
            { "AccountID", Global.AccountID },
            { "GameID", Global.GameID },
            { "ModID", Global.ModID }
        }));

        // Set up a periodic task to keep alive.
        Backend::Enqueuetask(1000, []() { if (!Global.Settings.isAway) Sendhello(true); });
        std::atexit(Sendgoodbye);
    }

    // Generate an identifier for the current hardware.
    std::string GenerateHWID()
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
}
