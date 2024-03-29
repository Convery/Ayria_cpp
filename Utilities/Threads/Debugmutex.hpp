/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-03-03
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

struct Debugmutex
{
    std::thread::id Currentowner{};
    std::timed_mutex Internal;

    Debugmutex() = default;
    Debugmutex(const Debugmutex&) = delete;
    Debugmutex& operator=(const Debugmutex&) = delete;
    ~Debugmutex()
    {
        (void)Internal.try_lock();
        Currentowner = {};
        Internal.unlock();
    }

    void lock()
    {
        if (Currentowner == std::this_thread::get_id())
        {
            Errorprint(va("Debugmutex: Recursive lock by thread %u!", *(uint32_t *)&Currentowner).c_str());
            volatile size_t Intentional_nullderef = 0; *(size_t *)Intentional_nullderef = 0xDEAD;
        }

        if (Internal.try_lock_for(std::chrono::seconds(10)))
        {
            Currentowner = std::this_thread::get_id();
        }
        else
        {
            Errorprint(va("Debugmutex: Timeout, locked by %u!", *(uint32_t *)&Currentowner).c_str());
            volatile size_t Intentional_nullderef = 0; *(size_t *)Intentional_nullderef = 0xF00D;
        }
    }
    void unlock()
    {
        (void)Internal.try_lock();
        Currentowner = {};
        Internal.unlock();
    }
};
