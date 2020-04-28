/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-28
    License: MIT

    Abstractions over hooking engines, priority:
    Polyhook::Capstone
    MHook
    Fallback
*/

#pragma once
#include <string>
#include <cstdint>
#include <cassert>
#include "Memprotect.hpp"

#if !defined(NO_HOOKLIB)
#if __has_include(<mhook-lib/mhook.h>)
#pragma comment(lib, "mhook.lib")
#include <mhook-lib/mhook.h>
#define FOUND_MHOOK

namespace Hooking
{
    namespace MHook
    {
        inline std::uintptr_t Stomphook(std::uintptr_t Target, std::uintptr_t Replacement)
        {
            void *Originalfunction = (void *)Target;
            if (Mhook_SetHook(&Originalfunction, (void *)Replacement))
                return reinterpret_cast<std::uintptr_t>(Originalfunction);
            return 0;
        }
    }
}
#endif

#if __has_include(<polyhook2/IHook.hpp>) && __has_include(<capstone/capstone.h>)
#include <polyhook2/CapstoneDisassembler.hpp>
#include <polyhook2/Detour/x86Detour.hpp>
#include <polyhook2/Detour/x64Detour.hpp>
#include <capstone/capstone.h>
#pragma comment(lib, "PolyHook_2.lib")
#pragma comment(lib, "capstone.lib")
#define FOUND_POLYHOOK

namespace Hooking
{
    namespace Polyhook
    {
        inline std::uintptr_t Stomphook(std::uintptr_t Target, std::uintptr_t Replacement)
        {
            uint64_t Originalfunction{};

            if constexpr (Build::is64bit)
            {
                static PLH::CapstoneDisassembler Disasm(PLH::Mode::x64);
                PLH::x64Detour(Target, Replacement, &Originalfunction, Disasm).hook();
            }
            else
            {
                static PLH::CapstoneDisassembler Disasm(PLH::Mode::x86);
                PLH::x86Detour(Target, Replacement, &Originalfunction, Disasm).hook();
            }

            return (std::uintptr_t)Originalfunction;
        }
    }
}
#endif
#endif

// For when the fancy libraries fail.
namespace Hooking
{
    namespace Fallback
    {
        // const 0100, W = Extended reg, R = MODRM extra, X = SIB extra, B = MODRM/SIB extra.
        struct Extensions { uint8_t Identifier : 4, W : 1, R : 1, X : 1, B : 1; };
        struct SIB { uint8_t Scale : 2, Index : 3, Base : 3; };
        struct MODRM { uint8_t Mod : 2, Reg : 3, Rm : 3; };

