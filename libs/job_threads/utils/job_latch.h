#pragma once

#include <cstddef>
#include <cstdint>
#include <latch>

namespace job::threads {

// Single-use countdown synchronization primitive backed by std::latch.
class JobLatch final {
public:
    explicit JobLatch(std::uint32_t count) noexcept :
        m_latch(static_cast<std::ptrdiff_t>(count))
    {
    }

    JobLatch(const JobLatch &) = delete;
    JobLatch &operator=(const JobLatch &) = delete;

    void countDown() noexcept
    {
        m_latch.count_down();
    }

    // Block until the counter reaches zero.
    void wait() const noexcept
    {
        m_latch.wait();
    }

    // Non-blocking: true when the counter has reached zero.
    [[nodiscard]] bool tryWait() const noexcept
    {
        return m_latch.try_wait();
    }

    // Decrement the counter and block until it reaches zero.
    void arriveAndWait() noexcept
    {
        m_latch.arrive_and_wait();
    }

private:
    std::latch m_latch;
};

} // namespace job::threads