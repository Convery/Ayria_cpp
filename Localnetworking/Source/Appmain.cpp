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
    Defaultmutex Bottleneck{};
    FD_SET Activesockets{};
    uint16_t Backendport{};
    size_t Listensocket{};

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

                    Entry->Address.sin_addr.s_addr = htonl(0xF0000000 | ++Proxycount);
                    Entry->Hostname = Hostname.data();
                    Entry->Instance = Server;
                    return Entry;
                }
            }
        }

        // No servers wants to deal with this address =(
        Hostblacklist.emplace_back(Hostname.data());
        return nullptr;
    }
    Proxyserver_t *getProxyserver(sockaddr_in *Hostname)
    {
        std::scoped_lock _(Bottleneck);

        for (auto &Item : Proxyservers)
        {
            if (FNV::Equal(Item.second.Address.sin_addr, Hostname->sin_addr))
            {
                return &Item.second;
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
            assert(Activesockets.fd_count < FD_SETSIZE);
            FD_SET(Serversocket, &Activesockets);
        }
    }

    // Helper for debug-builds.
    inline void setThreadname(std::string_view Name)
    {
        if constexpr (Build::isDebug)
        {
            #pragma pack(push, 8)
            using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
            #pragma pack(pop)

            __try
            {
                THREADNAME_INFO Info{ 0x1000, Name.data(), 0xFFFFFFFF };
                RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    // Poll all sockets in the background.
    DWORD __stdcall Pollsockets(void *)
    {
        // ~100 FPS in-case someone proxies game-data.
        constexpr timeval Defaulttimeout{ NULL, 10000 };
        constexpr uint32_t Buffersizelimit = 4096;
        char Buffer[Buffersizelimit];

        // Name the thread for easier debugging.
        setThreadname("Localnetworking_Mainthread");

        // Sleeps through select().
        while (true)
        {
            // Let's not poll when we don't have any sockets.
            if (Activesockets.fd_count == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            FD_SET ReadFD{ Activesockets }, WriteFD{ Activesockets };
            const auto Count{ Activesockets.fd_count + 1 };
            auto Timeout{ Defaulttimeout };

            // Now POSIX compatible.
            if (!select(Count, &ReadFD, &WriteFD, NULL, &Timeout)) continue;

            // Poll the servers for data and send it to the sockets.
            for (const auto &[Socket, Server] : Serversockets)
            {
                if (!FD_ISSET(Socket, &WriteFD)) continue;
                uint32_t Datasize{ Buffersizelimit };

                if (Server->onStreamread(Buffer, &Datasize) || Server->onPacketread(Buffer, &Datasize))
                {
                    // Servers can have multiple associated sockets, so we duplicate.
                    for (const auto &[Localsocket, Instance] : Serversockets)
                    {
                        if (Instance == Server)
                        {
                            send(Localsocket, (char *)Buffer, Datasize, NULL);
                        }
                    }
                }
            }

            // If there's data on the socket, forward to a server.
            LOOP: for (const auto &[Socket, Server] : Serversockets)
            {
                if (!FD_ISSET(Socket, &ReadFD)) continue;
                const auto Size = recv(Socket, Buffer, Buffersizelimit, NULL);

                // Broken connection.
                if (Size <= 0)
                {
                    // Winsock had some internal issue that we can't be arsed to recover from..
                    if (Size == -1) Infoprint(va("Error on socket %u - %u", Socket, WSAGetLastError()));
                    if (Server) Server->onDisconnect();
                    assert(Activesockets.fd_count);
                    FD_CLR(Socket, &Activesockets);
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
        }

        return 0;
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
        CreateThread(NULL, NULL, Pollsockets, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

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
    // Don't trust the bootstrapper to manage this.
    static bool Initialized = false;
    if (Initialized) return;
    Initialized = true;

    // Initialize the background thread.
    Localnetworking::Createbackend(4200);

    // Hook the various frontends.
    Localnetworking::Initializewinsock();
}
