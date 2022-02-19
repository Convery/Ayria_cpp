/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

namespace Core
{
    using Task_t = struct { uint32_t Last, Period; void(__cdecl *Callback)(); };
    static Inlinedvector<Task_t, 8> Backgroundtasks{};
    static Defaultmutex Threadsafe{};

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t PeriodMS, void(__cdecl *Callback)())
    {
        std::scoped_lock Guard(Threadsafe);
        Backgroundtasks.push_back({ 0, PeriodMS, Callback });
    }

    // Normal-priority thread.
    [[noreturn]] static unsigned __stdcall Backgroundthread(void *)
    {
        // Name this thread for easier debugging.
        setThreadname("Ayria_Backgroundthread");

        // Set SSE to behave properly, MSVC seems to be missing a flag.
        _mm_setcsr(_mm_getcsr() | 0x8040); // _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON

        // Runs until the application terminates or DLL unloads.
        while (true)
        {
            // Rolls over every 48 days.
            const auto Currenttime = GetTickCount();
            {
                std::scoped_lock Guard(Threadsafe);

                for (auto &Task : Backgroundtasks)
                {
                    // Take advantage of unsigned overflows.
                    if ((Currenttime - Task.Last) > Task.Period) [[unlikely]]
                    {
                        Task.Last = Currenttime;
                    Task.Callback();
                    }
                }
            }

            // Most tasks run with periods in seconds.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Export functionality to the plugins.
    extern "C" EXPORT_ATTR void __cdecl Createperiodictask(unsigned int PeriodMS, void(__cdecl * Callback)())
    {
        if (PeriodMS && Callback) [[likely]] Enqueuetask(PeriodMS, Callback);
    }

    // Access from the backend initializer.
    void Initialize()
    {
        static bool Started{};
        if (Started) return;
        Started = true;

        _beginthreadex(NULL, NULL, Backgroundthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }
}
