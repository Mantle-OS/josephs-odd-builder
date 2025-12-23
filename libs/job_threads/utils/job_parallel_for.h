#pragma once

#include <future>
#include <vector>
#include <algorithm>
#include <cstddef>

#include "job_thread_pool.h"

namespace job::threads {

template <typename Index, typename Func>
void parallel_for(ThreadPool &pool, Index first, Index last, Func f,
                  int priority = 0, std::size_t grain = 0)
{
    if (last <= first)
        return;

    // recursive Safety
    if (ThreadPool::inWorkerThread()) {
        for (Index i = first; i < last; ++i)
            f(i);
        return;
    }
    // Dont create to many
    const std::size_t n = static_cast<std::size_t>(last - first);
    if (n < 1024) {
        for (Index i = first; i < last; ++i)
            f(i);
        return;
    }

    const std::size_t hw = std::max<std::size_t>(1, pool.workerCount());

    std::size_t computed_grain = n / (hw * 4);
    const std::size_t min_grain = 64;

    const std::size_t g = (grain == 0) ?
                              std::max(min_grain, computed_grain) :
                              grain;


    // (n + g - 1) / g is the integer ceiling division
    std::size_t task_count = (n + g - 1) / g;

    std::vector<std::future<void>> futs;
    futs.reserve(task_count);

    for (Index i = first; i < last; ) {
        Index chunk_end = static_cast<Index>(std::min<std::size_t>(i + g, last));

        futs.emplace_back(pool.submit(priority, [i, chunk_end, f]{
            for (Index j = i; j < chunk_end; ++j)
                f(j);
        }));

        i = chunk_end;
    }

    for (auto &fu : futs)
        fu.get();
}

} // namespace job::threads
