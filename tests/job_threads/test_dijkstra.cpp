#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <set>
#include <limits>
#include <random>
#include <cmath>

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>
#include <utils/job_parallel_dijkstra.h>

using namespace job::threads;
using namespace std::chrono_literals;

namespace {

// Simple reference Dijkstra (single-threaded, exact)
DijkstraResult<float> sequentialDijkstra(const WeightedAdjList<float> &adj,
                                         std::size_t                    start)
{
    const std::size_t n = adj.size();
    DijkstraResult<float> res;
    res.distance.assign(n, WeightTraits<float>::inf());
    res.parent.assign(n, std::numeric_limits<std::size_t>::max());

    if (n == 0 || start >= n)
        return res;

    using Node = std::pair<float, std::size_t>; // (dist, v)
    std::priority_queue<
        Node,
        std::vector<Node>,
        std::greater<Node>
        > pq;

    res.distance[start] = 0.0f;
    pq.emplace(0.0f, start);

    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();

        if (d > res.distance[u])
            continue;

        for (const auto &edge : adj[u]) {
            const std::size_t v = edge.first;
            const float       w = edge.second;

            // Same precondition as library: non-negative weights only.
            REQUIRE(w >= 0.0f);

            const float cand = d + w;
            if (cand < res.distance[v]) {
                res.distance[v] = cand;
                res.parent[v]   = u;
                pq.emplace(cand, v);
            }
        }
    }

    return res;
}

// Small helper to build a reproducible random sparse graph.
WeightedAdjList<float> makeRandomGraph(std::size_t     n,
                                       std::size_t     maxOutDegree,
                                       std::uint32_t   seed)
{
    WeightedAdjList<float> adj(n);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::size_t> vDist(0, n - 1);
    std::uniform_real_distribution<float> wDist(0.0f, 10.0f);

    for (std::size_t u = 0; u < n; ++u) {
        const std::size_t outDeg = std::min<std::size_t>(maxOutDegree, n - 1);
        for (std::size_t k = 0; k < outDeg; ++k) {
            std::size_t v = vDist(rng);
            if (v == u)
                continue;
            adj[u].push_back({v, wDist(rng)});
        }
    }
    return adj;
}

} // namespace

TEST_CASE("parallelDijkstra computes correct shortest paths on a small graph",
          "[threading][graph][dijkstra][basic]")
{
    // Example graph:
    //  0 -> 1 (4) | 0 -> 2 (1) | 2 -> 1 (2) | 1 -> 3 (1) | 2 -> 3 (5) | 3 -> 4 (3)
    //
    // Shortest distances from 0:
    //  dist[0] = 0
    //  dist[1] = 3   (0 -> 2 -> 1)
    //  dist[2] = 1   (0 -> 2)
    //  dist[3] = 4   (0 -> 2 -> 1 -> 3)
    //  dist[4] = 7   (.. -> 3 -> 4)

    WeightedAdjList<float> adj(5);
    adj[0] = { {1, 4.0f}, {2, 1.0f} };
    adj[1] = { {3, 1.0f} };
    adj[2] = { {1, 2.0f}, {3, 5.0f} };
    adj[3] = { {4, 3.0f} };
    adj[4] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    std::vector<std::size_t> visitOrder;
    visitOrder.reserve(adj.size());

    auto res = parallelDijkstra<float>(*pool, adj, 0,
                                       [&](std::size_t v, float /*dist*/) {
                                           visitOrder.push_back(v);
                                       });

    pool->shutdown();

    REQUIRE(res.distance.size() == 5);
    REQUIRE(res.parent.size()   == 5);

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[2] == Catch::Approx(1.0f));
    REQUIRE(res.distance[1] == Catch::Approx(3.0f));
    REQUIRE(res.distance[3] == Catch::Approx(4.0f));
    REQUIRE(res.distance[4] == Catch::Approx(7.0f));

    // Parents now come from atomic {dist,parent} updates; the tree should be stable.
    REQUIRE(res.parent[0] == SIZE_MAX);
    REQUIRE(res.parent[2] == 0);
    REQUIRE(res.parent[1] == 2);
    REQUIRE(res.parent[3] == 1);
    REQUIRE(res.parent[4] == 3);

    // Path reconstruction sanity: 0 -> 2 -> 1 -> 3 -> 4
    auto path = reconstructPath(res, 4);
    REQUIRE(path == std::vector<std::size_t>{0, 2, 1, 3, 4});
}

