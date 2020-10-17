/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-15
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    static DWORD __stdcall Graphicsthread(void *)
    {
        // UI-thread, boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        setThreadname("Ayria_Graphics");

        // Initialize the subsystems.
        // TODO(tcn): Initialize pluginmenu.
        Overlay_t Ingameconsole({}, {});
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

            // For reference, my workstations E5 hits 200FPS while laptops i5 gets 50FPS. Cap to 60.
            std::this_thread::sleep_until(Lastframe + std::chrono::milliseconds(16));
        }

        return 0;
    }
    static DWORD __stdcall Backgroundthread(void *)
    {
        // Mainly IO bound thread, might as well sleep a lot.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

        // Name this thread for easier debugging.
        setThreadname("Ayria_Background");

        // Main loop, runs until the application terminates or DLL unloads.
        while (true)
        {
            // Notify the subsystems about a new frame.
            Matchmaking::doFrame();
            Clientinfo::doFrame();
            Updatenetworking();

            // TODO(tcn): Get async-job requests and process them here.
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
        Clientinfo::Initialize();

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
