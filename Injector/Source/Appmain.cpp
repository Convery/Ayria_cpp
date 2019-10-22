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
    std::wstring Workingdirectory{};
    std::wstring Executablepath{};
    STARTUPINFOW Startupinfo{};
    std::wstring Executable{};
    wchar_t Basemodule[512]{};
    std::wstring Arguments{};

    // Skip the first argument if it's the bootstrapper, because Windows.
    for (int i = !!std::wcsstr(Argv[0], L"Injector"); i < Argc; ++i)
    {
        std::wstring_view Argument = Argv[i];

        // Parse the executable-name.
        if (Executable.empty() && Argument.rfind(L".exe") != std::wstring::npos)
        {
            Executable = Argument.substr(Argument.find_last_of(L'\\'));
            if (Executable[0] == L'\\') Executable.erase(0, 1);
            Executablepath = Argument;
            continue;
        }

        // If the first argument after the executable is a sub-path, assume CWD.
        if (Workingdirectory.empty())
        {
            if (Argument.find_last_of(L'\\') != std::wstring::npos)
            {
                Workingdirectory = Argument;
                continue;
            }

            if (Executablepath.find_last_of(L'\\') != std::wstring::npos)
            {
                Workingdirectory = Executablepath.substr(0, Executablepath.find_last_of(L'\\') + 1);
            }
            else
            {
                GetModuleFileNameW(NULL, Basemodule, 512); Workingdirectory = Basemodule;
                Workingdirectory = Workingdirectory.substr(0, Workingdirectory.find_last_of(L'\\') + 1);
            }
        }

        // Take the rest as startup-arguments.
        Arguments += Argument.data() + L" "s;
    }

    // Sanity-checking.
    if (Executable.empty())
    {
        std::wprintf(L"Missing commands. Usage: Injector**.exe \"app.exe\" <working_directory> [startup args for app]\n");
        return 1;
    }
    if (Workingdirectory.empty())
    {
        if (Executablepath.find_last_of(L'\\') != std::wstring::npos)
        {
            Workingdirectory = Executablepath.substr(0, Executablepath.find_last_of(L'\\') + 1);
        }
        else
        {
            GetModuleFileNameW(NULL, Basemodule, 512); Workingdirectory = Basemodule;
            Workingdirectory = Workingdirectory.substr(0, Workingdirectory.find_last_of(L'\\') + 1);
        }
    }

    // Find if the executable matches our configuration.
    if(const auto Filehandle = _wfopen((Workingdirectory + Executable).c_str(), L"rb"))
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

    // Windows requires a mutable string for the command-line.
    wchar_t *Buffer = (wchar_t *)alloca(Arguments.size() * sizeof(wchar_t) + sizeof(wchar_t));
    std::wmemcpy(Buffer, Arguments.c_str(), Arguments.size() + 1);

    // Spawn the application and inject.
    if(CreateProcessW((Workingdirectory + Executable).c_str(), Buffer, NULL, NULL, NULL, CREATE_SUSPENDED | DETACHED_PROCESS, NULL, Workingdirectory.c_str(), &Startupinfo, &Processinfo))
    {
        GetModuleFileNameW(NULL, Basemodule, 512);
        std::wstring Modulepath = Basemodule;

        Modulepath = Modulepath.substr(0, Modulepath.find_last_of(L'\\'));
        Modulepath += sizeof(void *) == sizeof(uint64_t) ? L"\\Bootstrapper64.dll" : L"\\Bootstrapper32.dll";

        auto Libraryaddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
        auto Remotestring = VirtualAllocEx(Processinfo.hProcess, NULL, Modulepath.size() * sizeof(wchar_t) + sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

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