        // Really stupid 'disassembly'.
        inline std::basic_string<uint8_t> Disassemble(std::uintptr_t Target, size_t Minlength)
        {
            #define Add(x) { Result.append((uint8_t *)&x, sizeof(x)); Target += sizeof(x); }
            std::basic_string<uint8_t> Result;

            const auto parseReg = [&]()
            {
                const auto Size = *(size_t *)Target; Add(Size);
            };
            const auto parseIMM8 = [&]()
            {
                const auto IMM8 = *(uint8_t *)Target; Add(IMM8);
            };
            const auto parseIMM32 = [&]()
            {
                const auto IMM32 = *(uint32_t *)Target; Add(IMM32);
            };
            const auto parseMODRM = [&]()
            {
                const auto Mod = *(MODRM *)Target; Add(Mod);

                // Direct addressing mode, no offsets.
                if (Mod.Mod == 0b11) return;

                // SIB registers take an extra byte (SP, R12).
                if (Mod.Rm == 0b100 || Mod.Reg == 0b100)
                {
                    const auto Sib = *(SIB *)Target; Add(Sib);
                }

                // IMM8 offset.
                if (Mod.Mod == 0b01)
                {
                    const auto IMM8 = *(uint8_t *)Target; Add(IMM8);
                }

                // IMM32 offset.
                if (Mod.Mod == 0b10)
                {
                    const auto IMM32 = *(uint32_t *)Target; Add(IMM32);
                }

                // Reg pointer.
                if (Mod.Mod == 0b00)
                {
                    // BP registers take an extra IMM32 (BP, R13).
                    if (Mod.Rm == 0b100 || Mod.Reg == 0b100) { const auto IMM32 = *(uint32_t *)Target; Add(IMM32); }
                }
            };

            constexpr std::array<uint8_t, 256> Singleopcode
            {
                // 0 = No arg, 1 = MODRM, 2 = IMM8, 3 = IMM32
                // 4 = MODRM + IMM8, 5 = MODRM + IMM32
                // 6 = IMM32/64, 9 = No support

                //       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
                /* 00 */ 1, 1, 1, 1, 2, 3, 0, 0, 1, 1, 1, 1, 2, 3, 0, 9,
                /* 10 */ 1, 1, 1, 1, 2, 3, 0, 0, 1, 1, 1, 1, 2, 3, 0, 0,
                /* 20 */ 1, 1, 1, 1, 2, 3, 0, 0, 1, 1, 1, 1, 2, 3, 0, 0,
                /* 30 */ 1, 1, 1, 1, 2, 3, 0, 0, 1, 1, 1, 1, 2, 3, 0, 0,
                /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 60 */ 0, 0, 0, 1, 0, 0, 0, 0, 3, 5, 2, 4, 0, 0, 0, 0,
                /* 70 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                /* 80 */ 4, 5, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* A0 */ 6, 6, 6, 6, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0,
                /* B0 */ 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                /* C0 */ 4, 4, 1, 0, 0, 9, 4, 5, 9, 0, 0, 0, 0, 0, 0, 0,
                /* D0 */ 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                /* E0 */ 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 0, 2, 0, 0, 0, 0,
                /* F0 */ 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0, 1, 1,
            };
            constexpr std::array<uint8_t, 256> Doubleopcode
            {
                // 0 = No arg, 1 = MODRM, 2 = IMM8, 3 = IMM32
                // 4 = MODRM + IMM8, 5 = MODRM + IMM32
                // 6 = IMM32/64, 9 = No support

                //       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
                /* 00 */ 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 4,
                /* 10 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* 20 */ 2, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 0,
                /* 40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* 50 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* 60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1,
                /* 70 */ 4, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1,
                /* 80 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                /* 90 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* A0 */ 0, 0, 0, 1, 4, 1, 0, 0, 0, 0, 0, 1, 4, 1, 1, 1,
                /* B0 */ 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                /* C0 */ 1, 1, 4, 1, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* D0 */ 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
                /* E0 */ 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                /* F0 */ 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0
            };

            while (Result.size() < Minlength)
            {
                auto Opcode = *(uint8_t *)Target; Add(Opcode);

                // Double opcode.
                if (Opcode == 0x0F)
                {
                    Opcode = *(uint8_t *)Target; Add(Opcode);
                    switch (Doubleopcode[Opcode])
                    {
                        case 0: continue;
                        case 1: parseMODRM(); continue;
                        case 2: parseIMM8(); continue;
                        case 3: parseIMM32(); continue;
                        case 4: parseMODRM(); parseIMM8(); continue;
                        case 5: parseMODRM(); parseIMM32(); continue;
                        case 6: parseReg(); continue;

                        // ERROR
                        case 9: return {};
                    }

                    continue;
                }

                // Single opcode.
                switch (Singleopcode[Opcode])
                {
                    case 0: continue;
                    case 1: parseMODRM(); continue;
                    case 2: parseIMM8(); continue;
                    case 3: parseIMM32(); continue;
                    case 4: parseMODRM(); parseIMM8(); continue;
                    case 5: parseMODRM(); parseIMM32(); continue;
                    case 6: parseReg(); continue;

                    // ERROR
                    case 9: return {};
                }
            }
            #undef Add

            return Result;
        }
        inline std::uintptr_t Stomphook(std::uintptr_t Target, std::uintptr_t Replacement)
        {
            constexpr auto Jumpsize = Build::is64bit ? 14 : 5;
            const auto Lock = Memprotect::Makewriteable(Target, Jumpsize * 2);

            // Really stupid 'disassembly'.
            const auto Oldcode = Disassemble(Target, Jumpsize);
            if (Oldcode.empty()) return 0;

            // Allocate a trampoline.
            const auto Address = (std::uintptr_t)VirtualAlloc(NULL, Oldcode.size() + Jumpsize,
                                                              MEM_COMMIT | MEM_RESERVE,
                                                              PAGE_EXECUTE_READWRITE);
            if (!Address)return 0;

            // Add the prologue.
            std::memcpy((void *)Address, Oldcode.data(), Oldcode.size());

            // Add the jumps.
            if constexpr (Build::is64bit)
            {
                // JMP [RIP + 0] | FF 25 00 00 00 00 ... | OP MODRM IMM32 IMM64
                *(uint64_t *)(Address + Oldcode.size() + 2) = Target + Jumpsize;
                *(uint8_t *)(Address + Oldcode.size() + 1) = 0x25;
                *(uint8_t *)(Address + Oldcode.size() + 0) = 0xFF;
                *(uint64_t *)(Target + 2) = Replacement;
                *(uint8_t *)(Target + 1) = 0x25;
                *(uint8_t *)(Target + 0) = 0xFF;
            }
            else
            {
                // JMP short | E9 ... | OP IMM32
                *(uint32_t *)(Address + Oldcode.size() + 1) = Target + Jumpsize - Address - Oldcode.size() - 5;
                *(uint8_t *)(Address + Oldcode.size() + 0) = 0xE9;
                *(uint32_t *)(Target + 1) = Replacement - Target - 5;
                *(uint8_t *)(Target + 0) = 0xE9;
            }

            return Address;
        }
    }
}

namespace Hooking
{
    // Stomp-hooking inserts a jump at the target address.
    inline std::uintptr_t Stomphook(std::uintptr_t Target, std::uintptr_t Replacement)
    {
        #if defined (FOUND_POLYHOOK)
        {
            const auto Result = Polyhook::Stomphook(Target, Replacement);
            if (Result) return Result;
        }
        #endif

        #if defined (FOUND_MHOOK)
        {
            const auto Result = MHook::Stomphook(Target, Replacement);
            if (Result) return Result;
        }
        #endif

        return Fallback::Stomphook(Target, Replacement);
    }
    inline void *Stomphook(void *Target, void *Replacement)
    {
        return reinterpret_cast<void*>(Stomphook(reinterpret_cast<std::uintptr_t>(Target),
                                                 reinterpret_cast<std::uintptr_t>(Replacement)));
    }
}
