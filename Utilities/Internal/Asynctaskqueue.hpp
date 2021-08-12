/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-06-12
    License: MIT
*/

#pragma once
#include <future>
#include <thread>
#include <deque>
#include <optional>
#include "../Threads/Spinlock.hpp"

template <typename T>
struct Asynctaskqueue
{
    std::deque<std::future<T>> Taskqueue;
    std::thread Backgroundthread;
    std::atomic_flag Terminate;
    Spinlock Threadguard;

    void Enqueue(std::future<T> &&Task)
    {
        std::scoped_lock _(Threadguard);
        Taskqueue.push_back(std::move(Task));

        if (Backgroundthread.joinable() == false)
        {
            Terminate.clear();
            Backgroundthread = std::thread([&]()
            {
                while (!Terminate.test())
                {
                    if (!Taskqueue.empty())
                    {
                        std::scoped_lock _(Threadguard);
                        for (auto &Item : Taskqueue)
                        {
                            if (Item.valid())
                            {
                                Item.wait_for(std::chrono::milliseconds(100));
                                break;
                            }
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
    }
    void Clear()
    {
        Terminate.test_and_set();
        std::scoped_lock _(Threadguard);
        Taskqueue.clear();
    }

    std::optional<T> tryDequeue()
    {
        if (Taskqueue.empty())
            return std::nullopt;

        if (!Taskqueue.front().valid())
            return std::nullopt;

        const auto Object = Taskqueue.front().get();
        std::scoped_lock _(Threadguard);
        Taskqueue.pop_front();
        return Object;
    }
};
