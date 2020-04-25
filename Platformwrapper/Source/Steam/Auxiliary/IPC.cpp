/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    void *Originalfunction;
    // Also redirect module lookups for legacy compatibility.
    BOOL __stdcall Callback(DWORD Flags, LPCSTR Modulename, HMODULE *Handle)
    {
        char Filename[260]{};

        // Is this even a real module?
        if (const auto Result = ((decltype(GetModuleHandleExA) *)Originalfunction)(Flags, Modulename, Handle))
        {
            GetModuleFileNameA(*Handle, Filename, 260);
            if (std::strstr(Filename, "steam_api") || std::strstr(Filename, "Platformwrapper"))
            {
                constexpr auto *Clientlibrary = sizeof(void *) == sizeof(uint64_t) ? "steamclient64.dll" : "steamclient.dll";
                *Handle = GetModuleHandleA(Clientlibrary);
            }

            return Result;
        }

        return 0;
    }
    void Redirectmodulehandle()
    {
        Originalfunction = GetProcAddress(LoadLibraryA("kernel32.dll"), "GetModuleHandleExA");
        Mhook_SetHook(&Originalfunction, Callback);
    }

    // Block and wait for Steams IPC initialization event as some games need it.
    void InitializeIPC()
    {
        static bool Initialized = false;
        if (Initialized) return;
        Initialized = true;

        SECURITY_DESCRIPTOR pSecurityDescriptor;
        InitializeSecurityDescriptor(&pSecurityDescriptor, 1);
        SetSecurityDescriptorDacl(&pSecurityDescriptor, 1, 0, 0);

        SECURITY_ATTRIBUTES SemaphoreAttributes;
        SemaphoreAttributes.nLength = 12;
        SemaphoreAttributes.bInheritHandle = FALSE;
        SemaphoreAttributes.lpSecurityDescriptor = &pSecurityDescriptor;

        const auto Consumesemaphore = CreateSemaphoreA(&SemaphoreAttributes, 0, 512, "STEAM_DIPC_CONSUME");
        const auto Producesemaphore = CreateSemaphoreA(&SemaphoreAttributes, 1, 512, "SREAM_DIPC_PRODUCE");    // Intentional typo.
        const auto Sharedfilehandle = CreateFileMappingA(INVALID_HANDLE_VALUE, &SemaphoreAttributes, 4u, 0, 0x1000u, "STEAM_DRM_IPC");
        const auto Sharedfilemapping = MapViewOfFile(Sharedfilehandle, 0xF001Fu, 0, 0, 0);

        // Wait for the game to initialize or timeout if unused.
        if (NULL != WaitForSingleObject(Consumesemaphore, 20000))
        {
            ReleaseSemaphore(Producesemaphore, 1, NULL);
            UnmapViewOfFile(Sharedfilemapping);
            CloseHandle(Sharedfilehandle);
            CloseHandle(Consumesemaphore);
            CloseHandle(Producesemaphore);
            return;
        }

        // Parse the shared buffer to get the process information.
        const char *Buffer = (char *)Sharedfilemapping;
        Infoprint("Initializing Steam IPC process with:");
        Infoprint(va("ProcessID: 0x%X", *(uint32_t *)Buffer));      Buffer += sizeof(uint32_t);
        Infoprint(va("Activeprocess: 0x%X", *(uint32_t *)Buffer));  Buffer += sizeof(uint32_t);        const auto Startupmodule = Buffer;
        Infoprint(va("Startupmodule: \"%s\"", Buffer));             Buffer += std::strlen(Buffer) + 1; const auto Startevent = Buffer;
        Infoprint(va("Startupevent: \"%s\"", Buffer));              Buffer += std::strlen(Buffer) + 1;
        Infoprint(va("Terminationevent: \"%s\"", Buffer));

        // Parse the startup module.
        if (const auto Filehandle = std::fopen(Startupmodule, "rb"))
        {
            std::fseek(Filehandle, SEEK_SET, SEEK_END);
            const auto Size = std::ftell(Filehandle);
            std::rewind(Filehandle);

            const auto Filebuffer = std::make_unique<char[]>(Size);
            std::fread(Filebuffer.get(), Size, 1, Filehandle);
            std::fclose(Filehandle);

            uint32_t Entrysize{};
            auto Pointer = Filebuffer.get();
            Infoprint("Steam startup module with:");                Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);
            /*Infoprint(va("GUID1: %*s", Entrysize, Pointer));    */Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);
            /*Infoprint(va("GUID2: %*s", Entrysize, Pointer));    */Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);
            /*Infoprint(va("Instance: %*s", Entrysize, Pointer)); */Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);
            Infoprint(va("Module: %*s", Entrysize, Pointer));       Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);
            Infoprint(va("Path: %*s", Entrysize, Pointer));         Pointer += Entrysize; Entrysize = *(uint32_t *)Pointer; Pointer += sizeof(uint32_t);

            std::remove(Startupmodule);
        }

        // Acknowledge that the game has started.
        const auto Event = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, Startevent);
        SetEvent(Event); CloseHandle(Event);

        // Notify the game that we are done.
        ReleaseSemaphore(Producesemaphore, 1, NULL);

        // Clean up the IPC.
        UnmapViewOfFile(Sharedfilemapping);
        CloseHandle(Sharedfilehandle);
        CloseHandle(Consumesemaphore);
        CloseHandle(Producesemaphore);
    }
}
