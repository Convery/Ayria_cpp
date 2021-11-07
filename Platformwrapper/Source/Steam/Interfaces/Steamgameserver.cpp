/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-24
    License: MIT
*/

#include <Steam.hpp>

namespace Steam
{
    enum ESteamIPType
    {
        k_ESteamIPTypeIPv4 = 0,
        k_ESteamIPTypeIPv6 = 1,
    };
    struct SteamIPAddress_t
    {
        union
        {
            uint32_t m_unIPv4;		    // Host order
            uint8_t m_rgubIPv4[4];	    // Host order
            uint8_t m_rgubIPv6[16];		// Network order
            uint16_t m_rguwIPv6[8];		// Network order
            uint64_t m_ipv6Qword[2]{};	// Network order
        };
        ESteamIPType m_eType{ k_ESteamIPTypeIPv4 };

        void setString(const std::string &Address)
        {
            // Assume IPv4 uses '.' and v6 uses ':'
            if (Address.find('.') != Address.npos)
            {
                for (const auto &[Index, Item] : lz::enumerate(Tokenizestring(Address, '.')))
                {
                    m_rgubIPv4[Index] = std::strtoul(Item.c_str(), nullptr, 10);
                }
            }
            else
            {
                // Does not support the :: style shortening.
                assert(Address.find("::") != Address.npos);

                for (const auto &[Index, Item] : lz::enumerate(Tokenizestring(Address, ':')))
                {
                    m_rguwIPv6[Index] = std::strtoul(Item.c_str(), nullptr, 16);
                }
            }
        }
        std::string toString() const
        {
            if (m_eType == k_ESteamIPTypeIPv4)
            {
                return va("%u.%u.%u.%u", m_rgubIPv4[0], m_rgubIPv4[1], m_rgubIPv4[2], m_rgubIPv4[3]);
            }
            else
            {
                return va("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                    m_rguwIPv6[0], m_rguwIPv6[1], m_rguwIPv6[2], m_rguwIPv6[3],
                    m_rguwIPv6[4], m_rguwIPv6[5], m_rguwIPv6[6], m_rguwIPv6[7]);
            }
        }
    };
    constexpr SteamIPAddress_t IPv4Any{ .m_eType = k_ESteamIPTypeIPv4 };
    constexpr auto IPv6Any = SteamIPAddress_t{ .m_eType = k_ESteamIPTypeIPv6 };
    constexpr auto IPv6Loopback = SteamIPAddress_t{ .m_ipv6Qword = {0, 1}, .m_eType = k_ESteamIPTypeIPv6 };
    constexpr auto IPv4Loopback = SteamIPAddress_t{ .m_unIPv4 = 0x7f000001, .m_eType = k_ESteamIPTypeIPv4 };

    struct Steamserver_t
    {
        SteamIPAddress_t PublicIP;
        bool isActive, isDedicated, isSecure, isPasswordprotected;

        uint16_t Spectatorport, Gameport, Authport, Queryport;
        uint32_t Playercount, Playermax, Botcount, Spawncount;
        uint32_t Serverflags, ServerID, AppID;

        std::string Gamedata, Gametags, Gametype, Gamedescription, Gamedir;
        std::string Servername, Mapname, Spectatorname;
        std::string Moddir, Region, Product, Version;

        std::unordered_map<std::string, std::string> Keyvalues;
        Hashmap<uint64_t, std::pair<std::string, uint32_t>> Userdata;

        std::string getUserdataJSON() const
        {
            JSON::Array_t Array;
            Array.reserve(Userdata.size());
            for (const auto &[UserID, Pair] : Userdata)
            {
                const auto &[Username, Userscore] = Pair;

                Array.emplace_back(JSON::Object_t({
                    { "Userscore", Userscore },
                    { "Username", Username },
                    { "UserID", UserID }
                }));
            }

            return JSON::Dump(Array);
        }
        void setUserdataJSON(const JSON::Array_t &Array)
        {
            Userdata.clear(); Userdata.reserve(Array.size());

            for (const auto &Item : Array)
            {
                Userdata.emplace(Item.value<uint64_t>("UserID"),
                                 std::make_pair(Item.value<std::string>("Username"), Item.value<uint32_t>("Userscore")));
            }
        }
    };
    static Steamserver_t Localserver{};
    static Hashmap<uint64_t, std::pair<uint32_t, Blob>> Tickets{};

