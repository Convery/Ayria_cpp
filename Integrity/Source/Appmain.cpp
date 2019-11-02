/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-10-20
    License: MIT
*/

#include "Stdinclude.hpp"
#include <winternl.h>
#include <ntstatus.h>

#pragma comment(lib, "ntdll")

//#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#if _WIN64
#define pInstruction Rip
#else
#define pInstruction Eip
#endif

namespace HWBP
{
    enum Access : uint8_t { REMOVE = 0, READ = 1, WRITE = 2, EXEC = 4 };
    namespace Internal
    {
        struct Context_t
        {
            uintptr_t Address;
            HANDLE Threadhandle;
            uint8_t Accessflags;
            uint8_t Size;
        };
        DWORD __stdcall Thread(void *Param)
        {
            Context_t *Context = static_cast<Context_t *>(Param);
            CONTEXT Threadcontext{ CONTEXT_DEBUG_REGISTERS };

            // MSDN says thread-context on running threads is UB.
            if (DWORD(-1) == SuspendThread(Context->Threadhandle))
                return GetLastError();

            // Get the current debug-registers.
            if (!GetThreadContext(Context->Threadhandle, &Threadcontext))
                return GetLastError();

            // Special case: Removal of a breakpoint.
            if (Context->Accessflags == Access::REMOVE)
            {
                do
                {
                    if (Context->Address == Threadcontext.Dr0)
                    {
                        Threadcontext.Dr7 &= ~1;
                        Threadcontext.Dr0 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr1)
                    {
                        Threadcontext.Dr7 &= ~4;
                        Threadcontext.Dr1 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr2)
                    {
                        Threadcontext.Dr7 &= ~16;
                        Threadcontext.Dr2 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr3)
                    {
                        Threadcontext.Dr7 &= ~64;
                        Threadcontext.Dr3 = 0;
                        break;
                    }

                    // Nothing to do..
                    return 0;
                } while (false);

                Threadcontext.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                SetThreadContext(Context->Threadhandle, &Threadcontext);
                ResumeThread(Context->Threadhandle);
                return 0;
            }

            // Do we even have any available global registers?
            if (Threadcontext.Dr7 & 0x55) return DWORD(-1);

            // Convert the flags to x86-global.
            uint8_t Accessflags = (Context->Accessflags & Access::WRITE * 3) | (Context->Accessflags & Access::READ);
            uint8_t Sizeflags = 3 & (Context->Size % 8 == 0 ? 2 : Context->Size - 1);

            // Find the empty register and set it.
            for (int i = 0; i < 4; ++i)
            {
                if (!(Threadcontext.Dr7 & (1 << (i * 2))))
                {
                    #define Setbits(Register, Low, Count, Value) \
                    Register = (Register & ~(((1 << (Count)) - 1) << (Low)) | ((Value) << (Low)));
                    Setbits(Threadcontext.Dr7, 16 + i * 4, 2, Accessflags);
                    Setbits(Threadcontext.Dr7, 18 + i * 4, 2, Sizeflags);
                    Setbits(Threadcontext.Dr7, i * 2, 1, 1);
                    #undef Setbits

                    Threadcontext.Dr0 = i == 0 ? Context->Address : Threadcontext.Dr0;
                    Threadcontext.Dr1 = i == 1 ? Context->Address : Threadcontext.Dr1;
                    Threadcontext.Dr2 = i == 2 ? Context->Address : Threadcontext.Dr2;
                    Threadcontext.Dr3 = i == 3 ? Context->Address : Threadcontext.Dr3;

                    Threadcontext.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    SetThreadContext(Context->Threadhandle, &Threadcontext);
                    ResumeThread(Context->Threadhandle);
                    return 0;
                }
            }

            // WTF?
            return DWORD(-1);
        }
    }

    DWORD Set(HANDLE Threadhandle, uintptr_t Address, uint8_t Accessflags = Access::EXEC, uint8_t Size = 1)
    {
        Internal::Context_t Context{ Address, Threadhandle, Accessflags, Size };

        // Ensure that we have access rights.
        if (Context.Threadhandle == GetCurrentThread())
            Context.Threadhandle = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT |
                                              THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                                              FALSE, GetCurrentThreadId());

        // We need to suspend the current thread, so create a new one.
        auto Localthread = CreateThread(0, 0, Internal::Thread, &Context, 0, 0);
        if (!Localthread) return GetLastError();

