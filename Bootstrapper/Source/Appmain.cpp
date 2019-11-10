/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "Stdinclude.hpp"

void Loadallplugins();
#define Writeptr(where, what) { \
auto Protection = Memprotect::Unprotectrange(where, sizeof(size_t)); \
*(size_t *)where = (size_t)what; Memprotect::Protectrange(where, sizeof(size_t), Protection); }

// Saved directory.
std::vector<size_t> OriginalTLS{};

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

    return (PIMAGE_TLS_DIRECTORY)((DWORD_PTR)Modulehandle + Directory.VirtualAddress);
}

// Callback from Ntdll::CallTlsInitializers
void WINAPI Dummycallback(PVOID, DWORD, PVOID) {}
void WINAPI TLSCallback(PVOID a, DWORD b, PVOID c)
{
    const auto Directory = getTLSDirectory();
    HMODULE Valid;

    // Disable callbacks while loading plugins.
    Writeptr(Directory->AddressOfCallBacks, 0);

    // Load any plugins found in ./Ayria/
    Loadallplugins();

    // Restore the callback directory.
    for (size_t i = 0; i < OriginalTLS.size(); ++i)
        Writeptr((Directory->AddressOfCallBacks + i * sizeof(size_t)), OriginalTLS[i]);

    // If the original had callbacks, we need to call the first one.
    if (auto Callback = (size_t *)Directory->AddressOfCallBacks; *(size_t *)Callback)
        if(GetModuleHandleExA(6, (LPCSTR)*(size_t *)Callback, &Valid))
            ((decltype(TLSCallback) *)*(size_t *)Callback)(a, b, c);
}

// Sometimes plugins want to name their threads, and not all games support that..
LONG WINAPI Threadname(PEXCEPTION_POINTERS Info)
{
    // Via a Microsoft exception.
    if (Info->ExceptionRecord->ExceptionCode == 0x406D1388)
        return EXCEPTION_CONTINUE_EXECUTION;

    return EXCEPTION_CONTINUE_SEARCH;
}

// Entrypoint when loaded as a shared library.
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        (void)_mkdir("./Ayria/");
        (void)_mkdir("./Ayria/Logs/");
        (void)_mkdir("./Ayria/Local/");
        (void)_mkdir("./Ayria/Assets/");
        (void)_mkdir("./Ayria/Plugins/");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Save the callbacks and add ours.
        const auto Directory = getTLSDirectory();
        auto Callback = (size_t *)Directory->AddressOfCallBacks;
        if (*Callback) { OriginalTLS.push_back(*Callback); Writeptr(Callback, TLSCallback); }
        else
        {
            do
            {
                OriginalTLS.push_back(*Callback);
                Writeptr(Callback, Dummycallback);
            } while (*(++Callback));

            Callback--;
            Writeptr(Callback, TLSCallback);
        }

        // Sometimes plugins want to name their threads, and not all games support that..
        SetUnhandledExceptionFilter(Threadname);

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
    }

    return TRUE;
}

// Poke at the plugins from other modules.
std::vector<void *> Loadedplugins;
extern "C"
{
    EXPORT_ATTR bool Broadcastmessage(const void *Buffer, uint32_t Size)
    {
        // Forward to all plugins until one handles it.
        for (const auto &Item : Loadedplugins)
        {
            auto Callback = GetProcAddress(HMODULE(Item), "onMessage");
            if (!Callback) continue;
            auto Result = (reinterpret_cast<bool (*)(const void *, uint32_t)>(Callback))(Buffer, Size);
            if (Result) return true;
        }

        return false;
    }
    EXPORT_ATTR void onInitializationdone()
    {
        static bool Initialized = false;
        if (Initialized) return;
        Initialized = true;

        // Notify all plugins about being initialized.
        for (const auto &Item : Loadedplugins)
        {
            auto Callback = GetProcAddress(HMODULE(Item), "onInitialized");
            if (Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);
        }
    }
}

// Utility functionality.
void Loadallplugins()
{
    constexpr const char *Pluignextension = sizeof(void *) == sizeof(uint32_t) ? ".Ayria32" : ".Ayria64";
    std::vector<void *> Freshplugins;

    // Really just load all files from the directory.
    auto Results = FS::Findfiles("./Ayria/Plugins", Pluignextension);
    for (const auto &Item : Results)
    {
        /*
            TODO(tcn):
            We should really check that the plugins are signed by us.
            Then prompt the user for whitelisting.
        */

        auto Module = LoadLibraryA(va("./Ayria/Plugins/%s", Item.c_str()).c_str());
        if (Module)
        {
            Infoprint(va("Loaded plugin \"%s\"", Item.c_str()));
            Loadedplugins.push_back(Module);
            Freshplugins.push_back(Module);
        }
    }

    // Notify all plugins about starting up.
    for (const auto &Item : Freshplugins)
    {
        auto Callback = GetProcAddress(HMODULE(Item), "onStartup");
        if (Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);
    }

    // If no-one have notified us about the game being initialized, we notify ourselves in 3 sec.
    std::thread([]() { std::this_thread::sleep_for(std::chrono::seconds(3)); onInitializationdone(); }).detach();
}