TEST_CASE("parallelDijkstra respects unreachable nodes",
          "[threading][graph][dijkstra][unreachable]")
{
    // 0 -> 1 (1) | 1 -> (none) | 2,3 form a separate component
    WeightedAdjList<float> adj(4);
    adj[0] = { {1, 1.0f} };
    adj[1] = { };
    adj[2] = { {3, 2.0f} };
    adj[3] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res = parallelDijkstra<float>(*pool, adj, 0,
                                       [](std::size_t, float) {});

    pool->shutdown();

    const float inf = std::numeric_limits<float>::infinity();

    REQUIRE(res.distance.size() == 4);
    REQUIRE(res.parent.size()   == 4);

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(1.0f));
    REQUIRE(res.distance[2] == inf);
    REQUIRE(res.distance[3] == inf);

    REQUIRE(res.parent[0] == SIZE_MAX);
    REQUIRE(res.parent[1] == 0);
    REQUIRE(res.parent[2] == SIZE_MAX);
    REQUIRE(res.parent[3] == SIZE_MAX);

    // reconstructPath should give empty for unreachable
    auto path2 = reconstructPath(res, 2);
    auto path3 = reconstructPath(res, 3);
    REQUIRE(path2.empty());
    REQUIRE(path3.empty());
}

TEST_CASE("parallelDijkstra single-argument visitor sees all reachable vertices",
          "[threading][graph][dijkstra][visitor]")
{
    // Simple line: 0 -> 1 -> 2 -> 3
    WeightedAdjList<float> adj(4);
    adj[0] = { {1, 1.0f} };
    adj[1] = { {2, 1.0f} };
    adj[2] = { {3, 1.0f} };
    adj[3] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    std::set<std::size_t> visited;

    auto res = parallelDijkstra<float>(*pool, adj, 0,
                                       [&](std::size_t v) {
                                           visited.insert(v);
                                       });

    pool->shutdown();

    // Implementation may call the visitor multiple times; we only care
    // that every reachable vertex is seen at least once.
    REQUIRE(visited == std::set<std::size_t>{0, 1, 2, 3});

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(1.0f));
    REQUIRE(res.distance[2] == Catch::Approx(2.0f));
    REQUIRE(res.distance[3] == Catch::Approx(3.0f));
}

TEST_CASE("parallelDijkstra handles zero-weight edges",
          "[threading][graph][dijkstra][zero]")
{
    // 0 -> 1 (0), 0 -> 2 (5), 1 -> 2 (0)
    // Shortest path to 2 should be 0 -> 1 -> 2 (cost 0)

    WeightedAdjList<float> adj(3);
    adj[0] = { {1, 0.0f}, {2, 5.0f} };
    adj[1] = { {2, 0.0f} };
    adj[2] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res = parallelDijkstra<float>(*pool, adj, 0,
                                       [](std::size_t, float) {});

    pool->shutdown();

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(0.0f));
    REQUIRE(res.distance[2] == Catch::Approx(0.0f));  // Not a mustang 5.0!

    // Parent chain should be 0 -> 1 -> 2
    REQUIRE(res.parent[1] == 0);
    REQUIRE(res.parent[2] == 1);
}

TEST_CASE("reconstructPath returns empty for unreachable vertex",
          "[threading][graph][dijkstra][path][unreachable]")
{
    WeightedAdjList<float> adj(3);
    adj[0] = { {1, 1.0f} };
    adj[1] = { };
    adj[2] = { }; // isolated

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res  = parallelDijkstra<float>(*pool, adj, 0,
                                       [](std::size_t, float) {});
    auto path = reconstructPath(res, 2);

    pool->shutdown();

    REQUIRE(path.empty());
}

TEST_CASE("parallelDijkstra handles edge cases safely",
          "[threading][graph][dijkstra][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    SECTION("empty graph")
    {
        WeightedAdjList<float> adj;
        int                    calls = 0;

        auto res = parallelDijkstra<float>(*pool, adj, 0,
                                           [&](std::size_t, float) {
                                               ++calls;
                                           });

        REQUIRE(res.distance.empty());
        REQUIRE(res.parent.empty());
        REQUIRE(calls == 0);
    }

    SECTION("start vertex out of range")
    {
        WeightedAdjList<float> adj(2);
        adj[0] = { {1, 1.0f} };
        adj[1] = { };

        int calls = 0;

        auto res = parallelDijkstra<float>(*pool, adj, 99,
                                           [&](std::size_t, float) {
                                               ++calls;
                                           });

        REQUIRE(res.distance.size() == 2);
        REQUIRE(res.parent.size()   == 2);

        const float inf = std::numeric_limits<float>::infinity();
        REQUIRE(res.distance[0] == inf);
        REQUIRE(res.distance[1] == inf);
        REQUIRE(res.parent[0]   == SIZE_MAX);
        REQUIRE(res.parent[1]   == SIZE_MAX);
        REQUIRE(calls           == 0);
    }

    pool->shutdown();
}

