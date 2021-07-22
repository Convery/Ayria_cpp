/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#include "AABackend.hpp"

// Totally randomly selected constants here..
constexpr uint32_t Syncaddress = Hash::FNV1_32("Ayria") << 8;   // 228.58.137.0
constexpr uint16_t Syncport = Hash::FNV1_32("Ayria") & 0xFFFF;  // 14985
constexpr auto Buffersizelimit = 4096;                          // A single virtual page.

#pragma pack(push, 1)
struct Packet_t
{
    uint32_t SessionID;
    uint32_t Messagetype;
    uint32_t TransactionID;
};
#pragma pack(pop)

namespace Networking
{
    static sockaddr_in Multicast{ AF_INET, htons(Syncport), {{.S_addr = htonl(Syncaddress)}} };
    static size_t Sendersocket{}, Receiversocket{};
    static std::atomic<uint32_t> MessageID{};
    static uint32_t LocalsessionID{};

    // Track the account ID to make it easier for the services that need it.
    static Hashmap<uint32_t /* SessionID */, uint32_t /* AccountID */> Localclientmap{};
    void Associateclient(uint32_t SessionID, uint32_t AccountID) { Localclientmap[SessionID] = AccountID; }

    // Register handlers for the different packets.
    static Hashmap<uint32_t /* Messagetype */, Hashset<Callback_t>> Messagehandlers{};
    void Register(std::string_view Identifier, Callback_t Handler)
    {
        return Register(Hash::WW32(Identifier), Handler);
    }
    void Register(uint32_t Identifier, Callback_t Handler)
    {
        Messagehandlers[Identifier].insert(Handler);
    }

    // Broadcast a message to the LAN clients.
    void Publish(std::string_view Identifier, std::string_view Payload, bool isTransient)
    {
        return Publish(Hash::WW32(Identifier), Payload, isTransient);
    }
    void Publish(uint32_t Identifier, std::string_view Payload, bool isTransient)
    {
        // There is no network available at this time.
        if (false) [[unlikely]] return;

        // Developers should know better than to send 4KB as a single packet.
        const auto Totalsize = static_cast<int>(10 + Payload.size());
        assert(Totalsize < Buffersizelimit);

        const auto isB64 = Base64::isValid(Payload);
        const auto Buffersize = sizeof(Packet_t) + (isB64 ? Payload.size() : Base64::Encodesize(Payload.size()));
        auto Buffer = alloca(Buffersize);

        // Basic header.
        static_cast<Packet_t *>(Buffer)->TransactionID = isTransient ? 0 : ++MessageID;
        static_cast<Packet_t *>(Buffer)->SessionID = LocalsessionID;
        static_cast<Packet_t *>(Buffer)->Messagetype = Identifier;

        // Base64 payload.
        if (isB64) std::memcpy(static_cast<char *>(Buffer) + sizeof(Packet_t), Payload.data(), Payload.size());
        else
        {
            const auto B64 = Base64::Encode(Payload);
            std::memcpy(static_cast<char *>(Buffer) + sizeof(Packet_t), B64.data(), B64.size());
        }

        sendto(Sendersocket, static_cast<char *>(Buffer), Buffersize, NULL, (sockaddr *)&Multicast, sizeof(Multicast));
    }

    // Drop packets from bad clients on the floor.
    static Hashset<uint32_t> Blacklistedsessions{};
    void Blockclient(uint32_t SessionID) { Blacklistedsessions.insert(SessionID); }
    static bool isBlocked(uint32_t SessionID) { return Blacklistedsessions.contains(SessionID); }

    // Incase a plugin needs to know the address in the future.
    static std::unordered_map<uint32_t, IN_ADDR> Peerlist{};
    IN_ADDR getPeeraddress(uint32_t SessionID)
    {
        if (Peerlist.contains(SessionID)) [[likely]]
            return Peerlist[SessionID];
        return {};
    }

    // Save the packet for later synchronization.
    static void Logpacket(const Packet_t *Header, std::string_view Payload)
    {
        // We don't know where to save this packet yet.
        if (!Localclientmap.contains(Header->SessionID)) [[unlikely]] return;
        const auto AccountID = Localclientmap[Header->SessionID];

        // Insert the message into the database.
        // AccountID, Messagetype, TxID, B64Payload.
    }

