#pragma once

#include <future>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <iterator>

#include "job_thread_pool.h"

namespace job::threads {

template <typename It, typename T, typename MapFn, typename ReduceFn>
T parallel_reduce(ThreadPool& pool, It first, It last, T init,
                  MapFn map_fn, ReduceFn reduce_fn,
                  int priority = 0, std::size_t grain = 0)
{
    const std::size_t n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return init;

    const std::size_t hw = std::max<std::size_t>(1, pool.workerCount());
    const std::size_t g  = (grain == 0) ?
                              std::max<std::size_t>(1, n / (hw * 4)) :
                              grain;

    std::vector<std::future<T>> partials;
    partials.reserve(n / g + 1);

    auto it = first;
    while (it != last) {
        auto it_end = it;
        std::advance(it_end, std::min<std::size_t>(g, std::distance(it, last)));
        partials.emplace_back(pool.submit(priority, [=]{
            // Note: Obv... T must be default constructible
            T acc{};
            for (auto cur = it; cur != it_end; ++cur)
                acc = (cur == it) ? map_fn(*cur) : reduce_fn(acc, map_fn(*cur));

            return acc;
        }));
        it = it_end;
    }

    T result = init;
    for (auto& fu : partials)
        result = reduce_fn(result, fu.get());

    return result;
}

} // namespace job::threads
// CHECKPOINT: v1.0
