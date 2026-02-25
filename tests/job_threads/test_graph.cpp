#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <vector>
#include <string>
#include <future>
#include <algorithm>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <job_thread_graph.h>
#include <sched/job_fifo_scheduler.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobThreadGraph runs linear dependencies", "[threading][graph][linear]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 2);

    JobThreadGraph graph(pool);

    std::vector<std::string> order;
    std::mutex mut;

    auto make_task = [&](std::string name) {
        return [&, name] {
            std::lock_guard<std::mutex> lock(mut);
            order.push_back(name);
        };
    };

    REQUIRE(graph.addNode("A", make_task("A")));
    REQUIRE(graph.addNode("B", make_task("B")));
    REQUIRE(graph.addNode("C", make_task("C")));

    REQUIRE(graph.addEdge("A", "B"));
    REQUIRE(graph.addEdge("B", "C"));

    auto future = graph.run();
    REQUIRE(future.get() == true);

    REQUIRE(order.size() == 3);
    REQUIRE(order[0] == "A");
    REQUIRE(order[1] == "B");
    REQUIRE(order[2] == "C");

    pool->shutdown();
}

TEST_CASE("JobThreadGraph handles diamond dependency (A->B, A->C, B->D, C->D)", "[threading][graph][diamond]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);

    JobThreadGraph graph(pool);

    std::atomic<int> counter{0};

    auto task = [&] {
        std::this_thread::sleep_for(10ms);
        counter.fetch_add(1, std::memory_order_relaxed);
    };

    REQUIRE(graph.addNode("A", task));
    REQUIRE(graph.addNode("B", task));
    REQUIRE(graph.addNode("C", task));
    REQUIRE(graph.addNode("D", task));

    REQUIRE(graph.addEdge("A", "B"));
    REQUIRE(graph.addEdge("A", "C"));
    REQUIRE(graph.addEdge("B", "D"));
    REQUIRE(graph.addEdge("C", "D"));

    auto future = graph.run();
    REQUIRE(future.get() == true);
    REQUIRE(counter.load() == 4);

    pool->shutdown();
}

TEST_CASE("JobThreadGraph propagates failure", "[threading][graph][failure]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 2);

    JobThreadGraph graph(pool);
    std::atomic<bool> dependent_ran{false};

    REQUIRE(graph.addNode("Root", [] {
        throw std::runtime_error("Root failed");
    }));

    REQUIRE(graph.addNode("Dependent", [&] {
        dependent_ran.store(true);
    }));

    REQUIRE(graph.addEdge("Root", "Dependent"));

    auto future = graph.run();

    REQUIRE(future.get() == false);

    pool->waitForIdle(50ms);
    REQUIRE(dependent_ran.load() == false);

    pool->shutdown();
}

TEST_CASE("JobThreadGraph reset allows re-running", "[threading][graph][reset]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 2);
    JobThreadGraph graph(pool);

    std::atomic<int> run_count{0};

    REQUIRE(graph.addNode("A", [&]{ run_count++; }));
    REQUIRE(graph.addNode("B", [&]{ run_count++; }));
    REQUIRE(graph.addEdge("A", "B"));

    REQUIRE(graph.run().get() == true);
    REQUIRE(run_count.load() == 2);

    graph.reset();

    REQUIRE(graph.run().get() == true);
    REQUIRE(run_count.load() == 4);

    pool->shutdown();
}

TEST_CASE("JobThreadGraph run on empty graph returns true", "[threading][graph][empty]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);
    REQUIRE(g.run().get() == true);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph with null pool fails to run", "[threading][graph][nullpool]")
{
    JobThreadGraph g(nullptr);
    REQUIRE(g.run().get() == false);
}

TEST_CASE("JobThreadGraph rejects duplicate nodes", "[threading][graph][dupe]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);

    std::atomic<int> ran{0};
    REQUIRE(g.addNode("A", [&]{ ran++; }));
    REQUIRE_FALSE(g.addNode("A", [&]{ ran += 100; })); // rejected

    REQUIRE(g.run().get() == true);
    REQUIRE(ran.load() == 1);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph addEdge validates endpoints", "[threading][graph][validate]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);

    std::atomic<int> ran{0};
    REQUIRE(g.addNode("A", [&]{ ran++; }));
    REQUIRE_FALSE(g.addEdge("A", "B")); // B missing
    REQUIRE_FALSE(g.addEdge("X", "A")); // X missing

    REQUIRE(g.run().get() == true);
    REQUIRE(ran.load() == 1);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph detects simple cycle", "[threading][graph][cycle]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);

    std::atomic<int> ran{0};
    auto t = [&]{ ran++; };

    REQUIRE(g.addNode("A", t));
    REQUIRE(g.addNode("B", t));
    REQUIRE(g.addEdge("A", "B"));
    REQUIRE(g.addEdge("B", "A"));

    // Cycle means no root tasks (indegree > 0 for all), so run() returns false immediately
    REQUIRE(g.run().get() == false);
    REQUIRE(ran.load() == 0);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph handles multiple independent chains", "[threading][graph][multiroot]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 4);
    JobThreadGraph g(pool);

    std::vector<std::string> log;
    std::mutex m;
    auto push = [&](std::string s) {
        std::lock_guard<std::mutex> lg(m);
        log.push_back(std::move(s));
    };

    // Chain 1: A->B
    REQUIRE(g.addNode("A", [&]{ push("A"); }));
    REQUIRE(g.addNode("B", [&]{ push("B"); }));
    REQUIRE(g.addEdge("A", "B"));

    // Chain 2: C->D
    REQUIRE(g.addNode("C", [&]{ push("C"); }));
    REQUIRE(g.addNode("D", [&]{ push("D"); }));
    REQUIRE(g.addEdge("C", "D"));

    REQUIRE(g.run().get() == true);

    auto pos = [&](const char* s) {
        return std::find(log.begin(), log.end(), s) - log.begin();
    };
    REQUIRE(pos("A") < pos("B"));
    REQUIRE(pos("C") < pos("D"));

    pool->shutdown();
}