    // Only handles the Steam part of the server.
    static Steamserver_t fromJSON(std::string_view JSONString)
    {
        const auto Object = JSON::Parse(JSONString);
        Steamserver_t Server{};

        Server.isPasswordprotected = Object.value<bool>("isPasswordprotected");
        Server.isDedicated = Object.value<bool>("isDedicated");
        Server.isSecure = Object.value<bool>("isSecure");
        Server.isActive = Object.value<bool>("isActive");

        Server.Spectatorport = Object.value<uint16_t>("Spectatorport");
        Server.Queryport = Object.value<uint16_t>("Queryport");
        Server.Gameport = Object.value<uint16_t>("Gameport");
        Server.Authport = Object.value<uint16_t>("Authport");

        Server.Playercount = Object.value<uint32_t>("Playercount");
        Server.Serverflags = Object.value<uint32_t>("Serverflags");
        Server.Spawncount = Object.value<uint32_t>("Spawncount");
        Server.Playermax = Object.value<uint32_t>("Playermax");
        Server.Botcount = Object.value<uint32_t>("Botcount");
        Server.ServerID = Object.value<uint32_t>("ServerID");
        Server.AppID = Object.value<uint32_t>("AppID");

        Server.Gamedescription = Object.value<std::string>("Gamedescription");
        Server.Spectatorname = Object.value<std::string>("Spectatorname");
        Server.PublicIP.setString(Object.value<std::string>("PublicIP"));
        Server.Servername = Object.value<std::string>("Servername");
        Server.Gamedata = Object.value<std::string>("Gamedata");
        Server.Gametags = Object.value<std::string>("Gametags");
        Server.Gametype = Object.value<std::string>("Gametype");
        Server.Gamedir = Object.value<std::string>("Gamedir");
        Server.Mapname = Object.value<std::string>("Mapname");
        Server.Product = Object.value<std::string>("Product");
        Server.Version = Object.value<std::string>("Version");
        Server.Moddir = Object.value<std::string>("Moddir");
        Server.Region = Object.value<std::string>("Region");

        Server.Keyvalues = Object.value<std::unordered_map<std::string, std::string>>("Keyvalues");
        Server.setUserdataJSON(Object.value<JSON::Array_t>("Userdata"));
        return Server;
    }
    static JSON::Object_t toJSON(const Steamserver_t &Server)
    {
        const auto Steamgamedata = JSON::Object_t({
            { "isPasswordprotected", Server.isPasswordprotected },
            { "isDedicated", Server.isDedicated },
            { "isSecure", Server.isSecure },
            { "isActive", Server.isActive },

            { "Spectatorport", Server.Spectatorport },
            { "Queryport", Server.Queryport },
            { "Gameport", Server.Gameport },
            { "Authport", Server.Authport },

            { "Serverflags", Server.Serverflags },
            { "Playercount", Server.Playercount },
            { "Spawncount", Server.Spawncount },
            { "Playermax", Server.Playermax },
            { "Botcount", Server.Botcount },
            { "ServerID", Server.ServerID },
            { "AppID", Server.AppID },

            { "Gamedescription", Server.Gamedescription },
            { "PublicIP", Server.PublicIP.toString() },
            { "Gamedata", Server.Gamedata },
            { "Gametags", Server.Gametags },
            { "Gametype", Server.Gametype },
            { "Gamedir", Server.Gamedir },

            { "Spectatorname", Server.Spectatorname },
            { "Product", Server.Product },
            { "Version", Server.Version },
            { "Moddir", Server.Moddir },
            { "Region", Server.Region },

            { "Keyvalues", Server.Keyvalues },
            { "Userdata", Server.getUserdataJSON() },

            { "Servername", Server.Servername },
            { "Mapname", Server.Mapname }
        });

        return Steamgamedata;
    }

    namespace Gameserver
    {
        static void UpdateDB(const Steamserver_t &Server)
        {
            try
            {
                Database()
                    << "INSERT OR REPLACE INTO Gameserver VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);"
                    << Server.AppID
                    << Server.ServerID

                    << Server.Gamedescription
                    << Server.Spectatorname
                    << Server.Servername
                    << Server.Gamedata
                    << Server.Gametags
                    << Server.Gametype
                    << Server.Gamedir
                    << Server.Mapname
                    << Server.Product
                    << Server.Version
                    << Server.Moddir
                    << Server.Region

                    << JSON::Dump(Server.Keyvalues)
                    << Server.getUserdataJSON()
                    << Server.PublicIP.toString()

                    << Server.Spectatorport
                    << Server.Queryport
                    << Server.Gameport
                    << Server.Authport

                    << Server.Playercount
                    << Server.Serverflags
                    << Server.Spawncount
                    << Server.Playermax
                    << Server.Botcount

                    << Server.isPasswordprotected
                    << Server.isDedicated
                    << Server.isActive
                    << Server.isSecure;

            } catch (...) {}
        }

        // Later 2 interaction.
        static bool __cdecl onUpdate(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Server = fromJSON(std::string_view(Message, Length));
            if (Server.ServerID != Hash::WW32(std::string(LongID))) return false;

            UpdateDB(Server);
            return true;
        }

        static void doUpdate(bool andAyria = false)
        {
            UpdateDB(Localserver);

            const auto Message = JSON::Dump(toJSON(Localserver));
            Ayria.publishMessage("Steam::Serverupdate", Message.c_str(), (uint32_t)Message.size());

            if (andAyria) [[unlikely]]
            {
                const auto Request = JSON::Object_t({
                    { "Hostaddress", va("%s:%u", Localserver.PublicIP.toString(), Localserver.Gameport) },
                    { "Servername", Localserver.Servername },
                    { "Hostport", Localserver.Gameport },
                    { "GameID", Global.AppID },
                    { "Provider", "Steam"s }
                });
                Ayria.doRequest("Matchmaking::updateServer", Request);
            }
        }
        void Initialize()
        {
            static bool Initialized{};
            if (Initialized) return;
            Initialized = true;

            Ayria.subscribeMessage("Steam::Serverupdate", onUpdate);
        }

