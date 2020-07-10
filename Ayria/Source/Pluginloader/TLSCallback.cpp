/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common.hpp"

namespace Pluginloader
{
    #define Writeptr(where, what) { \
    const auto Lock = Memprotect::Makewriteable(where, sizeof(size_t)); \
    *(size_t *)where = (size_t)what; }

    // Parse the header to get the directory.
    static inline PIMAGE_TLS_DIRECTORY getTLSDirectory()
    {
        // Module(NULL) gets the host application.
        const HMODULE Modulehandle = GetModuleHandleA(NULL);
        if (!Modulehandle) std::abort();

        // Traverse the PE header.
        const auto *DOSHeader = (PIMAGE_DOS_HEADER)Modulehandle;
        const auto *NTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)Modulehandle + DOSHeader->e_lfanew);
        const IMAGE_DATA_DIRECTORY Directory = NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

        if (Directory.Size == 0) return nullptr;
        return (PIMAGE_TLS_DIRECTORY)((DWORD_PTR)Modulehandle + Directory.VirtualAddress);
    }

    // Callback from ntdll::CallTlsInitializers
    void __stdcall TLSCallback(PVOID a, DWORD b, PVOID c)
    {
        HMODULE Valid;
        const auto Directory = getTLSDirectory();
        auto Callbacks = (size_t *)Directory->AddressOfCallBacks;

        // Disable callbacks while loading plugins.
        Writeptr(&Callbacks[0], nullptr);

        // Outsourced.
        Loadplugins();
        RestoreTLSCallback();

        // Call the first real callback if we had one.
        Callbacks = (size_t *)Directory->AddressOfCallBacks;
        if (*Callbacks && GetModuleHandleExA(6, (LPCSTR)*Callbacks, &Valid))
            ((decltype(TLSCallback) *)*Callbacks)(a, b, c);
    }

    std::vector<size_t> OriginalTLS{};
    bool InstallTLSCallback()
    {
        if (const auto Directory = getTLSDirectory())
        {
            // Save any existing callbacks.
            auto Callbacks = (size_t *)Directory->AddressOfCallBacks;
            while (*Callbacks) { OriginalTLS.push_back(*(Callbacks++)); }

            // Overwrite with ours.
            Callbacks = (size_t *)Directory->AddressOfCallBacks;
            Writeptr(&Callbacks[0], TLSCallback);
            Writeptr(&Callbacks[1], nullptr);

            return true;
        }

        return false;
    }
    void RestoreTLSCallback()
    {
        // Restore the callback directory.
        const auto Directory = getTLSDirectory();
        auto Callbacks = (size_t *)Directory->AddressOfCallBacks;
        for (const auto &Address : OriginalTLS)
        {
            Writeptr(Callbacks, Address);
            Callbacks++;
        }
    }
}
