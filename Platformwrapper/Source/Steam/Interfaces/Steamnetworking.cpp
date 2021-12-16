/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-07
    License: MIT
*/

#include <Steam.hpp>

// We use for loops for results that return 0 or 1 values, ignore warnings.
#pragma warning(disable: 4702)

namespace Steam
{
    // Totally randomly selected constants..
    constexpr uint32_t Discoveryaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
    constexpr uint16_t Discoveryport = Hash::FNV1_32("Steam") & 0xFFFF;  // 51527
    constexpr uint32_t Maxpacketsize = 4096;

    struct Packet_t { uint32_t Size; std::string Data; };
    struct Endpoint_t
    {
        size_t Comsocket, Listensocket;
        sockaddr_in IPv4Address;

        SteamID_t SteamID;
        std::deque<Packet_t> Outgoing, Incoming;
    };

    static const sockaddr_in Multicast{ AF_INET, htons(Discoveryport), {{.S_addr = htonl(Discoveryaddress)}} };
    static Hashmap<size_t, sockaddr_in> Pendingconnections{};
    static Hashmap<uint64_t, sockaddr_in> Seenclients{};
    static std::vector<Endpoint_t> Endpoints{};
    static Hashmap<int32_t, size_t> Portmap{};
    static Hashset<size_t> Listensockets{};
    static size_t Discoverysocket{};

