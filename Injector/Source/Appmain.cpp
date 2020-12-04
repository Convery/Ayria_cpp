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
    PROCESS_INFORMATION Processinfo;
    STARTUPINFOW Startupinfo{};

    std::wstring Commandline;
    bool okVersion{};
    bool okConfig{};

    // Sanity-checking the config and target.
    do
    {
        if (Argc >= 1 && std::wcsstr(Argv[0], L"Injector")) { Argc--; Argv++; }
        if (Argc < 1) { std::wprintf(L"Missing command-line. Usage: Injector**.exe \"app.exe\"\n"); break; }

        // Find if the executable matches our configuration.
        if (const auto Filehandle = _wfopen(Argv[0], L"rb"))
        {
            char PEBuffer[sizeof(IMAGE_NT_HEADERS)]{};
            const auto NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(PEBuffer);
            const auto DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(PEBuffer);

            do
            {
                if (!std::fread(PEBuffer, sizeof(IMAGE_DOS_HEADER), 1, Filehandle) || DOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
                {
                    std::wprintf(L"Invalid PE-executable provided. Exiting.\n");
                    break;
                }
                std::fseek(Filehandle, DOSHeader->e_lfanew, SEEK_SET);
                if (!std::fread(PEBuffer, sizeof(IMAGE_NT_HEADERS), 1, Filehandle) || NTHeader->Signature != IMAGE_NT_SIGNATURE)
                {
                    std::wprintf(L"Invalid PE-executable provided. Exiting.\n");
                    break;
                }

                // TODO(tcn): If we ever support managed executables, check the .NET segments.
                okVersion = NTHeader->FileHeader.Machine == WORD(Build::is64bit ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386);

            } while (false);

            std::fclose(Filehandle);
        }

        // All checks passed.
        okConfig = true;
    } while (false);

    // Nothing we can do about bad configs.
    if (!okConfig) return 3;

    // Create a new commandline.
    for (int i = 0; i < Argc; ++i)
    {
        Commandline += L" ";
        Commandline += Argv[i];
    }

    // Switch version of the injector.
    if (!okVersion)
    {
        const auto Injector = Build::is64bit ? L"Injector32.exe" : L"Injector64.exe";

        std::wprintf(L"Switching injector version.\n");
        if (CreateProcessW(Injector, (LPWSTR)Commandline.c_str(), NULL, NULL, NULL, NULL, NULL, NULL, &Startupinfo, &Processinfo))
        {
            WaitForSingleObject(Processinfo.hProcess, INFINITE);
        }

        return 0;
    }

    // Spawn the application and inject.
    if (CreateProcessW(NULL, (LPWSTR)Commandline.c_str(), NULL, NULL, NULL, CREATE_SUSPENDED | DETACHED_PROCESS, NULL, NULL, &Startupinfo, &Processinfo))
    {
        wchar_t CWD[512]{}; GetCurrentDirectoryW(512, CWD);
        const auto Modulepath = std::wstring(CWD) + (Build::is64bit ? L"\\Bootstrapper64.dll"s : L"\\Bootstrapper32.dll"s);
        const auto Remotealloc = VirtualAllocEx(Processinfo.hProcess, NULL, Modulepath.size() * sizeof(wchar_t) + sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        const auto Remotestring = WriteProcessMemory(Processinfo.hProcess, Remotealloc, Modulepath.c_str(), Modulepath.size() * sizeof(wchar_t) + sizeof(wchar_t), NULL);

        if (!Remotealloc || !Remotestring)
        {
            std::wprintf(L"Could allocate the library string. Error: %u\n", GetLastError());
            TerminateProcess(Processinfo.hProcess, 3);
            return 3;
        }

        const auto Libraryaddress = GetProcAddress(LoadLibraryW(L"kernel32.dll"), "LoadLibraryW");
        const auto Result = CreateRemoteThread(Processinfo.hProcess, NULL, NULL, LPTHREAD_START_ROUTINE(Libraryaddress), Remotealloc, NULL, NULL);
        if (Result == NULL)
        {
            std::wprintf(L"Could not create thread. Error: %u\n", GetLastError());
            TerminateProcess(Processinfo.hProcess, 3);
            return 3;
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
        return 3;
    }

    return 0;
}
