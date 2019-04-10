/*
    Initial author: Convery (tcn@ayria.se)
    Started: 10-04-2019
    License: MIT
*/

#include "Stdinclude.hpp"

// Let's just keep everything in a single module.
extern size_t TLSCallbackaddress();
extern size_t PEEntrypoint();
uint8_t Originalstub[14]{};
size_t TLSAddress{};
size_t EPAddress{};
void PECallback()
{
    // Load all plugins.

    // Restore the entrypoint.
    if (const size_t Address = PEEntrypoint())
    {
        auto Protection = Memprotect::Unprotectrange(Address, 14);
        {
            std::memcpy((void *)Address, Originalstub, 14);
        }
        Memprotect::Protectrange(Address, 14, Protection);
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
            std::thread([]() {}).detach();
        }
    }

    // Return to the applications entrypoint.
    // NOTE(tcn): Clang and CL implements AddressOfReturnAddress differently, bug?
    #if defined (__clang__)
        *((size_t*)__builtin_frame_address(0) + 1) = PEEntrypoint();
    #else
        *(size_t *)_AddressOfReturnAddress() = PEEntrypoint();
    #endif
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
            if (TLSAddress = *(size_t *)Address)
            {
                auto Protection = Memprotect::Unprotectrange(Address, sizeof(size_t));
                {
                    *(size_t *)Address = NULL;
                }
                Memprotect::Protectrange(Address, sizeof(size_t), Protection);
            }
        }

        // Install our own entrypoint for the application.
        if (const size_t Address = PEEntrypoint())
        {
            auto Protection = Memprotect::Unprotectrange(Address, 14);
            {
                // Copy the original stub.
                std::memcpy(Originalstub, (void *)Address, 14);

                // Simple stomp-hooking.
                if (Build::is64bit)
                {
                    // JMP [RIP + 0]
                    std::memcpy((uint8_t *)Address, "\xFF\x25", 2);
                    std::memcpy((uint8_t *)Address + 2, "\x00\x00\x00\x00", 4);
                    std::memcpy((uint8_t *)Address + 6, &PECallback, sizeof(size_t));
                }
                else
                {
                    // JMP short
                    *(uint8_t  *)((uint8_t *)Address + 0) = 0xE9;
                    *(uint32_t *)((uint8_t *)Address + 1) = ((uint32_t)PECallback - (Address + 5));
                }
            }
            Memprotect::Protectrange(Address, 14, Protection);
        }

        // Done.

        //


    }

    return TRUE;
}

// Utility functionality.
extern "C"
{
    EXPORT_ATTR bool Broadcastmessage(const void *Buffer, uint32_t Size);
}

// Utility.
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
