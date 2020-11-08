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
    std::vector<Task_t> Backgroundtasks;

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t Period, void(__cdecl *Callback)())
    {
        Backgroundtasks.push_back({ 0, Period, Callback });
    }

    // TODO(tcn): Investigate if we can merge these threads.
    static DWORD __stdcall Backgroundthread(void *)
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

        return 0;
    }
    static DWORD __stdcall Graphicsthread(void *)
    {
        // UI-thread, boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        setThreadname("Ayria_Graphics");

        // Initialize the subsystems.
        // TODO(tcn): Initialize pluginmenu.
        Overlay_t<> Ingameconsole({}, {});
        Console::Overlay::Createconsole(&Ingameconsole);

        // Optional console for developers, runs its own thread.
        if (std::strstr(GetCommandLineA(), "-DEVCON")) Console::Windows::Createconsole(GetModuleHandleW((NULL)));

        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe{};
        while (true)
        {
            // Track the frame-time, should be less than 33ms at worst.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
            Lastframe = Thisframe;

            // Notify all elements about the frame.
            Ingameconsole.doFrame(Deltatime);
            Console::Windows::doFrame();

            // Log frame-average every 5 seconds.
            if constexpr (Build::isDebug)
            {
                static std::array<uint32_t, 256> Timings{};
                static float Elapsedtime{};
                static size_t Index{};

                Timings[Index % 256] = 1000000 * Deltatime;
                Elapsedtime += Deltatime;
                Index++;

                if (Elapsedtime >= 5.0f)
                {
                    const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                    Debugprint(va("Average framerate: %5.2f FPS %5lu us", 1000000.0 / (Sum / 256), Sum / 256));
                    Elapsedtime = 0;
                }
            }

            // Cap the FPS to ~60, as we only render if dirty we can get thousands of FPS.
            std::this_thread::sleep_until(Lastframe + std::chrono::milliseconds(16));
        }

        return 0;
    }

    // Initialize the system.
    void Initialize()
    {
        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // Initialize subsystems that plugins may need.
        Matchmaking::API_Initialize();
        Clientinfo::API_Initialize();
        Backend::API_Initialize();
        Social::API_Initialize();

        // Backend background tasks.
        Enqueuetask(0, Updatenetworking);

        // Default network groups.
        Joinmessagegroup(Generalport);
        Joinmessagegroup(Pluginsport);
        Joinmessagegroup(Matchmakeport);
        Joinmessagegroup(Fileshareport);

        // Workers.
        CreateThread(NULL, NULL, Graphicsthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        CreateThread(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }
}
