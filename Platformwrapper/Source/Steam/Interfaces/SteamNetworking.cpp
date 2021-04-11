/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "../Steam.hpp"
#pragma warning(disable : 4100)

namespace Steam
{
    // Totally randomly selected constants..
    constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
    constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985

    static Hashmap<uint32_t, std::string> Incomingdata;
    struct Endpoint_t { uint32_t vPort, Socket; mutable uint64_t HostID; };
    static Hashset<Endpoint_t, decltype(WW::Hash), decltype(WW::Equal)> Endpoints;

    static void __cdecl Networkhandler(uint32_t, const char *Message, uint32_t Length)
    {
        const auto Request = JSON::Parse(std::string_view(Message, Length));
        const auto SenderID = Request.value<uint64_t>("SenderID");
        const auto TargetID = Request.value<uint64_t>("TargetID");
        const auto vPort = Request.value<uint32_t>("vPort");

        // Are we even interested in this?
        if (TargetID != Global.XUID.FullID) return;

        for (auto &Item : Endpoints)
        {
            if (!vPort || vPort == vPort)
            {
                Item.HostID = SenderID;
                Endpoints.rehash(Endpoints.size());
                Incomingdata[Item.Socket].append(Base64::Decode(Request.value<std::string>("B64Message")));
                break;
            }
        }
    }
    static void Transmitmessage(uint32_t vPort, uint64_t TargetID, std::string &&B64Message)
    {
        const auto Payload = JSON::Object_t({
            { "SenderID", Global.XUID.FullID },
            { "B64Message", B64Message },
            { "TargetID", TargetID },
            { "vPort", vPort }
        });
        const auto Request = JSON::Object_t({
            { "Messagetype", "Steam_Networkpacket"s },
            { "Message", JSON::Dump(Payload) }
        });

        Ayriarequest("Network::Sendmessage", Request);
    }

    static bool Initialized = false;
    static void Initialize()
    {
        if (Initialized) return;
        Initialized = true;

        if (const auto Callback = Ayria.addMessagehandler)
            Callback("Steam_Networkpacket", Networkhandler);
    }

    struct SteamNetworking
    {
        using P2PSessionState_t = struct
        {
            uint8_t m_bConnectionActive;    // true if we've got an active open connection
            uint8_t m_bConnecting;			// true if we're currently trying to establish a connection
            uint8_t m_eP2PSessionError;		// last error recorded (see enum above)
            uint8_t m_bUsingRelay;			// true if it's going through a relay server (TURN)
            int32_t m_nBytesQueuedForSend;
            int32_t m_nPacketsQueuedForSend;
            uint32_t m_nRemoteIP;		    // potential IP:Port of remote host. Could be TURN server.
            uint16_t m_nRemotePort;			// Only exists for compatibility with older authentication api's
        };
        enum ESNetSocketConnectionType : uint32_t
        {
            k_ESNetSocketConnectionTypeNotConnected = 0,
            k_ESNetSocketConnectionTypeUDP = 1,
            k_ESNetSocketConnectionTypeUDPRelay = 2,
        };
        enum ESNetSocketState : uint32_t
        {
            k_ESNetSocketStateInvalid = 0,

            // communication is valid
            k_ESNetSocketStateConnected = 1,

            // states while establishing a connection
            k_ESNetSocketStateInitiated = 10,				// the connection state machine has started

            // p2p connections
            k_ESNetSocketStateLocalCandidatesFound = 11,	// we've found our local IP info
            k_ESNetSocketStateReceivedRemoteCandidates = 12,// we've received information from the remote machine, via the Steam back-end, about their IP info

            // direct connections
            k_ESNetSocketStateChallengeHandshake = 15,		// we've received a challenge packet from the server

