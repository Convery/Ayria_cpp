/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-11
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Localnetworking.hpp"

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
    std::unordered_map<std::string, Proxyserver_t> Proxyservers{};
    std::unordered_map<size_t, IServer *> Serversockets{};
    std::vector<std::string> Hostblacklist{};
    std::vector<size_t> Pluginhandles{};
    std::mutex Bottleneck{};
    uint16_t Backendport{};
    size_t Listensocket{};
    bool Shouldquit{};

    // Resolve the hostname and return a unique address if proxied.
    Proxyserver_t *getProxyserver(std::string_view Hostname)
    {
        std::scoped_lock _(Bottleneck);

        // Common case, already resolved the host through getHostbyname.
        if (const auto Result = Proxyservers.find(Hostname.data()); Result != Proxyservers.end())
            return &Result->second;

        // Blacklist for uninteresting hostnames.
        if (Hostblacklist.end() != std::find(Hostblacklist.begin(), Hostblacklist.end(), Hostname.data()))
            return nullptr;

        // Ask the plugins if any are interested in this hostname.
        for (const auto &Handle : Pluginhandles)
        {
            if (const auto Callback = GetProcAddress((HMODULE)Handle, "Createserver"))
            {
                if (const auto Server = (reinterpret_cast<IServer * (__cdecl *)(const char *)>(Callback))(Hostname.data()))
                {
                    auto Entry = &Proxyservers[Hostname.data()];
                    static uint16_t Proxycount{ 1 };

                    Entry->Address.sin_addr.s_addr = 0xF0000000 | ++Proxycount;
                    Entry->Hostname = Hostname.data();
                    Entry->Instance = Server;
                    return Entry;
                }
            }
        }

        // No servers wants to deal with this address =(
        Hostblacklist.push_back(Hostname.data());
        return nullptr;
    }
    Proxyserver_t *getProxyserver(sockaddr_in *Hostname)
    {
        std::scoped_lock _(Bottleneck);

        for (const auto &Item : Proxyservers)
        {
            if (FNV::Equal(Item.second.Address.sin_addr, Hostname->sin_addr))
            {
                return &Proxyservers[Item.first];
            }
        }

        return nullptr;
    }

    // Associate a socket with a server.
    void Connectserver(size_t Clientsocket, IServer *Serverinstance)
    {
        assert(Serverinstance);

        SOCKADDR_IN Server{ AF_INET, Backendport, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        while (SOCKET_ERROR == connect(Clientsocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)))
            if (WSAEWOULDBLOCK != WSAGetLastError()) break;

        const auto Serversocket = accept(Listensocket, NULL, NULL);
        if (Serversocket != INVALID_SOCKET)
        {
            Serversockets[Serversocket] = Serverinstance;
        }
    }

    // Poll all sockets in the background.
    void Pollsockets()
    {
        // ~100 FPS in-case someone proxies game-data.
        const timeval Timeout{ NULL, 10000 };
        char Buffer[8192];

        // Sleeps through select().
        while (!Shouldquit)
        {
            FD_SET ReadFD{};
            FD_SET WriteFD{};
            for (const auto &Item : Serversockets) FD_SET(Item.first, &ReadFD);
            for (const auto &Item : Serversockets) FD_SET(Item.first, &WriteFD);

            // NOTE(tcn): Set nfds and reset Timeout on POSIX.
            if (!select(NULL, &ReadFD, &WriteFD, NULL, &Timeout)) continue;

            // If there's data on the socket, forward to a server.
            LOOP: for (const auto &[Socket, Server] : Serversockets)
            {
                if (!FD_ISSET(Socket, &ReadFD)) continue;
                const auto Size = recv(Socket, Buffer, 8192, 0);

                // Broken connection.
                if (Size <= 0)
                {
                    // Winsock had some internal issue that we can't be arsed to recover from..
                    if (Size == -1) Infoprint(va("Error on socket - %u", WSAGetLastError()));
                    if (Server) Server->onDisconnect();
                    Serversockets.erase(Socket);
                    closesocket(Socket);
                    goto LOOP;
                }

                // We assume that the client properly inherits properly.
                if (!Server->onStreamwrite(Buffer, Size))
                {
                    for (const auto &Item : Proxyservers)
                    {
                        if (Item.second.Instance == Server)
                        {
                            IServer::Address_t Universalformat{ ntohl(Item.second.Address.sin_addr.s_addr), ntohs(Item.second.Address.sin_port) };
                            Server->onPacketwrite(Buffer, Size, &Universalformat);
                        }
                    }
                }
            }

            // Poll the servers for data and send it to the sockets.
            for (const auto &[Socket, Server] : Serversockets)
            {
                if (!FD_ISSET(Socket, &WriteFD)) continue;
                uint32_t Datasize{ 8192 };

                if (Server->onStreamread(Buffer, &Datasize) || Server->onPacketread(Buffer, &Datasize))
                {
                    // Servers can have multiple associated sockets, so we duplicate.
                    for (const auto &[Localsocket, Instance] : Serversockets)
                    {
                        if (Instance == Server)
                        {
                            send(Localsocket, Buffer, Datasize, NULL);
                        }
                    }
                }
            }
        }
    }

    // Initialize the server backend.
    void Createbackend(uint16_t Serverport)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        Listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        // Find an available port to listen on, if we fail after 64 tries we have other issues.
        SOCKADDR_IN Server{ AF_INET, 0, {{.S_addr = htonl(INADDR_LOOPBACK)}} };
        for (int i = 0; i < 64; ++i)
        {
            Backendport = htons(Serverport++);
            Server.sin_port = Backendport;

            if (0 == bind(Listensocket, (SOCKADDR *)&Server, sizeof(SOCKADDR_IN)))
            {
                listen(Listensocket, 32);
                break;
            };
        }

        // Notify the developer and spawn the server.
        Debugprint(va("Spawning localnet backend on %u", ntohs(Backendport)));
        std::atexit([]() { Shouldquit = true; });
        std::thread(Pollsockets).detach();

        // Load all plugins from disk.
        for (const auto &Item : FS::Findfiles("./Ayria/Plugins", Build::is64bit ? "64" : "32"))
        {
            if (const auto Module = LoadLibraryA(va("./Ayria/Plugins/%s", Item.c_str()).c_str()))
            {
                if (!GetProcAddress(Module, "Createserver")) FreeLibrary(Module);
                else Pluginhandles.push_back(size_t(Module));
            }
        }
    }
}

// Entrypoint when loaded as a plugin.
extern "C" EXPORT_ATTR void __cdecl onStartup(bool)
{
    // Initialize the background thread.
    Localnetworking::Createbackend(4200);

    // Hook the various frontends.
    Localnetworking::Initializewinsock();
}
