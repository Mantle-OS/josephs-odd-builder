#pragma once

#include <cstdint>
#include <array>

namespace job::threads {

enum class SchedulingPolicy : uint8_t {
    Other      = 0,  //SCHED_OTHER,
    FIFO       = 1,  //SCHED_FIFO,
    RoundRobin = 2   //SCHED_RR
};

struct JobThreadOptions final {
    static constexpr uint8_t  kDefaultPriority      = 20;
    static constexpr uint8_t  kDefaultRtPriority    = 50;
    static constexpr uint8_t  kCoreUnbound          = 0xFF; // 255
    static constexpr uint16_t kDefaultHeartbeatMs   = 50;

    bool realtime{false};
    bool lockMemory{false};
    bool pinToCore{false};

    SchedulingPolicy policy{SchedulingPolicy::Other};

    // Valid 0–99
    uint8_t priority{0};

    // 0xFF(255) = unpinned
    uint8_t coreId{0xFF};

    // ms interval
    uint16_t heartbeat{50}; // WAIT ??????

    std::array<char, 32> name{'\0'};


    [[nodiscard]] constexpr bool valid() const noexcept
    {
        bool ret = true;
        if (realtime && policy == SchedulingPolicy::Other)
            ret = false;

        if (priority > 99)
            ret = false;

        return ret;
    }

    // Preset's
    [[nodiscard]] static constexpr JobThreadOptions normal() noexcept
    {
        JobThreadOptions opts{};
        opts.priority = kDefaultPriority;
        return opts;
    }

    [[nodiscard]] static constexpr JobThreadOptions realtimeDefault() noexcept
    {
        JobThreadOptions opts{};
        opts.realtime   = true;
        opts.lockMemory = true;
        opts.policy     = SchedulingPolicy::FIFO;
        opts.priority   = kDefaultRtPriority;
        opts.heartbeat  = 50;
        return opts;
    }
};

} // namespace job::threads
