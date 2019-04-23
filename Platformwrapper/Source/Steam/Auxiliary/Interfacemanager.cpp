/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    std::vector<std::pair<Interfacetype_t, Interface_t *>> *Interfacestore;
    robin_hood::unordered_flat_map<Interfacetype_t, Interface_t *> Currentinterfaces;
    robin_hood::unordered_flat_map<std::string_view, Interface_t *> *Interfacenames;

    // A nice little dummy interface for debugging.
    struct Dummyinterface : Interface_t
    {
        template<int N> void Dummyfunc() { Errorprint(__FUNCSIG__); assert(false); };
        Dummyinterface()
        {
            std::any K;

            #define Createmethod(N) K = &Dummyinterface::Dummyfunc<N>; VTABLE[N] = *(void **)&K;

            // 70 methods here.
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
        }
        #undef Createmethod
    };

    // Return a specific version of the interface by name or the latest by their category / type.
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t *Interface)
    {
        if (!Interfacestore) Interfacestore = new std::remove_pointer_t<decltype(Interfacestore)>;
        if (!Interfacenames) Interfacenames = new std::remove_pointer_t<decltype(Interfacenames)>;

        Interfacestore->push_back(std::make_pair(Type, Interface));
        Interfacenames->emplace(Name, Interface);
    }
    Interface_t **Fetchinterface(std::string_view Name)
    {
        // See if we even have the interface implemented.
        const auto Result = Interfacenames->find(Name);
        if (Result == Interfacenames->end())
        {
            // Return the dummy interface for debugging.
            Errorprint(va("Interface missing for interface-name %*s", Name.size(), Name.data()));
            static auto Debug = new Dummyinterface();
            return (Interface_t **)&Debug;
        }

        // Find the type from the store.
        for (const auto &[Type, Ptr] : *Interfacestore)
        {
            if (Ptr == Result->second)
            {
                Currentinterfaces.emplace(Type, Ptr);
                return &Currentinterfaces[Type];
            }
        }

        // This should never be hit.
        assert(false); return nullptr;
    }
    Interface_t **Fetchinterface(Interfacetype_t Type)
    {
        // See if we have any interface selected for this type.
        if (const auto Result = Currentinterfaces.find(Type); Result != Currentinterfaces.end())
        {
            //Debugprint(va("Fetching interface %i", Type));
            return &Result->second;
        }

        // Return the dummy interface for debugging.
        Errorprint(va("Interface missing for interface-type %i", Type));
        static auto Debug = new Dummyinterface();
        return (Interface_t **)&Debug;
    }

    // Poke at a module until it gives up its secrets.
    extern const std::vector<std::pair<std::string, std::string>> Scanstrings;
    bool Scanforinterfaces(std::string_view Filename)
    {
        if (auto Filehandle = std::fopen(Filename.data(), "rb"))
        {
            std::vector<std::string> Foundnames;

            // Load the entire file into memory.
            std::fseek(Filehandle, SEEK_SET, SEEK_END);
            const auto Size = std::ftell(Filehandle);
            std::rewind(Filehandle);

            auto Filebuffer = std::make_unique<char[]>(Size);
            std::fread(Filebuffer.get(), Size, 1, Filehandle);
            std::fclose(Filehandle);

            // TODO(tcn): SSE4 has nice string-matching stuff.
            // Scan for the names in the file, slow and should be optimized later.
            for (long i = 0; i < Size; ++i)
            {
                // Naive skip forward.
                if (Filebuffer[i] != 'S') continue;

                // Match against the scan-strings.
                for (const auto &[Scanstring, Name] : Scanstrings)
                {
                    if (std::strstr(&Filebuffer[i], Scanstring.c_str()))
                    {
                        // Load the interface to mark it as active.
                        Debugprint(va("Loading interface %s", Name.c_str()));
                        Foundnames.push_back(Scanstring);
                        Fetchinterface(Name);
                        break;
                    }
                }

            }

            // Bad result-count. =(
            if (Foundnames.size() == 0) return false;

            // Save the interfaces to a cache of sorts.
            // TODO(tcn): When we have the virtual FS, store a .txt

            return true;
        }

        return false;
    }
    const std::vector<std::pair<std::string, std::string>> Scanstrings
    {
        { "STEAMAPPS_INTERFACE_VERSION001", "SteamApps001" },
        { "STEAMAPPS_INTERFACE_VERSION002", "SteamApps002" },
        { "STEAMAPPS_INTERFACE_VERSION003", "SteamApps003" },
        { "STEAMAPPS_INTERFACE_VERSION004", "SteamApps004" },
        { "STEAMAPPS_INTERFACE_VERSION005", "SteamApps005" },
        { "STEAMAPPS_INTERFACE_VERSION006", "SteamApps006" },
        { "STEAMAPPS_INTERFACE_VERSION007", "SteamApps007" },

        { "SteamFriends001", "SteamFriends001" },
        { "SteamFriends002", "SteamFriends002" },
        { "SteamFriends003", "SteamFriends003" },
        { "SteamFriends004", "SteamFriends004" },
        { "SteamFriends005", "SteamFriends005" },
        { "SteamFriends006", "SteamFriends006" },
        { "SteamFriends007", "SteamFriends007" },
        { "SteamFriends008", "SteamFriends008" },
        { "SteamFriends009", "SteamFriends009" },
        { "SteamFriends010", "SteamFriends010" },
        { "SteamFriends011", "SteamFriends011" },
        { "SteamFriends012", "SteamFriends012" },
        { "SteamFriends013", "SteamFriends013" },
        { "SteamFriends014", "SteamFriends014" },
        { "SteamFriends015", "SteamFriends015" },

        { "SteamGameServer001", "SteamGameserver001" },
        { "SteamGameServer002", "SteamGameserver002" },
        { "SteamGameServer003", "SteamGameserver003" },
        { "SteamGameServer004", "SteamGameserver004" },
        { "SteamGameServer005", "SteamGameserver005" },
        { "SteamGameServer006", "SteamGameserver006" },
        { "SteamGameServer007", "SteamGameserver007" },
        { "SteamGameServer008", "SteamGameserver008" },
        { "SteamGameServer009", "SteamGameserver009" },
        { "SteamGameServer010", "SteamGameserver010" },
        { "SteamGameServer011", "SteamGameserver011" },
        { "SteamGameServer012", "SteamGameserver012" },

        { "SteamMatchMaking001", "SteamMatchmaking001" },
        { "SteamMatchMaking002", "SteamMatchmaking002" },
        { "SteamMatchMaking003", "SteamMatchmaking003" },
        { "SteamMatchMaking004", "SteamMatchmaking004" },
        { "SteamMatchMaking005", "SteamMatchmaking005" },
        { "SteamMatchMaking006", "SteamMatchmaking006" },
        { "SteamMatchMaking007", "SteamMatchmaking007" },
        { "SteamMatchMaking008", "SteamMatchmaking008" },
        { "SteamMatchMaking009", "SteamMatchmaking009" },

        { "SteamNetworking001", "SteamNetworking001" },
        { "SteamNetworking002", "SteamNetworking002" },
        { "SteamNetworking003", "SteamNetworking003" },
        { "SteamNetworking004", "SteamNetworking004" },
        { "SteamNetworking005", "SteamNetworking005" },

        { "STEAMREMOTESTORAGE_INTERFACE_VERSION001", "SteamRemotestorage001" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION002", "SteamRemotestorage002" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION003", "SteamRemotestorage003" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION004", "SteamRemotestorage004" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION005", "SteamRemotestorage005" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION006", "SteamRemotestorage006" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION007", "SteamRemotestorage007" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION008", "SteamRemotestorage008" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION009", "SteamRemotestorage009" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION010", "SteamRemotestorage010" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION011", "SteamRemotestorage011" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION012", "SteamRemotestorage012" },

        { "STEAMSCREENSHOTS_INTERFACE_VERSION001", "SteamScreenshots001" },
        { "STEAMSCREENSHOTS_INTERFACE_VERSION002", "SteamScreenshots002" },

        { "SteamUser001", "SteamUser001" },
        { "SteamUser002", "SteamUser002" },
        { "SteamUser003", "SteamUser003" },
        { "SteamUser004", "SteamUser004" },
        { "SteamUser005", "SteamUser005" },
        { "SteamUser006", "SteamUser006" },
        { "SteamUser007", "SteamUser007" },
        { "SteamUser008", "SteamUser008" },
        { "SteamUser009", "SteamUser009" },
        { "SteamUser010", "SteamUser010" },
        { "SteamUser011", "SteamUser011" },
        { "SteamUser012", "SteamUser012" },
        { "SteamUser013", "SteamUser013" },
        { "SteamUser014", "SteamUser014" },
        { "SteamUser015", "SteamUser015" },
        { "SteamUser016", "SteamUser016" },
        { "SteamUser017", "SteamUser017" },
        { "SteamUser018", "SteamUser018" },

        { "STEAMUSERSTATS_INTERFACE_VERSION001", "SteamUserstats001" },
        { "STEAMUSERSTATS_INTERFACE_VERSION002", "SteamUserstats002" },
        { "STEAMUSERSTATS_INTERFACE_VERSION003", "SteamUserstats003" },
        { "STEAMUSERSTATS_INTERFACE_VERSION004", "SteamUserstats004" },
        { "STEAMUSERSTATS_INTERFACE_VERSION005", "SteamUserstats005" },
        { "STEAMUSERSTATS_INTERFACE_VERSION006", "SteamUserstats006" },
        { "STEAMUSERSTATS_INTERFACE_VERSION007", "SteamUserstats007" },
        { "STEAMUSERSTATS_INTERFACE_VERSION008", "SteamUserstats008" },
        { "STEAMUSERSTATS_INTERFACE_VERSION009", "SteamUserstats009" },
        { "STEAMUSERSTATS_INTERFACE_VERSION010", "SteamUserstats010" },
        { "STEAMUSERSTATS_INTERFACE_VERSION011", "SteamUserstats011" },

        { "SteamUtils001", "SteamUtilities001" },
        { "SteamUtils002", "SteamUtilities002" },
        { "SteamUtils003", "SteamUtilities003" },
        { "SteamUtils004", "SteamUtilities004" },
        { "SteamUtils005", "SteamUtilities005" },
        { "SteamUtils006", "SteamUtilities006" },
        { "SteamUtils007", "SteamUtilities007" },

        { "SteamMasterServerUpdater001", "SteamMasterserverupdater001" },

        { "SteamMatchMakingServers001", "SteamMatchmakingservers001" },
        { "SteamMatchMakingServers002", "SteamMatchmakingservers002" },

        { "SteamClient001", "SteamClient001" },
        { "SteamClient002", "SteamClient002" },
        { "SteamClient003", "SteamClient003" },
        { "SteamClient004", "SteamClient004" },
        { "SteamClient005", "SteamClient005" },
        { "SteamClient006", "SteamClient006" },
        { "SteamClient007", "SteamClient007" },
        { "SteamClient008", "SteamClient008" },
        { "SteamClient009", "SteamClient009" },
        { "SteamClient010", "SteamClient010" },
        { "SteamClient011", "SteamClient011" },
        { "SteamClient012", "SteamClient012" },
        { "SteamClient013", "SteamClient013" },
        { "SteamClient014", "SteamClient014" },
        { "SteamClient015", "SteamClient015" },
        { "SteamClient016", "SteamClient016" },
        { "SteamClient017", "SteamClient017" },

        /* TODO(tcn): Add more interfaces here later. */
    };
}