TEST_CASE("parallelDijkstra scales on a larger tree-shaped graph",
          "[threading][graph][dijkstra][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 8);

    // Full binary tree with unit weights.
    constexpr std::size_t kDepth = 10;
    constexpr std::size_t kNodes = (1u << (kDepth + 1)) - 1u; // 2^(d+1) - 1

    WeightedAdjList<float> adj(kNodes);
    for (std::size_t i = 0; i < kNodes; ++i) {
        const std::size_t left  = 2 * i + 1;
        const std::size_t right = 2 * i + 2;

        if (left < kNodes)
            adj[i].push_back({left, 1.0f});

        if (right < kNodes)
            adj[i].push_back({right, 1.0f});
    }

    auto res = parallelDijkstra<float>(*pool, adj, 0,
                                       [](std::size_t, float) {});

    pool->shutdown();

    const float inf = std::numeric_limits<float>::infinity();

    for (std::size_t v = 0; v < kNodes; ++v) {
        INFO("vertex " << v << " distance " << res.distance[v]);
        REQUIRE(res.distance[v] != inf);

        // Exact depth check: for heap-style indexing, depth = floor(log2(v+1))
        const float depth = std::floor(std::log2(static_cast<float>(v + 1)));
        REQUIRE(res.distance[v] == Catch::Approx(depth));
    }
}

TEST_CASE("parallelDijkstra matches sequential Dijkstra on random graph",
          "[threading][graph][dijkstra][seq-compare]")
{
    constexpr std::size_t kNodes       = 200;
    constexpr std::size_t kMaxOut      = 8;
    constexpr std::uint32_t kSeedGraph = 0x2EC1920C;

    auto adj = makeRandomGraph(kNodes, kMaxOut, kSeedGraph);

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    const std::size_t start = 0;

    auto ref = sequentialDijkstra(adj, start);
    auto res = parallelDijkstra<float>(*pool, adj, start,
                                       [](std::size_t, float) {});

    pool->shutdown();

    REQUIRE(res.distance.size() == ref.distance.size());
    REQUIRE(res.parent.size()   == ref.parent.size());

    const float inf = std::numeric_limits<float>::infinity();

    for (std::size_t i = 0; i < kNodes; ++i) {
        if (ref.distance[i] == WeightTraits<float>::inf()) {
            REQUIRE(res.distance[i] == inf);
            REQUIRE(res.parent[i]   == SIZE_MAX);
        } else {
            REQUIRE(res.distance[i] == Catch::Approx(ref.distance[i]));
        }
    }
}

TEST_CASE("parallelDijkstra distances are deterministic across worker counts",
          "[threading][graph][dijkstra][determinism]")
{
    constexpr std::size_t   kNodes       = 150;
    constexpr std::size_t   kMaxOut      = 6;
    constexpr std::uint32_t kSeedGraph   = 0x2EC1920C;
    const std::size_t       startVertex  = 0;

    auto adj = makeRandomGraph(kNodes, kMaxOut, kSeedGraph);

    auto sched1 = std::make_shared<FifoScheduler>();
    auto pool1  = ThreadPool::create(sched1, 1);

    auto sched4 = std::make_shared<FifoScheduler>();
    auto pool4  = ThreadPool::create(sched4, 4);

    auto sched8 = std::make_shared<FifoScheduler>();
    auto pool8  = ThreadPool::create(sched8, 8);

    auto res1 = parallelDijkstra<float>(*pool1, adj, startVertex,
                                        [](std::size_t, float) {});
    auto res4 = parallelDijkstra<float>(*pool4, adj, startVertex,
                                        [](std::size_t, float) {});
    auto res8 = parallelDijkstra<float>(*pool8, adj, startVertex,
                                        [](std::size_t, float) {});

    pool1->shutdown();
    pool4->shutdown();
    pool8->shutdown();

    REQUIRE(res1.distance.size() == res4.distance.size());
    REQUIRE(res1.distance.size() == res8.distance.size());

    const std::size_t n = res1.distance.size();
    for (std::size_t i = 0; i < n; ++i) {
        REQUIRE(res4.distance[i] == Catch::Approx(res1.distance[i]));
        REQUIRE(res8.distance[i] == Catch::Approx(res1.distance[i]));
    }
}

TEST_CASE("parallelDijkstra easy API is consistent with explicit delta", "[threading][graph][dijkstra][delta]")
{
    constexpr std::size_t   kNodes       = 64;
    constexpr std::size_t   kMaxOut      = 4;
    constexpr std::uint32_t kSeedGraph   = 0xDE1A7A;
    const std::size_t       startVertex  = 0;

    auto adj = makeRandomGraph(kNodes, kMaxOut, kSeedGraph);

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    // Easy API: delta chosen internally.
    auto resEasy = parallelDijkstra<float>(*pool, adj, startVertex);

    // Explicit delta: use the library's heuristic directly.
    auto delta = detail::suggestDelta(adj);

    auto resExplicit = parallelDijkstra<float>(*pool,
                                               adj,
                                               startVertex,
                                               [](std::size_t, float) {},
                                               delta);

    pool->shutdown();

    REQUIRE(resEasy.distance.size() == resExplicit.distance.size());

    const std::size_t n = resEasy.distance.size();
    for (std::size_t i = 0; i < n; ++i)
        REQUIRE(resEasy.distance[i] == Catch::Approx(resExplicit.distance[i]));
}