            // failure states
            k_ESNetSocketStateDisconnecting = 21,			// the API shut it down, and we're in the process of telling the other end
            k_ESNetSocketStateLocalDisconnect = 22,			// the API shut it down, and we've completed shutdown
            k_ESNetSocketStateTimeoutDuringConnect = 23,	// we timed out while trying to creating the connection
            k_ESNetSocketStateRemoteEndDisconnected = 24,	// the remote end has disconnected from us
            k_ESNetSocketStateConnectionBroken = 25,		// connection has been broken; either the other end has disappeared or our local network connection has broke
        };
        enum EP2PSend : uint32_t
        {
            // Basic UDP send. Packets can't be bigger than 1200 bytes (your typical MTU size). Can be lost, or arrive out of order (rare).
            // The sending API does have some knowledge of the underlying connection, so if there is no NAT-traversal accomplished or
            // there is a recognized adjustment happening on the connection, the packet will be batched until the connection is open again.
            k_EP2PSendUnreliable = 0,

            // As above, but if the underlying p2p connection isn't yet established the packet will just be thrown away. Using this on the first
            // packet sent to a remote host almost guarantees the packet will be dropped.
            // This is only really useful for kinds of data that should never buffer up, i.e. voice payload packets
            k_EP2PSendUnreliableNoDelay = 1,

            // Reliable message send. Can send up to 1MB of data in a single message.
            // Does fragmentation/re-assembly of messages under the hood, as well as a sliding window for efficient sends of large chunks of data.
            k_EP2PSendReliable = 2,

            // As above, but applies the Nagle algorithm to the send - sends will accumulate
            // until the current MTU size (typically ~1200 bytes, but can change) or ~200ms has passed (Nagle algorithm).
            // Useful if you want to send a set of smaller messages but have the coalesced into a single packet
            // Since the reliable stream is all ordered, you can do several small message sends with k_EP2PSendReliableWithBuffering and then
            // do a normal k_EP2PSendReliable to force all the buffered data to be sent.
            k_EP2PSendReliableWithBuffering = 3,
        };

        using SNetListenSocket_t = uint32_t;
        using SNetSocket_t = uint32_t;

        ESNetSocketConnectionType GetSocketConnectionType(SNetSocket_t hSocket)
        {
            return k_ESNetSocketConnectionTypeUDP;
        }
        SNetListenSocket_t CreateListenSocket0(int32_t nVirtualP2PPort, uint32_t nIP, uint16_t nPort)
        {
            Initialize();
            return GetTickCount() & 0xFFFFFFFF;
        }
        SNetListenSocket_t CreateListenSocket1(int32_t nVirtualP2PPort, uint32_t nIP, uint16_t nPort, bool bAllowUseOfPacketRelay)
        {
            return CreateListenSocket0(nVirtualP2PPort, nIP, nPort);
        }
        SNetSocket_t CreateConnectionSocket(uint32_t nIP, uint16_t nPort, int32_t nTimeoutSec)
        {
            Initialize();

            const uint32_t Socket = GetTickCount();
            Endpoints.insert({ nPort, Socket, 0 });
            return Socket;
        }
        SNetSocket_t CreateP2PConnectionSocket0(SteamID_t steamIDTarget, int32_t nVirtualPort, int32_t nTimeoutSec)
        {
            Initialize();

            const uint32_t Socket = GetTickCount();
            Endpoints.insert({ (uint32_t)nVirtualPort, Socket, steamIDTarget.FullID });
            return Socket;
        }
        SNetSocket_t CreateP2PConnectionSocket1(SteamID_t steamIDTarget, int32_t nVirtualPort, int32_t nTimeoutSec, bool bAllowUseOfPacketRelay)
        {
            return CreateP2PConnectionSocket0(steamIDTarget, nVirtualPort, nTimeoutSec);
        }