    // Fetch as many messages as possible from the network.
    static void __cdecl Pollnetwork()
    {
        // There is no network available at this time.
        if (false) [[unlikely]] return;

        // Check for data on the active socket.
        {
            FD_SET ReadFD{}; FD_SET(Receiversocket, &ReadFD);
            constexpr timeval Defaulttimeout{ NULL, 1 };
            auto Timeout{ Defaulttimeout };
            auto Count{ 1 };

            if (!select(Count, &ReadFD, nullptr, nullptr, &Timeout)) [[likely]] return;
        }

        // Allocate packet-data on the stack rather than heap as it's only 4Kb.
        const auto Buffer = alloca(Buffersizelimit);

        // Get all available packets.
        while (true)
        {
            sockaddr_in Clientaddress; int Addresslen = sizeof(Clientaddress);
            const auto Packetlength = recvfrom(Receiversocket, (char *)Buffer, Buffersizelimit, NULL, (sockaddr *)&Clientaddress, &Addresslen);
            if (Packetlength < static_cast<int>(sizeof(Packet_t))) break;  // Also covers any errors.

            // Some basic sanitychecking..
            const auto Header = static_cast<Packet_t *>(Buffer);
            if (Header->SessionID == 0) [[unlikely]] continue;                          // Borked client.
            if (isBlocked(Header->SessionID)) [[likely]] continue;                      // Ignored session, likely our own.
            if (!Messagehandlers.contains(Header->Messagetype)) [[unlikely]] continue;  // Not a packet we know how to handle.

            // Basic sanity checking before processing the packet, if they can't even format the payload we don't care.
            const auto Payload = std::string_view(static_cast<char *>(Buffer) + sizeof(Packet_t), Packetlength - sizeof(Packet_t));
            if (!Base64::isValid(Payload)) [[unlikely]] return;

            // Save the message and address for later synchronization.
            if (Header->TransactionID) Logpacket(Header, Payload);
            Peerlist[Header->SessionID] = Clientaddress.sin_addr;

            // Decode the incomming message and forward it to the handler(s).
            const auto Decoded = Base64::Decode_inplace(const_cast<char *>(Payload.data()), Payload.size());
            const Nodeinfo_t Node{ .SessionID = Header->SessionID, .AccountID = Localclientmap[Header->SessionID] };

            // Let the compiler decide how to vectorize this..
            std::ranges::for_each(Messagehandlers[Header->Messagetype],
                                  [&](const auto &CB) { if (CB) [[likely]] CB(Node, Decoded.data(), Decoded.size()); });
        }
    }

