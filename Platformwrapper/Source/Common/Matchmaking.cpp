/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-08-04
    License: MIT

    LAN Matchmaking-broadcasts over UDP.
*/

#include "Matchmaking.hpp"
#include <Objbase.h>

namespace Matchmaking
{
    // Internal properties.
    std::vector<std::shared_ptr<Server_t>> Servers;
    std::shared_ptr<Server_t> Localhost{};
    uint32_t Identifier{ 0xFFFFFFFF };

    // Fetches the local server, or creates a new one.
    std::shared_ptr<Server_t> Localserver()
    {
        if(Localhost) return Localhost;

        // TODO(tcn): Fill the hostname property with our address.
        Localhost = std::make_shared<Server_t>(Server_t());
        Localhost->Lastmessage = time(nullptr);

        // Proper unique ID.
        GUID SecureID;

        // If this fails, we have bigger issues than proper randomness.
        if (CoCreateGuid(&SecureID))
        {
            srand(time(nullptr) & 0xFFFFEFFF);
            for (int i = 0; i < 4; ++i) ((uint32_t *)&SecureID)[i] = rand();
            for (int i = 1; i < 15; i = rand() % 16 + 1) ((uint8_t *)&SecureID)[i] ^= ((uint8_t *)&SecureID)[i - 1];
        }

        // It doesn't have to be too unique so we discard 64bits (may use the full later).
        Localhost->SessionID = Hash::FNV1_64(&SecureID, sizeof(SecureID));

        return Localhost;
    }

    // A list of all currently active servers on the net.
    std::vector<std::shared_ptr<Server_t>> Externalservers()
    {
        LOOP: // Do some cleanup of outdated servers.
        for(auto Iterator = Servers.begin(); Iterator != Servers.end(); ++Iterator)
        {
            // Let's not assume that time64_t is UNIX-time.
            if(difftime(time(nullptr), (*Iterator)->Lastmessage) > 35)
            {
                Servers.erase(Iterator);
                goto LOOP;
            }
        }

        return Servers;
    }

    // Listen for new broadcasted messages.
    std::thread Startlistening(uint32_t GameID)
    {
        Identifier = GameID;
        return std::thread([]()
        {
            WSAData Unused;
            SOCKADDR_IN Address{};
            unsigned long Argument = TRUE;
            const timeval Timeout{ 2, NULL };
            WSAStartup(MAKEWORD(2, 2), &Unused);

            // Would have preferred IPX, but UDP will have to do for now.
            const auto Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(Socket == INVALID_SOCKET) return;

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

            // Poll forever.
            while(true)
            {
                FD_ZERO(&ReadFD);
                FD_SET(Socket, &ReadFD);

                // NOTE(tcn): Not POSIX compatible.
                if(select(NULL, &ReadFD, nullptr, nullptr, &Timeout))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };

                    while(true)
                    {
                        const auto Length = recvfrom(Socket, Buffer, Buffersize, 0, (SOCKADDR *)&Client, &Clientsize);
                        if (Length == 0 || Length == SOCKET_ERROR) break;
                        Buffer[Length] = '\0';

                        // Is this packet for us to play with?
                        if (*(uint32_t *)Buffer != Identifier) continue;

                        try
                        {
                            // Try to parse the message after the identifier, may extend in the future.
                            auto Object = nlohmann::json::parse(Buffer + sizeof(uint32_t));
                            uint64_t SessionID = Object.value("SessionID", uint64_t());
                            std::string Event = Object.value("Event", "None");
                            std::string Payload = Object.value("Data", "");

                            // We receive our own sessions update as well, to help developers.
                            if(Localhost && Localhost->SessionID == SessionID) continue;

                            RETRY: // Find the server to update or create a new one.
                            auto Server = std::find_if(Servers.begin(), Servers.end(), [&](auto &A) { return SessionID == A->SessionID; });
                            if(Server == Servers.end())
                            {
                                Server_t Newserver{};
                                Newserver.SessionID = SessionID;
                                Newserver.Hostname = inet_ntoa(Client.sin_addr);
                                Servers.push_back(std::make_shared<Server_t>(Newserver));
                                goto RETRY;
                            }

                            // Highly advanced protection against fuckery here..
                            if(!std::strstr((*Server)->Hostname.c_str(), inet_ntoa(Client.sin_addr))) continue;

                            // Termination event, add the server to the cleanup.
                            if(std::strstr(Event.c_str(), "Terminated"))
                            {
                                (*Server)->Lastmessage = 0;
                                continue;
                            }

                            // Parse the payload-data as JSON, for some reason std::string doesn't decay properly.
                            (*Server)->Gamedata = nlohmann::json::parse(Base64::Decode(Payload).c_str());

                            // We should update this every ~16 sec.
                            (*Server)->Lastmessage = time(nullptr);
                        }
                        catch (std::exception &e) { (void)e; }
                    }
                }

                // Broadcast every 16 seconds.
                if(Localhost)
                {
                    // Let's not assume that time64_t is UNIX-time.
                    if(difftime(time(nullptr), Localhost->Lastmessage) >= 16)
                    {
                        Localhost->Lastmessage = time(nullptr);

                        // Create the message in JSON so developers can read it.
                        auto Message = nlohmann::json::object();
                        Message["SessionID"] = Localhost->SessionID;
                        Message["Data"] = Base64::Encode(Localhost->Gamedata.dump());
                        if(Localhost->Hostflags.Terminated)
                        {
                            Message["Event"] = "Terminated";
                            Localhost = nullptr;
                        }

                        // Windows does not like to send partial messages, so prefix the identifier.
                        auto Payload = Message.dump().insert(0, (const char *)&Identifier, sizeof(uint32_t));

                        // Forward to the OS.
                        Address.sin_addr.S_un.S_addr = INADDR_BROADCAST;
                        sendto(Socket, Payload.data(), uint32_t(Payload.size()), NULL, (SOCKADDR *)&Address, sizeof(Address));
                    }
                }
            }
        });
    }

    // Notify about our properties changing.
    void Broadcastupdate()
    {
        // Up to 2 sec delay, faster if there's other clients around.
        if(Localhost) Localhost->Lastmessage = 0;
    }
}
