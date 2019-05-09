/*
    Initial author: Convery (tcn@ayria.se)
    Started: 11-04-2019
    License: MIT
*/

#pragma once
#include <memory>
#include "Memprotect.hpp"

// This is a really simple hook that should not be used naively.
namespace Simplehook
{
    struct Stomphook
    {
        uint8_t Originalstub[14]{};
        void *Savedlocation{};
        void *Savedtarget{};

        void Installhook(void *Location = nullptr, void *Target = nullptr)
        {
            if (!Location) Location = Savedlocation;
            if (!Target) Target = Savedtarget;
            Savedlocation = Location;
            Savedtarget = Target;

            auto Protection = Memprotect::Unprotectrange(Location, 14);
            {
                // Copy the original stub.
                std::memcpy(Originalstub, (void *)Location, 14);

                // Simple stomp-hooking.
                if constexpr (sizeof(void *) == sizeof(uint64_t))
                {
                    // JMP [RIP + 0], Target
                    *(uint16_t *)((uint8_t *)Location + 0) = 0x25FF;
                    *(uint32_t *)((uint8_t *)Location + 2) = 0x000000000;
                    *(uint64_t *)((uint8_t *)Location + 6) = (size_t)&Target;
                }
                else
                {
                    // JMP short Target
                    *(uint8_t *)((uint8_t *)Location + 0) = 0xE9;
                    *(size_t  *)((uint8_t *)Location + 1) = ((size_t)Target - ((size_t)Location + 5));
                }
            }
            Memprotect::Protectrange(Location, 14, Protection);
        }
        void Installhook(std::uintptr_t Target, std::uintptr_t Location)
        {
            return Installhook(reinterpret_cast<void *>(Target), reinterpret_cast<void *>(Location));
        }
        void Installhook(void *Target, std::uintptr_t Location)
        {
            return Installhook(reinterpret_cast<void *>(Target), reinterpret_cast<void *>(Location));
        }
        void Installhook(std::uintptr_t Target, void *Location = nullptr)
        {
            return Installhook(reinterpret_cast<void *>(Target), reinterpret_cast<void *>(Location));
        }

        void Removehook()
        {
            if (Savedlocation)
            {
                auto Protection = Memprotect::Unprotectrange(Savedlocation, 14);
                {
                    std::memcpy((void *)Savedlocation, Originalstub, 14);
                }
                Memprotect::Protectrange(Savedlocation, 14, Protection);
            }
        }
    };
}
