/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#include "Stdinclude.hpp"

size_t OriginalTLS{};
void Loadallplugins();

// Utility functions.
size_t AddressofTLS()
{
    // Module(NULL) gets the host application.
    HMODULE Modulehandle = GetModuleHandleA(NULL);
    if(!Modulehandle) return 0;

    // Traverse the PE header.
    PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
    PIMAGE_NT_HEADERS NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY TLSDirectory = NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

    // Get the callback address.
    if(!TLSDirectory.VirtualAddress) return 0;
    return size_t(((size_t *)((DWORD_PTR)Modulehandle + TLSDirectory.VirtualAddress))[3]);
}

// Sometimes plugins want to name their threads, and not all games support that..
LONG WINAPI Threadname(PEXCEPTION_POINTERS Info)
{
    if(Info->ExceptionRecord->ExceptionCode == 0x406D1388)
        return EXCEPTION_CONTINUE_EXECUTION;
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

// Called after Ntdll.ldr's ctors but before main.
void WINAPI TLSCallback(PVOID a, DWORD b, PVOID c)
{
    // Sometimes plugins want to name their threads, and not all games support that..
    AddVectoredExceptionHandler(0, Threadname);

    // Fixup the TLS callbacks.
    if(const auto Address = AddressofTLS())
    {
        auto Protection = Memprotect::Unprotectrange(Address, sizeof(size_t));
        {
            // Remove the TLS callback in-case plugins spawn new threads.
            *(size_t *)Address = 0;

            // Check all the places plugins may reside.
            Loadallplugins();

            // Restore the original TLS callback.
            *(size_t *)Address = OriginalTLS;
        }
        Memprotect::Protectrange(Address, sizeof(size_t), Protection);
    }
    else
    {
        // Check all the places plugins may reside.
        Loadallplugins();
    }

    // If there was a TLS callback already, call it directly.
    if(OriginalTLS) return ((decltype(TLSCallback) *)OriginalTLS)(a, b, c);
}

// Entrypoint when loaded as a shared library.
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID lpvReserved)
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

        // Set TLS to run our callback when fully loaded.
        if(const auto Address = AddressofTLS())
        {
            auto Protection = Memprotect::Unprotectrange(Address, sizeof(size_t));
            {
                OriginalTLS = *(size_t *)Address;
                *(size_t *)Address = (size_t)TLSCallback;
            }
            Memprotect::Protectrange(Address, sizeof(size_t), Protection);

            // Opt out of further notifications.
            DisableThreadLibraryCalls(hDllHandle);
        }
    }
    if(nReason == DLL_THREAD_ATTACH)
    {
        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Call TLS on the first thread as a fallback.
        TLSCallback(hDllHandle, nReason, lpvReserved);
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
        if(Callback) (reinterpret_cast<void (*)(bool)>(Callback))(false);
    }
}
