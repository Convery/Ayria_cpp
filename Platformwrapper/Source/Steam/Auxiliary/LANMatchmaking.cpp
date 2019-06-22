/*
    Initial author: Convery (tcn@ayria.se)
    Started: 20-06-2019
    License: MIT
*/

#include "../Steam.hpp"
#include <WinSock2.h>
#include <Objbase.h>

// GUID serialization.
void to_json(nlohmann::json &JSON, const GUID &ID)
{
    JSON = va("%lu-%hu-%hu-%llu", ID.Data1, ID.Data2, ID.Data3, *(uint64_t *)ID.Data4);
}
void from_json(const nlohmann::json &JSON, GUID &ID)
{
    std::sscanf(JSON.get<std::string>().c_str(), "%lu-%hu-%hu-%llu", &ID.Data1, &ID.Data2, &ID.Data3, (uint64_t *)& ID.Data4);
}

// To use GUID as a key.
namespace std
{
    template<> struct hash<GUID>
    {
        size_t operator()(const GUID &Value) const
        {
            return Hash::FNV1a_32(&Value, sizeof(Value));
        }
    };
}

namespace LANMatchmaking
{
    std::unordered_map<GUID, Message_t> Messagequeue{};
    std::vector<std::shared_ptr<Server_t>> Servers;
    std::shared_ptr<Server_t> Localserver;
    size_t Socket{ INVALID_SOCKET };
    SessionID_t Localsession{};
    std::mutex Threadguard;
    uint32_t GIdentifier;

    void Broadcast(std::string &&Eventtype, std::string &&Data)
    {
        // Create the message in JSON so developers can read it.
        auto Message = nlohmann::json::object();
        Message["Event"] = std::move(Eventtype);
        Message["SessionID"] = Localsession;

        // We only allow base64 data to be sent..
        if (Base64::Validate(Data)) Message["Data"] = std::move(Data);
        else Message["Data"] = Base64::Encode(Data);

        // Windows does not like to send partial messages, so prefix the identifier.
        auto Payload = Message.dump().insert(0, (const char *)&GIdentifier, sizeof(uint32_t));

        // Broadcast address.
        SOCKADDR_IN Address{};
        Address.sin_family = AF_INET;
        Address.sin_port = htons(0x1234);
        Address.sin_addr.S_un.S_addr = INADDR_BROADCAST;

        // Forward to the OS.
        sendto(Socket, Payload.data(), uint32_t(Payload.size()), NULL, (SOCKADDR *)&Address, sizeof(Address));
    }
    std::vector<std::shared_ptr<Server_t>> Listservers()
    {
        // Take ownership of the message-queue.
        std::unordered_map<GUID, Message_t> Localqueue{};
        Threadguard.lock(); { Messagequeue.swap(Localqueue); } Threadguard.unlock();

        // Process any new requests into the server-list.
        for(const auto &[SessionID, Message] : Localqueue)
        {
            // Early cleanup rather than waiting for timeouts.
            if(Message.Eventtype == "Endsession"s)
            {
                auto Server = std::find_if(Servers.begin(), Servers.end(), [&](auto &A) { return SessionID == A->SessionID; });
                if (Server != Servers.end()) Servers.erase(Server);
                Debugprint("Endsession");
                continue;
            }

            try
            {
                // Parse the payload-data as JSON, for some reason std::string doesn't decay.
                auto Object = nlohmann::json::parse(Base64::Decode(Message.Data).c_str());

                // We receive our own sessions update as well, to help developers.
                if (Localsession == SessionID) continue;

                RETRY: // Find the server to update or create a new one.
                auto Server = std::find_if(Servers.begin(), Servers.end(), [&](auto &A) { return SessionID == A->SessionID; });
                if (Server == Servers.end())
                {
                    Server_t Newserver;
                    Newserver.SessionID = SessionID;
                    Newserver.Hostaddress = Message.Sender;
                    Servers.push_back(std::make_shared<Server_t>(Newserver));

                    Debugprint(va("Found new LAN server"));
                    goto RETRY;
                }

                // Highly advanced protection against fuckery here..
                if (!std::strstr((*Server)->Hostaddress.c_str(), Message.Sender.c_str()))
                {
                    Debugprint("Invalid hostaddress");
                    continue;
                }

                // Just mindlessly overwrite the servers info.
                for (const auto &[Key, Value] : Object.items())
                {
                    (*Server)->Gamedata.emplace(Key, Value);
                }

                // We should update this every ~15 sec.
                (*Server)->Lastmessage = time(nullptr);
            }
            catch (std::exception &e) { Debugprint("Parse error"); (void)e; }
        }

        LOOP: // Cleanup of outdated servers.
        for (auto Iterator = Servers.begin(); Iterator != Servers.end(); ++Iterator)
        {
            // Let's not assume that time64_t is UNIX-time.
            if (difftime(time(nullptr), (*Iterator)->Lastmessage) > 35)
            {
                Debugprint("Server cleanup");
                Servers.erase(Iterator);
                goto LOOP;
            }
        }

        return Servers;
    }
    std::thread Initialize(uint32_t Identifier)
    {
        GIdentifier = Identifier;
        return std::thread([]()
        {
            WSAData Unused;
            SOCKADDR_IN Address{};
            unsigned long Argument = TRUE;
            const timeval Timeout{ 5, NULL };
            WSAStartup(MAKEWORD(2, 2), &Unused);

            // Would have preferred IPX, but UDP will have to do for now.
            Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (Socket == INVALID_SOCKET) return;

            // Enable broadcasting and address reuse so developers can run multiple instances on one machine.
            setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (char *)&Argument, sizeof(Argument));
            setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&Argument, sizeof(Argument));
            ioctlsocket(Socket, FIONBIO, &Argument);

            // Only listen on our port.
            Address.sin_family = AF_INET;
            Address.sin_port = htons(0x1234);
            bind(Socket, (SOCKADDR *)&Address, sizeof(Address));

            // Create a nice little buffer on the stack.
            constexpr size_t Buffersize{ 8192 };
            char Buffer[Buffersize]{};
            FD_SET ReadFD;

            while(true)
            {
                FD_ZERO(&ReadFD);
                FD_SET(Socket, &ReadFD);

                // NOTE(tcn): Not POSIX compatible.
                if (select(NULL, &ReadFD, nullptr, nullptr, &Timeout))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };

                    while(true)
                    {
                        const auto Length = recvfrom(Socket, Buffer, Buffersize, 0, (SOCKADDR *)&Client, &Clientsize);
                        if (Length == 0 || Length == SOCKET_ERROR) break;
                        Buffer[Length] = '\0';

                        // Is this packet for us to play with?
                        if (*(uint32_t *)Buffer != GIdentifier) continue;

                        try // Try to parse the message.
                        {
                            Message_t Message;

                            auto Object = nlohmann::json::parse(Buffer + sizeof(uint32_t));
                            Message.SessionID = Object.value("SessionID", GUID());
                            Message.Eventtype = Object.value("Event", "");
                            Message.Sender = inet_ntoa(Client.sin_addr);
                            Message.Data = Object.value("Data", "");

                            Threadguard.lock();
                            {
                                Messagequeue[Message.SessionID] = std::move(Message);
                            }
                            Threadguard.unlock();
                        }
                        catch (std::exception &e) { (void)e; }
                    }
                }