        bool AcceptP2PSessionWithUser(SteamID_t steamIDRemote)
        {
            return true;
        }
        bool AllowP2PPacketRelay(bool bAllow)
        {
            return true;
        }
        bool CloseP2PSessionWithUser(SteamID_t steamIDRemote)
        {
            return true;
        }
        bool CloseP2PChannelWithUser(SteamID_t steamIDRemote, int32_t iVirtualPort)
        {
            return true;
        }
        bool DestroyListenSocket(SNetListenSocket_t hSocket, bool bNotifyRemoteEnd)
        {
            return true;
        }
        bool DestroySocket(SNetSocket_t hSocket, bool bNotifyRemoteEnd)
        {
            return true;
        }
        bool GetListenSocketInfo(SNetListenSocket_t hListenSocket, uint32_t *pnIP, uint16_t *pnPort)
        {
            *pnIP = Syncaddress;
            *pnPort = Syncport;
            return true;
        }
        bool GetP2PSessionState(SteamID_t steamIDRemote, P2PSessionState_t *pConnectionState)
        {
            for (const auto &Item : Endpoints)
            {
                if (Item.HostID == steamIDRemote.FullID)
                {
                    if (Incomingdata.contains(Item.Socket))
                    {
                        pConnectionState->m_nBytesQueuedForSend = (int32_t)Incomingdata[Item.Socket].size() / GetMaxPacketSize(0);
                        pConnectionState->m_nPacketsQueuedForSend = (int32_t)Incomingdata[Item.Socket].size();
                    }

                    break;
                }
            }

            pConnectionState->m_nRemoteIP = Syncaddress;
            pConnectionState->m_nRemotePort = Syncport;
            pConnectionState->m_bConnectionActive = 1;
            pConnectionState->m_eP2PSessionError = 0;
            pConnectionState->m_bUsingRelay = false;
            pConnectionState->m_bConnecting = 0;
            return true;
        }
        bool GetSocketInfo(SNetSocket_t hSocket, SteamID_t *pSteamIDRemote, ESNetSocketState *peSocketStatus, uint32_t *punIPRemote, uint16_t *punPortRemote)
        {
            for (const auto &Item : Endpoints)
            {
                if (Item.Socket == hSocket)
                {
                    pSteamIDRemote->FullID = Item.HostID;
                    break;
                }
            }

            *peSocketStatus = k_ESNetSocketStateConnected;
            *punIPRemote = Syncaddress;
            *punPortRemote = Syncport;
            return false;
        }
        bool IsDataAvailable(SNetListenSocket_t hListenSocket, uint32_t *pcubMsgSize, SNetSocket_t *phSocket)
        {
            for (const auto &[Socket, Data] : Incomingdata)
            {
                if (!Data.empty())
                {
                    *pcubMsgSize = (uint32_t)Data.size();
                    *phSocket = Socket;
                    return true;
                }
            }

            return false;
        }
        bool IsDataAvailableOnSocket(SNetSocket_t hSocket, uint32_t *pcubMsgSize)
        {
            if (!Incomingdata.contains(hSocket)) return false;
            if (Incomingdata[hSocket].empty()) return false;

            *pcubMsgSize = (uint32_t)Incomingdata[hSocket].size();
            return true;
        }
        bool IsP2PPacketAvailable0(uint32_t *pcubMsgSize)
        {
            return IsP2PPacketAvailable1(pcubMsgSize, 0);
        }
        bool IsP2PPacketAvailable1(uint32_t *pcubMsgSize, int32_t iVirtualPort)
        {
            uint32_t Unused;
            return IsDataAvailable(0, pcubMsgSize, &Unused);
        }
        bool ReadP2PPacket0(void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SteamID_t *psteamIDRemote)
        {
            return ReadP2PPacket1(pubDest, cubDest, pcubMsgSize, psteamIDRemote, 0);
        }
        bool ReadP2PPacket1(void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SteamID_t *psteamIDRemote, int32_t iVirtualPort)
        {
            for (const auto &Item : Endpoints)
            {
                if (Incomingdata.contains(Item.Socket))
                {
                    auto Buffer = &Incomingdata[Item.Socket];
                    if (!Buffer->empty())
                    {
                        const auto Min = std::min(cubDest, uint32_t(Buffer->size()));
                        std::memcpy(pubDest, Buffer->data(), Min);
                        psteamIDRemote->FullID = Item.HostID;
                        Buffer->erase(0, Min);
                        *pcubMsgSize = Min;
                        return true;
                    }
                }
            }

            return false;
        }
        bool RetrieveData1(SNetListenSocket_t hListenSocket, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SNetSocket_t *phSocket)
        {
            for (const auto &Item : Endpoints)
            {
                if (Incomingdata.contains(Item.Socket))
                {
                    auto Buffer = &Incomingdata[Item.Socket];
                    if (!Buffer->empty())
                    {
                        const auto Min = std::min(cubDest, uint32_t(Buffer->size()));
                        std::memcpy(pubDest, Buffer->data(), Min);
                        *phSocket = Item.Socket;
                        Buffer->erase(0, Min);
                        *pcubMsgSize = Min;
                        return true;
                    }
                }
            }

            return false;
        }
        bool RetrieveData0(SNetListenSocket_t hListenSocket, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize)
        {
            uint32_t Unused;
            return RetrieveData1(hListenSocket, pubDest, cubDest, pcubMsgSize, &Unused);
        }
        bool RetrieveDataFromSocket(SNetSocket_t hSocket, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize)
        {
            if (Incomingdata.contains(hSocket))
            {
                auto Buffer = &Incomingdata[hSocket];
                if (!Buffer->empty())
                {
                    const auto Min = std::min(cubDest, uint32_t(Buffer->size()));
                    std::memcpy(pubDest, Buffer->data(), Min);
                    Buffer->erase(0, Min);
                    *pcubMsgSize = Min;
                    return true;
                }
            }

            return false;
        }
        bool SendDataOnSocket(SNetSocket_t hSocket, void *pubData, uint32_t cubData, bool bReliable)
        {
            for (const auto &Item : Endpoints)
            {
                if (Item.Socket == hSocket)
                {
                    Transmitmessage(Item.vPort, Item.HostID, Base64::Encode(std::string_view((char *)pubData, cubData)));
                    return true;
                }
            }

            return false;
        }
        bool SendP2PPacket0(SteamID_t steamIDRemote, const void *pubData, uint32_t cubData, EP2PSend eP2PSendType)
        {
            return SendP2PPacket1(steamIDRemote, pubData, cubData, eP2PSendType, 0);
        }
        bool SendP2PPacket1(SteamID_t steamIDRemote, const void *pubData, uint32_t cubData, EP2PSend eP2PSendType, int32_t iVirtualPort)
        {
            Transmitmessage(iVirtualPort, steamIDRemote.FullID, Base64::Encode(std::string_view((char *)pubData, cubData)));
            return true;
        }

