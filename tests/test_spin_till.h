#pragma once

#include <chrono>
#include <thread>

template <class Pred, class Clock = std::chrono::steady_clock, class Rep, class Period>
bool spin_until(Pred &&pred, const std::chrono::duration<Rep,Period> &timeout)
{
    auto start = Clock::now();
    auto end = start + timeout;
    while (Clock::now() < end) {
        if (pred())
            return true;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return pred();
}

//VERSION v1.0