    // Do any networking we need.
    static void __cdecl Networktask()
    {
        // Check for incoming connections.
        {
            fd_set ListenFD{};
            for (const auto &Socket : Listensockets)
                FD_SET(Socket, &ListenFD);

            constexpr timeval Defaulttimeout{ NULL, 1 };
            const auto Count{ ListenFD.fd_count };
            auto Timeout{ Defaulttimeout };

            if (select(Count, &ListenFD, nullptr, nullptr, &Timeout)) [[unlikely]]
            {
                for (const auto &Socket : Listensockets)
                {
                    if (FD_ISSET(Socket, &ListenFD)) [[unlikely]]
                    {
                        sockaddr_in Clientinfo; int Size = sizeof(Clientinfo);
                        const auto Newsock = accept(Socket, (sockaddr *)&Clientinfo, &Size);
                        if (Newsock == INVALID_SOCKET) [[unlikely]] continue;

                        if (std::ranges::none_of(Endpoints,
                            [Client = Hash::WW32(Clientinfo)](const auto &Item)
                            { return Hash::WW32(Item) == Client; },
                            &Endpoint_t::IPv4Address))
                        {
                            Endpoints.emplace_back(Newsock, Socket, Clientinfo);

                            const auto Result = std::shared_ptr<Tasks::SocketStatusCallback_t>();
                            Result->m_hListenSocket = Socket & 0xFFFFFFFF;
                            Result->m_hSocket = Newsock & 0xFFFFFFFF;
                            Result->m_eSNetSocketState = 1;

                            const auto TaskID = Tasks::Createrequest();
                            Tasks::Completerequest(TaskID, Tasks::ECallbackType::SocketStatusCallback_t, Result);
                        }
                    }
                }
            }
        }

        // Check pending connection-attempts.
        {
            fd_set ConnectFD{}, FailureFD;
            for (const auto &Socket : Pendingconnections | std::views::keys)
                FD_SET(Socket, &ConnectFD);
            FailureFD = ConnectFD;

            constexpr timeval Defaulttimeout{ NULL, 1 };
            const auto Count{ ConnectFD.fd_count };
            auto Timeout{ Defaulttimeout };

            if (select(Count, nullptr, &ConnectFD, &FailureFD, &Timeout)) [[unlikely]]
            {
                for (const auto &[Socket, Clientinfo] : Pendingconnections)
                {
                    if (FD_ISSET(Socket, &FailureFD)) [[unlikely]]
                    {
                        const auto Result = std::shared_ptr<Tasks::SocketStatusCallback_t>();
                        Result->m_hSocket = Socket & 0xFFFFFFFF;
                        Result->m_eSNetSocketState = 23;

                        const auto TaskID = Tasks::Createrequest();
                        Tasks::Completerequest(TaskID, Tasks::ECallbackType::SocketStatusCallback_t, Result);

                        Pendingconnections.erase(Socket);
                        break;
                    }

                    if (FD_ISSET(Socket, &ConnectFD))
                    {
                        Endpoints.push_back({ Socket, {}, Clientinfo });

                        const auto Result = std::shared_ptr<Tasks::SocketStatusCallback_t>();
                        Result->m_hSocket = Socket & 0xFFFFFFFF;
                        Result->m_eSNetSocketState = 1;

                        const auto TaskID = Tasks::Createrequest();
                        Tasks::Completerequest(TaskID, Tasks::ECallbackType::SocketStatusCallback_t, Result);

                        Pendingconnections.erase(Socket);
                        break;
                    }
                }
            }
        }

        // Push and pull any available data.
        {
            fd_set ReadFD{}, WriteFD;
            for (const auto &Item : Endpoints)
                FD_SET(Item.Comsocket, &ReadFD);
            WriteFD = ReadFD;

            constexpr timeval Defaulttimeout{ NULL, 1 };
            const auto Count{ WriteFD.fd_count };
            auto Timeout{ Defaulttimeout };

            if (select(Count, &ReadFD, &WriteFD, nullptr, &Timeout))
            {
                auto Buffer = (char *)alloca(Maxpacketsize);

                for (auto &Item : Endpoints)
                {
                    if (FD_ISSET(Item.Comsocket, &ReadFD))
                    {
                        const auto Headersize = recv(Item.Comsocket, Buffer, 4, MSG_PEEK);
                        if (Headersize < 4)
                        {
                            if (Headersize == 0)
                            {
                                const auto Result = std::shared_ptr<Tasks::SocketStatusCallback_t>();
                                Result->m_hSocket = Item.Comsocket & 0xFFFFFFFF;
                                Result->m_steamIDRemote = Item.SteamID;
                                Result->m_eSNetSocketState = 24;

                                const auto TaskID = Tasks::Createrequest();
                                Tasks::Completerequest(TaskID, Tasks::ECallbackType::SocketStatusCallback_t, Result);

                                closesocket(Item.Comsocket);
                                Item.Comsocket = 0;
                            }

                            continue;
                        }

                        std::unique_ptr<char[]> TMP;
                        const int64_t Wanted = ((Packet_t *)Buffer)->Size + 4;

                        // Above max-size, just abort.
                        if (Wanted > Maxpacketsize)
                        {
                            closesocket(Item.Comsocket);
                            Item.Comsocket = 0;
                            continue;
                        }

                        const auto Read = recv(Item.Comsocket, Buffer, Wanted, MSG_PEEK);
                        if (Read < Wanted) continue;

                        recv(Item.Comsocket, Buffer, Wanted, NULL);

                        Packet_t Incoming{ ((Packet_t *)Buffer)->Size };
                        Incoming.Data = { Buffer + 4, ((Packet_t *)Buffer)->Size };
                        Item.Incoming.push_back(Incoming);
                    }

                    if (FD_ISSET(Item.Comsocket, &WriteFD))
                    {
                        if (!Item.Outgoing.empty())
                        {
                            auto Packet = Item.Outgoing.front();
                            Packet.Data.insert(0, (char *)&Packet.Size, 4);

                            const auto Sent = send(Item.Comsocket, Packet.Data.data(), int(Packet.Data.size()), NULL);

                            if (Sent <  int(Packet.Data.size())) Packet.Data.erase(0, 4);
                            else Item.Outgoing.pop_front();
                        }
                    }
                }
            }
        }

        // Lastly, discover local clients.
        {
            fd_set ReadFD{};
            FD_SET(Discoverysocket, &ReadFD);

            constexpr timeval Defaulttimeout{ NULL, 1 };
            auto Timeout{ Defaulttimeout };
            const auto Count{ 1 };

            if (select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[unlikely]]
            {
                uint64_t SteamID{};
                sockaddr_in Clientinfo; int Size = sizeof(Clientinfo);
                recvfrom(Discoverysocket, (char *)SteamID, 8, NULL, (sockaddr *)&Clientinfo, &Size);

                if (SteamID)
                {
                    SteamID = ntohll(SteamID);
                    Seenclients[SteamID] = Clientinfo;
                    std::ranges::for_each(Endpoints,[SteamID, Client = Hash::WW32(Clientinfo)](auto &Item)
                    {
                        if (Hash::WW32(Item.IPv4Address) == Client)
                            Item.SteamID.FullID = SteamID;
                    });
                }
            }
        }

        // And do some minor cleanup.
        const auto Invalid = std::ranges::remove(Endpoints, 0, &Endpoint_t::Comsocket);
        Endpoints.erase(Invalid.begin(), Invalid.end());

        for (auto it = Portmap.begin(); it != Portmap.end();)
        {
            if (it->second == 0) Portmap.erase(it);
            else ++it;
        }
    }
    static void __cdecl doDiscovery()
    {
        const uint64_t Buffer = htonll(Global.XUID.FullID);
        sendto(Discoverysocket, (char *)&Buffer, 8, NULL, PSOCKADDR(&Multicast), sizeof(Multicast));
    }
    static void Initialize()
    {
        static bool Initialized = false;
        if (Initialized) return;
        Initialized = true;

        const sockaddr_in Localhost{ AF_INET, htons(Discoveryport), {{.S_addr = htonl(INADDR_ANY)}} };
        const ip_mreq Request{ {{.S_addr = htonl(Discoveryaddress)}} };
        unsigned long Argument{ 1 };
        unsigned long Error{ 0 };
        WSADATA Unused;

        // We only need WS 1.1, no need for more.
        (void)WSAStartup(MAKEWORD(1, 1), &Unused);
        Discoverysocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Discoverysocket, FIONBIO, &Argument);