        DWORD Result{}; // Wait for the thread to finish.
        while (!GetExitCodeThread(Localthread, &Result) || Result == STILL_ACTIVE) Sleep(0);

        // Close the handle if we opened it.
        if (Context.Threadhandle == GetCurrentThread())
            CloseHandle(Context.Threadhandle);

        return Result;
    }
    DWORD Set(uintptr_t Address, uint8_t Accessflags = Access::EXEC, uint8_t Size = 1)
    {
        return Set(GetCurrentThread(), Address, Accessflags, Size);
    }
    DWORD Remove(HANDLE Threadhandle, uintptr_t Address)
    {
        return Set(Threadhandle, Address, Access::REMOVE);
    }
    DWORD Remove(uintptr_t Address)
    {
        return Remove(GetCurrentThread(), Address);
    }
}

namespace Helpers
{
    uintptr_t getStartaddress(HANDLE Threadhandle)
    {
        uintptr_t Address{};
        HANDLE Duplicate;

        // Reopen the handle with proper access.
        if (!DuplicateHandle(GetCurrentProcess(), Threadhandle, GetCurrentProcess(), &Duplicate, THREAD_QUERY_INFORMATION, FALSE, 0))
            return 0;

        // Query windows with class ThreadQuerySetWin32StartAddress
        NtQueryInformationThread(Duplicate, (THREADINFOCLASS)9, &Address, sizeof(uintptr_t), NULL);
        CloseHandle(Duplicate);
        return Address;
    }
}

namespace Customcontext
{
    std::unordered_map<HANDLE, CONTEXT> Savedcontext;
    BOOL WINAPI getContext(HANDLE hThread, LPCONTEXT lpContext)
    {
        auto Result = Savedcontext.find(hThread);
        if (Result != Savedcontext.end())
        {
            *lpContext = Result->second;
            return TRUE;
        }

        return GetThreadContext(hThread, lpContext);
    }
}

