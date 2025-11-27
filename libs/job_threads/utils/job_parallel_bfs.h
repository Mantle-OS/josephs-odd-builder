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

// alignas(64) prevents "False Sharing".
struct alignas(64) BfsShard {
    std::vector<std::size_t> buffer;
    std::mutex mutex;
};

template <typename Visitor>
BfsResult parallel_bfs_impl(ThreadPool &pool, const std::vector<std::vector<std::size_t>> &adj, std::size_t startVertex, Visitor &&visit)
{
    const std::size_t n = adj.size();

    BfsResult ret;

    ret.depth.assign(n, std::numeric_limits<std::size_t>::max());
    ret.parent.assign(n, std::numeric_limits<std::size_t>::max());

    if (n == 0 || startVertex >= n)
        return ret;

    ret.depth[startVertex] = 0;

    // visited[i] == true => vertex i has been enqueued.
    std::vector<std::atomic<bool>> visited(n);
    for (std::size_t i = 0; i < n; ++i)
        visited[i].store(false, std::memory_order_relaxed);

    visited[startVertex].store(true, std::memory_order_relaxed);

    std::vector<std::size_t> frontier;
    frontier.reserve(n);
    frontier.push_back(startVertex);

    std::size_t depth = 0;

    // Optimization: Scale shards with worker count to minimize lock contention.
    const std::size_t num_shards = std::max<std::size_t>(1, pool.workerCount() * 2);
    std::vector<std::unique_ptr<BfsShard>> shards(num_shards);
    for(auto &s : shards)
        s = std::make_unique<BfsShard>();

    while (!frontier.empty()) {
        // Pre-reserve to avoid allocation "spikes" during the parallel section
        for(auto& s : shards) {
            if (s->buffer.capacity() < frontier.size() / num_shards)
                s->buffer.reserve(frontier.size() / num_shards);

            s->buffer.clear();
        }

        parallel_for(pool, std::size_t{0}, frontier.size(), [&](std::size_t idx) {
            const std::size_t v = frontier[idx];

            // callback
            visit(depth, v);
            BfsShard &shard = *shards[idx % num_shards];

            // Local batch
            std::vector<std::size_t> local_batch;
            const auto &neighbors = adj[v];

            constexpr std::size_t kBatchSize = 64;
            local_batch.reserve(std::min(neighbors.size(), kBatchSize));

            for (std::size_t u : neighbors) {
                if (u >= n)
                    continue;

                bool expected = false;
                // Atomic "Claim"
                if (visited[u].compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
                    ret.parent[u] = v;
                    ret.depth[u]  = depth + 1;

                    local_batch.push_back(u);

                    // Flush only when we have a decent chunk
                    if (local_batch.size() >= kBatchSize) {
                        std::lock_guard<std::mutex> lock(shard.mutex);
                        shard.buffer.insert(shard.buffer.end(), local_batch.begin(), local_batch.end());
                        local_batch.clear();
                    }
                }
            }

            // Flush the rest down the "toy"let
            if (!local_batch.empty()) {
                std::lock_guard<std::mutex> lock(shard.mutex);
                shard.buffer.insert(shard.buffer.end(), local_batch.begin(), local_batch.end());
            }
        });

        std::size_t total_size = 0;
        for(const auto &s : shards)
            total_size += s->buffer.size();

        frontier.clear();
        frontier.reserve(total_size);
        for(const auto &s : shards)
            frontier.insert(frontier.end(), s->buffer.begin(), s->buffer.end());


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
BfsResult parallel_bfs(ThreadPool &pool, const std::vector<std::vector<std::size_t>> &adj, std::size_t startVertex, Visitor &&visit)
{
    return detail::parallel_bfs_impl(pool, adj, startVertex, std::forward<Visitor>(visit));
}

// Visitor taking only (vertex) – depth ignored
template <typename Visitor,
         typename = std::enable_if_t<std::is_invocable_v<Visitor, std::size_t> &&
                                     !std::is_invocable_v<Visitor, std::size_t, std::size_t>>,
         typename = void>
BfsResult parallel_bfs(ThreadPool &pool, const std::vector<std::vector<std::size_t>> &adj, std::size_t startVertex, Visitor &&visit)
{
    auto wrapper = [&](std::size_t /*depth*/, std::size_t v) {
        visit(v);
    };

    return detail::parallel_bfs_impl(pool, adj, startVertex, wrapper);
}

} // namespace job::threads
// CHECKPOINT: v1.3

