#pragma once
#include <latch>

namespace job::threads {

// Single-use latch backed by std::latch (C++20+).
struct JobLatch {
    explicit JobLatch(std::uint32_t n) :
        lat(static_cast<std::ptrdiff_t>(n))
    {
    }

    JobLatch(const JobLatch&)            = delete;
    JobLatch& operator=(const JobLatch&) = delete;

    inline void countDown() noexcept
    {
        lat.count_down();
    }

    // Block until the counter reaches zero.
    inline void wait()
    {
        lat.wait();
    }

    // Non-blocking: true if already at zero.
    inline bool tryWait() const noexcept
    {
        return lat.try_wait();
    }
    inline void arriveAndWait()
    {
        lat.arrive_and_wait();
    }

private:
    std::latch lat;
};

} // namespace job::threads