                // Check if we should broadcast our status.
                if(Localserver)
                {
                    // Let's not assume that time64_t is UNIX-time.
                    if(difftime(time(nullptr), Localserver->Lastmessage) > 15)
                    {
                        Updateserver();
                    }
                }
            }
        });
    }
    std::shared_ptr<Server_t> Createserver()
    {
        // Do we already have a server?
        if (Localserver) return Localserver;

        // TODO(tcn): Replace localhost with LAN address.
        Localserver = std::make_shared<Server_t>(Server_t());
        Localserver->Lastmessage = time(nullptr);
        Localserver->SessionID = Createsession();
        return Localserver;
    }
    SessionID_t Createsession()
    {
        // If we already have a session, we return that.
        if (Localsession.Data1) return Localsession;

        // If this fails, we have bigger issues than proper randomness.
        if (CoCreateGuid(&Localsession))
        {
            srand(time(nullptr) & 0xFFFFEFFF);
            for (int i = 0; i < 4; ++i) ((uint32_t *)&Localsession)[i] = rand();
            for (int i = 1; i < 15; i = rand() % 16 + 1) ((uint8_t *)&Localsession)[i] ^= ((uint8_t *)&Localsession)[i - 1];
        }

        // Ensure that we have some bit set.
        Localsession.Data1 |= 1 << 31;
        return Localsession;
    }
    void Terminatesession()
    {
        Broadcast("Endsession", "");
        Localsession.Data1 = 0;
    }
    void Updateserver()
    {
        Traceprint();
        Localserver->Lastmessage = time(nullptr);
        Broadcast("Serverupdate", Localserver->Gamedata.dump());
    }
}