        bool Start(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, uint32_t unServerFlags, const char *pchGameDir, const char *pchVersion)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.Spectatorport = usSpectatorPort;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Authport = usSteamPort;
            Localserver.Queryport = usQueryPort;
            Localserver.Gameport = unGamePort;
            Localserver.Gamedir = pchGameDir;
            Localserver.Version = pchVersion;

            Localserver.ServerID = Global.XUID.UserID;
            Localserver.AppID = Global.AppID;
            Localserver.isActive = true;
            doUpdate();

            const auto Request = JSON::Object_t({
                { "Hostaddress", va("%s:%u", Localserver.PublicIP.toString(), Localserver.Gameport) },
                { "Servername", Localserver.Servername },
                { "Hostport", Localserver.Gameport },
                { "GameID", Global.AppID },
                { "Provider", "Steam"s }
            });
            const auto Result = Ayria.doRequest("Matchmaking::startServer", Request);
            return !Result.contains("Error");
        }
        bool Start(uint32_t unGameIP, uint16_t usSteamPort, uint16_t unGamePort, uint16_t usQueryPort, uint32_t unServerFlags, AppID_t nAppID, const char *pchVersion)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Queryport = usQueryPort;
            Localserver.Authport = usSteamPort;
            Localserver.Gameport = unGamePort;
            Localserver.Version = pchVersion;

            Localserver.ServerID = Global.XUID.UserID;
            Localserver.AppID = Global.AppID;
            Localserver.isActive = true;
            doUpdate();

            const auto Request = JSON::Object_t({
                { "Hostaddress", va("%s:%u", Localserver.PublicIP.toString(), Localserver.Gameport) },
                { "Servername", Localserver.Servername },
                { "Hostport", Localserver.Gameport },
                { "GameID", Global.AppID },
                { "Provider", "Steam"s }
            });