std::unordered_map<HANDLE, uintptr_t> Applicationthreads;
std::vector<std::pair<uintptr_t, uintptr_t>> Hookedaddresses;
LONG __stdcall Exceptionhandler(EXCEPTION_POINTERS *Exceptioninfo)
{
    // Not very interesting.
    if (Exceptioninfo->ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;

    // Hooked functions.
    for (const auto &Item : Hookedaddresses)
    {
        if (Item.first == Exceptioninfo->ContextRecord->pInstruction)
        {
            Exceptioninfo->ContextRecord->pInstruction = Item.second;
            Exceptioninfo->ContextRecord->Dr6 = 0;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

namespace NTDll
{
    void *RealNTClose{ NtClose };
    NTSTATUS __stdcall CustomNTClose(HANDLE Handle)
    {
        DWORD Flags{};
        if (!GetHandleInformation(Handle, &Flags))
        {
            return STATUS_INVALID_HANDLE;
        }

        if (Flags & HANDLE_FLAG_PROTECT_FROM_CLOSE)
        {
            return STATUS_SUCCESS;
        }

        return ((decltype(NtClose) *)RealNTClose)(Handle);
    };

    void *RealNTQuerysystem{ NtQuerySystemInformation };
    NTSTATUS __stdcall CustomNTQuerysystem(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
                                           ULONG SystemInformationLength, PULONG ReturnLength)
    {
        const auto Result = ((decltype(NtQuerySystemInformation) *)RealNTQuerysystem)(SystemInformationClass, SystemInformation,
                                                                                      SystemInformationLength, ReturnLength);
        // SystemKernelDebuggerInformation
        if (SystemInformationClass == 35)
        {
            struct Kerneldbg { BOOLEAN Enabled; BOOLEAN notPresent; };
            auto Info = static_cast<Kerneldbg *>(SystemInformation);
            Info->notPresent = TRUE;
            Info->Enabled = FALSE;
        }

        return Result;
    }

    void *RealNTQueryprocess{ NtQueryInformationProcess };
    NTSTATUS __stdcall CustomNTQueryprocess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                            PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
    {
        const auto Result = ((decltype(NtQueryInformationProcess) *)RealNTQueryprocess)(ProcessHandle, ProcessInformationClass,
                                                                                        ProcessInformation, ProcessInformationLength,
                                                                                        ReturnLength);

        if (ProcessInformationClass == ProcessDebugPort)
        {
            *static_cast<DWORD_PTR *>(ProcessInformation) = 0;
        }
        if (ProcessInformationClass == 30)
        {
            *static_cast<DWORD_PTR *>(ProcessInformation) = 0;
        }
        if (ProcessInformationClass == 31)
        {
            *static_cast<DWORD_PTR *>(ProcessInformation) = 1;
        }

        return Result;
    }

    void *RealNTCreate{ NtCreateFile };
    NTSTATUS __stdcall CustomNTCreate(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                                      PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
                                      ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
                                      ULONG EaLength)
    {
        // Opening a exe is bad..
        if (auto Pos = std::wcsstr(ObjectAttributes->ObjectName->Buffer, L".exe"))
        {
            std::wmemcpy(Pos, L".bak", 4);
            CreateDisposition |= FILE_OPEN_IF;
        }

        return ((decltype(NtCreateFile) *)RealNTCreate)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
                                                        AllocationSize, FileAttributes, ShareAccess, CreateDisposition,
                                                        CreateOptions, EaBuffer, EaLength);
    }

}
namespace Kernel32
{
    void *RealClosehandle{ CloseHandle };
    BOOL __stdcall CustomClosehandle(HANDLE Handle)
    {
        DWORD Flags{};
        if (!GetHandleInformation(Handle, &Flags))
        {
            return FALSE;
        }

        if (Flags & HANDLE_FLAG_PROTECT_FROM_CLOSE)
        {
            return TRUE;
        }

        return ((decltype(CloseHandle) *)RealClosehandle)(Handle);
    };

    void *RealisDebugged{ IsDebuggerPresent };
    BOOL __stdcall CustomisDebugged()
    {
        // Only intercept the main module.
        if (Applicationthreads.find(GetCurrentThread()) == Applicationthreads.end())
            return FALSE;

        return ((decltype(IsDebuggerPresent) *)RealisDebugged)();
    }
}

// Entrypoint when loaded as a plugin.
extern "C"
{
    EXPORT_ATTR void onStartup(bool)
    {
        AddVectoredExceptionHandler(1, Exceptionhandler);

        Mhook_SetHook(&NTDll::RealNTClose, NTDll::CustomNTClose);
        Mhook_SetHook(&NTDll::RealNTCreate, NTDll::CustomNTCreate);
        Mhook_SetHook(&NTDll::RealNTQuerysystem, NTDll::CustomNTQuerysystem);
        Mhook_SetHook(&NTDll::RealNTQueryprocess, NTDll::CustomNTQueryprocess);

        Mhook_SetHook(&Kernel32::RealisDebugged, Kernel32::CustomisDebugged);
        Mhook_SetHook(&Kernel32::RealClosehandle, Kernel32::CustomClosehandle);
    }
    EXPORT_ATTR void onInitialized(bool) { /* Do .data edits */ }
    EXPORT_ATTR bool onMessage(const void *, uint32_t) { return false; }
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_THREAD_ATTACH)
    {
        HMODULE Modulehandle;
        const auto Thread = GetCurrentThread();
        const auto Address = Helpers::getStartaddress(Thread);

        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)Address, &Modulehandle))
        {
            char Filename[512]{};
            GetModuleFileNameA(Modulehandle, Filename, 512);
            if (std::strstr(Filename, ".exe")) Applicationthreads[Thread] = Address;
        }
    }
    if (nReason == DLL_THREAD_DETACH)
    {
        auto Result = Applicationthreads.find(GetCurrentThread());
        if (Result != Applicationthreads.end())
            Applicationthreads.erase(Result);
    }

    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayria's default directories exist.
        _mkdir("./Ayria/");
        _mkdir("./Ayria/Plugins/");
        _mkdir("./Ayria/Assets/");
        _mkdir("./Ayria/Local/");
        _mkdir("./Ayria/Logs/");

        // Only keep a log for this session.
        Logging::Clearlog();
    }
    return TRUE;
}
#else
__attribute__((constructor)) void DllMain()
{
    // Ensure that Ayrias default directories exist.
    mkdir("./Ayria/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Plugins/", S_IRWXU | S_IRWXG);
    mkdir("./Ayria/Assets/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Local/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Logs/", S_IRWU | S_IRWG);

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif
