/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Plugins
{
    // Helper to write a pointer.
    #define Writeptr(where, what) { \
    const auto Lock = Memprotect::Makewriteable(where, sizeof(uintptr_t)); \
    *(uintptr_t *)where = (uintptr_t)what; }

    // If TLS is not properly supported, fallback to simple hooks.
    static std::vector<uintptr_t> OriginalTLS;
    static uint8_t EPCode[14];
    static void *EPTrampoline;

    // Callbacks from the hooks.
    void __stdcall TLSCallback(PVOID a, DWORD b, PVOID c)
    {
        HMODULE Valid;

        if (const auto Directory = (PIMAGE_TLS_DIRECTORY)getTLSEntry())
        {
            auto Callbacks = (uintptr_t *)Directory->AddressOfCallBacks;

            // Disable TLS while loading plugins.
            Writeptr(&Callbacks[0], nullptr);
            Plugins::Initialize();

            // Restore the TLS directory.
            for (const auto &Address : OriginalTLS)
            {
                Writeptr(Callbacks, Address);
                Callbacks++;
            }

            // Call the first real callback if we had one.
            Callbacks = (size_t *)Directory->AddressOfCallBacks;
            if (*Callbacks && GetModuleHandleExA(6, (LPCSTR)*Callbacks, &Valid))
                ((decltype(TLSCallback) *)*Callbacks)(a, b, c);
        }
    }
    void EPCallback()
    {
        assert(EPTrampoline);

        // Read the PE header.
        if (const auto EP = getEntrypoint())
        {
            // Restore the code in-case the app does integrity checking.
            const auto RTTI = Memprotect::Makewriteable(EP, 14);
            std::memcpy((void *)EP, EPCode, 14);
        }

        // Load all plugins.
        Plugins::Initialize();

        // Resume via the trampoline.
        (reinterpret_cast<decltype(EPCallback) *>(EPTrampoline))();
    }

    // Different types of hooking.
    bool InstallTLSHook()
    {
        if (const auto Directory = (PIMAGE_TLS_DIRECTORY)getTLSEntry())
        {
            // Save any and all existing callbacks.
            auto Callbacks = (uintptr_t *)Directory->AddressOfCallBacks;
            while (*Callbacks) OriginalTLS.push_back(*(Callbacks++));

            // Replace with ours.
            Callbacks = (uintptr_t *)Directory->AddressOfCallBacks;
            Writeptr(&Callbacks[0], TLSCallback);
            Writeptr(&Callbacks[1], NULL);

            return true;
        }

        return false;
    }
    bool InstallEPHook()
    {
        if (const auto EP = getEntrypoint())
        {
            // Save the code in-case the app does integrity checking.
            std::memcpy(EPCode, (void *)EP, 14);

            // Jump to our function when EP is called.
            EPTrampoline = Hooking::Stomphook(EP, EPCallback);
            if (EPTrampoline)
            {
                return true;
            }
        }

        return false;
    }

    // Helpers that may be useful elsewhere.
    uintptr_t getEntrypoint()
    {
        // Module(NULL) gets the host application.
        const HMODULE Modulehandle = GetModuleHandleA(NULL);
        if (!Modulehandle) return NULL;

        // Traverse the PE header.
        const auto *DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
        const auto *NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
        return (DWORD_PTR)Modulehandle + NTHeader->OptionalHeader.AddressOfEntryPoint;
    }
    uintptr_t getTLSEntry()
    {
        // Module(NULL) gets the host application.
        const HMODULE Modulehandle = GetModuleHandleA(NULL);
        if (!Modulehandle) return NULL;

        // Traverse the PE header.
        const auto *DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
        const auto *NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
        const auto Directory = NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

        if (Directory.Size == 0) return NULL;
        return (DWORD_PTR)Modulehandle + Directory.VirtualAddress;
    }

    // Track the plugins for the "initialized" signal.
    static std::unordered_set<HMODULE> Pluginhandles;
    static bool hasInitialized = false;

    // Should be called from external sources.
    extern "C" EXPORT_ATTR void __cdecl onInitialized(bool)
    {
        // Only initialize once.
        if (hasInitialized) return;
        hasInitialized = true;

        // If the plugin has a callback..
        for (const auto &Item : Pluginhandles)
        {
            if (const auto Callback = GetProcAddress(Item, "onInitialized"))
            {
                (reinterpret_cast<void(__cdecl *)(bool)>(Callback))(false);
            }
        }
    }

    // Simply load all plugins from disk.
    void Initialize()
    {
        // Load all plugins from disk.
        for (const auto &Item : FS::Findfiles(L"./Ayria/Plugins", Build::is64bit ? L"64" : L"32"))
        {
            if (const auto Module = LoadLibraryW((L"./Ayria/Plugins/"s + Item).c_str()))
            {
                Pluginhandles.insert(Module);
            }
        }

        // Notify the plugins about startup.
        for (const auto &Item : Pluginhandles)
        {
            if (const auto Callback = GetProcAddress(Item, "onStartup"))
            {
                (reinterpret_cast<void(__cdecl *)(bool)>(Callback))(false);
            }
        }

        // Ensure that a "onInitialized" is sent 'soon'.
        static auto doOnce{ []()-> bool
        {
            std::thread([]()
            {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                onInitialized(false);
            }).detach();
            return {};
        }() };
    }
}
