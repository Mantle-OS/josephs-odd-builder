#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <set>
#include <mutex>

#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>

#include <utils/job_parallel_bfs.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("parallel_bfs visits all reachable nodes level by level", "[threading][bfs][graph][basic]")
{
    // Graph:
    //  0 -> 1,2 | 1 -> 3 | 2 -> 3,4 | 3 -> (none) | 4 -> 5 | 5 -> (none)
    //
    // Levels from 0:
    //  depth 0: {0}
    //  depth 1: {1,2}
    //  depth 2: {3,4}
    //  depth 3: {5}

    std::vector<std::vector<std::size_t>> adj = {
        {1, 2},    // 0
        {3},       // 1
        {3, 4},    // 2
        {},        // 3
        {5},       // 4
        {}         // 5
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    // Collect visits as (depth, vertex).
    std::vector<std::pair<std::size_t, std::size_t>> visits;
    std::mutex                                       visitsMutex;

    parallel_bfs(*pool, adj, 0, [&](std::size_t depth, std::size_t v) {
        std::lock_guard<std::mutex> lock(visitsMutex);
        visits.emplace_back(depth, v);
    });

    pool->shutdown();

    // Group by depth as sets (order inside a level is not guaranteed).
    std::set<std::size_t> d0, d1, d2, d3;
    for (auto [d, v] : visits) {
        if (d == 0)
            d0.insert(v);
        else if (d == 1)
            d1.insert(v);
        else if (d == 2)
            d2.insert(v);
        else if (d == 3)
            d3.insert(v);
        else
            FAIL("Unexpected depth in BFS result");
    }

    REQUIRE(d0 == std::set<std::size_t>{0});
    REQUIRE(d1 == std::set<std::size_t>{1, 2});
    REQUIRE(d2 == std::set<std::size_t>{3, 4});
    REQUIRE(d3 == std::set<std::size_t>{5});
}

TEST_CASE("parallel_bfs does not cross disconnected components", "[threading][bfs][graph][components]")
{
    // Two components:
    // 1)  0 -> 1 | 1 -> (none)
    // 2)  2 -> 3 | 3 -> (none)
    std::vector<std::vector<std::size_t>> adj = {
        {1},  // 0
        {},   // 1
        {3},  // 2
        {}    // 3
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    std::set<std::size_t> visited0;
    std::set<std::size_t> visited2;
    std::mutex            m0;
    std::mutex            m2;

    parallel_bfs(*pool, adj, 0, [&](std::size_t /*depth*/, std::size_t v) {
        std::lock_guard<std::mutex> lock(m0);
        visited0.insert(v);
    });

    parallel_bfs(*pool, adj, 2, [&](std::size_t /*depth*/, std::size_t v) {
        std::lock_guard<std::mutex> lock(m2);
        visited2.insert(v);
    });

    pool->shutdown();

    REQUIRE(visited0 == std::set<std::size_t>{0, 1});
    REQUIRE(visited2 == std::set<std::size_t>{2, 3});
}

TEST_CASE("parallel_bfs returns correct depth and parent arrays", "[threading][bfs][graph][result]")
{
    // Same basic graph as the first test
    std::vector<std::vector<std::size_t>> adj = {
        {1, 2},    // 0
        {3},       // 1
        {3, 4},    // 2
        {},        // 3
        {5},       // 4
        {}         // 5
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    BfsResult res = parallel_bfs(*pool, adj, 0,
                                 [](std::size_t /*depth*/, std::size_t /*v*/) {
                                     // no-op visitor
                                 });

    pool->shutdown();

    // depth: 0,1,1,2,2,3
    REQUIRE(res.depth.size() == 6);
    REQUIRE(res.depth[0] == 0);
    REQUIRE(res.depth[1] == 1);
    REQUIRE(res.depth[2] == 1);
    REQUIRE(res.depth[3] == 2);
    REQUIRE(res.depth[4] == 2);
    REQUIRE(res.depth[5] == 3);

    // parent chain sanity (using SIZE_MAX as "none")
    REQUIRE(res.parent[0] == std::numeric_limits<std::size_t>::max());
    REQUIRE(res.parent[1] == 0);
    REQUIRE(res.parent[2] == 0);
    REQUIRE((res.parent[3] == 1 || res.parent[3] == 2)); // both are valid BFS trees
    REQUIRE(res.parent[4] == 2);
    REQUIRE(res.parent[5] == 4);
}

TEST_CASE("parallel_bfs returns correct depths and parents", "[threading][bfs][graph][meta]")
{
    // 0 -> 1,2 | 1 -> 3 | 2 -> 3
    std::vector<std::vector<std::size_t>> adj = {
        {1, 2}, // 0
        {3},    // 1
        {3},    // 2
        {}      // 3
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    BfsResult res = parallel_bfs(*pool, adj, 0,
                                 [](std::size_t, std::size_t) { /* ignore */ });

    pool->shutdown();

    REQUIRE(res.depth[0] == 0);
    REQUIRE(res.depth[1] == 1);
    REQUIRE(res.depth[2] == 1);
    REQUIRE(res.depth[3] == 2);

    REQUIRE(res.parent[0] == SIZE_MAX);
    REQUIRE((res.parent[1] == 0 || res.parent[2] == 0));
    REQUIRE((res.parent[3] == 1 || res.parent[3] == 2));
}



TEST_CASE("parallel_bfs visits each node at most once", "[threading][bfs][graph][uniq]")
{
    // Graph with overlapping paths:
    //  0 -> 1,2 | 1 -> 3 | 2 -> 3 | 3 -> 4 | 4 -> (none)
    //
    // Node 3 has two parents; we must still visit it only once.

    std::vector<std::vector<std::size_t>> adj = {
        {1, 2},  // 0
        {3},     // 1
        {3},     // 2
        {4},     // 3
        {}       // 4
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    std::vector<int> visitCount(adj.size(), 0);
    std::mutex       m;

    parallel_bfs(*pool, adj, 0, [&](std::size_t /*depth*/, std::size_t v) {
        std::lock_guard<std::mutex> lock(m);
        ++visitCount[v];
    });

    pool->shutdown();

    for (std::size_t i = 0; i < visitCount.size(); ++i) {
        INFO("vertex " << i << " visited " << visitCount[i] << " times");
        REQUIRE(visitCount[i] <= 1);
    }
}

TEST_CASE("parallel_bfs handles self-loops and duplicate edges", "[threading][bfs][graph][edge]")
{
    // Graph:
    //  0 -> 0, 1, 1 (self-loop and duplicate edge to 1)
    //  1 -> (none)

    std::vector<std::vector<std::size_t>> adj = {
        {0, 1, 1},  // 0
        {}          // 1
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    std::vector<int> visitCount(adj.size(), 0);
    std::mutex m;

    parallel_bfs(*pool, adj, 0, [&](std::size_t /*depth*/, std::size_t v) {
        std::lock_guard<std::mutex> lock(m);
        ++visitCount[v];
    });

    pool->shutdown();

    // Each node should be visited exactly once
    REQUIRE(visitCount[0] == 1);
    REQUIRE(visitCount[1] == 1);
}

TEST_CASE("parallel_bfs handles edge cases safely", "[threading][bfs][graph][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    SECTION("empty graph does nothing")
    {
        std::vector<std::vector<std::size_t>> adj;
        int calls = 0;

        parallel_bfs(*pool, adj, 0, [&](std::size_t, std::size_t) {
            ++calls;
        });

        REQUIRE(calls == 0);
    }

    SECTION("start vertex out of range does nothing")
    {
        std::vector<std::vector<std::size_t>> adj = { {1}, {} };

        int calls = 0;
        parallel_bfs(*pool, adj, 99, [&](std::size_t, std::size_t) {
            ++calls;
        });

        REQUIRE(calls == 0);
    }

    pool->shutdown();
}


TEST_CASE("parallel_bfs handles large graphs efficiently", "[threading][bfs][graph][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    // 8 threads
    auto pool  = ThreadPool::create(sched, 8);

    // BinTree 10 -> ~1024 nodes
    constexpr std::size_t kDepth = 10;
    constexpr std::size_t kNodes = (std::size_t{1} << (kDepth + 1)) - 1; // 2^11 - 1 = 2047


    std::vector<std::vector<std::size_t>> adj(kNodes);
    for (std::size_t i = 0; i < kNodes; ++i) {
        std::size_t left  = 2 * i + 1;
        std::size_t right = 2 * i + 2;

        if (left < kNodes)
            adj[i].push_back(left);

        if (right < kNodes)
            adj[i].push_back(right);
    }

    std::atomic<std::size_t> visitedCount{0};

    parallel_bfs(*pool, adj, 0, [&](std::size_t /*depth*/, std::size_t /*v*/) {
        visitedCount.fetch_add(1, std::memory_order_relaxed);
    });

    pool->shutdown();
    // We have been everywhere !
    REQUIRE(visitedCount == kNodes);
}

// CHECKPOINT v1.0
