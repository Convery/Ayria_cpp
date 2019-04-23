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
        void *Savedlocation;
        void *Savedtarget;

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
                    // JMP [RIP + 0]
                    std::memcpy((uint8_t *)Location, "\xFF\x25", 2);
                    std::memcpy((uint8_t *)Location + 2, "\x00\x00\x00\x00", 4);
                    std::memcpy((uint8_t *)Location + 6, &Target, sizeof(size_t));
                }
                else
                {
                    // JMP short
                    *(uint8_t *)((uint8_t *)Location + 0) = 0xE9;
                    *(size_t  *)((uint8_t *)Location + 1) = ((size_t)Target - ((size_t)Location + 5));
                }
            }
            Memprotect::Protectrange(Location, 14, Protection);
        }
        void Installhook(std::uintptr_t Target, std::uintptr_t Location = 0)
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