        // TODO(tcn): Investigate if WS2's IP_ADD_MEMBERSHIP (12) gets mapped to the WS1's original (5) internally.
        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Discoverysocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Discoverysocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Discoverysocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            closesocket(Discoverysocket);
            assert(false);
            return;
        }

        Ayria.createPeriodictask(100, Networktask);
        Ayria.createPeriodictask(5000, doDiscovery);
    }

    // Helpers to filter endpoints.
    static inline auto byCom(uint32_t Socket)
    {
        return Endpoints | std::views::filter([=](const auto &Item) { return Socket == (Item.Comsocket & 0xFFFFFFFF); });
    }
    static inline auto bySteamID(SteamID_t ID)
    {
        return Endpoints | std::views::filter([=](const auto &Item) { return ID.FullID == Item.SteamID.FullID; });
    }
    static inline auto bySocket(size_t Socket)
    {
        return Endpoints | std::views::filter([=](const auto &Item) { return Socket == Item.Comsocket; });
    }
    static inline auto byListen(uint32_t Socket)
    {
        return Endpoints | std::views::filter([=](const auto &Item) { return Socket == (Item.Listensocket & 0xFFFFFFFF); });
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

            if (Portmap.contains(nVirtualP2PPort))
                return Portmap[nVirtualP2PPort] & 0xFFFFFFFF;

            DWORD Arg = 1;
            const auto Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            ioctlsocket(Socket, FIONBIO, &Arg);

            do
            {
                const sockaddr_in Hostname = { AF_INET, htons(nPort), {.S_un {.S_addr = htonl(nIP) } } };

                if (bind(Socket, (const sockaddr *)&Hostname, sizeof(Hostname)) == SOCKET_ERROR) break;
                if (listen(Socket, 10) == SOCKET_ERROR) break;

                // TODO(tcn): Verify if windows x64 uses the higher bits.
                Portmap[nVirtualP2PPort] = Socket;
                Listensockets.insert(Socket);
                return Socket & 0xFFFFFFFF;
            } while (false);

            closesocket(Socket);
            return 0;
        }
        SNetListenSocket_t CreateListenSocket1(int32_t nVirtualP2PPort, uint32_t nIP, uint16_t nPort, bool bAllowUseOfPacketRelay)
        {
            return CreateListenSocket0(nVirtualP2PPort, nIP, nPort);
        }
        SNetSocket_t CreateConnectionSocket(uint32_t nIP, uint16_t nPort, int32_t nTimeoutSec)
        {
            Initialize();

            DWORD Arg = 1;
            const auto Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            ioctlsocket(Socket, FIONBIO, &Arg);

            const sockaddr_in Hostname = { AF_INET, htons(nPort), {.S_un {.S_addr = htonl(nIP) } } };
            connect(Socket, (const sockaddr *)&Hostname, sizeof(Hostname));

            Pendingconnections[Socket] = Hostname;
            return Socket & 0xFFFFFFFF;
        }
        SNetSocket_t CreateP2PConnectionSocket0(SteamID_t steamIDTarget, int32_t nVirtualPort, int32_t nTimeoutSec)
        {
            Initialize();

            // No idea how to connect to them..
            if (!Seenclients.contains(steamIDTarget.FullID))
                return 0;

            DWORD Arg = 1;
            const auto Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            ioctlsocket(Socket, FIONBIO, &Arg);

            const sockaddr_in Hostname = Seenclients[steamIDTarget.FullID];
            connect(Socket, (const sockaddr *)&Hostname, sizeof(Hostname));

            if (0 == nVirtualPort) nVirtualPort = steamIDTarget.UserID;
            Pendingconnections[Socket] = Hostname;
            Portmap[nVirtualPort] = Socket;
            return Socket & 0xFFFFFFFF;
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
            for (auto &Item : bySteamID(steamIDRemote))
            {
                closesocket(Item.Comsocket);
                Item.Comsocket = 0;
            }

            return true;
        }
        bool CloseP2PChannelWithUser(SteamID_t steamIDRemote, int32_t iVirtualPort)
        {
            if (0 == iVirtualPort) return CloseP2PSessionWithUser(steamIDRemote);
            if (!Portmap.contains(iVirtualPort)) return false;
            auto &Socket = Portmap[iVirtualPort];

            for (auto &Item : bySteamID(steamIDRemote))
            {
                if (Socket == Item.Comsocket)
                {
                    closesocket(Item.Comsocket);
                    Item.Comsocket = 0;
                    Socket = 0;
                    break;
                }
            }

            return true;
        }
        bool DestroyListenSocket(SNetListenSocket_t hSocket, bool bNotifyRemoteEnd)
        {
            size_t Old = 0;

            for (auto &Socket : Listensockets)
            {
                if (hSocket == (Socket & 0xFFFFFFFF))
                {
                    closesocket(Socket);
                    Old = Socket;
                    break;
                }
            }

            Listensockets.erase(Old);
            return true;
        }
        bool DestroySocket(SNetSocket_t hSocket, bool bNotifyRemoteEnd)
        {
            for (auto &Item : byCom(hSocket))
            {
                closesocket(Item.Comsocket);
                Item.Comsocket = 0;
            }

            return true;
        }
        bool GetListenSocketInfo(SNetListenSocket_t hListenSocket, uint32_t *pnIP, uint16_t *pnPort)
        {
            sockaddr_in Clientinfo{ AF_INET }; int Size = sizeof(Clientinfo);

            for (const auto &Socket : Listensockets)
            {
                if (hListenSocket == (Socket & 0xFFFFFFFF))
                {
                    if (0 == getsockname(Socket, (sockaddr *)&Clientinfo, &Size))
                    {
                        *pnIP = ntohl(Clientinfo.sin_addr.S_un.S_addr);
                        *pnPort = ntohs(Clientinfo.sin_port);
                        return true;
                    }
                }
            }

            return false;
        }
        bool GetP2PSessionState(SteamID_t steamIDRemote, P2PSessionState_t *pConnectionState)
        {
            for (const auto &Item : bySteamID(steamIDRemote))
            {
                int32_t Totalbytes = 0;
                for (const auto &Packet : Item.Incoming)
                    Totalbytes += Packet.Size;

                pConnectionState->m_nRemoteIP = ntohl(Item.IPv4Address.sin_addr.S_un.S_addr);
                pConnectionState->m_nPacketsQueuedForSend = (int32_t)Item.Outgoing.size();
                pConnectionState->m_nRemotePort = ntohs(Item.IPv4Address.sin_port);
                pConnectionState->m_nBytesQueuedForSend = Totalbytes;
                pConnectionState->m_bConnectionActive = 1;
                pConnectionState->m_eP2PSessionError = 0;
                pConnectionState->m_bUsingRelay = false;
                pConnectionState->m_bConnecting = 0;
                return true;
            }

            *pConnectionState = {};
            return true;
        }
        bool GetSocketInfo(SNetSocket_t hSocket, SteamID_t *pSteamIDRemote, ESNetSocketState *peSocketStatus, uint32_t *punIPRemote, uint16_t *punPortRemote)
        {
            for (const auto &Item : byCom(hSocket))
            {
                *punIPRemote = ntohl(Item.IPv4Address.sin_addr.S_un.S_addr);
                *punPortRemote = ntohs(Item.IPv4Address.sin_port);
                *peSocketStatus = k_ESNetSocketStateConnected;
                *pSteamIDRemote = Item.SteamID;
                return true;
            }

            return false;
        }
        bool IsDataAvailable(SNetListenSocket_t hListenSocket, uint32_t *pcubMsgSize, SNetSocket_t *phSocket)
        {
            for (const auto &Item : byListen(hListenSocket))
            {
                if (Item.Incoming.empty()) continue;;
                *pcubMsgSize = Item.Incoming.front().Size;
                *phSocket = (Item.Comsocket & 0xFFFFFFFF);
                return true;
            }

            return false;
        }
        bool IsDataAvailableOnSocket(SNetSocket_t hSocket, uint32_t *pcubMsgSize)
        {
            for (const auto &Item : bySocket(hSocket))
            {

                if (Item.Incoming.empty()) continue;
                *pcubMsgSize = Item.Incoming.front().Size;
                return true;
            }

            return false;
        }
        bool IsP2PPacketAvailable0(uint32_t *pcubMsgSize)
        {
            return IsP2PPacketAvailable1(pcubMsgSize, 0);
        }
        bool IsP2PPacketAvailable1(uint32_t *pcubMsgSize, int32_t iVirtualPort)
        {
            if (!Portmap.contains(iVirtualPort)) [[unlikely]] return false;
            return IsDataAvailableOnSocket(Portmap[iVirtualPort] & 0xFFFFFFFF, pcubMsgSize);
        }
        bool ReadP2PPacket0(void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SteamID_t *psteamIDRemote)
        {
            for (auto &Item : Endpoints)
            {
                if (Item.Incoming.empty()) continue;

                const auto &Packet = Item.Incoming.front();
                const auto Min = std::min(cubDest, Packet.Size);

                std::memcpy(pubDest, Packet.Data.data(), Min);
                *psteamIDRemote = Item.SteamID;
                *pcubMsgSize = Min;

                Item.Incoming.pop_front();
                return true;
            }

            return false;
        }
        bool ReadP2PPacket1(void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SteamID_t *psteamIDRemote, int32_t iVirtualPort)
        {
            if (0 == iVirtualPort) return ReadP2PPacket0(pubDest, cubDest, pcubMsgSize, psteamIDRemote);
            if (!Portmap.contains(iVirtualPort)) [[unlikely]] return false;

            for (auto &Item : bySocket(Portmap[iVirtualPort]))
            {
                if (Item.Incoming.empty()) continue;

                const auto &Packet = Item.Incoming.front();
                const auto Min = std::min(cubDest, Packet.Size);

                std::memcpy(pubDest, Packet.Data.data(), Min);
                *psteamIDRemote = Item.SteamID;
                *pcubMsgSize = Min;

                Item.Incoming.pop_front();
                return true;
            }

            return false;
        }
        bool RetrieveData1(SNetListenSocket_t hListenSocket, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize, SNetSocket_t *phSocket)
        {
            for (auto &Item : byListen(hListenSocket))
            {
                if (!Item.Incoming.empty())
                {
                    const auto &Packet = Item.Incoming.front();
                    const auto Min = std::min(cubDest, Packet.Size);

                    std::memcpy(pubDest, Packet.Data.data(), Min);
                    *phSocket = Item.Comsocket & 0xFFFFFFFF;
                    *pcubMsgSize = Min;

                    Item.Incoming.pop_front();
                    return true;
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
            for (auto &Item : byCom(hSocket))
            {
                if (!Item.Incoming.empty())
                {
                    const auto &Packet = Item.Incoming.front();
                    const auto Min = std::min(cubDest, Packet.Size);

                    std::memcpy(pubDest, Packet.Data.data(), Min);
                    *pcubMsgSize = Min;

                    Item.Incoming.pop_front();
                    return true;
                }
            }

            return false;
        }
        bool SendDataOnSocket(SNetSocket_t hSocket, void *pubData, uint32_t cubData, bool bReliable)
        {
            for (auto &Item : byCom(hSocket))
            {
                Item.Outgoing.push_back({ cubData, { (char *)pubData, cubData } });
                return true;
            }

            return false;
        }
        bool SendP2PPacket0(SteamID_t steamIDRemote, const void *pubData, uint32_t cubData, EP2PSend eP2PSendType)
        {
            for (auto &Item : bySteamID(steamIDRemote))
            {
                Item.Outgoing.push_back({ cubData, { (char *)pubData, cubData } });
                return true;
            }

            return false;
        }
        bool SendP2PPacket1(SteamID_t steamIDRemote, const void *pubData, uint32_t cubData, EP2PSend eP2PSendType, int32_t iVirtualPort)
        {
            if (0 == iVirtualPort) return SendP2PPacket0(steamIDRemote, pubData, cubData, eP2PSendType);
            if (!Portmap.contains(iVirtualPort)) return false;

            for (auto &Item : byCom(Portmap[iVirtualPort] & 0xFFFFFFFF))
            {
                Item.Outgoing.push_back({ cubData, { (char *)pubData, cubData } });
                return true;
            }

            return false;
        }

        int32_t GetMaxPacketSize(SNetSocket_t hSocket)
        {
            return Maxpacketsize;
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
