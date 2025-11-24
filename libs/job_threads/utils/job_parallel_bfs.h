#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <limits>

#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"


//
// Very small, opinionated helper:
//
//  * Graph is represented as adjacency list:
//      using Graph = std::vector<std::vector<std::size_t>>;
//  * Vertices are 0..N-1 (index into adjacency list).
//  * BFS is level-synchronous and parallel within each frontier.
//  * Visitor is called once per vertex, with (depth, vertexIndex).
//
// This is intentionally "batteries included" for the common case.
// Maybe I sould add a fancier graph models ? ... I could template harder lol !!!
//

namespace job::threads {

struct BfsResult {
    std::vector<std::size_t> depth;   // depth[v] = distance from start
    std::vector<std::size_t> parent;  // parent[v] = predecessor of v (SIZE_MAX = none)
};

namespace detail {

template <typename Visitor>
BfsResult parallel_bfs_impl(ThreadPool &pool,
                            const std::vector<std::vector<std::size_t>> &adj,
                            std::size_t startVertex,
                            Visitor &&visit)
{
    const std::size_t n = adj.size();

    BfsResult ret;
    ret.depth.assign(n, std::numeric_limits<std::size_t>::max());
    ret.parent.assign(n, std::numeric_limits<std::size_t>::max());

    if (n == 0 || startVertex >= n)
        return ret;

    ret.depth[startVertex] = 0;

    // visited[i] == true => vertex i has been enqueued and will be / was visited.
    std::vector<std::atomic<bool>> visited(n);
    for (std::size_t i = 0; i < n; ++i)
        visited[i].store(false, std::memory_order_relaxed);

    visited[startVertex].store(true, std::memory_order_relaxed);

    std::vector<std::size_t> frontier;
    frontier.reserve(n);
    frontier.push_back(startVertex);

    std::size_t depth = 0;

    while (!frontier.empty()) {
        std::vector<std::size_t> nextFrontier;
        nextFrontier.reserve(frontier.size() * 2);
        std::mutex nextMutex;

        parallel_for(pool, std::size_t{0}, frontier.size(), [&](std::size_t idx) {
            const std::size_t v = frontier[idx];

            // User callback: per-vertex, per-depth.
            visit(depth, v);

            const auto &neighbors = adj[v];
            for (std::size_t u : neighbors) {
                if (u >= n)
                    continue; // Defensive: ignore OOR edges.

                bool expected = false;
                // First thread to flip this from false->true "owns" the discovery.
                if (visited[u].compare_exchange_strong(expected,
                                                       true,
                                                       std::memory_order_relaxed)) {
                    {
                        std::lock_guard<std::mutex> lock(nextMutex);
                        nextFrontier.push_back(u);
                    }
                    ret.parent[u] = v;
                    ret.depth[u]  = depth + 1;
                }
            }
        });

        frontier.swap(nextFrontier);
        ++depth;
    }

    return ret;
}

} // namespace detail

////////////////////////////
// API overloads
////////////////////////////

// Visitor taking (depth, vertex)
template <typename Visitor,
         typename = std::enable_if_t<std::is_invocable_v<Visitor, std::size_t, std::size_t>>>
BfsResult parallel_bfs(ThreadPool &pool,
                       const std::vector<std::vector<std::size_t>> &adj,
                       std::size_t startVertex,
                       Visitor &&visit)
{
    return detail::parallel_bfs_impl(pool,
                                     adj,
                                     startVertex,
                                     std::forward<Visitor>(visit));
}

// Visitor taking only (vertex) – depth ignored
template <typename Visitor,
         typename = std::enable_if_t<std::is_invocable_v<Visitor, std::size_t> &&
                                     !std::is_invocable_v<Visitor, std::size_t, std::size_t>>,
         typename = void>
BfsResult parallel_bfs(ThreadPool &pool,
                       const std::vector<std::vector<std::size_t>> &adj,
                       std::size_t startVertex,
                       Visitor &&visit)
{
    auto wrapper = [&](std::size_t /*depth*/, std::size_t v) {
        visit(v);
    };

    return detail::parallel_bfs_impl(pool,
                                     adj,
                                     startVertex,
                                     wrapper);
}

} // namespace job::threads
// CHECKPOINT: v1.2
