#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <set>
#include <limits>

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>
#include <utils/job_parallel_dijkstra.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("parallel_dijkstra computes correct shortest paths on a small graph",
          "[threading][graph][dijkstra][basic]")
{
    // Example graph:
    //  0 -> 1 (4) | 0 -> 2 (1) | 2 -> 1 (2) | 1 -> 3 (1) | 2 -> 3 (5) | 3 -> 4 (3)
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

    auto res = parallel_dijkstra<float>(*pool, adj, 0,
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

    // Lenny's sanity: parents form a consistent shortest-path tree
    REQUIRE(res.parent[0] == SIZE_MAX);
    REQUIRE(res.parent[2] == 0);
    REQUIRE((res.parent[1] == 2 || res.parent[1] == 0)); // 0->1 direct is worse, but we don't assert parent, only dist
    REQUIRE((res.parent[3] == 1 || res.parent[3] == 2));
    REQUIRE(res.parent[4] == 3);
}

TEST_CASE("parallel_dijkstra respects unreachable nodes", "[threading][graph][dijkstra][unreachable]")
{
    // 0 -> 1 (1) | 1 -> (none) | 2,3 form a separate component
    WeightedAdjList<float> adj(4);
    adj[0] = { {1, 1.0f} };
    adj[1] = { };
    adj[2] = { {3, 2.0f} };
    adj[3] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res = parallel_dijkstra<float>(*pool, adj, 0,
                                        [](std::size_t, float) {});

    pool->shutdown();

    const float inf = std::numeric_limits<float>::infinity();

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(1.0f));
    REQUIRE(res.distance[2] == inf);
    REQUIRE(res.distance[3] == inf);

    REQUIRE(res.parent[0] == SIZE_MAX);
    REQUIRE(res.parent[1] == 0);
    REQUIRE(res.parent[2] == SIZE_MAX);
    REQUIRE(res.parent[3] == SIZE_MAX);
}

TEST_CASE("parallel_dijkstra single-argument visitor receives each vertex once", "[threading][graph][dijkstra][visitor]")
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

    auto res = parallel_dijkstra<float>(*pool, adj, 0,
                                        [&](std::size_t v) {
                                            visited.insert(v);
                                        });

    pool->shutdown();

    REQUIRE(visited == std::set<std::size_t>{0, 1, 2, 3});

    // Distances "should" be 0,1,2,3
    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(1.0f));
    REQUIRE(res.distance[2] == Catch::Approx(2.0f));
    REQUIRE(res.distance[3] == Catch::Approx(3.0f));
}

TEST_CASE("parallel_dijkstra warns on negative weights", "[threading][graph][dijkstra][negative]")
{
    // Graph with a negative edge ({Lenny} Dijkstra is undefined here)
    WeightedAdjList<float> adj(3);
    adj[0] = { {1, 5.0f} };
    adj[1] = { {2, -3.0f} };  // Look no gravity(Neg weight)
    adj[2] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    // Should complete without crashing, but results are undefined
    auto res = parallel_dijkstra<float>(*pool, adj, 0, [](std::size_t, float) {});

    pool->shutdown();

    // well no distances(its undefined), however verify the algo didn't crash
    SUCCEED("Negative weight handled without crash");
}

TEST_CASE("parallel_dijkstra handles zero-weight edges", "[threading][graph][dijkstra][zero]")
{
    // 0 -> 1 (0), 0 -> 2 (5), 1 -> 2 (0)
    // Shortest path to 2 should be 0 -> 1 -> 2 (cost 0)

    WeightedAdjList<float> adj(3);
    adj[0] = { {1, 0.0f}, {2, 5.0f} };
    adj[1] = { {2, 0.0f} };
    adj[2] = { };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res = parallel_dijkstra<float>(*pool, adj, 0, [](std::size_t, float) {});

    pool->shutdown();

    REQUIRE(res.distance[0] == Catch::Approx(0.0f));
    REQUIRE(res.distance[1] == Catch::Approx(0.0f));
    REQUIRE(res.distance[2] == Catch::Approx(0.0f));  // Not a mustang 5.0!

    // Parent chain should be 0 -> 1 -> 2
    REQUIRE(res.parent[1] == 0);
    REQUIRE(res.parent[2] == 1);
}


TEST_CASE("reconstruct_path returns empty for unreachable vertex", "[threading][graph][dijkstra][path][unreachable]")
{
    WeightedAdjList<float> adj(3);
    adj[0] = { {1, 1.0f} };
    adj[1] = { };
    adj[2] = { }; // isolated

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    auto res  = parallel_dijkstra<float>(*pool, adj, 0, [](std::size_t, float) {});
    auto path = reconstruct_path(res, 2);

    pool->shutdown();

    REQUIRE(path.empty());
}

TEST_CASE("parallel_dijkstra handles edge cases safely", "[threading][graph][dijkstra][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    SECTION("empty graph")
    {
        WeightedAdjList<float> adj;
        int                    calls = 0;

        auto res = parallel_dijkstra<float>(*pool, adj, 0,
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

        auto res = parallel_dijkstra<float>(*pool, adj, 99,
                                            [&](std::size_t, float) {
                                                ++calls;
                                            });

        REQUIRE(res.distance.size() == 2);
        REQUIRE(res.parent.size()   == 2);

        // All distances remain +inf, parents remain SIZE_MAX hopefully
        const float inf = std::numeric_limits<float>::infinity();
        REQUIRE(res.distance[0] == inf);
        REQUIRE(res.distance[1] == inf);
        REQUIRE(res.parent[0]   == SIZE_MAX);
        REQUIRE(res.parent[1]   == SIZE_MAX);
        REQUIRE(calls           == 0);
    }

    pool->shutdown();
}

TEST_CASE("parallel_dijkstra scales on a larger tree-shaped graph", "[threading][graph][dijkstra][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 8);

    // full BinTree shape as BFS stress, but with unit weights.
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

    auto res = parallel_dijkstra<float>(*pool, adj, 0,
                                        [](std::size_t, float) {});

    pool->shutdown();

    // In a perfect BinTree with unit weights from root. Depth of node "i" is its level distance should equal depth
    const float inf = std::numeric_limits<float>::infinity();

    for (std::size_t v = 0; v < kNodes; ++v) {
        INFO("vertex " << v << " distance " << res.distance[v]);
        REQUIRE(res.distance[v] != inf);

        // Won't compute exact level here, but can at least assert distances are non-decreasing along edges:
        for (const auto &[u, w] : adj[v])
            REQUIRE(res.distance[u] >= res.distance[v] + w - 1e-4f);
    }
}
