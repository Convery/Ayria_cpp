/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-09-24
    License: MIT

    Quick and dirty injector for Windows.
*/

#include "Stdinclude.hpp"
#include <Windows.h>

int wmain(int Argc, wchar_t **Argv)
{
    PROCESS_INFORMATION Processinfo{};
    STARTUPINFOW Startupinfo{};
    wchar_t Basemodule[512]{};

    // Sanity-checking.
    if (Argc < 2)
    {
        std::wprintf(L"Missing commandline. Usage: Injector**.exe \"app.exe\"\n");
        return 1;
    }

    // Find if the executable matches our configuration.
    if(const auto Filehandle = _wfopen(!!std::wcsstr(Argv[0], L"Injector") ? Argv[1] : Argv[0], L"rb"))
    {
        char PEBuffer[sizeof(IMAGE_NT_HEADERS)]{};
        const auto NTHeader = (PIMAGE_NT_HEADERS)PEBuffer;
        const auto DOSHeader = (PIMAGE_DOS_HEADER)PEBuffer;

        do
        {
            // Sanity-checking.
            if(!std::fread(PEBuffer, sizeof(IMAGE_DOS_HEADER), 1, Filehandle) || DOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
            {
                std::wprintf(L"Invalid PE-executable provided. Exiting.\n");
                break;
            }

            // Move to the header.
            std::fseek(Filehandle, DOSHeader->e_lfanew, SEEK_SET);

            // Sanity-checking.
            if(!std::fread(PEBuffer, sizeof(IMAGE_NT_HEADERS), 1, Filehandle) || NTHeader->Signature != IMAGE_NT_SIGNATURE)
            {
                std::wprintf(L"Invalid PE-executable provided. Exiting.\n");
                break;
            }
        } while(false);
        std::fclose(Filehandle);

        // Do we need to switch bootstrapper?
        if(NTHeader->FileHeader.Machine != WORD(sizeof(void *) == sizeof(uint64_t) ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386))
        {
            std::wprintf(L"Switching injector version.\n");

            if(!std::wcsstr(Argv[0], L"Injector"))
            {
                const auto Injector = sizeof(void *) == sizeof(uint64_t) ? L"\\Injector32.exe " : L"\\Injector64.exe ";
                if(CreateProcessW(Injector, GetCommandLineW(), NULL, NULL, NULL, NULL, NULL, NULL, &Startupinfo, &Processinfo))
                {
                    WaitForSingleObject(Processinfo.hProcess, INFINITE);
                }
            }
            else
            {
                // Apparently we are allowed to modify the command-line in wide mode..
                std::wmemcpy(std::wcsstr(GetCommandLineW(), L".exe") - 2, sizeof(void *) == sizeof(uint64_t) ? L"32" : L"64", 2);
                if(CreateProcessW(NULL, GetCommandLineW(), NULL, NULL, NULL, NULL, NULL, NULL, &Startupinfo, &Processinfo))
                {
                    WaitForSingleObject(Processinfo.hProcess, INFINITE);
                }
            }

            return 0;
        }
    }

    std::wstring Commandline = GetCommandLineW();
    Commandline.erase(0, Commandline.find(L".exe") + 6);

    // Spawn the application and inject.
    if(CreateProcessW(NULL, (wchar_t *)Commandline.c_str(), NULL, NULL, NULL, CREATE_SUSPENDED | DETACHED_PROCESS, NULL, NULL, &Startupinfo, &Processinfo))
    {
        GetModuleFileNameW(NULL, Basemodule, 512);
        std::wstring Modulepath = Basemodule;

        Modulepath = Modulepath.substr(0, Modulepath.find_last_of(L'\\'));
        Modulepath += sizeof(void *) == sizeof(uint64_t) ? L"\\Bootstrapper64.dll" : L"\\Bootstrapper32.dll";
        auto Remotestring = VirtualAllocEx(Processinfo.hProcess, NULL, Modulepath.size() * sizeof(wchar_t) + sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (!Remotestring)
        {
            std::wprintf(L"Could allocate the library string. Error: %u\n", GetLastError());
            return 1;
        }

        auto Libraryaddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
        WriteProcessMemory(Processinfo.hProcess, Remotestring, Modulepath.c_str(), Modulepath.size() * sizeof(wchar_t) + sizeof(wchar_t), NULL);
        auto Result = CreateRemoteThread(Processinfo.hProcess, NULL, NULL, LPTHREAD_START_ROUTINE(Libraryaddress), Remotestring, NULL, NULL);

        if(Result == NULL)
        {
            std::wprintf(L"Could not create thread. Error: %u\n", GetLastError());
            TerminateProcess(Processinfo.hProcess, 0xDEADC0DE);
            return 1;
        }
        else
        {
            WaitForSingleObject(Result, 3000);
            ResumeThread(Processinfo.hThread);
        }
    }
    else
    {
        std::wprintf(L"Could not create process. Error: %u\n", GetLastError());
        return 1;
    }

    return 0;
}
