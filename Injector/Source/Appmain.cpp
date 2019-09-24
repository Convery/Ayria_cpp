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
    std::vector<std::wstring> Arguments;
    PROCESS_INFORMATION Processinfo{};
    std::wstring Workingdirectory{};
    std::wstring Executablepath{};
    STARTUPINFOW Startupinfo{};

    // Skip the first argument if it's the bootstrapper, because Windows.
    for(int i = !!std::wcsstr(Argv[0], L"Injector"); i < Argc; ++i)
    {
        const std::wstring Argument = Argv[i];

        // Parse the executables location.
        if(Executablepath.empty() && Argument.rfind(L".exe") != std::wstring::npos)
        {
            Executablepath = Argv[i];
            continue;
        }

        // If the first argument after the executable is a sub-path, assume CWD.
        if(Workingdirectory.empty() && !!std::wcsstr(Executablepath.c_str(), Argv[i]))
        {
            Workingdirectory = Argv[i];
            continue;
        }

        // Take the rest as startup-arguments.
        Arguments.push_back(Argv[i]);
    }

    // Sanity-checking.
    if(Executablepath.empty())
    {
        std::wprintf(L"Missing commands. Usage: Injector**.exe \"app_path.exe\" <working_directory> [startup args for app]\n");
        return 1;
    }
    else
    {
        if(Workingdirectory.empty())
        {
            Workingdirectory = Executablepath.substr(0, Executablepath.find_last_of(L'\\'));
        }
    }

    // Find if the executable matches our configuration.
    if(const auto Filehandle = _wfopen(Executablepath.c_str(), L"rb"))
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
                if(CreateProcessW(sizeof(void *) == sizeof(uint64_t) ? L"Injector32.exe " : L"Injector64.exe ",
                                  GetCommandLineW(), NULL, NULL, NULL, NULL, NULL, NULL, &Startupinfo, &Processinfo))
                {
                    WaitForSingleObject(Processinfo.hProcess, INFINITE);
                    return 0;
                }

            }
            else
            {
                // Apparently we are allowed to modify the command-line in wide mode..
                std::wmemcpy(std::wcsstr(GetCommandLineW(), L".exe") - 2, sizeof(void *) == sizeof(uint64_t) ? L"32" : L"64", 2);
                if(CreateProcessW(NULL, GetCommandLineW(), NULL, NULL, NULL, NULL, NULL, NULL, &Startupinfo, &Processinfo))
                {
                    WaitForSingleObject(Processinfo.hProcess, INFINITE);
                    return 0;
                }
            }
        }
    }

    // Windows requires a mutable string for the command-line.
    for(const auto &Item : Arguments) Executablepath += Item + L" ";
    wchar_t *Buffer = (wchar_t *)alloca(Executablepath.size() * sizeof(wchar_t) + sizeof(wchar_t));
    std::wmemset(Buffer, L'\0', Executablepath.size() + 1);
    std::wmemcpy(Buffer, Executablepath.c_str(), Executablepath.size());

    // Spawn the application and inject.
    if(CreateProcessW(NULL, Buffer, NULL, NULL, NULL, CREATE_SUSPENDED | DETACHED_PROCESS, NULL, Workingdirectory.c_str(), &Startupinfo, &Processinfo))
    {
        wchar_t CWD[512]{};
        GetCurrentDirectoryW(512, CWD);

        std::wstring DLLPath(CWD);
        DLLPath += sizeof(void *) == sizeof(uint64_t) ? L"\\Bootstrapper64.dll" : L"\\Bootstrapper32.dll";

        auto Libraryaddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
        auto Remotestring = VirtualAllocEx(Processinfo.hProcess, NULL, DLLPath.size() * sizeof(wchar_t) + sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        WriteProcessMemory(Processinfo.hProcess, Remotestring, DLLPath.c_str(), DLLPath.size() * sizeof(wchar_t) + sizeof(wchar_t), NULL);
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
