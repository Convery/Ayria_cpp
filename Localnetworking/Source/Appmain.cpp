/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-11
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Localnetworking.hpp"

// Entrypoint when loaded as a plugin.
extern "C" EXPORT_ATTR void __cdecl onStartup(bool)
{
    // Initialize the background thread.
    Localnetworking::Createbackend(4200);

    // Hook the various frontends.
    Localnetworking::Initializewinsock();
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
    }

    return TRUE;
}
#else
__attribute__((constructor)) void __stdcall DllMain()
{
    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Assets");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif

namespace Localnetworking
{
    std::unordered_map<IServer::Endpoints_t, IServer *, decltype(FNV::Hash), decltype(FNV::Equal)> Associations;
    std::unordered_map<size_t, IServer *> Streamsockets, Datagramsockets;
    std::unordered_map<std::string, IServer *> Resolvedhosts;
    std::unordered_map<uint16_t, std::string> Proxyports;
    std::vector<size_t> Pluginhandles;
    std::mutex Bottleneck;
    uint16_t Backendport;
    size_t Listensocket;
    size_t Proxysocket;

    // Poll all sockets in the background.
    void Pollthread()
    {
        // ~100 FPS in-case someone proxies game-data.
        timeval Timeout{ NULL, 10000 };
        char Buffer[8192];

        // Sleeps through select().
        while (true)
        {
            FD_SET ReadFD{};
            FD_SET(Listensocket, &ReadFD);
            for (const auto &[Socket, _] : Streamsockets) FD_SET(Socket, &ReadFD);
            for (const auto &[Socket, _] : Datagramsockets) FD_SET(Socket, &ReadFD);

            /*
                NOTE(tcn): Not portable!!one!
                POSIX select timeout is not const and nfds should not be NULL.
            */
            if (select(NULL, &ReadFD, NULL, NULL, &Timeout))
            {
                // If there's data on the listen-socket, it's a new connection.
                if (FD_ISSET(Listensocket, &ReadFD))
                {
                    SOCKADDR_IN Client{}; int Clientsize{ sizeof(SOCKADDR_IN) };
                    if (const auto Socket = accept(Listensocket, (SOCKADDR *)&Client, &Clientsize); Socket != size_t(-1))
                    {
                        std::lock_guard _(Bottleneck);
                        auto Entry = Proxyports.find(Client.sin_port);
                        if (Entry == Proxyports.end()) { closesocket(Socket); break; }

                        auto Server = Streamsockets.emplace(Socket, Resolvedhosts[Entry->second]);
                        if (Server.second) Server.first->second->onConnect();
                        else Debugprint("Failed to allocate stream-socket");
                        Proxyports.erase(Entry);
                    }
                }

                // If there's data on the sockets, we just forward it as is.
                std::function<void(std::unordered_map<size_t, IServer *> &)> Lambda = [&](std::unordered_map<size_t, IServer *> &Sockets) -> void
                {
                    for (const auto &[Socket, Server] : Sockets)
                    {
                        if (!FD_ISSET(Socket, &ReadFD)) continue;
                        const auto Size = recv(Socket, Buffer, 8192, 0);

                        // Broken connection.
                        if (Size <= 0)
                        {
                            // Lock scope.
                            {
                                // Winsock had some internal issue that we can't be arsed to recover from..
                                if (Size == -1) Infoprint(va("Error on socket - %u", WSAGetLastError()));
                                if (Server) Server->onDisconnect();
                                std::lock_guard _(Bottleneck);
                                Sockets.erase(Socket);
                                closesocket(Socket);
                            }
                            return Lambda(Sockets);
                        }

                        // Client-server associations indicate a datagram socket.
                        for (const auto &[Endpoint, Instance] : Associations)
                            if (Server == Instance)
                                Server->onPacketwrite(Buffer, Size, &Endpoint);

                        Server->onStreamwrite(Buffer, Size);

                        // NOTE(tcn): Not using std::find_if yet because MSVC breaks the lambda captures.
                        // auto Result = std::find_if(Associations.begin(), Associations.end(), [&Server](auto &a) { return a.second == Server; });
                        // if (Result == Associations.end()) Server->onStreamwrite(Buffer, Size);
                        // else Server->onPacketwrite(Buffer, Size, &Result->first);
                    }
                };
                Lambda(Datagramsockets);
                Lambda(Streamsockets);
            }

            // We process datagrams the same way as regular streams.
            for (const auto &[_, Server] : Datagramsockets)
            {
                uint32_t Datasize{ 8192 };
                if (Server->onPacketread(Buffer, &Datasize))
                {
                    // Servers can have multiple associated sockets, so we duplicate.
                    for (const auto &[Socket, Instance] : Datagramsockets)
                        if (Instance == Server)
                            send(Socket, Buffer, Datasize, 0);
                }
            }
            for (const auto &[_, Server] : Streamsockets)
            {
                uint32_t Datasize{ 8192 };
                if (Server->onStreamread(Buffer, &Datasize))
                {
                    for (const auto &[Socket, Instance] : Streamsockets)
                        if (Instance == Server)
                            send(Socket, Buffer, Datasize, 0);
                }
            }
        }
    }

    // Whether or not a plugin claims ownership of this host.
    std::vector<std::string> Blacklist{};
    bool isProxiedhost(std::string_view Hostname)
    {
        // Common case, already resolved the host through getHostbyname.
        if (Resolvedhosts.find(Hostname.data()) != Resolvedhosts.end()) return true;

        std::lock_guard _(Bottleneck); // Blacklist for uninteresting hostnames that no plugin wants to handle.
        if (std::find(Blacklist.begin(), Blacklist.end(), Hostname.data()) != Blacklist.end()) return false;

        // Request a server to associate with the hostname.
        for (const auto &Handle : Pluginhandles)
        {
            if (auto Callback = GetProcAddress((HMODULE)Handle, "Createserver"))
            {
                if (auto Server = (reinterpret_cast<IServer * (*)(const char *)>(Callback))(Hostname.data()))
                {
                    Resolvedhosts[Hostname.data()] = Server;
                    return true;
                }
            }
        }

        // No servers can handle this host, blacklist.
        Blacklist.push_back(Hostname.data());
        return false;
    }

    // Notify the backend about a client-port that will connect to us.
    void Associateport(std::string_view Hostname, uint16_t Port)
    {
        std::lock_guard _(Bottleneck);
        Proxyports[Port] = Hostname;
    }

    // A datagram socket, which internally is sent over TCP.
    size_t Createsocket(IServer::Address_t Serveraddress, std::string_view Hostname)
    {
        SOCKADDR_IN Server{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        SOCKADDR_IN Client{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        auto Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int Serversize{ sizeof(SOCKADDR_IN) };
        int Clientsize{ sizeof(SOCKADDR_IN) };

        // Never block or complain.
        unsigned long Argument = TRUE;
        ioctlsocket(Socket, FIONBIO, &Argument);

        // Assign a random port to the client for identification..
        bind(Socket, (SOCKADDR *)&Client, sizeof(SOCKADDR_IN));
        getsockname(Socket, (SOCKADDR *)&Client, &Clientsize);

        // Find wherever the proxy is listening and connect to it..
        getsockname(Proxysocket, (SOCKADDR *)&Server, &Serversize);
        connect(Socket, (SOCKADDR *)&Server, Serversize);

        std::lock_guard _(Bottleneck);
        // Accept and associate the address-pairs, looks messy but: [] = [] = IServer *
        Associations[{ IServer::Address_t{ Client.sin_addr.S_un.S_addr, Client.sin_port }, Serveraddress}] = \
        Datagramsockets[accept(Proxysocket, (SOCKADDR *)&Client, &Clientsize)] = Resolvedhosts[Hostname.data()];

        Debugprint(va("Proxy socket: 0x%X", Socket));
        return Socket;
    }

    // Initialize the server backend.
    void Createbackend(uint16_t Serverport)
    {
        WSADATA wsaData;
        unsigned long Argument = TRUE;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // We really shouldn't block on these sockets, but it's not a problem for now.
        Listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Proxysocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        // Find an available port to listen on, if we fail after 64 tries we have other issues.
        SOCKADDR_IN Server{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        bind(Proxysocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN));
        for (int i = 0; i < 64; ++i)
        {
            Backendport = Serverport++;
            Server.sin_port = htons(Backendport);
            if (0 == bind(Listensocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)))
            {
                listen(Listensocket, 16);
                listen(Proxysocket, 16);
                break;
            };
        }

        // LEGACY(tcn): Load all plugins from disk.
        constexpr const char *Pluignextension = sizeof(void *) == sizeof(uint32_t) ? ".Localnet32" : ".Localnet64";
        auto Results = FS::Findfiles("./Ayria/Plugins", Pluignextension);
        for (const auto &Item : Results)
        {
            auto Module = LoadLibraryA(va("./Ayria/Plugins/%s", Item.c_str()).c_str());
            if (Module)
            {
                Infoprint(va("Loaded localnet plugin \"%s\"", Item.c_str()));
                Pluginhandles.push_back(size_t(Module));
            }
        }

        // Notify the developer.
        Debugprint(va("Spawning localnet backend on %u", Backendport));

        // Keep the polling away from the main thread.
        std::thread(Pollthread).detach();
    }
}
