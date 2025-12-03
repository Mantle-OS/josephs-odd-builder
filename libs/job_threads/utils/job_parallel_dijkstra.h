#pragma once

#include <vector>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <concepts>
#include <atomic>
#include <mutex>
#include <memory>
#include <cassert>

#include <job_logger.h>
#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"

namespace job::threads
{

//  Me: Ohh Lenny ...
//  ALGO: My name is not Lenny its Edsger
//  Me: Okay Lenny why you gotta be so damn ..... Greedy
// - https://en.wikipedia.org/wiki/Lenny_Dykstra vs https://en.wikipedia.org/wiki/Edsger_W._Dijkstra
// - https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm

/*
 !!!!!!!!!!! NOTES  !!!!!!!!!!!
 1) This implementation assumes non-negative edge weights.
 2) Performance Note (Memory Alignment)
 * std::atomic on a 16-byte struct (e.g., double + size_t) usually requires 16-byte alignment to use hardware CMPXCHG16B.
 * The compiler handles this automatically with std::atomic, but it's good practice to ensure your structure fits:
 *
 * float (4) + uint32_t (4) = 8 bytes (Always lock-free on x64).
 * double (8) + uint64_t (8) = 16 bytes (Lock-free on most modern x64, might use locks on some ARM or older hardware).
 *
 * If you are on a platform where 16-byte atomics are implemented with a mutex (check std::atomic<Node>::is_always_lock_free),
 * this will act like a fine-grained lock per vertex. This is "likely" acceptable or exactly what you may want to ensure correctnes
*/

// <caveman>Me templete hard</caveman>
template <typename W>
struct WeightTraits {
    static constexpr W zero() noexcept { return W(0); }
    static constexpr W inf()  noexcept {
        if constexpr (std::numeric_limits<W>::has_infinity) return std::numeric_limits<W>::infinity();
        else return std::numeric_limits<W>::max();
    }
};

template <typename W>
concept Weight =
    std::totally_ordered<W> &&
    std::is_trivially_copyable_v<W> &&
    requires(W a, W b) {
        { a + b } -> std::convertible_to<W>;
        { WeightTraits<W>::inf() } -> std::same_as<W>;
        { WeightTraits<W>::zero() } -> std::same_as<W>;
    };

template <Weight W>
using WeightedAdjList = std::vector<std::vector<std::pair<std::size_t, W>>>;

template <Weight W>
struct DijkstraResult {
    std::vector<W> distance;
    std::vector<std::size_t> parent;
};

template <Weight W>
std::vector<std::size_t> reconstructPath(const DijkstraResult<W> &result,
                                         std::size_t target)
{
    std::vector<std::size_t> path;
    const std::size_t n = result.parent.size();
    if (target >= n)
        return path;

    if (result.distance.size() == n)
        if (result.distance[target] == WeightTraits<W>::inf())
            return path;

    // Where are your parents ?
    for (std::size_t v = target; v != std::numeric_limits<std::size_t>::max(); v = result.parent[v]) {
        path.push_back(v);
        if (path.size() > n) {
            JOB_LOG_ERROR("[reconstruct_path] Cycle detected in parent tree!");
            return {};
        }
    }
    std::reverse(path.begin(), path.end());
    return path;
}

namespace detail {


// Sample average edge weight, return ~avg/10
template <Weight W>
W suggestDelta(const WeightedAdjList<W> &adj) {
    if (adj.empty())
        return W(1);

    // Heuristic parameters
    constexpr size_t kTargetSamples = 1000;

    // Stride to sample evenly across the vertex range (avoid locality bias)
    // e.g., if 100k vertices, check every 100th vertex
    const size_t n = adj.size();
    const size_t step = std::max(size_t{1}, n / 100);

    double total_weight = 0.0;
    size_t count = 0;

    for (size_t i = 0; i < n; i += step) {
        for (const auto &edge : adj[i]) {
            // Cast to double for accumulation to avoid overflow/precision loss ??? YIKES
            total_weight += static_cast<double>(edge.second);
            count++;
            // Once we have enough statistical significance, stop.
            if (count >= kTargetSamples)
                break;
        }
        if (count >= kTargetSamples)
            break;
    }

    // Fallback for disconnected graph or 0 edges
    if (count == 0)
        return W(1);

    double avg = total_weight / count;

    // The "Avg / 10" heuristic
    double estimated_delta = avg / 10.0;

    // Its the Safety Dance[Clamp] Delta must be > 0. and For "Integers": if avg=5, avg/10 = 0. We must floor at 1.
    if (estimated_delta < 1.0)
        estimated_delta = 1.0;

    return static_cast<W>(estimated_delta);
}

template <typename W>
struct DijkstraNode {
    W dist;
    std::size_t parent;
};

template <typename W>
using DijkstraGraph = std::atomic<DijkstraNode<W>>;

struct alignas(64) DijkstraShard {
    std::vector<std::size_t> buffer;
    std::mutex mutex;
};
using DijkstraShards = std::vector<std::unique_ptr<DijkstraShard>>;


template <Weight W, typename Visitor>
[[nodiscard]] DijkstraResult<W> parallelDijkstraImpl(ThreadPool &pool,  const WeightedAdjList<W> &adj,  std::size_t startVertex, Visitor &&visit, W delta)
{

    // if constexpr (!std::atomic<DijkstraNode<W>>::is_always_lock_free) {
    //     JOB_LOG_WARN("[parallelDijkstra] DijkstraNode<W> is not lock-free on this platform; "
    //                  "per-vertex CAS may use a fallback lock.");
    // }
    // What are you doing ?????
    assert(delta > WeightTraits<W>::zero() && "[Parallel Dijkstra] delta must be positive !");

    DijkstraResult<W> res;
    const std::size_t n = adj.size();

    if (n == 0)
        return res;

    // I can not see you !!!!
    // Return a fully populated result indicating "Unreachable"
    if (startVertex >= n) {
        res.distance.assign(n, WeightTraits<W>::inf());
        res.parent.assign(n, std::numeric_limits<std::size_t>::max());
        return res;
    }

    // Initialization (Packed Atomic Node Array)
    auto nodes = std::make_unique<DijkstraGraph<W>[]>(n);
    const DijkstraNode<W> init_val {
        WeightTraits<W>::inf(),
        std::numeric_limits<std::size_t>::max()
    };

    parallel_for(pool, size_t{0}, n, [&](size_t i) {
        nodes[i].store(init_val, std::memory_order_relaxed);
    });
    nodes[startVertex].store({W(0), std::numeric_limits<std::size_t>::max()},
                             std::memory_order_relaxed);


    // "Shards" of truth
    const std::size_t num_shards = std::max<std::size_t>(1, pool.workerCount() * 2);
    DijkstraShards light_shards(num_shards);
    DijkstraShards heavy_shards(num_shards);

    for(size_t i = 0; i < num_shards; ++i) {
        light_shards[i] = std::make_unique<DijkstraShard>();
        heavy_shards[i] = std::make_unique<DijkstraShard>();
    }

    // ME: Look its Turner's Thesis(not really...)
    // ALGO(Lenny): Joseph stop talking about American Frontierism
    // ME: Okay Lenny ... "The Initial Frontier"
    std::vector<std::size_t> current_bucket;
    current_bucket.reserve(1024);
    current_bucket.push_back(startVertex);
    W bucket_limit = delta;

    // Loopie lue  (Buckets)
    while (!current_bucket.empty()) {
        // This persists across all inner "light waves"
        std::vector<std::size_t> next_bucket;
        next_bucket.reserve(current_bucket.size());

        bool settled = false;
        // Inception ! (Inner loop)
        while (!settled) {

            // Clear shards for this specific wave
            for(auto &s : light_shards)
                s->buffer.clear();

            for(auto &s : heavy_shards)
                s->buffer.clear();

            parallel_for(pool, size_t{0}, current_bucket.size(), [&](size_t idx) {
                // This is a bucket in a general store ....
                const std::size_t u = current_bucket[idx];

                DijkstraNode<W> u_node = nodes[u].load(std::memory_order_acquire);
                // Acquire & ensures we see the store that put us in the bucket
                W d_u = u_node.dist;

                // If strict delta stepping, we "might" skip if d_u > bucket_limit,
                // but Lenny's (damn it Joseph My name is Eager. ->  "Eager relaxation"
                visit(u, d_u);

                size_t shard_idx = idx % num_shards;
                DijkstraShard &light_shard = *light_shards[shard_idx];
                DijkstraShard &heavy_shard = *heavy_shards[shard_idx];

                std::vector<std::size_t> local_light;
                std::vector<std::size_t> local_heavy;
                local_light.reserve(32);
                local_heavy.reserve(32);

                const auto &neighbors = adj[u];

                for (const auto &edge : neighbors) {
                    std::size_t v = edge.first;
                    W weight = edge.second;

                    // What are you doing ?????
                    assert(weight >= WeightTraits<W>::zero() && "[Parallel Dijkstra] Negative weights forbidden !");

                    W new_dist = d_u + weight;
                    DijkstraNode<W> old_node = nodes[v].load(std::memory_order_relaxed);

                    // NOTE: This is a bucketed SSSP scheme inspired(keyword) by delta-stepping.
                    // "light" vs "heavy" by new_dist vs bucket_limit, not strictly
                    // by edge weight. Correctness is preserved (monotone relaxations until
                    // convergence), but the schedule differs from the textbook delta-stepping.
                    //
                    // SHIT !!!! loop could theoretically spin forever if another thread keeps winning with worse distances (unlikely but possible).
                    while (new_dist < old_node.dist) {

                        // Parent is U ...I think
                        DijkstraNode<W> new_node {new_dist, u};
                        if (nodes[v].compare_exchange_weak(old_node, new_node,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed)) {
                            // winner winner chicken dinner
                            // Only on successful CAS We accept non deterministic "wins" here.
                            if (new_dist < bucket_limit)
                                local_light.push_back(v);
                            else
                                local_heavy.push_back(v);
                            break;
                        }
                        // If CAS fails, old_node is updated with the current value of nodes[v].
                        // retry loop with updated old_dist
                    }
                }

                // You go here
                if (!local_light.empty()) {
                    std::lock_guard<std::mutex> lk(light_shard.mutex);
                    light_shard.buffer.insert(light_shard.buffer.end(), local_light.begin(), local_light.end());
                }

                // But you go here
                if (!local_heavy.empty()) {
                    std::lock_guard<std::mutex> lk(heavy_shard.mutex);
                    heavy_shard.buffer.insert(heavy_shard.buffer.end(), local_heavy.begin(), local_heavy.end());
                }
            });

            // (ALE ... No not beer) Aggregate.Light.Edges -> Next Wave in SAME bucket
            std::vector<std::size_t> next_wave;
            for(const auto &s : light_shards)
                if(!s->buffer.empty())
                    next_wave.insert(next_wave.end(), s->buffer.begin(), s->buffer.end());


            // Aggregate Heavy Edges -> Append to NEXT bucket accumulator
            // We extract heavy edges now, before shards are cleared at the top of the next inner loop.
            for(const auto &s : heavy_shards)
                if(!s->buffer.empty())
                    next_bucket.insert(next_bucket.end(), s->buffer.begin(), s->buffer.end());


            if (next_wave.empty())
                settled = true;
            else
                current_bucket = std::move(next_wave);

        } // End Inner Loop ()

        // Move to next bucket "range"
        current_bucket = std::move(next_bucket);
        bucket_limit = bucket_limit + delta;
    }

    // Finallly !!!!! "who knew it took so long to get from one distance to another. One day there will be a algo for that...."
    // copy to result
    res.distance.resize(n);
    res.parent.resize(n);
    parallel_for(pool, size_t{0}, n, [&](size_t i) {
        // Load final consistent state
        DijkstraNode<W> node = nodes[i].load(std::memory_order_relaxed);
        res.distance[i] = node.dist;
        res.parent[i]   = node.parent;
    });

    return res;
}

} // detail


///////////////////////////////////
// The "Pro" APIs
///////////////////////////////////

// Case A: Full Visitor (vertex, distance)
template <Weight W, typename Visitor>
    requires std::is_invocable_v<Visitor, std::size_t, W>
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex,
                                   Visitor &&visit,
                                   W delta)
{
    return detail::parallelDijkstraImpl(pool, adj, startVertex, std::forward<Visitor>(visit), delta);
}

