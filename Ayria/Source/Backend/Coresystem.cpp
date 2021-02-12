/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-15
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    using Task_t = struct { uint32_t Last, Period; void(__cdecl *Callback)(); };
    static Inlinedvector<Task_t, 8> Backgroundtasks{};

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t Period, void(__cdecl *Callback)())
    {
        Backgroundtasks.push_back({ 0, Period, Callback });
    }

    // TODO(tcn): Investigate if we can merge these threads.
    [[noreturn]] static DWORD __stdcall Backgroundthread(void *)
    {
        // Name this thread for easier debugging.
        setThreadname("Ayria_Background");

        // Main loop, runs until the application terminates or DLL unloads.
        while (true)
        {
            // Notify the subsystems about a new frame.
            const auto Currenttime = GetTickCount();
            for (auto &Task : Backgroundtasks)
            {
                if ((Task.Last + Task.Period) < Currenttime) [[unlikely]]
                {
                    Task.Last = Currenttime;
                    Task.Callback();
                }
            }

            // Most tasks run with periods in seconds.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    [[noreturn]] static DWORD __stdcall Graphicsthread(void *)
    {
        // UI-thread, boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        setThreadname("Ayria_Graphics");

        // Disable DPI scaling on Windows 10.
        if (const auto Callback = GetProcAddress(GetModuleHandleA("User32.dll"), "SetThreadDpiAwarenessContext"))
            reinterpret_cast<size_t (__stdcall *)(size_t)>(Callback)(static_cast<size_t>(-2));

        // Initialize the subsystems.
        // TODO(tcn): Initialize pluginmenu, move the overlay storage somewhere.
        Overlay_t<false> Ingameconsole({}, {});
        Console::Overlay::Createconsole(&Ingameconsole);

        // Optional console for developers.
        if (Global.enableExternalconsole) Console::Windows::Createconsole(GetModuleHandleW(NULL));

        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe{};
        while (true)
        {
            // Track the frame-time, should be less than 33ms at worst.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = Thisframe - Lastframe;
            Lastframe = Thisframe;

            // Notify all elements about the frame.
            Ingameconsole.doFrame(std::chrono::duration<float>(Deltatime).count());
            Console::Windows::doFrame();

            // Log frame-average every 5 seconds.
            if constexpr (Build::isDebug)
            {
                static std::array<uint64_t, 256> Timings{};
                static float Elapsedtime{};
                static size_t Index{};

                Timings[Index % 256] = std::chrono::duration_cast<std::chrono::nanoseconds>(Deltatime).count();
                Elapsedtime += std::chrono::duration<float>(Deltatime).count();
                Index++;

                if (Elapsedtime >= 5.0f)
                {
                    const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                    constexpr auto NPS = 1000000000.0;
                    const auto Avg = Sum / 256;

                    Debugprint(va("Average framerate: %5.2f FPS %8lu ns", double(NPS / Avg), Avg));
                    Elapsedtime = 0;
                }
            }

            // Cap the FPS to ~60, as we only render if dirty we can get thousands of FPS.
            std::this_thread::sleep_until(Thisframe + std::chrono::milliseconds(1000 / 60));
        }
    }

    // Save the configuration to disk.
    void Saveconfig()
    {
        JSON::Object_t Config{};
        Config["enableExternalconsole"] = Global.enableExternalconsole;
        Config["Username"] = std::u8string_view(Global.Username);
        Config["Locale"] = std::u8string_view(Global.Locale);
        Config["enableIATHooking"] = Global.enableIATHooking;
        Config["UserID"] = Global.UserID;

        FS::Writefile(L"./Ayria/Settings.json", JSON::Dump(Config));
    }

    // Export functionality to the plugins.
    namespace Export
    {
        extern "C" EXPORT_ATTR void __cdecl Createperiodictask(unsigned int PeriodMS, void(__cdecl *Callback)())
        {
            if (PeriodMS && Callback)
                Enqueuetask(PeriodMS, Callback);
        }
    }

    // Initialize the system.
    void Initialize()
    {
        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // As of Windows 10 update 20H2 (v2004) we need to set the interrupt resolution for each process.
        timeBeginPeriod(1);

        // Initialize the global state from disk settings.
        {
            const auto Config = JSON::Parse(FS::Readfile<char>(L"./Ayria/Settings.json"));
            Global.enableExternalconsole = Config.value<bool>("enableExternalconsole");
            Global.enableIATHooking = Config.value<bool>("enableIATHooking");
            Global.UserID = Config.value("UserID", 0xDEADC0DE);

            const auto Locale = Config.value("Locale", u8"english"s);
            const auto Username = Config.value("Username", u8"AYRIA"s);
            std::memcpy(Global.Locale, Locale.data(), std::min(Locale.size(), sizeof(Global.Locale) - 1));
            std::memcpy(Global.Username, Username.data(), std::min(Username.size(), sizeof(Global.Username) - 1));

            // Save the configuration on exit.
            std::atexit([]() { if (Global.modifiedConfig) Saveconfig(); });
        }

        // Initialize subsystems that plugins may need.
        //Matchmaking::API_Initialize();
        Clientinfo::Initialize();
        Fileshare::Initialize();
        Network::Initialize();
        Social::Initialize();

        // Workers.
        CreateThread(NULL, NULL, Graphicsthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        CreateThread(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }
}