        int32_t GetMaxPacketSize(SNetSocket_t hSocket)
        {
            return 4096;
        }
    };

    static std::any Hackery;
    #define Createmethod(Index, Class, Function) Hackery = &Class::Function; VTABLE[Index] = *(void **)&Hackery;
    struct SteamNetworking001 : Interface_t<12>
    {
        SteamNetworking001()
        {
            Createmethod(0, SteamNetworking, CreateListenSocket0);
            Createmethod(1, SteamNetworking, CreateP2PConnectionSocket0);
            Createmethod(2, SteamNetworking, CreateConnectionSocket);
            Createmethod(3, SteamNetworking, DestroySocket);
            Createmethod(4, SteamNetworking, DestroyListenSocket);
            Createmethod(5, SteamNetworking, SendDataOnSocket);
            Createmethod(6, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(7, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(8, SteamNetworking, IsDataAvailable);
            Createmethod(9, SteamNetworking, RetrieveData0);
            Createmethod(10, SteamNetworking, GetSocketInfo);
            Createmethod(11, SteamNetworking, GetListenSocketInfo);
        };
    };
    struct SteamNetworking002 : Interface_t<14>
    {
        SteamNetworking002()
        {
            Createmethod(0, SteamNetworking, CreateListenSocket1);
            Createmethod(1, SteamNetworking, CreateP2PConnectionSocket1);
            Createmethod(2, SteamNetworking, CreateConnectionSocket);
            Createmethod(3, SteamNetworking, DestroySocket);
            Createmethod(4, SteamNetworking, DestroyListenSocket);
            Createmethod(5, SteamNetworking, SendDataOnSocket);
            Createmethod(6, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(7, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(8, SteamNetworking, IsDataAvailable);
            Createmethod(9, SteamNetworking, RetrieveData1);
            Createmethod(10, SteamNetworking, GetSocketInfo);
            Createmethod(11, SteamNetworking, GetListenSocketInfo);
            Createmethod(12, SteamNetworking, GetSocketConnectionType);
            Createmethod(13, SteamNetworking, GetMaxPacketSize);
        };
    };
    struct SteamNetworking003 : Interface_t<20>
    {
        SteamNetworking003()
        {
            Createmethod(0, SteamNetworking, SendP2PPacket0);
            Createmethod(1, SteamNetworking, IsP2PPacketAvailable0);
            Createmethod(2, SteamNetworking, ReadP2PPacket0);
            Createmethod(3, SteamNetworking, AcceptP2PSessionWithUser);
            Createmethod(4, SteamNetworking, CloseP2PSessionWithUser);
            Createmethod(5, SteamNetworking, GetP2PSessionState);
            Createmethod(6, SteamNetworking, CreateListenSocket1);
            Createmethod(7, SteamNetworking, CreateP2PConnectionSocket1);
            Createmethod(8, SteamNetworking, CreateConnectionSocket);
            Createmethod(9, SteamNetworking, DestroySocket);
            Createmethod(10, SteamNetworking, DestroyListenSocket);
            Createmethod(11, SteamNetworking, SendDataOnSocket);
            Createmethod(12, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(13, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(14, SteamNetworking, IsDataAvailable);
            Createmethod(15, SteamNetworking, RetrieveData1);
            Createmethod(16, SteamNetworking, GetSocketInfo);
            Createmethod(17, SteamNetworking, GetListenSocketInfo);
            Createmethod(18, SteamNetworking, GetSocketConnectionType);
            Createmethod(19, SteamNetworking, GetMaxPacketSize);
        };
    };
    struct SteamNetworking004 : Interface_t<20>
    {
        SteamNetworking004()
        {
            Createmethod(0, SteamNetworking, SendP2PPacket1);
            Createmethod(1, SteamNetworking, IsP2PPacketAvailable1);
            Createmethod(2, SteamNetworking, ReadP2PPacket1);
            Createmethod(3, SteamNetworking, AcceptP2PSessionWithUser);
            Createmethod(4, SteamNetworking, CloseP2PSessionWithUser);
            Createmethod(5, SteamNetworking, GetP2PSessionState);
            Createmethod(6, SteamNetworking, CreateListenSocket1);
            Createmethod(7, SteamNetworking, CreateP2PConnectionSocket1);
            Createmethod(8, SteamNetworking, CreateConnectionSocket);
            Createmethod(9, SteamNetworking, DestroySocket);
            Createmethod(10, SteamNetworking, DestroyListenSocket);
            Createmethod(11, SteamNetworking, SendDataOnSocket);
            Createmethod(12, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(13, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(14, SteamNetworking, IsDataAvailable);
            Createmethod(15, SteamNetworking, RetrieveData1);
            Createmethod(16, SteamNetworking, GetSocketInfo);
            Createmethod(17, SteamNetworking, GetListenSocketInfo);
            Createmethod(18, SteamNetworking, GetSocketConnectionType);
            Createmethod(19, SteamNetworking, GetMaxPacketSize);
        };
    };
    struct SteamNetworking005 : Interface_t<22>
    {
        SteamNetworking005()
        {
            Createmethod(0, SteamNetworking, SendP2PPacket1);
            Createmethod(1, SteamNetworking, IsP2PPacketAvailable1);
            Createmethod(2, SteamNetworking, ReadP2PPacket1);
            Createmethod(3, SteamNetworking, AcceptP2PSessionWithUser);
            Createmethod(4, SteamNetworking, CloseP2PSessionWithUser);
            Createmethod(5, SteamNetworking, CloseP2PChannelWithUser);
            Createmethod(6, SteamNetworking, GetP2PSessionState);
            Createmethod(7, SteamNetworking, AllowP2PPacketRelay);
            Createmethod(8, SteamNetworking, CreateListenSocket1);
            Createmethod(9, SteamNetworking, CreateP2PConnectionSocket1);
            Createmethod(10, SteamNetworking, CreateConnectionSocket);
            Createmethod(11, SteamNetworking, DestroySocket);
            Createmethod(12, SteamNetworking, DestroyListenSocket);
            Createmethod(13, SteamNetworking, SendDataOnSocket);
            Createmethod(14, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(15, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(16, SteamNetworking, IsDataAvailable);
            Createmethod(17, SteamNetworking, RetrieveData1);
            Createmethod(18, SteamNetworking, GetSocketInfo);
            Createmethod(19, SteamNetworking, GetListenSocketInfo);
            Createmethod(20, SteamNetworking, GetSocketConnectionType);
            Createmethod(21, SteamNetworking, GetMaxPacketSize);
        };
    };
    struct SteamNetworking006 : Interface_t<22>
    {
        SteamNetworking006()
        {
            Createmethod(0, SteamNetworking, SendP2PPacket1);
            Createmethod(1, SteamNetworking, IsP2PPacketAvailable1);
            Createmethod(2, SteamNetworking, ReadP2PPacket1);
            Createmethod(3, SteamNetworking, AcceptP2PSessionWithUser);
            Createmethod(4, SteamNetworking, CloseP2PSessionWithUser);
            Createmethod(5, SteamNetworking, CloseP2PChannelWithUser);
            Createmethod(6, SteamNetworking, GetP2PSessionState);
            Createmethod(7, SteamNetworking, AllowP2PPacketRelay);
            Createmethod(8, SteamNetworking, CreateListenSocket1);
            Createmethod(9, SteamNetworking, CreateP2PConnectionSocket1);
            Createmethod(10, SteamNetworking, CreateConnectionSocket);
            Createmethod(11, SteamNetworking, DestroySocket);
            Createmethod(12, SteamNetworking, DestroyListenSocket);
            Createmethod(13, SteamNetworking, SendDataOnSocket);
            Createmethod(14, SteamNetworking, IsDataAvailableOnSocket);
            Createmethod(15, SteamNetworking, RetrieveDataFromSocket);
            Createmethod(16, SteamNetworking, IsDataAvailable);
            Createmethod(17, SteamNetworking, RetrieveData1);
            Createmethod(18, SteamNetworking, GetSocketInfo);
            Createmethod(19, SteamNetworking, GetListenSocketInfo);
            Createmethod(20, SteamNetworking, GetSocketConnectionType);
            Createmethod(21, SteamNetworking, GetMaxPacketSize);
        };
    };

    struct Steamnetworkingloader
    {
        Steamnetworkingloader()
        {
            #define Register(x, y, z) static z HACK ## z{}; Registerinterface(x, y, (Interface_t<> *)&HACK ## z);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking001", SteamNetworking001);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking002", SteamNetworking002);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking003", SteamNetworking003);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking004", SteamNetworking004);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking005", SteamNetworking005);
            Register(Interfacetype_t::NETWORKING, "SteamNetworking006", SteamNetworking006);
        }
    };
    static Steamnetworkingloader Interfaceloader{};

}