// Case B: Simple Visitor (vertex only)
template <Weight W, typename Visitor>
    requires std::is_invocable_v<Visitor, std::size_t> && (!std::is_invocable_v<Visitor, std::size_t, W>)
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex,
                                   Visitor &&visit,
                                   W delta)
{
    auto wrapper = [&](std::size_t v, W /*dist*/) { visit(v); };
    return detail::parallelDijkstraImpl(pool, adj, startVertex, wrapper, delta);
}

// Case C: No Visitor
template <Weight W>
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex,
                                   W delta)
{
    return detail::parallelDijkstraImpl(pool, adj, startVertex, [](std::size_t, W){}, delta);
}


///////////////////////////////////
// The "Easy" APIs
///////////////////////////////////

// Case A: Full Visitor
template <Weight W, typename Visitor>
    requires std::is_invocable_v<Visitor, std::size_t, W>
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex,
                                   Visitor &&visit)
{
    return parallelDijkstra(pool, adj, startVertex, std::forward<Visitor>(visit), detail::suggestDelta(adj));
}

// Case B: Simple Visitor
template <Weight W, typename Visitor>
    requires std::is_invocable_v<Visitor, std::size_t> && (!std::is_invocable_v<Visitor, std::size_t, W>)
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex,
                                   Visitor &&visit)
{
    return parallelDijkstra(pool, adj, startVertex, std::forward<Visitor>(visit), detail::suggestDelta(adj));
}

// Case C: No Visitor
template <Weight W>
DijkstraResult<W> parallelDijkstra(ThreadPool &pool,
                                   const WeightedAdjList<W> &adj,
                                   std::size_t startVertex)
{
    return parallelDijkstra(pool, adj, startVertex, detail::suggestDelta(adj));
}

} // namespace job::threads
// CHECKPOINT v1.6
