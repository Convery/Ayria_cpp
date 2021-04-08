/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-06
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    struct Steamgamesearch
    {
        enum EGameSearchErrorCode_t
        {
            k_EGameSearchErrorCode_OK = 1,
            k_EGameSearchErrorCode_Failed_Search_Already_In_Progress = 2,
            k_EGameSearchErrorCode_Failed_No_Search_In_Progress = 3,
            k_EGameSearchErrorCode_Failed_Not_Lobby_Leader = 4, // if not the lobby leader can not call SearchForGameWithLobby
            k_EGameSearchErrorCode_Failed_No_Host_Available = 5, // no host is available that matches those search params
            k_EGameSearchErrorCode_Failed_Search_Params_Invalid = 6, // search params are invalid
            k_EGameSearchErrorCode_Failed_Offline = 7, // offline, could not communicate with server
            k_EGameSearchErrorCode_Failed_NotAuthorized = 8, // either the user or the application does not have priveledges to do this
            k_EGameSearchErrorCode_Failed_Unknown_Error = 9, // unknown error
        };
        enum EPlayerResult_t
        {
            k_EPlayerResultFailedToConnect = 1, // failed to connect after confirming
            k_EPlayerResultAbandoned = 2,		// quit game without completing it
            k_EPlayerResultKicked = 3,			// kicked by other players/moderator/server rules
            k_EPlayerResultIncomplete = 4,		// player stayed to end but game did not conclude successfully ( nofault to player )
            k_EPlayerResultCompleted = 5,		// player completed game
        };

        EGameSearchErrorCode_t AcceptGame();
        EGameSearchErrorCode_t AddGameSearchParams(const char *pchKeyToFind, const char *pchValuesToFind);
        EGameSearchErrorCode_t CancelRequestPlayersForGame();
        EGameSearchErrorCode_t DeclineGame();
        EGameSearchErrorCode_t EndGame(uint64_t ullUniqueGameID);
        EGameSearchErrorCode_t EndGameSearch();
        EGameSearchErrorCode_t HostConfirmGameStart(uint64_t ullUniqueGameID);
        EGameSearchErrorCode_t RequestPlayersForGame(int nPlayerMin, int nPlayerMax, int nMaxTeamSize);
        EGameSearchErrorCode_t RetrieveConnectionDetails(SteamID_t steamIDHost, char *pchConnectionDetails, int cubConnectionDetails);
        EGameSearchErrorCode_t SearchForGameSolo(int nPlayerMin, int nPlayerMax);
        EGameSearchErrorCode_t SearchForGameWithLobby(SteamID_t steamIDLobby, int nPlayerMin, int nPlayerMax);
        EGameSearchErrorCode_t SetConnectionDetails(const char *pchConnectionDetails, int cubConnectionDetails);
        EGameSearchErrorCode_t SetGameHostParams(const char *pchKey, const char *pchValue);
        EGameSearchErrorCode_t SubmitPlayerResult(uint64_t ullUniqueGameID, SteamID_t steamIDPlayer, EPlayerResult_t EPlayerResult);
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;

    struct SteamMatchGameSearch001 : Interface_t<14>
    {
        SteamMatchGameSearch001()
        {
            Createmethod(0, Steamgamesearch, AddGameSearchParams);
            Createmethod(1, Steamgamesearch, SearchForGameWithLobby);
            Createmethod(2, Steamgamesearch, SearchForGameSolo);
            Createmethod(3, Steamgamesearch, AcceptGame);
            Createmethod(4, Steamgamesearch, DeclineGame);
            Createmethod(5, Steamgamesearch, RetrieveConnectionDetails);
            Createmethod(6, Steamgamesearch, EndGameSearch);
            Createmethod(7, Steamgamesearch, SetGameHostParams);
            Createmethod(8, Steamgamesearch, SetConnectionDetails);
            Createmethod(9, Steamgamesearch, RequestPlayersForGame);
            Createmethod(10, Steamgamesearch, HostConfirmGameStart);
            Createmethod(11, Steamgamesearch, CancelRequestPlayersForGame);
            Createmethod(12, Steamgamesearch, SubmitPlayerResult);
            Createmethod(13, Steamgamesearch, EndGame);
        }
    };

    struct Steamgamesearchloader
    {
        Steamgamesearchloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::GAMESEARCH , "SteamMatchGameSearch001", SteamMatchGameSearch001);
        }
    };
    static Steamgamesearchloader Interfaceloader{};
}
