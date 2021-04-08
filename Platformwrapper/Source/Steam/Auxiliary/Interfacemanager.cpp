/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    std::deque<std::pair<Interfacetype_t, Interface_t<> *>> *Interfacestore;
    std::unordered_map<Interfacetype_t, Interface_t<> *> Currentinterfaces;
    std::unordered_map<std::string_view, Interface_t<> *> *Interfacenames;
    extern const Hashmap<std::string, std::string> Scanstrings;

    // A nice little dummy interface for debugging.
    struct Dummyinterface : Interface_t<71>
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
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t<> *Interface)
    {
        if (!Interfacestore) Interfacestore = new std::remove_pointer_t<decltype(Interfacestore)>;
        if (!Interfacenames) Interfacenames = new std::remove_pointer_t<decltype(Interfacenames)>;

        Interfacestore->push_front(std::make_pair(Type, Interface));
        Interfacenames->emplace(Name, Interface);
    }
    Interface_t<> **Fetchinterface(std::string_view Name)
    {
        // See if we got a Steam name.
        if (Scanstrings.contains(Name))
            Name = Scanstrings.at(Name).c_str();

        // See if we even have the interface implemented.
        const auto Result = Interfacenames->find(Name);
        if (Result == Interfacenames->end())
        {
            // Return the dummy interface for debugging.
            Errorprint(va("Interface missing for interface-name %*s", Name.size(), Name.data()));
            static const auto Debug = new Dummyinterface();
            return (Interface_t<> **)&Debug;
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
    Interface_t<> **Fetchinterface(Interfacetype_t Type)
    {
        // See if we have any interface selected for this type.
        if (const auto Result = Currentinterfaces.find(Type); Result != Currentinterfaces.end())
        {
            //Debugprint(va("Fetching interface %i", Type));
            return &Result->second;
        }

        // Return the latest interface as a hail-Mary.
        for (const auto &[lType, Ptr] : *Interfacestore)
        {
            if (Type == lType)
            {
                Errorprint(va("Interface missing for interface-type %i, substituting", Type));
                Currentinterfaces.emplace(lType, Ptr);
                return &Currentinterfaces[lType];
            }
        }

        // Return the dummy interface for debugging.
        Errorprint(va("Interface missing for interface-type %i", Type));
        static const auto Debug = new Dummyinterface();
        return (Interface_t<> **)&Debug;
    }
    size_t Getinterfaceversion(Interfacetype_t Type)
    {
        const auto Result = Currentinterfaces.find(Type);
        if(Result == Currentinterfaces.end())
        {
            Errorprint(va("Could not find an interface for %u", Type));
            return 0;
        }

        for(const auto &[Name, Ptr] : *Interfacenames)
        {
            if(Ptr == Result->second)
            {
                // Parse the last 3 bytes ("0XX")
                return std::atoi(Name.data() + Name.size() - 3);
            }
        }

        return 0;
    }

    // Poke at a module until it gives up its secrets.
    bool Scanforinterfaces(std::string_view Filename)
    {
        const auto Filebuffer = FS::Readfile(Filename);
        if(Filebuffer.empty()) return false;
        size_t Foundnames{};

        // Scan through the binary for interface-names.
        const Patternscan::Range_t Range = { size_t(Filebuffer.data()), size_t(Filebuffer.data()) + Filebuffer.size() };
        for (const auto Items = Patternscan::Findpatterns(Range, "53");  const auto Address : Items) // 'S' in hexadecimal.
        {
            // Match against the scan-strings.
            for(const auto &[Scanstring, Name] : Scanstrings)
            {
                if(std::strstr((char *)Address, Scanstring.c_str()))
                {
                    // Load the interface to mark it as active.
                    Fetchinterface(Name);
                    Foundnames++;
                }
            }
        }

        // Did we find any results?
        return !!Foundnames;
    }
    const Hashmap<std::string, std::string> Scanstrings
    {
        {"STEAMAPPLIST_INTERFACE_VERSION001,", "SteamApplist001"},

        { "STEAMAPPS_INTERFACE_VERSION001", "SteamApps001" },
        { "STEAMAPPS_INTERFACE_VERSION002", "SteamApps002" },
        { "STEAMAPPS_INTERFACE_VERSION003", "SteamApps003" },
        { "STEAMAPPS_INTERFACE_VERSION004", "SteamApps004" },
        { "STEAMAPPS_INTERFACE_VERSION005", "SteamApps005" },
        { "STEAMAPPS_INTERFACE_VERSION006", "SteamApps006" },
        { "STEAMAPPS_INTERFACE_VERSION007", "SteamApps007" },
        { "STEAMAPPS_INTERFACE_VERSION008", "SteamApps008" },

        { "SteamBilling001", "SteamBilling001" },
        { "SteamBilling002", "SteamBilling002" },

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
        { "SteamClient018", "SteamClient018" },
        { "SteamClient019", "SteamClient019" },
        { "SteamClient020", "SteamClient020" },

        { "SteamContentServer001", "SteamContentserver001" },
        { "SteamContentServer002", "SteamContentserver002" },

        { "SteamController001", "SteamController001" },
        { "SteamController002", "SteamController002" },
        { "SteamController003", "SteamController003" },
        { "SteamController004", "SteamController004" },
        { "SteamController005", "SteamController005" },
        { "SteamController006", "SteamController006" },
        { "SteamController007", "SteamController007" },
        { "SteamController008", "SteamController008" },

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
        { "SteamFriends016", "SteamFriends016" },
        { "SteamFriends017", "SteamFriends017" },


        { "SteamGameCoordinator001", "SteamCoordinator001" },
        { "SteamMatchGameSearch001", "SteamMatchsearch001" },

        { "SteamGameServer001", "SteamGameserver001" },
        { "SteamGameServer002", "SteamGameserver002" },
        { "SteamGameServer003", "SteamGameserver003" },
        { "SteamGameServer004", "SteamGameserver004" },
        { "SteamGameServer005", "SteamGameserver005" },
        { "SteamGameServer006", "SteamGameserver006" },
        { "SteamGameServer007", "SteamGameserver007" },
        { "SteamGameServer008", "SteamGameserver008" },
        { "SteamGameServer001", "SteamGameserver009" },
        { "SteamGameServer010", "SteamGameserver010" },
        { "SteamGameServer011", "SteamGameserver011" },
        { "SteamGameServer012", "SteamGameserver012" },
        { "SteamGameServer013", "SteamGameserver013" },

        { "SteamGameServerStats001", "SteamGameserverstats01" },

        { "STEAMHTMLSURFACE_INTERFACE_VERSION_001", "SteamHTML001" },
        { "STEAMHTMLSURFACE_INTERFACE_VERSION_002", "SteamHTML002" },
        { "STEAMHTMLSURFACE_INTERFACE_VERSION_003", "SteamHTML003" },
        { "STEAMHTMLSURFACE_INTERFACE_VERSION_004", "SteamHTML004" },
        { "STEAMHTMLSURFACE_INTERFACE_VERSION_005", "SteamHTML005" },

        { "STEAMHTTP_INTERFACE_VERSION001", "SteamHTTP001" },
        { "STEAMHTTP_INTERFACE_VERSION002", "SteamHTTP002" },
        { "STEAMHTTP_INTERFACE_VERSION003", "SteamHTTP003" },

        { "SteamInput001", "SteamInput001" },
        { "SteamInput002", "SteamInput002" },

        { "STEAMINVENTORY_INTERFACE_V001", "SteamInventory001" },
        { "STEAMINVENTORY_INTERFACE_V002", "SteamInventory002" },
        { "STEAMINVENTORY_INTERFACE_V003", "SteamInventory003" },

        { "SteamMasterServerUpdater001", "SteamServerupdater001" },

        { "SteamMatchMaking001", "SteamMatchmaking001" },
        { "SteamMatchMaking002", "SteamMatchmaking002" },
        { "SteamMatchMaking003", "SteamMatchmaking003" },
        { "SteamMatchMaking004", "SteamMatchmaking004" },
        { "SteamMatchMaking005", "SteamMatchmaking005" },
        { "SteamMatchMaking006", "SteamMatchmaking006" },
        { "SteamMatchMaking007", "SteamMatchmaking007" },
        { "SteamMatchMaking008", "SteamMatchmaking008" },
        { "SteamMatchMaking009", "SteamMatchmaking009" },

        { "SteamMatchMakingServers001", "SteamMatchmakingservers001" },
        { "SteamMatchMakingServers002", "SteamMatchmakingservers002" },

        { "STEAMMUSIC_INTERFACE_VERSION001", "SteamMusic001" },

        { "STEAMMUSICREMOTE_INTERFACE_VERSION001", "SteamRemotemusic001" },

        { "SteamNetworking001", "SteamNetworking001" },
        { "SteamNetworking002", "SteamNetworking002" },
        { "SteamNetworking003", "SteamNetworking003" },
        { "SteamNetworking004", "SteamNetworking004" },
        { "SteamNetworking005", "SteamNetworking005" },
        { "SteamNetworking006", "SteamNetworking006" },

        { "SteamNetworkingMessages001", "SteamNetworkingmessages001" },
        { "SteamNetworkingMessages002", "SteamNetworkingmessages002" },

        { "SteamNetworkingSockets001", "SteamNetworkingsockets001" },
        { "SteamNetworkingSockets002", "SteamNetworkingsockets002" },
        { "SteamNetworkingSockets003", "SteamNetworkingsockets003" },
        { "SteamNetworkingSockets004", "SteamNetworkingsockets004" },
        { "SteamNetworkingSockets005", "SteamNetworkingsockets005" },
        { "SteamNetworkingSockets006", "SteamNetworkingsockets006" },
        { "SteamNetworkingSockets007", "SteamNetworkingsockets007" },
        { "SteamNetworkingSockets008", "SteamNetworkingsockets008" },
        { "SteamNetworkingSockets009", "SteamNetworkingsockets009" },

        { "SteamNetworkingUtils001", "SteamNetworkingutilities001" },
        { "SteamNetworkingUtils002", "SteamNetworkingutilities002" },
        { "SteamNetworkingUtils003", "SteamNetworkingutilities003" },

        { "STEAMPARENTALSETTINGS_INTERFACE_VERSION001", "SteamParentalsettings001" },

        { "SteamParties001", "SteamParties001" },
        { "SteamParties002", "SteamParties002" },

        { "STEAMREMOTEPLAY_INTERFACE_VERSION001", "SteamRemoteplay001" },

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
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION013", "SteamRemotestorage013" },
        { "STEAMREMOTESTORAGE_INTERFACE_VERSION014", "SteamRemotestorage014" },

        { "STEAMSCREENSHOTS_INTERFACE_VERSION001", "SteamScreenshots001" },
        { "STEAMSCREENSHOTS_INTERFACE_VERSION002", "SteamScreenshots002" },
        { "STEAMSCREENSHOTS_INTERFACE_VERSION003", "SteamScreenshots003" },

        { "STEAMTV_INTERFACE_V001", "SteamTV001" },

        { "STEAMUGC_INTERFACE_VERSION001", "SteamUGC001" },
        { "STEAMUGC_INTERFACE_VERSION002", "SteamUGC002" },
        { "STEAMUGC_INTERFACE_VERSION003", "SteamUGC003" },
        { "STEAMUGC_INTERFACE_VERSION004", "SteamUGC004" },
        { "STEAMUGC_INTERFACE_VERSION005", "SteamUGC005" },
        { "STEAMUGC_INTERFACE_VERSION006", "SteamUGC006" },
        { "STEAMUGC_INTERFACE_VERSION007", "SteamUGC007" },
        { "STEAMUGC_INTERFACE_VERSION008", "SteamUGC008" },
        { "STEAMUGC_INTERFACE_VERSION009", "SteamUGC009" },
        { "STEAMUGC_INTERFACE_VERSION010", "SteamUGC010" },
        { "STEAMUGC_INTERFACE_VERSION011", "SteamUGC011" },
        { "STEAMUGC_INTERFACE_VERSION012", "SteamUGC012" },
        { "STEAMUGC_INTERFACE_VERSION013", "SteamUGC013" },
        { "STEAMUGC_INTERFACE_VERSION014", "SteamUGC014" },
        { "STEAMUGC_INTERFACE_VERSION015", "SteamUGC015" },

        { "STEAMUNIFIEDMESSAGES_INTERFACE_VERSION001", "SteamUnifiedmessages001" },

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
        { "SteamUser019", "SteamUser019" },
        { "SteamUser020", "SteamUser020" },
        { "SteamUser021", "SteamUser021" },

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
        { "STEAMUSERSTATS_INTERFACE_VERSION012", "SteamUserstats012" },

        { "SteamUtils001", "SteamUtilities001" },
        { "SteamUtils002", "SteamUtilities002" },
        { "SteamUtils003", "SteamUtilities003" },
        { "SteamUtils004", "SteamUtilities004" },
        { "SteamUtils005", "SteamUtilities005" },
        { "SteamUtils006", "SteamUtilities006" },
        { "SteamUtils007", "SteamUtilities007" },
        { "SteamUtils008", "SteamUtilities008" },
        { "SteamUtils009", "SteamUtilities009" },
        { "SteamUtils010", "SteamUtilities010" },

        { "STEAMVIDEO_INTERFACE_V001", "SteamVideo001" },
        { "STEAMVIDEO_INTERFACE_V002", "SteamVideo002" },
    };
}
