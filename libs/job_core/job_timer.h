#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <unistd.h>

namespace job::core {
    struct JobTimer {
        uint64_t id{0};
        std::chrono::steady_clock::time_point nextFire;
        std::chrono::milliseconds interval{0};
        bool repeat{false};
        std::function<void()> callback;
    };
} // job::core
