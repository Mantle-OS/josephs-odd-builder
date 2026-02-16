#pragma once

#include <algorithm>
#include <cstddef>
#include <exception>

#include <job_assert.h>

#include "job_latch.h"
#include "job_thread_pool.h"

namespace job::threads {

enum class AccessPattern : uint8_t {
    Linear  = 0,  // contiguous blocks per task
    Strided = 1   // interleaved blocks per task
};
// “Callable must be safe to invoke concurrently from multiple threads.”
template <typename Func>
void parallel_for(ThreadPool &pool,
                  std::size_t first,
                  std::size_t last,
                  Func &&f,
                  int priority = 0,
                  std::size_t grain = 0,
                  AccessPattern mode = AccessPattern::Linear)
{
    JOB_ASSERT(last >= first);
    if (last <= first)
        return;

    // recursive/inline safety
    if (ThreadPool::inWorkerThread()) {
        for (std::size_t i = first; i < last; ++i)
            f(i);
        return;
    }

    const std::size_t n  = last - first;
    const std::size_t hw = std::max<std::size_t>(1, pool.workerCount());

    // grain calculation
    const std::size_t min_grain  = 64;
    const std::size_t k          = hw * 8;
    const std::size_t ceil_div   = (n / k) + (n % k != 0);
    const std::size_t auto_grain = std::max(min_grain, ceil_div);
    const std::size_t g          = (grain == 0) ? auto_grain : grain;

    // Serial Fallback for small N
    if (n <= g || hw == 1) {
        for (std::size_t i = first; i < last; ++i)
            f(i);
        return;
    }

    // setup the tasks
    const std::size_t chunk_count = (n / g) + (n % g != 0);
    const std::size_t max_tasks   = hw * 8;
    const std::size_t task_count  = std::min(chunk_count, max_tasks);

    // stack allocated latch
    JobLatch latch(static_cast<std::uint32_t>(task_count));

    // exception capture
    std::mutex exc_mtx;
    std::exception_ptr first_exc = nullptr;

    for (std::size_t t = 0; t < task_count; ++t) {
        // Fire and forget
        pool.submit(priority, [=, &f, &latch, &exc_mtx, &first_exc]() {
            auto run_chunk = [&](std::size_t c) {
                const std::size_t offset = c * g;
                const std::size_t start  = first + offset;
                const std::size_t end    = std::min<std::size_t>(offset + g, n) + first;
                for (std::size_t i = start; i < end; ++i)
                    f(i);
            };

            try {
                if (mode == AccessPattern::Strided) {
                    for (std::size_t c = t; c < chunk_count; c += task_count)
                        run_chunk(c);
                } else {
                    const std::size_t base = chunk_count / task_count;
                    const std::size_t rem  = chunk_count % task_count;
                    const std::size_t begin_chunk = t * base + std::min(t, rem);
                    const std::size_t count       = base + (t < rem ? 1 : 0);
                    const std::size_t end_chunk   = begin_chunk + count;
                    for (std::size_t c = begin_chunk; c < end_chunk; ++c)
                        run_chunk(c);
                }
            } catch (...) {
                std::lock_guard lock(exc_mtx);
                if (!first_exc)
                    first_exc = std::current_exception();
            }

            // We win
            latch.countDown();
        });
    }

    // block
    latch.wait();

    // errors
    if (first_exc)
        std::rethrow_exception(first_exc);
}

} // namespace job::threads
