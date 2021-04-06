/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-06
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    enum ESteamPartyBeaconLocationData
    {
        k_ESteamPartyBeaconLocationDataInvalid,
        k_ESteamPartyBeaconLocationDataName = 1,
        k_ESteamPartyBeaconLocationDataIconURLSmall = 2,
        k_ESteamPartyBeaconLocationDataIconURLMedium = 3,
        k_ESteamPartyBeaconLocationDataIconURLLarge = 4,
    };
    enum ESteamPartyBeaconLocationType
    {
        k_ESteamPartyBeaconLocationType_Invalid,
        k_ESteamPartyBeaconLocationType_ChatGroup = 1,

        k_ESteamPartyBeaconLocationType_Max,
    };
    struct SteamPartyBeaconLocation_t
    {
        ESteamPartyBeaconLocationType m_eType;
        uint64_t m_ulLocationID;
    };

    static Hashset<SteamPartyBeaconLocation_t, decltype(WW::Hash), decltype(WW::Equal)> Beacons;
    static Hashmap<PartyBeaconID_t, Hashset<uint32_t>> Partymembers;

    struct Steamparties
    {
        PartyBeaconID_t GetBeaconByIndex(uint32_t unIndex)
        {
            for (const auto &[ID, _] : Partymembers)
            {
                if (unIndex-- == 0)
                {
                    return ID;
                }
            }

            return {};
        }

        SteamAPICall_t ChangeNumOpenSlots(PartyBeaconID_t ulBeacon, uint32_t unOpenSlots);
        SteamAPICall_t CreateBeacon(uint32_t unOpenSlots, SteamPartyBeaconLocation_t *pBeaconLocation, const char *pchConnectString, const char *pchMetadata);
        SteamAPICall_t JoinParty(PartyBeaconID_t ulBeaconID);

        bool DestroyBeacon(PartyBeaconID_t ulBeacon)
        {
            Partymembers.erase(ulBeacon);
            return true;
        }
        bool GetAvailableBeaconLocations(SteamPartyBeaconLocation_t *pLocationList, uint32_t uMaxNumLocations)
        {
            assert(pLocationList);
            size_t Outposition{};

            for (const auto &Item : Beacons)
            {
                if (Outposition == (uMaxNumLocations - 1))
                    break;

                pLocationList[Outposition++] = Item;
            }

            return true;
        }
        bool GetBeaconDetails(PartyBeaconID_t ulBeaconID, SteamID_t *pSteamIDBeaconOwner, SteamPartyBeaconLocation_t *pLocation, char *pchMetadata, int cchMetadata);
        bool GetBeaconLocationData(SteamPartyBeaconLocation_t BeaconLocation, ESteamPartyBeaconLocationData eData, char *pchDataStringOut, int cchDataStringOut);
        bool GetNumAvailableBeaconLocations(uint32_t *puNumLocations)
        {
            Hashset<uint64_t> IDs;

            for (const auto &Item : Beacons)
                IDs.insert(Item.m_ulLocationID);

            *puNumLocations = (uint32_t)IDs.size();
            return true;
        }

        uint32_t GetNumActiveBeacons()
        {
            return (uint32_t)Beacons.size();
        }

        void CancelReservation(PartyBeaconID_t ulBeacon, SteamID_t steamIDUser)
        {
        }
        void OnReservationCompleted(PartyBeaconID_t ulBeacon, SteamID_t steamIDUser);
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamParties001 : Interface_t
    {
        SteamParties001()
        {
            /* Missing SDK info */
        }
    };
    struct SteamParties002 : Interface_t
    {
        SteamParties002()
        {
            Createmethod(0, Steamparties, GetNumActiveBeacons);
            Createmethod(1, Steamparties, GetBeaconByIndex);
            Createmethod(2, Steamparties, GetBeaconDetails);
            Createmethod(3, Steamparties, JoinParty);
            Createmethod(4, Steamparties, GetNumAvailableBeaconLocations);
            Createmethod(5, Steamparties, GetAvailableBeaconLocations);
            Createmethod(6, Steamparties, CreateBeacon);
            Createmethod(7, Steamparties, OnReservationCompleted);
            Createmethod(8, Steamparties, CancelReservation);
            Createmethod(9, Steamparties, ChangeNumOpenSlots);
            Createmethod(10, Steamparties, DestroyBeacon);
            Createmethod(11, Steamparties, GetBeaconLocationData);
        }
    };

    struct Steamgamesearchloader
    {
        Steamgamesearchloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, &HACK ## z);
            Register(Interfacetype_t::GAMESEARCH , "SteamParties001", SteamParties001);
            Register(Interfacetype_t::GAMESEARCH , "SteamParties002", SteamParties002);
        }
    };
    static Steamgamesearchloader Interfaceloader{};
}