TEST_CASE("JobThreadGraph fan-in waits for all prerequisites", "[threading][graph][fanin]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 3);
    JobThreadGraph g(pool);

    std::atomic<int> finished{0};
    std::atomic<int> d_started{0};

    REQUIRE(g.addNode("B", [&]{ std::this_thread::sleep_for(5ms); finished++; }));
    REQUIRE(g.addNode("C", [&]{ std::this_thread::sleep_for(5ms); finished++; }));
    REQUIRE(g.addNode("D", [&]{
        d_started.store(finished.load(), std::memory_order_relaxed);
    }));

    REQUIRE(g.addEdge("B", "D"));
    REQUIRE(g.addEdge("C", "D"));

    REQUIRE(g.run().get() == true);
    REQUIRE(d_started.load() == 2);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph partial failure blocks only dependents", "[threading][graph][partialfail]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 4);
    JobThreadGraph g(pool);

    std::atomic<bool> u_executed{false};
    std::atomic<bool> v_executed{false};

    // Branch 1: X -> Y (Y throws)
    REQUIRE(g.addNode("X", []{}));
    REQUIRE(g.addNode("Y", []{ throw std::runtime_error("boom"); }));
    REQUIRE(g.addEdge("X", "Y"));

    // Branch 2: U (independent)
    REQUIRE(g.addNode("U", [&]{ u_executed.store(true); }));

    // Dependent of Y (should not run)
    REQUIRE(g.addNode("V", [&]{ v_executed.store(true); }));
    REQUIRE(g.addEdge("Y", "V"));

    REQUIRE(g.run().get() == false);

    pool->waitForIdle(50ms);

    REQUIRE(u_executed.load() == true);
    REQUIRE(v_executed.load() == false);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph clear removes all nodes", "[threading][graph][clear]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);

    std::atomic<int> ran{0};
    REQUIRE(g.addNode("A", [&]{ ran++; }));
    REQUIRE(g.run().get() == true);
    REQUIRE(ran.load() == 1);

    g.clear();
    REQUIRE(g.run().get() == true); // empty graph success
    pool->shutdown();
}

TEST_CASE("JobThreadGraph rerun without reset executes again", "[threading][graph][rerun]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 2);
    JobThreadGraph g(pool);

    std::atomic<int> count{0};
    REQUIRE(g.addNode("A", [&]{ count++; }));
    REQUIRE(g.addNode("B", [&]{ count++; }));
    REQUIRE(g.addEdge("A", "B"));

    REQUIRE(g.run().get() == true);
    REQUIRE(g.run().get() == true);
    REQUIRE(count.load() == 4);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph handles long chain", "[threading][graph][stress]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 8);
    JobThreadGraph g(pool);

    constexpr int N = 200;
    std::atomic<int> ran{0};
    for (int i = 0; i < N; ++i) {
        REQUIRE(g.addNode("n" + std::to_string(i), [&]{ ran++; }));
        if (i)
            REQUIRE(g.addEdge("n" + std::to_string(i-1), "n" + std::to_string(i)));
    }
    REQUIRE(g.run().get() == true);
    REQUIRE(ran.load() == N);
    pool->shutdown();
}

TEST_CASE("JobThreadGraph reset recomputes depsLeft", "[threading][graph][reset-indegee]")
{
    auto pool = ThreadPool::create(std::make_shared<FifoScheduler>(), 4);
    JobThreadGraph g(pool);

    std::atomic<int> d_seen{0};

    REQUIRE(g.addNode("A", []{}));
    REQUIRE(g.addNode("B", []{}));
    REQUIRE(g.addNode("C", []{}));
    REQUIRE(g.addNode("D", [&]{ d_seen++; }));

    REQUIRE(g.addEdge("A", "B"));
    REQUIRE(g.addEdge("A", "C"));
    REQUIRE(g.addEdge("B", "D"));
    REQUIRE(g.addEdge("C", "D"));

    REQUIRE(g.run().get() == true);
    REQUIRE(d_seen.load() == 1);

    g.reset();
    REQUIRE(g.run().get() == true);
    REQUIRE(d_seen.load() == 2);
    pool->shutdown();
}
