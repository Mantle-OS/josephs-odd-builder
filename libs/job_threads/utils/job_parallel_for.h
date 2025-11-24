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

    const std::size_t n = static_cast<std::size_t>(last - first);
    const std::size_t hw = std::max<std::size_t>(1, pool.workerCount());
    const std::size_t g  = (grain == 0) ?
                              std::max<std::size_t>(1, n / (hw * 4)) :
                              grain;

    std::vector<std::future<void>> futs;
    futs.reserve(n / g + 1);

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
// CHECKPOINT: v1.0
