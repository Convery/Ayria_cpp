/*
    Initial author: Convery (tcn@ayria.se)
    Started: 10-04-2019
    License: MIT
*/

#include "Stdinclude.hpp"

// Let's just keep everything in a single module.
extern size_t TLSCallbackaddress();
extern "C" size_t EPAddress{};
extern size_t PEEntrypoint();
extern void Loadallplugins();
Simplehook::Stomphook Hook;
extern "C" void Return();
size_t TLSAddress{};
void PECallback()
{
    // Check all the places plugins may reside.
    Loadallplugins();

    // Restore the entrypoint.
    if (const size_t Address = PEEntrypoint())
    {
        Hook.Removehook();
    }

    // Restore TLS.
    if (const size_t Address = TLSCallbackaddress())
    {
        if (TLSAddress)
        {
            auto Protection = Memprotect::Unprotectrange(Address, sizeof(size_t));
            {
                *(size_t *)Address = TLSAddress;
            }
            Memprotect::Protectrange(Address, sizeof(size_t), Protection);

            // Trigger the applications TLS callback.
            Debugprint("Starting TLS CB");
            std::thread([]() {}).join();
        }
    }

    Debugprint("Returning");
    Return();
}

// Entrypoint when loaded as a shared library.
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        _mkdir("./Ayria/");
        _mkdir("./Ayria/Plugins/");
        _mkdir("./Ayria/Assets/");
        _mkdir("./Ayria/Local/");
        _mkdir("./Ayria/Logs/");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Disable TLS callbacks as they run before main.
        if (const size_t Address = TLSCallbackaddress())
        {
            if (TLSAddress = *(size_t *)Address; TLSAddress)
            {
                auto Protection = Memprotect::Unprotectrange(Address, sizeof(size_t));
                {
                    *(size_t *)Address = NULL;
                }
                Memprotect::Protectrange(Address, sizeof(size_t), Protection);
            }
        }

        // Install our own entrypoint for the application.
        if (EPAddress = PEEntrypoint(); EPAddress)
        {
            Debugprint(va("EP at %llx", EPAddress));
            Hook.Installhook(EPAddress, PECallback);
        }
    }

    return TRUE;
}

// Poke at the plugins from other modules.
std::vector<void *> Loadedplugins;
extern "C"
{
    // MSVC does not support x64 assembly so we need the jump in a new func.
    #if !defined(_WIN64)
    void Return()
    {
        // NOTE(tcn): Clang and CL implements AddressOfReturnAddress differently, bug?
        #if defined (__clang__)
        *((size_t *)__builtin_frame_address(0) + 1) = PEEntrypoint();
        #else
        *(size_t *)_AddressOfReturnAddress() = PEEntrypoint();
        #endif
    }
    #endif

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
        // Notify all plugins about being initialized.
        for (const auto &Item : Loadedplugins)
        {
            auto Callback = GetProcAddress(HMODULE(Item), "onInitialized");
            if (Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);
        }
    }
}

// Utility functionality.
std::string Temporarydir() { char Buffer[260]{}; GetTempPathA(260, Buffer); return std::move(Buffer); }
size_t TLSCallbackaddress()
{
    // Module(NULL) gets the host application.
    HMODULE Modulehandle = GetModuleHandleA(NULL);
    if (!Modulehandle) return 0;

    // Traverse the PE header.
    PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
    PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY TLSDirectory = NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

    // Get the callback address.
    if (!TLSDirectory.VirtualAddress) return 0;
    return size_t(((size_t *)((DWORD_PTR)Modulehandle + TLSDirectory.VirtualAddress))[3]);
}
size_t PEEntrypoint()
{
    // Module(NULL) gets the host application.
    HMODULE Modulehandle = GetModuleHandleA(NULL);
    if (!Modulehandle) return 0;

    // Traverse the PE header.
    PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
    PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);

    return (size_t)((DWORD_PTR)Modulehandle + NTHeader->OptionalHeader.AddressOfEntryPoint);
}
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
        if(Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);
    }
}