            const auto Result = Ayria.doRequest("Matchmaking::startServer", Request);
            return !Result.contains("Error");
        }
        bool Terminate()
        {
            Ayria.doRequest("Matchmaking::stopServer", {});
            Localserver.isActive = false;
            doUpdate();
            return true;
        }
        bool isActive()
        {
            return Localserver.isActive;
        }
    }

    struct SteamGameserver
    {
        enum EBeginAuthSessionResult
        {
            k_EBeginAuthSessionResultOK = 0,						// Ticket is valid for this game and this steamID.
            k_EBeginAuthSessionResultInvalidTicket = 1,				// Ticket is not valid.
            k_EBeginAuthSessionResultDuplicateRequest = 2,			// A ticket has already been submitted for this steamID
            k_EBeginAuthSessionResultInvalidVersion = 3,			// Ticket is from an incompatible interface version
            k_EBeginAuthSessionResultGameMismatch = 4,				// Ticket is not for this game
            k_EBeginAuthSessionResultExpiredTicket = 5,				// Ticket has expired
        };
        enum EUserHasLicenseForAppResult
        {
            k_EUserHasLicenseResultHasLicense = 0,					// User has a license for specified app
            k_EUserHasLicenseResultDoesNotHaveLicense = 1,			// User does not have a license for the specified app
            k_EUserHasLicenseResultNoAuth = 2,						// User has not been authenticated
        };

        EBeginAuthSessionResult BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, SteamID_t steamID)
        {
            return k_EBeginAuthSessionResultOK;
        }
        EUserHasLicenseForAppResult UserHasLicenseForApp(SteamID_t steamID, AppID_t appID)
        {
            return k_EUserHasLicenseResultHasLicense;
        }
        HAuthTicket GetAuthSessionTicket(void *pTicket, int cbMaxTicket, uint32_t *pcbTicket)
        {
            return GetTickCount();
        }
        SteamAPICall_t AssociateWithClan(SteamID_t steamIDClan)
        {
            const auto Result = new Tasks::AssociateWithClanResult_t();
            Result->m_eResult = EResult::k_EResultOK;

            const auto TaskID = Tasks::Createrequest();
            Tasks::Completerequest(TaskID, Tasks::ECallbackType::AssociateWithClanResult_t, Result);
            return TaskID;
        }
        SteamAPICall_t ComputeNewPlayerCompatibility(SteamID_t steamIDNewPlayer)
        {
            const auto Result = new Tasks::ComputeNewPlayerCompatibilityResult_t();
            Result->m_SteamIDCandidate = steamIDNewPlayer;
            Result->m_eResult = EResult::k_EResultOK;

            const auto TaskID = Tasks::Createrequest();
            Tasks::Completerequest(TaskID, Tasks::ECallbackType::ComputeNewPlayerCompatibilityResult_t, Result);
            return TaskID;
        }
        SteamAPICall_t GetServerReputation()
        {
            const auto Result = new Tasks::GSReputation_t();
            Result->m_eResult = EResult::k_EResultOK;
            Result->m_unReputationScore = 1;

            const auto TaskID = Tasks::Createrequest();
            Tasks::Completerequest(TaskID, Tasks::ECallbackType::GSReputation_t, Result);
            return TaskID;
        }
        SteamID_t CreateUnauthenticatedUserConnection()
        {
            SteamID_t Tmp{};
            Tmp.UserID = GetTickCount();
            Tmp.Universe = SteamID_t::Universe_t::Public;
            Tmp.Accounttype = SteamID_t::Accounttype_t::Anonymous;

            return Tmp;
        }
        SteamID_t GetSteamID()
        {
            return Global.XUID;
        }
        SteamIPAddress_t GetPublicIP1()
        {
            // TODO(tcn): Win adapter info
            return Localserver.PublicIP;
        }

        bool BGetUserAchievementStatus(SteamID_t steamID, const char *pchAchievementName)
        {
            try
            {
                std::string Canonicalname{};
                int32_t Clientprogress{};
                int32_t Maxprogress{};

                // Resolve the name (in-case the client asks by API_Name).
                Database()
                    << "SELECT Name, Maxprogress FROM Achievement WHERE (AppID = ? AND (API_Name = ? OR Name = ?));"
                    << Global.AppID << std::string(pchAchievementName) << std::string(pchAchievementName)
                    >> std::tie(Canonicalname, Maxprogress);

                // Get the clients progress.
                Database()
                    << "SELECT Currentprogress FROM Achievementprogress WHERE (AppID = ? AND ClientID = ? AND Name = ?);"
                    << Global.AppID << fromSteamID(steamID) << Canonicalname
                    >> Clientprogress;

                return Clientprogress == Maxprogress;

            } catch (...) {}

            return false;
        }
        bool BLoggedOn()
        {
            return true;
        }
        bool BSecure()
        {
            return Localserver.isSecure;
        }
        bool BSetServerType0(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.Spectatorport = usSpectatorPort;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Queryport = usQueryPort;
            Localserver.Gameport = unGamePort;
            Localserver.Gamedir = pchGameDir;
            Localserver.Version = pchVersion;
            Gameserver::doUpdate(true);
            return true;
        }
        bool BSetServerType1(uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t unSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.Spectatorport = unSpectatorPort;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Queryport = usQueryPort;
            Localserver.Gameport = unGamePort;
            Localserver.Gamedir = pchGameDir;
            Localserver.Version = pchVersion;
            Gameserver::doUpdate(true);
            return true;
        }
        bool BUpdateUserData(SteamID_t steamIDUser, const char *pchPlayerName, uint32_t uScore)
        {
            return SetUserData(steamIDUser, pchPlayerName, uScore);
        }
        bool CreateUnauthenticatedUser(SteamID_t *pSteamID)
        {
            *pSteamID = CreateUnauthenticatedUserConnection();
            return true;
        }
        bool GSSetStatus(int32_t nAppIdServed, uint32_t unServerFlags, int cPlayers, int cPlayersMax, int cBotPlayers, int unGamePort, const char *pchServerName, const char *pchGameDir, const char *pchMapName, const char *pchVersion)
        {
            Localserver.Serverflags = unServerFlags;
            Localserver.Servername = pchServerName;
            Localserver.Playermax = cPlayersMax;
            Localserver.Botcount = cBotPlayers;
            Localserver.Playercount = cPlayers;
            Localserver.Gameport = unGamePort;
            Localserver.Version = pchVersion;
            Localserver.Gamedir = pchGameDir;
            Localserver.Mapname = pchMapName;
            Gameserver::doUpdate(true);
            return true;
        }
        bool GetSteam2GetEncryptionKeyToSendToNewClient(void *pvEncryptionKey, uint32_t *pcbEncryptionKey, uint32_t cbMaxEncryptionKey)
        {
            *(uint32_t *)pvEncryptionKey = Hash::WW32("Steam2");
            *pcbEncryptionKey = 4;
            return true;
        }
        bool GetUserAchievementStatus(SteamID_t steamID, const char *pchAchievementName)
        {
            return BGetUserAchievementStatus(steamID, pchAchievementName);
        }
        bool HandleIncomingPacket(const void *pData, int cbData, uint32_t srcIP, uint16_t srcPort) { return true; }
        bool InitGameServer(uint32_t unGameIP, uint16_t unGamePort, uint16_t usQueryPort, uint32_t unServerFlags, AppID_t nAppID, const char *pchVersion)
        {
            return Gameserver::Start(unGameIP, 0, unGamePort, usQueryPort, unServerFlags, nAppID, pchVersion);
        }
        bool RemoveUserConnect(uint32_t unUserID) { return true; }
        bool RequestUserGroupStatus(SteamID_t steamIDUser, SteamID_t steamIDGroup) { return true; }
        bool SendSteam2UserConnect(uint32_t unUserID, const void *pvRawKey, uint32_t unKeyLen, uint32_t unIPPublic, uint16_t usPort, const void *pvCookie, uint32_t cubCookie)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SendSteam3UserConnect(SteamID_t steamID, uint32_t unIPPublic, const void *pvCookie, uint32_t cubCookie)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SendUserConnect(uint32_t, uint32_t, uint16_t, const void *, uint32_t)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SendUserConnectAndAuthenticate0(SteamID_t steamIDUser, uint32_t, void *, uint32_t)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SendUserConnectAndAuthenticate1(uint32_t unIPClient, const void *pvAuthBlob, uint32_t cubAuthBlobSize, SteamID_t *pSteamIDUser)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SendUserDisconnect0(SteamID_t steamID, uint32_t unUserID)
        {
            SendUserDisconnect1(steamID);
            return true;
        }
        bool SendUserStatusResponse(SteamID_t steamID, int nSecondsConnected, int nSecondsSinceLast)
        {
            // TODO(tcn): Implement this.
            assert(false);
            return false;
        }
        bool SetServerType0(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint32_t unGamePort, const char *pchGameDir, const char *pchVersion)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Gameport = unGamePort;
            Localserver.Gamedir = pchGameDir;
            Localserver.Version = pchVersion;
            Gameserver::doUpdate(true);
            return true;
        }
        bool SetServerType1(int32_t nGameAppId, uint32_t unServerFlags, uint32_t unGameIP, uint16_t unGamePort, uint16_t usSpectatorPort, uint16_t usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode)
        {
            Localserver.PublicIP.m_eType = k_ESteamIPTypeIPv4;
            Localserver.Spectatorport = usSpectatorPort;
            Localserver.PublicIP.m_unIPv4 = unGameIP;
            Localserver.Serverflags = unServerFlags;
            Localserver.Queryport = usQueryPort;
            Localserver.Gameport = unGamePort;
            Localserver.Gamedir = pchGameDir;
            Localserver.Version = pchVersion;
            Gameserver::doUpdate(true);
            return true;
        }
        bool SetUserData(SteamID_t steamIDUser, const char *pchPlayerName, uint32_t uScore)
        {
            Localserver.Userdata[steamIDUser.FullID] = { pchPlayerName, uScore };
            Gameserver::doUpdate();
            return true;
        }
        bool UpdateStatus0(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName)
        {
            Localserver.Spectatorname = pSpectatorServerName;
            Localserver.Playermax = cPlayersMax;
            Localserver.Playercount = cPlayers;
            Localserver.Botcount = cBotPlayers;
            Localserver.Mapname = pchMapName;
            Gameserver::doUpdate();
            return true;
        }
        bool UpdateStatus1(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pchMapName)
        {
            Localserver.Playermax = cPlayersMax;
            Localserver.Playercount = cPlayers;
            Localserver.Botcount = cBotPlayers;
            Localserver.Mapname = pchMapName;
            Gameserver::doUpdate();
            return true;
        }
        bool WasRestartRequested()
        {
            return false;
        }

        int GetNextOutgoingPacket(void *pOut, int cbMaxOut, uint32_t *pNetAdr, uint16_t *pPort)
        {
            return 0;
        }
        uint32_t GetPublicIP0()
        {
            // TODO(tcn): Win adapter info
            return Localserver.PublicIP.m_unIPv4;
        }

        void CancelAuthTicket(HAuthTicket hAuthTicket) {}
        void ClearAllKeyValues()
        {
            Localserver.Keyvalues.clear();
            Gameserver::doUpdate();
        }
        void EnableHeartbeats(bool bActive) {}
        void EndAuthSession(SteamID_t steamID) {}
        void ForceHeartbeat() {}
        void GetGameplayStats() {}
        void LogOff() { Gameserver::Terminate(); }
        void LogOn0() {}
        void LogOn1(const char *pszAccountName, const char *pszPassword) {}
        void LogOn2(const char *pszToken) {}
        void LogOnAnonymous() {}
        void SendUserDisconnect1(SteamID_t steamIDUser)
        {
            // TODO(tcn): Implement this.
            assert(false);
        }
        void SetBotPlayerCount(int cBotplayers)
        {
            Localserver.Botcount = cBotplayers;
            Gameserver::doUpdate();
        }
        void SetDedicatedServer(bool bDedicatedServer)
        {
            Localserver.isDedicated = bDedicatedServer;
            Gameserver::doUpdate();
        }
        void SetGameData(const char *pchGameData)
        {
            Localserver.Gamedata = pchGameData;
            Gameserver::doUpdate();
        }
        void SetGameDescription(const char *pchGameDescription)
        {
            Localserver.Gamedescription = pchGameDescription;
            Gameserver::doUpdate();
        }
        void SetGameTags(const char *pchGameTags)
        {
            Localserver.Gametags = pchGameTags;
            Gameserver::doUpdate();
        }
        void SetGameType(const char *pchGameType)
        {
            Localserver.Gametype = pchGameType;
            Gameserver::doUpdate();
        }
        void SetHeartbeatInterval(int iHeartbeatInterval) {}
        void SetKeyValue(const char *pKey, const char *pValue)
        {
            Localserver.Keyvalues[pKey] = pValue;
            Gameserver::doUpdate();
        }
        void SetMapName(const char *pszMapName)
        {
            Localserver.Mapname = pszMapName;
            Gameserver::doUpdate();
        }
        void SetMaxPlayerCount(int cPlayersMax)
        {
            Localserver.Playermax = cPlayersMax;
            Gameserver::doUpdate();
        }
        void SetModDir(const char *pchModDir)
        {
            Localserver.Moddir = pchModDir;
            Gameserver::doUpdate();
        }
        void SetPasswordProtected(bool bPasswordProtected)
        {
            Localserver.isPasswordprotected = bPasswordProtected;
            Gameserver::doUpdate();
        }
        void SetProduct(const char *pchProductName)
        {
            Localserver.Product = pchProductName;
            Gameserver::doUpdate();
        }
        void SetRegion(const char *pchRegionName)
        {
            Localserver.Region = pchRegionName;
            Gameserver::doUpdate();
        }
        void SetServerName(const char *pszServerName)
        {
            Localserver.Servername = pszServerName;
            Gameserver::doUpdate(true);
        }
        void SetSpawnCount(uint32_t ucSpawn)
        {
            Localserver.Spawncount = ucSpawn;
            Gameserver::doUpdate();
        }
        void SetSpectatorPort(uint16_t unSpectatorPort)
        {
            Localserver.Spectatorport = unSpectatorPort;
            Gameserver::doUpdate();
        }
        void SetSpectatorServerName(const char *pszSpectatorServerName)
        {
            Localserver.Spectatorname = pszSpectatorServerName;
            Gameserver::doUpdate();
        }
        void UpdateServerStatus(int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName)
        {
            UpdateStatus0(cPlayers, cPlayersMax, cBotPlayers, pchServerName, pSpectatorServerName, pchMapName);
        }
        void UpdateSpectatorPort(uint16_t unSpectatorPort)
        {
            SetSpectatorPort(unSpectatorPort);
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;
    struct SteamGameserver001 : Interface_t<>
    {
        SteamGameserver001()
        {
            /* No steamworks SDK info */
        };
    };
    struct SteamGameserver002 : Interface_t<21>
    {
        SteamGameserver002()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, SetSpawnCount);
            Createmethod(4, SteamGameserver, GetSteam2GetEncryptionKeyToSendToNewClient);
            Createmethod(5, SteamGameserver, SendSteam2UserConnect);
            Createmethod(6, SteamGameserver, SendSteam3UserConnect);
            Createmethod(7, SteamGameserver, RemoveUserConnect);
            Createmethod(8, SteamGameserver, SendUserDisconnect0);
            Createmethod(9, SteamGameserver, SendUserStatusResponse);
            Createmethod(10, SteamGameserver, GSSetStatus);
            Createmethod(11, SteamGameserver, UpdateStatus0);
            Createmethod(12, SteamGameserver, BSecure);
            Createmethod(13, SteamGameserver, GetSteamID);
            Createmethod(14, SteamGameserver, SetServerType0);
            Createmethod(15, SteamGameserver, SetServerType1);
            Createmethod(16, SteamGameserver, UpdateStatus1);
            Createmethod(17, SteamGameserver, CreateUnauthenticatedUser);
            Createmethod(18, SteamGameserver, SetUserData);
            Createmethod(19, SteamGameserver, UpdateSpectatorPort);
            Createmethod(20, SteamGameserver, SetGameType);
        };
    };
    struct SteamGameserver003 : Interface_t<17>
    {
        SteamGameserver003()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, GetSteam2GetEncryptionKeyToSendToNewClient);
            Createmethod(6, SteamGameserver, SendUserConnect);
            Createmethod(7, SteamGameserver, RemoveUserConnect);
            Createmethod(8, SteamGameserver, SendUserDisconnect0);
            Createmethod(9, SteamGameserver, SetSpawnCount);
            Createmethod(10, SteamGameserver, SetServerType1);
            Createmethod(11, SteamGameserver, UpdateStatus1);
            Createmethod(12, SteamGameserver, CreateUnauthenticatedUser);
            Createmethod(13, SteamGameserver, SetUserData);
            Createmethod(14, SteamGameserver, UpdateSpectatorPort);
            Createmethod(15, SteamGameserver, SetGameType);
            Createmethod(16, SteamGameserver, GetUserAchievementStatus);
        };
    };
    struct SteamGameserver004 : Interface_t<14>
    {
        SteamGameserver004()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate0);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType0);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
        };
    };
    struct SteamGameserver005 : Interface_t<15>
    {
        SteamGameserver005()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
        };
    };
    struct SteamGameserver006 : Interface_t<15>
    {
        SteamGameserver006()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
        };
    };
    struct SteamGameserver007 : Interface_t<16>
    {
        SteamGameserver007()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
        };
    };
    struct SteamGameserver008 : Interface_t<17>
    {
        SteamGameserver008()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP0);
        };
    };
    struct SteamGameserver009 : Interface_t<19>
    {
        SteamGameserver009()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameType);
            Createmethod(13, SteamGameserver, BGetUserAchievementStatus);
            Createmethod(14, SteamGameserver, GetGameplayStats);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP0);
            Createmethod(17, SteamGameserver, SetGameData);
            Createmethod(18, SteamGameserver, UserHasLicenseForApp);
        };
    };
    struct SteamGameserver010 : Interface_t<23>
    {
        SteamGameserver010()
        {
            Createmethod(0, SteamGameserver, LogOn0);
            Createmethod(1, SteamGameserver, LogOff);
            Createmethod(2, SteamGameserver, BLoggedOn);
            Createmethod(3, SteamGameserver, BSecure);
            Createmethod(4, SteamGameserver, GetSteamID);
            Createmethod(5, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(6, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(7, SteamGameserver, SendUserDisconnect1);
            Createmethod(8, SteamGameserver, BUpdateUserData);
            Createmethod(9, SteamGameserver, BSetServerType1);
            Createmethod(10, SteamGameserver, UpdateServerStatus);
            Createmethod(11, SteamGameserver, UpdateSpectatorPort);
            Createmethod(12, SteamGameserver, SetGameTags);
            Createmethod(13, SteamGameserver, GetGameplayStats);
            Createmethod(14, SteamGameserver, GetServerReputation);
            Createmethod(15, SteamGameserver, RequestUserGroupStatus);
            Createmethod(16, SteamGameserver, GetPublicIP0);
            Createmethod(17, SteamGameserver, SetGameData);
            Createmethod(18, SteamGameserver, UserHasLicenseForApp);
            Createmethod(19, SteamGameserver, GetAuthSessionTicket);
            Createmethod(20, SteamGameserver, BeginAuthSession);
            Createmethod(21, SteamGameserver, EndAuthSession);
            Createmethod(22, SteamGameserver, CancelAuthTicket);
        };
    };
    struct SteamGameserver011 : Interface_t<44>
    {
        SteamGameserver011()
        {
            Createmethod(0, SteamGameserver, InitGameServer);
            Createmethod(1, SteamGameserver, SetProduct);
            Createmethod(2, SteamGameserver, SetGameDescription);
            Createmethod(3, SteamGameserver, SetModDir);
            Createmethod(4, SteamGameserver, SetDedicatedServer);
            Createmethod(5, SteamGameserver, LogOn1);
            Createmethod(6, SteamGameserver, LogOnAnonymous);
            Createmethod(7, SteamGameserver, LogOff);
            Createmethod(8, SteamGameserver, BLoggedOn);
            Createmethod(9, SteamGameserver, BSecure);
            Createmethod(10, SteamGameserver, GetSteamID);
            Createmethod(11, SteamGameserver, WasRestartRequested);
            Createmethod(12, SteamGameserver, SetMaxPlayerCount);
            Createmethod(13, SteamGameserver, SetBotPlayerCount);
            Createmethod(14, SteamGameserver, SetServerName);
            Createmethod(15, SteamGameserver, SetMapName);
            Createmethod(16, SteamGameserver, SetPasswordProtected);
            Createmethod(17, SteamGameserver, SetSpectatorPort);
            Createmethod(18, SteamGameserver, SetSpectatorServerName);
            Createmethod(19, SteamGameserver, ClearAllKeyValues);
            Createmethod(20, SteamGameserver, SetKeyValue);
            Createmethod(21, SteamGameserver, SetGameTags);
            Createmethod(22, SteamGameserver, SetGameData);
            Createmethod(23, SteamGameserver, SetRegion);
            Createmethod(24, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(25, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(26, SteamGameserver, SendUserDisconnect1);
            Createmethod(27, SteamGameserver, BUpdateUserData);
            Createmethod(28, SteamGameserver, GetAuthSessionTicket);
            Createmethod(29, SteamGameserver, BeginAuthSession);
            Createmethod(30, SteamGameserver, EndAuthSession);
            Createmethod(31, SteamGameserver, CancelAuthTicket);
            Createmethod(32, SteamGameserver, UserHasLicenseForApp);
            Createmethod(33, SteamGameserver, RequestUserGroupStatus);
            Createmethod(34, SteamGameserver, GetGameplayStats);
            Createmethod(35, SteamGameserver, GetServerReputation);
            Createmethod(36, SteamGameserver, GetPublicIP0);
            Createmethod(37, SteamGameserver, HandleIncomingPacket);
            Createmethod(38, SteamGameserver, GetNextOutgoingPacket);
            Createmethod(39, SteamGameserver, EnableHeartbeats);
            Createmethod(40, SteamGameserver, SetHeartbeatInterval);
            Createmethod(41, SteamGameserver, ForceHeartbeat);
            Createmethod(42, SteamGameserver, AssociateWithClan);
            Createmethod(43, SteamGameserver, ComputeNewPlayerCompatibility);
        };
    };
    struct SteamGameserver012 : Interface_t<44>
    {
        SteamGameserver012()
        {
            Createmethod(0, SteamGameserver, InitGameServer);
            Createmethod(1, SteamGameserver, SetProduct);
            Createmethod(2, SteamGameserver, SetGameDescription);
            Createmethod(3, SteamGameserver, SetModDir);
            Createmethod(4, SteamGameserver, SetDedicatedServer);
            Createmethod(5, SteamGameserver, LogOn2);
            Createmethod(6, SteamGameserver, LogOnAnonymous);
            Createmethod(7, SteamGameserver, LogOff);
            Createmethod(8, SteamGameserver, BLoggedOn);
            Createmethod(9, SteamGameserver, BSecure);
            Createmethod(10, SteamGameserver, GetSteamID);
            Createmethod(11, SteamGameserver, WasRestartRequested);
            Createmethod(12, SteamGameserver, SetMaxPlayerCount);
            Createmethod(13, SteamGameserver, SetBotPlayerCount);
            Createmethod(14, SteamGameserver, SetServerName);
            Createmethod(15, SteamGameserver, SetMapName);
            Createmethod(16, SteamGameserver, SetPasswordProtected);
            Createmethod(17, SteamGameserver, SetSpectatorPort);
            Createmethod(18, SteamGameserver, SetSpectatorServerName);
            Createmethod(19, SteamGameserver, ClearAllKeyValues);
            Createmethod(20, SteamGameserver, SetKeyValue);
            Createmethod(21, SteamGameserver, SetGameTags);
            Createmethod(22, SteamGameserver, SetGameData);
            Createmethod(23, SteamGameserver, SetRegion);
            Createmethod(24, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(25, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(26, SteamGameserver, SendUserDisconnect1);
            Createmethod(27, SteamGameserver, BUpdateUserData);
            Createmethod(28, SteamGameserver, GetAuthSessionTicket);
            Createmethod(29, SteamGameserver, BeginAuthSession);
            Createmethod(30, SteamGameserver, EndAuthSession);
            Createmethod(31, SteamGameserver, CancelAuthTicket);
            Createmethod(32, SteamGameserver, UserHasLicenseForApp);
            Createmethod(33, SteamGameserver, RequestUserGroupStatus);
            Createmethod(34, SteamGameserver, GetGameplayStats);
            Createmethod(35, SteamGameserver, GetServerReputation);
            Createmethod(36, SteamGameserver, GetPublicIP0);
            Createmethod(37, SteamGameserver, HandleIncomingPacket);
            Createmethod(38, SteamGameserver, GetNextOutgoingPacket);
            Createmethod(39, SteamGameserver, EnableHeartbeats);
            Createmethod(40, SteamGameserver, SetHeartbeatInterval);
            Createmethod(41, SteamGameserver, ForceHeartbeat);
            Createmethod(42, SteamGameserver, AssociateWithClan);
            Createmethod(43, SteamGameserver, ComputeNewPlayerCompatibility);
        };
    };
    struct SteamGameserver013 : Interface_t<44>
    {
        SteamGameserver013()
        {
            Createmethod(0, SteamGameserver, InitGameServer);
            Createmethod(1, SteamGameserver, SetProduct);
            Createmethod(2, SteamGameserver, SetGameDescription);
            Createmethod(3, SteamGameserver, SetModDir);
            Createmethod(4, SteamGameserver, SetDedicatedServer);
            Createmethod(5, SteamGameserver, LogOn2);
            Createmethod(6, SteamGameserver, LogOnAnonymous);
            Createmethod(7, SteamGameserver, LogOff);
            Createmethod(8, SteamGameserver, BLoggedOn);
            Createmethod(9, SteamGameserver, BSecure);
            Createmethod(10, SteamGameserver, GetSteamID);
            Createmethod(11, SteamGameserver, WasRestartRequested);
            Createmethod(12, SteamGameserver, SetMaxPlayerCount);
            Createmethod(13, SteamGameserver, SetBotPlayerCount);
            Createmethod(14, SteamGameserver, SetServerName);
            Createmethod(15, SteamGameserver, SetMapName);
            Createmethod(16, SteamGameserver, SetPasswordProtected);
            Createmethod(17, SteamGameserver, SetSpectatorPort);
            Createmethod(18, SteamGameserver, SetSpectatorServerName);
            Createmethod(19, SteamGameserver, ClearAllKeyValues);
            Createmethod(20, SteamGameserver, SetKeyValue);
            Createmethod(21, SteamGameserver, SetGameTags);
            Createmethod(22, SteamGameserver, SetGameData);
            Createmethod(23, SteamGameserver, SetRegion);
            Createmethod(24, SteamGameserver, SendUserConnectAndAuthenticate1);
            Createmethod(25, SteamGameserver, CreateUnauthenticatedUserConnection);
            Createmethod(26, SteamGameserver, SendUserDisconnect1);
            Createmethod(27, SteamGameserver, BUpdateUserData);
            Createmethod(28, SteamGameserver, GetAuthSessionTicket);
            Createmethod(29, SteamGameserver, BeginAuthSession);
            Createmethod(30, SteamGameserver, EndAuthSession);
            Createmethod(31, SteamGameserver, CancelAuthTicket);
            Createmethod(32, SteamGameserver, UserHasLicenseForApp);
            Createmethod(33, SteamGameserver, RequestUserGroupStatus);
            Createmethod(34, SteamGameserver, GetGameplayStats);
            Createmethod(35, SteamGameserver, GetServerReputation);
            Createmethod(36, SteamGameserver, GetPublicIP1);
            Createmethod(37, SteamGameserver, HandleIncomingPacket);
            Createmethod(38, SteamGameserver, GetNextOutgoingPacket);
            Createmethod(39, SteamGameserver, EnableHeartbeats);
            Createmethod(40, SteamGameserver, SetHeartbeatInterval);
            Createmethod(41, SteamGameserver, ForceHeartbeat);
            Createmethod(42, SteamGameserver, AssociateWithClan);
            Createmethod(43, SteamGameserver, ComputeNewPlayerCompatibility);
        };
    };

    struct Steamgameserverloader
    {
        Steamgameserverloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver001", SteamGameserver001);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver002", SteamGameserver002);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver003", SteamGameserver003);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver004", SteamGameserver004);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver005", SteamGameserver005);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver006", SteamGameserver006);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver007", SteamGameserver007);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver008", SteamGameserver008);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver009", SteamGameserver009);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver010", SteamGameserver010);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver011", SteamGameserver011);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver012", SteamGameserver012);
            Register(Interfacetype_t::GAMESERVER, "SteamGameserver013", SteamGameserver013);
        }
    };
    static Steamgameserverloader Interfaceloader{};
}

