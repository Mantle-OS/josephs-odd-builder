#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

namespace job::core {

class JobTimer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

public:
    constexpr JobTimer() noexcept = default;

    JobTimer(uint64_t id, TimePoint next, Duration interval,
             bool repeat = false, bool active = false,
             std::function<void()> callback = nullptr) noexcept;

    [[nodiscard]] uint64_t id() const noexcept;
    void set_id(uint64_t id) noexcept;

    [[nodiscard]] bool repeat() const noexcept;
    void set_repeat(bool repeat) noexcept;

    [[nodiscard]] bool isActive() const noexcept;
    void set_isActive(bool isActive) noexcept;

    [[nodiscard]] TimePoint next() const noexcept;
    void set_next(const TimePoint &next) noexcept;

    [[nodiscard]] Duration interval() const noexcept;
    void set_interval(const Duration &interval) noexcept;

    [[nodiscard]] bool expired(const TimePoint &right_now = Clock::now()) const noexcept;

    [[nodiscard]] std::function<void()> callback() const;
    void set_callback(const std::function<void ()> &newCallback);

    void fire() noexcept;
    void cancel() noexcept;


private:
    void scheduleNext() noexcept;

    uint64_t                m_id{0};
    TimePoint               m_nextFire{};
    Duration                m_interval{0};
    bool                    m_repeat{false};
    bool                    m_active{true};
    std::function<void()>   m_callback{};
};

} // namespace job::core
// CHECKPOINT: v1.0
