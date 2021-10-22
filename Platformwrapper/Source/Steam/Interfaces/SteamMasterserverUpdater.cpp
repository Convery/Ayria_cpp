/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    static Hashmap<std::string, std::string> Keyvalues{};
    static Hashset<std::string> Masterservers{};
    static JSON::Object_t Localsession{};
    static bool Initialized{};
    static bool Active{};

    static void Sendupdate()
    {
        if (!Active) return;

        JSON::Object_t KVs;
        KVs.reserve(Keyvalues.size());
        for (const auto &[Key, Value] : Keyvalues)
            KVs[Key] = Value;

        auto Gamedata = Localsession;
        Gamedata["Keyvalues"] = KVs;

        const auto Request = JSON::Object_t({
            { "B64Gamedata", Base64::Encode(JSON::Dump(Gamedata))},
            { "ProviderID", Hash::WW32("Steam_Masterserver") },
            { "GameID", Global.ApplicationID }
        });

        Ayriarequest("Matchmakingsessions::Update", Request);
    }

    struct SteamMasterServerUpdater
    {
        bool AddMasterServer(const char *pServerAddress)
        {
            Masterservers.insert(pServerAddress);
            return true;
        }
        bool HandleIncomingPacket(const void *pData, int cbData, uint32_t srcIP, uint16_t srcPort)
        {
            return false;
        }
        bool RemoveMasterServer(const char *pServerAddress)
        {
            Masterservers.erase(pServerAddress);
            return true;
        }
        bool WasRestartRequested()
        {
            return false;
        }

        int GetMasterServerAddress(int iServer, char *pOut, int outBufferSize)
        {
            if (iServer >= Masterservers.size()) return 0;

            for (const auto &String : Masterservers)
            {
                if (iServer-- == 0)
                {
                    int Length = std::min(outBufferSize, (int)String.size());
                    std::strncpy(pOut, String.c_str(), Length);
                    return Length;
                }
            }

            return 0;
        }
        int GetNextOutgoingPacket( void *pOut, int cbMaxOut, uint32_t *pNetAdr, uint16_t *pPort )
        {
            return 0;
        }
        int GetNumMasterServers()
        {
            return (int)Masterservers.size();
        }

        void ClearAllKeyValues()
        {
            Keyvalues.clear();
        }
        void ForceHeartbeat()
        {
        }
        void NotifyShutdown()
        {
            Active = false;

            const auto Request = JSON::Object_t({ { "ProviderID", Hash::WW32("Steam_Masterserver") } });
            Ayriarequest("Matchmakingsessions::Terminate", Request);
        }
        void SetActive(bool bActive)
        {
            if (bActive && !Initialized)
            {
                Initialized = true;
                Ayria.Createperiodictask(5000, Sendupdate);
            }

            Active = bActive;
        }
        void SetBasicServerData(unsigned short nProtocolVersion, bool bDedicatedServer, const char *pRegionName,
            const char *pProductName, unsigned short nMaxReportedClients, bool bPasswordProtected, const char *pGameDescription)
        {
            Debugprint(va("Update Steam-gameserver:\n> Dedicated: %s\n> Passwordprotected: %s\n> Product: %s\n> Description: %s\n> Maxplayers: %u",
                bDedicatedServer ? "TRUE" : "FALSE", bPasswordProtected ? "TRUE" : "FALSE", pProductName, pGameDescription, nMaxReportedClients));

            Localsession["Gamedescription"] = std::string(pGameDescription);
            Localsession["isPasswordprotected"] = bPasswordProtected;
            Localsession["Protocolversion"] = nProtocolVersion;
            Localsession["Product"] = std::string(pProductName);
            Localsession["Region"] = std::string(pRegionName);
            Localsession["Playermax"] = nMaxReportedClients;
            Localsession["isDedicated"] = bDedicatedServer;
        }
        void SetHeartbeatInterval(int iHeartbeatInterval)
        {
        }
        void SetKeyValue(const char *pKey, const char *pValue)
        {
            Keyvalues[pKey] = pValue;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamMasterserverupdater001 : Interface_t<14>
    {
        SteamMasterserverupdater001()
        {
            Createmethod(0, SteamMasterServerUpdater, SetActive);
            Createmethod(1, SteamMasterServerUpdater, SetHeartbeatInterval);
            Createmethod(2, SteamMasterServerUpdater, HandleIncomingPacket);
            Createmethod(3, SteamMasterServerUpdater, GetNextOutgoingPacket);
            Createmethod(4, SteamMasterServerUpdater, SetBasicServerData);
            Createmethod(5, SteamMasterServerUpdater, ClearAllKeyValues);
            Createmethod(6, SteamMasterServerUpdater, SetKeyValue);
            Createmethod(7, SteamMasterServerUpdater, NotifyShutdown);
            Createmethod(8, SteamMasterServerUpdater, WasRestartRequested);
            Createmethod(9, SteamMasterServerUpdater, ForceHeartbeat);
            Createmethod(10, SteamMasterServerUpdater, AddMasterServer);
            Createmethod(11, SteamMasterServerUpdater, RemoveMasterServer);
            Createmethod(12, SteamMasterServerUpdater, GetNumMasterServers);
            Createmethod(13, SteamMasterServerUpdater, GetMasterServerAddress);
        }
    };

    struct SteamMasterserverupdaterloader
    {
        SteamMasterserverupdaterloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::MASTERSERVERUPDATER, "SteamMasterserverupdater001", SteamMasterserverupdater001);
        }
    };
    static SteamMasterserverupdaterloader Interfaceloader{};
}
