/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common.hpp"

namespace Pluginloader
{
    std::unordered_set<size_t> Pluginhandles{};

    extern "C"
    {
        EXPORT_ATTR void __cdecl onInitialized(bool)
        {
            // Only notify the plugins once to prevent confusion.
            static bool Initialized = false;
            if (Initialized) return;
            Initialized = true;

            for (const auto &Item : Pluginhandles)
            {
                if (const auto Callback = GetProcAddress(HMODULE(Item), "onInitialized"))
                {
                    (reinterpret_cast<void(__cdecl *)(bool)>(Callback))(false);
                }
            }
        }
    }

    void Loadplugins()
    {
        // Initialize our subsystems.
        onStartup();

        // Load all plugins from disk.
        for (const auto &Item : FS::Findfiles("./Ayria/Plugins", Build::is64bit ? "64" : "32"))
        {
            if (const auto Module = LoadLibraryA(va("./Ayria/Plugins/%s", Item.c_str()).c_str()))
            {
                Pluginhandles.insert(size_t(Module));
            }
        }

        // Notify the plugins about startup.
        for (const auto &Item : Pluginhandles)
        {
            if (const auto Callback = GetProcAddress(HMODULE(Item), "onStartup"))
            {
                (reinterpret_cast<void(__cdecl *)(bool)>(Callback))(false);
            }
        }

        // Ensure that a "onInitialized" is sent 'soon'.
        static auto doOnce{ []()-> bool { std::thread([]() {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                onInitialized(false);
            }).detach();
            return {};
        }() };
    }
}
