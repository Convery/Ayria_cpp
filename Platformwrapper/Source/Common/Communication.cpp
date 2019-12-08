/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#include "Common.hpp"

// LAN communication subsystem.
namespace Communication
{
    // Packet structure: ["MEEP"][Base64 JSON{ Origin = SessionID, Event = Type, Payload = Base64 }]
    std::unordered_map<uint64_t, std::shared_ptr<Node_t>> Externalnodes{};
    std::unordered_map<std::string, Eventcallback_t> *Eventhandlers{};
    std::shared_ptr<Node_t> Localhost{};
    size_t Socket{ INVALID_SOCKET };
    uint16_t Broadcastport{};

    // For internal use, get our Node_t.
    std::shared_ptr<Node_t> getLocalhost()
    {
        // Initialize if needed.
        if (!Localhost)
        {
            Node_t Local{};
            Local.Address = 0x7F000001;

            // Pseudo-random for LAN..
            srand(time(nullptr) & 0xFEFEFEFE);
            Local.SessionID = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            for (int i = 1; i < 7; i = rand() % 8 + 1) ((uint8_t *)&Local.SessionID)[i] ^= ((uint8_t *)&Local.SessionID)[i - 1];

            Localhost = std::make_shared<Node_t>(std::move(Local));
        }

        return Localhost;
    }

    // Callbacks on events, fuzzy matching on the event name.
    void addEventhandler(const std::string &Event, Eventcallback_t Callback)
    {
        if (!Eventhandlers) Eventhandlers = new std::remove_pointer_t<decltype(Eventhandlers)>();
        (*Eventhandlers)[Event] = Callback;
    }

    // Push a message to all LAN clients.
    void Broadcast(const std::string Eventname, std::string &&Payload)
    {
        auto Message = nlohmann::json::object();
        Message["Payload"] = Base64::isValid(Payload) ? Payload : Base64::Encode(Payload);
        Message["Origin"] = getLocalhost()->SessionID;
        Message["Event"] = Eventname;

        // Windows does not like to send partial messages, so prefix the identifier.
        const auto Packet = Base64::Encode(Message.dump()).insert(0, "MEEP", 4);

        // Ask the OS nicely to pass the payload around to all clients.
        const SOCKADDR_IN Address{ AF_INET, htons(Broadcastport), {{.S_addr = INADDR_BROADCAST}} };
        sendto(Socket, Packet.data(), uint32_t(Packet.size()), NULL, (SOCKADDR *)&Address, sizeof(Address));
    }

    // Spawn a thread that polls for packets in the background.
    void Initialize(const uint16_t Port)
    {
        static unsigned long Argument{ 1 };
        static void *Threadhandle{};
        if (Threadhandle) return;
        Broadcastport = Port;

        #if defined(_WIN32) // Windows needs initialization and termination.
        std::atexit([]() { TerminateThread(Threadhandle, 0); });
        WSAData Unused; WSAStartup(MAKEWORD(2, 2), &Unused);
        #endif

        // Would have preferred IPX, but UDP will have to do for now.
        Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (Socket == INVALID_SOCKET) return;

        // Enable broadcasting and address reuse so developers can run multiple instances on one machine.
        setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (char *)&Argument, sizeof(Argument));
        setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
        ioctlsocket(Socket, FIONBIO, &Argument);

        // Only listen on the specified port.
        const SOCKADDR_IN Address{ AF_INET, htons(Broadcastport) };
        bind(Socket, (SOCKADDR *)&Address, sizeof(Address));

        // Sleeps through select().
        auto Thread = std::thread([&]()
        {
            constexpr size_t Buffersize{ 4096 };
            const timeval Timeout{ 1, NULL };
            char Buffer[Buffersize];

            while (true)
            {
                FD_SET ReadFD{};
                FD_SET(Socket, &ReadFD);

                /*
                    NOTE(tcn): Not portable!!one!
                    POSIX select timeout is not const and nfds should not be NULL.
                */
                if (select(NULL, &ReadFD, NULL, NULL, &Timeout))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
                    Buffer[recvfrom(Socket, Buffer, Buffersize, 0, (SOCKADDR *)&Client, &Clientsize)] = '\0';

                    // Verify the identifier so we know the packet is for us.
                    if (*(uint32_t *)Buffer != *(uint32_t *)"MEEP") continue;

                    // nlohmann may throw.
                    try
                    {
                        auto Message = nlohmann::json::parse(Base64::Decode(Buffer + sizeof(uint32_t)));
                        const auto Event = Message.value("Event", std::string());
                        const auto Origin = Message.value("Origin", uint64_t());
                        auto Payload = Message.value("Payload", std::string());

                        // We receive our own broadcasts as well on Windows.
                        if (getLocalhost()->SessionID == Origin) continue;
                        if (Origin == 0) continue;

                        // Verify that we can support the other nodes version.
                        const auto Handler = Eventhandlers->find(Event);
                        if (Handler == Eventhandlers->end()) continue;

                        // Verify the IP to prevent fuckery.
                        auto &Node = Externalnodes[Origin];
                        if (!Node) Node = std::make_shared<Node_t>();
                        if (Node->Address && Node->Address != Client.sin_addr.s_addr) return;

                        // Ensure that the node is properly initialized and up to date.
                        Node->Timestamp = uint32_t(time(NULL) - Ayria::Global.Startuptimestamp);
                        Node->Address = Client.sin_addr.s_addr;
                        Node->SessionID = Origin;

                        // Forward to the handler and continue polling.
                        Handler->second(Node, std::move(Payload));
                    } catch (std::exception & e) { Debugprint(va("%s: %s", __FUNCTION__, e.what())); (void)e; }
                }
            }
        });

        // Windows needs the native handle to terminate the thread.
        Threadhandle = Thread.native_handle(); Thread.detach();
    }
}
