/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "Stdinclude.hpp"

#define Writeptr(where, what) { const auto Lock = Memprotect::Makewriteable(where, sizeof(size_t)); *(size_t *)where = (size_t)what; }

// The type of plugins to support.
constexpr const char *Pluginextension = Build::is64bit ? "64" : "32";

// Saved directory and all plugins.
std::vector<size_t> OriginalTLS{};
std::vector<void *> Loadedplugins;

// Parse the header to get the directory.
PIMAGE_TLS_DIRECTORY getTLSDirectory()
{
    // Module(NULL) gets the host application.
    const HMODULE Modulehandle = GetModuleHandleA(NULL);
    if (!Modulehandle) std::abort();

    // Traverse the PE header.
    const PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
    const PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
    const IMAGE_DATA_DIRECTORY Directory = NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

    if (Directory.Size == 0) return nullptr;
    return (PIMAGE_TLS_DIRECTORY)((DWORD_PTR)Modulehandle + Directory.VirtualAddress);
}

// Poke at the plugins from other modules.
extern "C"
{
    EXPORT_ATTR void __cdecl onInitialized(bool)
    {
        // Only notify the plugins once to prevent confusion.
        static bool Initialized = false;
        if (Initialized) return;
        Initialized = true;

        for (const auto &Item : Loadedplugins)
        {
            if (const auto Callback = GetProcAddress(HMODULE(Item), "onInitialized"))
            {
                (reinterpret_cast<void (__cdecl *)(bool)>(Callback))(false);
            }
        }
    }
    EXPORT_ATTR void __cdecl onReload(const char *Pluginname)
    {
        // Get the latest version of the plugin.
        auto Results = FS::Findfiles("./Ayria/Plugins", Pluginname);
        std::sort(Results.begin(), Results.end(), [](auto &a, auto &b) -> bool
        {
            return FS::Filestats(("./Ayria/Plugins/"s + a).c_str()).Modified >
                   FS::Filestats(("./Ayria/Plugins/"s + b).c_str()).Modified;
        });

        // Actually only get the first one.
        for (const auto &Filename : Results)
        {
            // Sanity-checking.
            if (!std::strstr(Filename.c_str(), Pluginextension)) continue;

            for (const auto Plugin : Loadedplugins)
            {
                char Localname[512]{};
                GetModuleFileNameA((HMODULE)Plugin, Localname, 512);
                if (std::strstr(Localname, Pluginname))
                {
                    if (const auto Module = LoadLibraryA(("./Ayria/Plugins/%s"s + Filename).c_str()))
                    {
                        if (const auto Callback = GetProcAddress(Module, "onReload"))
                        {
                            (reinterpret_cast<void(__cdecl*)(void*)>(Callback))(Plugin);
                        }

                        Loadedplugins.push_back(Module);
                        FreeLibrary((HMODULE)Plugin);
                    }
                    return;
                }
            }
        }
    }
    EXPORT_ATTR bool __cdecl Broadcastevent(const void *Data, uint32_t Length)
    {
        uint32_t Handledcount{};

        // Forward to all plugins to enable snooping.
        for (const auto &Item : Loadedplugins)
        {
            if (const auto Callback = GetProcAddress(HMODULE(Item), "onEvent"))
            {
                Handledcount += (reinterpret_cast<bool (__cdecl *)(const void *, uint32_t)>(Callback))(Data, Length);
            }
        }

        return !!Handledcount;
    }
}

// Utility functionality.
void Loadallplugins()
{
    std::vector<void *> Freshplugins;

    // Really just load all files from the directory.
    const auto Results = FS::Findfiles("./Ayria/Plugins", Pluginextension);
    for (const auto &Item : Results)
    {
        /*
            TODO(tcn):
            We should really check that the plugins are signed by us.
            Then prompt the user for whitelisting.
        */
        if (const auto Module = LoadLibraryA(("./Ayria/Plugins/"s + Item).c_str()))
        {
            Infoprint(va("Loaded plugin \"%s\"", Item.c_str()));
            Loadedplugins.push_back(Module);
            Freshplugins.push_back(Module);
        }
    }

    // Notify all plugins about starting up.
    for (const auto &Item : Freshplugins)
    {
        const auto Callback = GetProcAddress(HMODULE(Item), "onStartup");
        if (Callback) (reinterpret_cast<void (__cdecl *)(bool)>(Callback))(false);
    }
}

// Callback from ntdll::CallTlsInitializers
void __stdcall TLSCallback(PVOID a, DWORD b, PVOID c)
{
    HMODULE Valid;
    const auto Directory = getTLSDirectory();
    auto Callbacks = (size_t*)Directory->AddressOfCallBacks;

    // Disable callbacks while loading plugins.
    Writeptr(&Callbacks[0], 0);

    // Load any plugins found in ./Ayria/
    Loadallplugins();

    // Restore the callback directory.
    for (const auto& Address : OriginalTLS)
        Writeptr(Callbacks++, Address);

    // Call the first real callback if we had one.
    Callbacks = (size_t*)Directory->AddressOfCallBacks;
    if (*Callbacks && GetModuleHandleExA(6, (LPCSTR)*Callbacks, &Valid))
        ((decltype(TLSCallback)*)*Callbacks)(a, b, c);

    // If no-one have notified us about the game being initialized, we notify ourselves in 3 sec.
    std::thread([]() { std::this_thread::sleep_for(std::chrono::seconds(3)); onInitialized(false); }).detach();
}

// Some games use do not handle exceptions well, so we'll have to catch them.
LONG __stdcall onUnhandledexception(PEXCEPTION_POINTERS Info)
{
    // MSVC thread naming exception for debuggers.
    if (Info->ExceptionRecord->ExceptionCode == 0x406D1388)
    {
        // Double-check, and allow any debugger to handle it if available.
        if (Info->ExceptionRecord->ExceptionInformation[0] == 0x1000 && !IsDebuggerPresent())
        {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // OpenSSLs RAND_poll() causes Windows to throw if RPC services are down.
    if (Info->ExceptionRecord->ExceptionCode == RPC_S_SERVER_UNAVAILABLE)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// Entrypoint when loaded as a shared library.
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

        // Save any existing callbacks.
        if (const auto Directory = getTLSDirectory())
        {
            auto Callbacks = (size_t *)Directory->AddressOfCallBacks;
            while (*Callbacks) { OriginalTLS.push_back(*(Callbacks++)); }

            // Overwrite with ours.
            Callbacks = (size_t *)Directory->AddressOfCallBacks;
            Writeptr(&Callbacks[0], TLSCallback);
            Writeptr(&Callbacks[1], nullptr);

            // Opt out of further notifications.
            DisableThreadLibraryCalls(hDllHandle);
        }

        // Sometimes plugins want to name their threads, someone have to catch that.
        SetUnhandledExceptionFilter(onUnhandledexception);
    }

    // Alternative for games without TLS support.
    if (nReason == DLL_THREAD_ATTACH)
    {
        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Load any plugins found in ./Ayria/
        Loadallplugins();

        // If no-one have notified us about the game being initialized, we notify ourselves in 3 sec.
        std::thread([]() { std::this_thread::sleep_for(std::chrono::seconds(3)); onInitialized(false); }).detach();
    }

    return TRUE;
}