    // Initialize the system.
    void Initialize()
    {
        WSADATA Unused;
        unsigned long Error{ 0 };
        unsigned long Argument{ 1 };
        const ip_mreq Request{ {{.S_addr = htonl(Syncaddress)}} };
        const sockaddr_in Localhost{ AF_INET, htons(Syncport), {{.S_addr = htonl(INADDR_ANY)}} };

        // We only need WS 1.1, no need to load more modules.
        WSAStartup(MAKEWORD(1, 1), &Unused);
        Sendersocket = socket(AF_INET, SOCK_DGRAM, 0);
        Receiversocket = socket(AF_INET, SOCK_DGRAM, 0);
        Error |= ioctlsocket(Receiversocket, FIONBIO, &Argument);

        // TODO(tcn): Investigate if WS2's IP_ADD_MEMBERSHIP (12) gets mapped to the WS1's original (5) internally.
        // Join the multicast group, reuse address if multiple clients are on the same PC (mainly for developers).
        Error |= setsockopt(Receiversocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Request, sizeof(Request));
        Error |= setsockopt(Receiversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        Error |= bind(Receiversocket, (sockaddr *)&Localhost, sizeof(Localhost));

        // TODO(tcn): Proper error handling.
        if (Error) [[unlikely]]
        {
            //Global.Settings.noNetworking = true;
            closesocket(Receiversocket);
            closesocket(Sendersocket);
            assert(false);
            return;
        }

        // Make a unique identifier for our session.
        LocalsessionID = Hash::WW32(GetTickCount64() ^ Sendersocket);

        // Ensure that we don't process our own packets.
        Blacklistedsessions.insert(LocalsessionID);

        // Should only need to check for packets 5 times a second.
        //Backend::Enqueuetask(200, Pollnetwork);
        //Global.Settings.noNetworking = false;
    }

    #include <openssl/bio.h>
    // Send a request and return the async response.
    static Response_t doRequest(std::string &&URL, std::string &&Data)
    {
        std::string Host{}, URI{}, Port{};
        Response_t Result{ 500 };
        std::smatch Match{};

        // Ensure proper formatting.
        if (!URL.starts_with("http")) URL.insert(0, "http://");

        // Extract the important parts from the URL.
        static const std::regex RX("\\/\\/([^\\/:\\s]+):?(\\d+)?(.*)", std::regex_constants::optimize);
        if (!std::regex_match(URL, Match, RX) || Match.size() < 3) return Result;
        Host = Match[1].str();
        Port = Match[2].str();
        URI = Match[3].str();

        // Fallback if it's not available in the URL.
        const bool isHTTPS = URL.starts_with("https");
        if (Port.empty()) Port = isHTTPS ? "433" : "80";
        if (URI.empty()) URI = "/";

        // Totally legit HTTP..
        const auto Request = va("%s %s HTTP1.1\r\nContent-length: %zu\r\n\r\n%s",
                                Data.empty() ? "GET" : "POST", URI.c_str(),
                                Data.size(), Data.empty() ? "" : Data.c_str());

        // Common networking.
        addrinfo *Resolved{};
        HTTPResponse_t Parser{};
        auto Buffer = alloca(4096);

        addrinfo Hint{ .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM };
        if (0 != getaddrinfo(Host.c_str(), Port.c_str(), &Hint, &Resolved)) return Result;

        const auto Socket = socket(Resolved->ai_family, Resolved->ai_socktype, Resolved->ai_protocol);
        if (0 != connect(Socket, Resolved->ai_addr, Resolved->ai_addrlen))
        {
            freeaddrinfo(Resolved);
            closesocket(Socket);
            return Result;
        }

        // SSL context needed.
        if (isHTTPS)
        {
            auto Context = SSL_CTX_new(TLS_method());
            SSL_CTX_set_max_proto_version(Context, TLS1_3_VERSION);

            auto BIO = BIO_new_socket(Socket, BIO_CLOSE);
            auto State = SSL_new(Context);
            SSL_set_bio(State, BIO, BIO);
            SSL_connect(State);

            RETRY_HANDSHAKE: auto Return = SSL_read(State, Buffer, 4096);
            if (Return <= 0)
            {
                if (SSL_ERROR_WANT_READ != SSL_get_error(State, Return))
                {
                    freeaddrinfo(Resolved);
                    closesocket(Socket);
                    return Result;
                }

                goto RETRY_HANDSHAKE;
            }

            RETRY_SEND: Return = SSL_write(State, Request.data(), Request.size());
            if (Return <= 0)
            {
                if (SSL_ERROR_WANT_WRITE != SSL_get_error(State, Return))
                {
                    freeaddrinfo(Resolved);
                    closesocket(Socket);
                    return Result;
                }

                goto RETRY_SEND;
            }

            while (true)
            {
                Return = SSL_read(State, Buffer, 4096);
                if (Return < 0)
                {
                    if (SSL_ERROR_WANT_READ != SSL_get_error(State, Return))
                    {
                        freeaddrinfo(Resolved);
                        closesocket(Socket);
                        return Result;
                    }

                    continue;
                }

                if (0 == Return)
                {
                    Result.Statuscode = Parser.Statuscode;
                    Result.Body = Parser.Body;
                    freeaddrinfo(Resolved);
                    closesocket(Socket);
                    return Result;
                }

                Parser.Parse(std::string_view((char *)Buffer, Return));
                if (Parser.isBad) break;
            }
        }
        else
        {
            auto Return = send(Socket, Request.data(), Request.size(), NULL);
            if (Return <= 0)
            {
                freeaddrinfo(Resolved);
                closesocket(Socket);
                return Result;
            }

            while (true)
            {
                Return = recv(Socket, (char *)Buffer, 4096, NULL);
                if (Return < 0) break;

                if (0 == Return)
                {
                    Result.Statuscode = Parser.Statuscode;
                    Result.Body = Parser.Body;
                    freeaddrinfo(Resolved);
                    closesocket(Socket);
                    return Result;
                }

                Parser.Parse(std::string_view((char *)Buffer, Return));
                if (Parser.isBad) break;
            }
        }

        freeaddrinfo(Resolved);
        closesocket(Socket);
        return Result;
    }
    std::future<Response_t> GET(std::string URL)
    {
        return std::async(std::launch::async, doRequest, std::move(URL), std::string());
    }
    std::future<Response_t> POST(std::string URL, std::string Data)
    {
        return std::async(std::launch::async, doRequest, std::move(URL), std::move(Data));
    }
}
