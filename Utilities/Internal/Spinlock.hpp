/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-20
    License: MIT

    STL compatible lock.
*/

#pragma once
#include <thread>
#include <atomic>
#include <emmintrin.h>

struct Spinlock
{
    std::atomic_flag Flag = ATOMIC_FLAG_INIT;

    void unlock() noexcept { Flag.clear(std::memory_order_release); }
    bool try_lock() noexcept { return !Flag.test_and_set(std::memory_order_acquire); }

    void lock() noexcept
    {
        for (size_t i = 0; i < 16; ++i) { if (try_lock()) return; }
        for (size_t i = 0; i < 128; ++i) { if (try_lock()) return; _mm_pause(); }

        while(true)
        {
            for (size_t i = 0; i < 1024; ++i)
            {
                if (try_lock()) return;
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
            }

            std::this_thread::yield();
        }
    }
};
