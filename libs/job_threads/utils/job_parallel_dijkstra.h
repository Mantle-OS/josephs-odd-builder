#pragma once

#include <vector>
#include <queue>
#include <limits>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <concepts>

#include <job_logger.h>
#include "job_thread_pool.h"

namespace job::threads
{
//
//   Ohh Lenny ... My name is not Lenny its Edsger okay Lenny .....
//  * Weighted adjacency list:
//        using Graph = std::vector<std::vector<std::pair<std::size_t, Weight>>>;
//    adj[v] = list of (neighbor, edgeWeight).
//  * Vertices are 0..N-1.
// https://en.wikipedia.org/wiki/Lenny_Dykstra vs https://en.wikipedia.org/wiki/Edsger_W._Dijkstra
//

// <caveman>Me templete hard</caveman>
template <typename W>
concept Weight = std::is_arithmetic_v<W> || requires(W a, W b) {
    { a + b } -> std::convertible_to<W>;
    { a < b } -> std::convertible_to<bool>;
    { std::numeric_limits<W>::infinity() } -> std::convertible_to<W>;
};

template <Weight W>
struct DijkstraResult
{
    std::vector<W>              distance;  // distance[v] from start, or +inf if unreachable
    std::vector<std::size_t>    parent;    // parent[v] in shortest-path tree, or SIZE_MAX
};


template <Weight W>
using WeightedAdjList = std::vector<std::vector<std::pair<std::size_t, W>>>;

template <Weight W>
std::vector<std::size_t> reconstruct_path(const DijkstraResult<W> &result,
                                          std::size_t target)
{
    std::vector<std::size_t> path;

    const std::size_t n = result.parent.size();
    if (target >= n)
        return path;

    // If we have distances, use +inf as "unreachable" marker.
    if (result.distance.size() == n) {
        const W inf = std::numeric_limits<W>::infinity();
        if (result.distance[target] == inf)
            return path;
    }

    for (std::size_t v = target; v != SIZE_MAX; v = result.parent[v]) {
        path.push_back(v);

        // Defensive: break if parent[] has a cycle.
        if (path.size() > n) {
            JOB_LOG_ERROR("[reconstruct_path] Cycle detected in parent tree!");
            return {};
        }
    }

    std::reverse(path.begin(), path.end());
    return path;
}

// Internal core that always uses a (vertex, distance) visitor
template <Weight W, typename FullVisitor>
DijkstraResult<W> dijkstra_core(ThreadPool &/*pool*/, const WeightedAdjList<W> &adj, std::size_t startVertex, FullVisitor &&visit)
{
    DijkstraResult<W> res;

    const std::size_t n = adj.size();
    res.distance.assign(n, std::numeric_limits<W>::infinity());
    res.parent.assign(n, SIZE_MAX);

    if (n == 0 || startVertex >= n)
        return res;

    using Node = std::pair<W, std::size_t>; // (dist, vertex)
    auto cmp = [](const Node &a, const Node &b) {
        return a.first > b.first; // min-heap by distance
    };

    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

    res.distance[startVertex] = W(0);
    res.parent[startVertex]   = SIZE_MAX;
    pq.emplace(W(0), startVertex);

    while (!pq.empty()) {
        auto [dist, v] = pq.top();
        pq.pop();

        // Outdated entry ?
        if (dist > res.distance[v])
            continue;

        // Notify visitor once per settled vertex
        visit(v, dist);

        // Relax edges
        const auto &neighbors = adj[v];
        for (const auto &[u, w] : neighbors) {
            if (u >= n)
                continue; // defensive: ignore OOR

            // Lenny(stop calling me that) Dijkstra assumes non-negative weights... so log once if we see a negative
            if (w < W(0))
                JOB_LOG_WARN("[parallel_dijkstra] Negative edge weight detected results are undefined for this graph.");

            const W newDist = dist + w;
            if (newDist < res.distance[u]) {
                res.distance[u] = newDist;
                res.parent[u]   = v;
                pq.emplace(newDist, u);
            }
        }
    }

    return res;
}

////////////////////
// API
////////////////////
// visitor(vertex, distance)
template <Weight W, typename Visitor, typename = std::enable_if_t< std::is_invocable_v<Visitor, std::size_t, W>>>
DijkstraResult<W> parallel_dijkstra(ThreadPool &pool, const WeightedAdjList<W> &adj, std::size_t startVertex, Visitor &&visit)
{
    // !!!!! WARNING !!!!!:
    //  For now, the core is sequential (I don't(Think) yet parallelize the PQ).
    //  ThreadPool is passed in so the API matches other helpers and I can(could if I cared) parallelize internally later without changing signatures.
    return dijkstra_core<W>(pool, adj, startVertex, std::forward<Visitor>(visit));
}

// visitor(vertex) | I am selfish I only care about Lenny's(aghh) visitors
template <Weight W, typename Visitor,
         typename = std::enable_if_t< std::is_invocable_v<Visitor, std::size_t> && !std::is_invocable_v<Visitor, std::size_t, W>>,
         typename = void>
DijkstraResult<W> parallel_dijkstra(ThreadPool &pool, const WeightedAdjList<W> &adj, std::size_t startVertex, Visitor &&visit)
{
    auto fullVisitor = [&](std::size_t v, W /*dist*/) {
        visit(v);
    };
    return dijkstra_core<W>(pool, adj, startVertex, fullVisitor);
}

} // namespace job::threads
// CHECKPOINT: v1.0
