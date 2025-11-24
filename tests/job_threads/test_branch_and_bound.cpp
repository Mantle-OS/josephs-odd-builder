#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <limits>
#include <cmath>

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>
#include <utils/job_branch_and_bound.h>

using namespace job::threads;
using namespace std::chrono_literals;

// Tiny toy state for search:
// depth = how far down the tree we are
// cost = accumulated cost along path
// path = sequence of choices (0 = left, 1 = right) for debugging
struct BbToyState {
    int              depth{0};
    int              cost{0};
    std::vector<int> path;
};

// GOOD
TEST_CASE("parallel_branch_and_bound finds optimal leaf in tiny binary tree", "[threading][bnb][search][basic]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    constexpr int kMaxDepth = 4;

    BbToyState root;
    root.depth = 0;
    root.cost  = 0;

    auto branch = [](const BbToyState &state, std::vector<BbToyState> &out) {
        if (state.depth >= kMaxDepth)
            return;

        // Left child at the super market .....  expensive step (+10)
        BbToyState left = state;
        left.depth += 1;
        left.cost  += 10;
        left.path.push_back(0);

        // Right child: cheap step (+1)
        BbToyState right = state;
        right.depth += 1;
        right.cost  += 1;
        right.path.push_back(1);

        out.push_back(std::move(left));
        out.push_back(std::move(right));
    };

    auto bound = [](const BbToyState &state) {
        // Simple lower bound: cost so far.
        // solve a minimization problem, so any node whose cost already exceeds the best known can be pruned.
        return state.cost;
    };

    auto objective = [](const BbToyState &state) {
        return state.cost;
    };

    auto isSolution = [](const BbToyState &state) {
        return state.depth == kMaxDepth;
    };

    auto result = parallel_branch_and_bound<BbToyState, int>(
        *pool,
        root,
        branch,
        bound,
        objective,
        isSolution
        );

    pool->shutdown();

    REQUIRE(result.found);
    // Best path is always "go right" kMaxDepth times => cost = kMaxDepth
    REQUIRE(result.bestCost == kMaxDepth);
    REQUIRE(result.bestState.depth == kMaxDepth);

    // actually took the cheap path ?
    REQUIRE(result.bestState.path.size() == static_cast<std::size_t>(kMaxDepth));
    for (int step : result.bestState.path)
        REQUIRE(step == 1);

    // Should have expanded *some* nodes and pruned *some* others.
    REQUIRE(result.nodesExpanded > 0);
    REQUIRE(result.nodesPruned > 0);
}

// Slightly more “real” example: toy knapsack with bounding.
struct KnapsackState {
    int              index{0};        // next item index
    int              weight{0};       // current total weight
    int              value{0};        // current total value
    std::vector<int> chosen;          // which items we took (0/1)
};

// GOOD
TEST_CASE("parallel_branch_and_bound solves tiny knapsack instance", "[threading][bnb][search][knapsack]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    // KnapsackState: weights: {2, 3, 4, 5}, values: {3, 4, 5, 8}, capacity: 5
    // Optimal solution: take item 0 and 1 ? No (weight=5, value=7).
    // Better: just item 3: weight=5, value=8.
    const std::vector<int> weights = {2, 3, 4, 5};
    const std::vector<int> values  = {3, 4, 5, 8};
    constexpr int          kCap    = 5;

    KnapsackState root;
    root.index  = 0;
    root.weight = 0;
    root.value  = 0;

    auto branch = [&](const KnapsackState &s, std::vector<KnapsackState> &out) {
        if (s.index >= static_cast<int>(weights.size()))
            return;

        // skip item
        KnapsackState skip = s;
        skip.index += 1;
        skip.chosen.push_back(0);

        // take item, if it fits
        KnapsackState take = s;
        take.index  += 1;
        take.weight += weights[s.index];
        take.value  += values[s.index];
        take.chosen.push_back(1);

        out.push_back(std::move(skip));
        if (take.weight <= kCap)
            out.push_back(std::move(take));
    };

    auto bound = [&](const KnapsackState &s) -> int {
        // Upper bound on achievable value from this state: current value + sum of *all* remaining item values (optimistic to asy the least).
        int remaining = 0;
        for (int i = s.index; i < static_cast<int>(values.size()); ++i)
            remaining += values[i];

        // maximizing value, but the framework expects a "cost" to minimize, so we can flip sign: cost = -value.
        int optimisticBestValue = s.value + remaining;
        return -optimisticBestValue;
    };

    auto objective = [](const KnapsackState &s) -> int {
        // Maximize value -> minimize negative value.
        return -s.value;
    };

    auto isSolution = [&](const KnapsackState &s) {
        // A solution is any leaf: we ran out of items(Weight constraint handled in branching.)
        return s.index == static_cast<int>(weights.size());
    };

    auto result = parallel_branch_and_bound<KnapsackState, int>(
        *pool,
        root,
        branch,
        bound,
        objective,
        isSolution
        );

    pool->shutdown();

    REQUIRE(result.found);
    // Best (max) value is 8 -> bestCost = -8
    REQUIRE(result.bestCost == -8);

    const auto &best = result.bestState;
    REQUIRE(best.value == 8);
    REQUIRE(best.weight == 5);
    REQUIRE(best.chosen.size() == weights.size());

    // Exactly one item taken in this case the last one
    int takeCount = 0;
    for (int bit : best.chosen)
        if (bit == 1)
            ++takeCount;

    REQUIRE(takeCount == 1);
    REQUIRE(best.chosen.back() == 1);

    REQUIRE(result.nodesExpanded > 0);
    REQUIRE(result.nodesPruned > 0);
}
// GOOD
TEST_CASE("parallel_branch_and_bound handles ties in optimal cost", "[threading][bnb][ties]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    struct State {
        int depth{0};
        int cost{0};
        int id{0};  // To distinguish solutions
    };

    State root;

    auto branch = [](const State &s, std::vector<State> &out) {
        if (s.depth >= 3) return;

        // Both children have the same cost
        State left = s;
        left.depth += 1;
        left.cost  += 1;
        left.id     = s.id * 2;

        State right = s;
        right.depth += 1;
        right.cost  += 1;
        right.id     = s.id * 2 + 1;

        out.push_back(std::move(left));
        out.push_back(std::move(right));
    };

    auto bound = [](const State &s) {
        return s.cost;
    };
    auto objective = [](const State &s) {
        return s.cost;
    };
    auto isSolution = [](const State &s) {
        return s.depth == 3;
    };

    auto res = parallel_branch_and_bound<State, int>(
        *pool, root, branch, bound, objective, isSolution
        );

    pool->shutdown();

    REQUIRE(res.found);
    REQUIRE(res.bestCost == 3);  // All paths have cost 3

    // Any of the 8 leaf nodes is valid, we don't care which one we got
    SUCCEED("One of the tied optima was returned");
}

TEST_CASE("parallel_branch_and_bound respects timeout and aborts search early",
          "[threading][bnb][timeout]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    struct State { int depth{0}; };

    State root;

    constexpr int kMaxDepth = 22; // ~4 million leaves if fully expanded

    auto branch = [](const State &s, std::vector<State> &out) {
        if (s.depth >= kMaxDepth)
            return;

        State left  = s;
        State right = s;
        left.depth  += 1;
        right.depth += 1;

        out.push_back(std::move(left));
        out.push_back(std::move(right));
    };

    auto bound = [](const State &) -> int {
        return 0;
    }; // no pruning

    auto objective = [](const State &s) -> int {
        return s.depth;
    };

    auto isSolution = [](const State &s) {
        return s.depth == 1'000'000; // effectively unreachable given timeout
    };

    auto res = parallel_branch_and_bound<State, int>(
        *pool,
        root,
        branch,
        bound,
        objective,
        isSolution,
        std::numeric_limits<int>::max(),
        std::chrono::milliseconds(100)
        );

    pool->shutdown();

    REQUIRE_FALSE(res.found);
    REQUIRE(res.elapsedSeconds > 0.0);
    REQUIRE(res.elapsedSeconds < 0.5);
    REQUIRE(res.nodesExpanded > 0);
}


TEST_CASE("parallel_branch_and_bound reports non-zero elapsed time", "[threading][bnb][timing]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    SECTION("even for trivial searches")
    {
        struct State {
            int value{42};
        };

        State root;

        auto branch = [](const State &, std::vector<State> &) {
            // No children - root is a leaf
        };

        auto bound = [](const State &s) -> int {
            return s.value;
        };

        auto objective = [](const State &s) -> int {
            return s.value;
        };

        auto isSolution = [](const State &) {
            return true;  // Root is immediately a solution
        };

        auto res = parallel_branch_and_bound<State, int>(
            *pool, root, branch, bound, objective, isSolution
            );

        // Even a trivial search takes *some* time
        REQUIRE(res.elapsedSeconds > 0.0);
        REQUIRE(res.elapsedSeconds < 1.0);  // But not absurdly long

        INFO("Trivial search took " << res.elapsedSeconds << " seconds");
    }

    SECTION("for moderately complex searches")
    {
        struct State {
            int depth{0};
        };

        State root;

        constexpr int kMaxDepth = 8;  // 2^8 = 256 leaves ... I think

        auto branch = [](const State &s, std::vector<State> &out) {
            if (s.depth >= kMaxDepth) return;

            State left = s;
            left.depth += 1;

            State right = s;
            right.depth += 1;

            out.push_back(std::move(left));
            out.push_back(std::move(right));
        };

        auto bound = [](const State &s) -> int {
            return s.depth;
        };

        auto objective = [](const State &s) -> int {
            return s.depth;
        };

        auto isSolution = [](const State &s) {
            return s.depth == kMaxDepth;
        };

        auto res = parallel_branch_and_bound<State, int>(
            *pool, root, branch, bound, objective, isSolution
            );

        REQUIRE(res.found);
        REQUIRE(res.elapsedSeconds > 0.0);
        REQUIRE(res.elapsedSeconds < 5.0);  // Should finish quickly

        // Should have expanded a reasonable number of nodes
        REQUIRE(res.nodesExpanded > 0);
        REQUIRE(res.nodesExpanded < 1000);  // Not the full tree (pruning works)

        INFO("Moderate search expanded " << res.nodesExpanded  << " nodes in " << res.elapsedSeconds << " seconds");
    }

    pool->shutdown();
}



// Edge's

TEST_CASE("parallel_branch_and_bound handles nodes with many children", "[threading][bnb][branching]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    struct State {
        int depth{0};
        int cost{0};
    };

    State root;

    constexpr int kMaxDepth = 4;
    constexpr int kBranchingFactor = 5;  // THE KEY: > 2 children per node

    auto branch = [](const State &s, std::vector<State> &out) {
        if (s.depth >= kMaxDepth) return;

        // Create kBranchingFactor children
        for (int i = 0; i < kBranchingFactor; ++i) {
            State child = s;
            child.depth += 1;
            child.cost  += i + 1;  // Different costs
            out.push_back(std::move(child));
        }
    };

    auto bound = [](const State &s) -> int {
        return s.cost;
    };

    auto objective = [](const State &s) -> int {
        return s.cost;
    };

    auto isSolution = [](const State &s) {
        return s.depth == kMaxDepth;
    };

    auto res = parallel_branch_and_bound<State, int>(
        *pool, root, branch, bound, objective, isSolution
        );

    pool->shutdown();

    REQUIRE(res.found);
    REQUIRE(res.bestCost == kMaxDepth);  // Best path: always choose child 0 (cost +1)
    REQUIRE(res.nodesExpanded > 0);
    REQUIRE(res.nodesPruned > 0);

    INFO("High-branching tree: expanded " << res.nodesExpanded
                                          << " nodes, pruned " << res.nodesPruned);
}


TEST_CASE("parallel_branch_and_bound handles trivial and no-solution cases", "[threading][bnb][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    SECTION("root is immediately a solution")
    {
        struct State {
            int value{42};
        };

        State root;

        auto branch = [](const State &, std::vector<State> &) {
            // No children ever.... must be sterile
        };

        auto bound = [](const State &s) -> int {
            return s.value;
        };

        auto objective = [](const State &s) -> int {
            return s.value;
        };

        auto isSolution = [](const State &) {
            return true;
        };

        auto res = parallel_branch_and_bound<State, int>(
            *pool,
            root,
            branch,
            bound,
            objective,
            isSolution
            );

        REQUIRE(res.found);
        REQUIRE(res.bestCost == 42);
        REQUIRE(res.nodesExpanded == 1);
        REQUIRE(res.nodesPruned   == 0);
    }

    SECTION("no node ever satisfies isSolution")
    {
        struct State {
            int depth{0};
        };

        State root;

        auto branch = [](const State &s, std::vector<State> &out) {
            if (s.depth >= 5)
                return;

            State child = s;
            child.depth += 1;
            out.push_back(child);
        };

        auto bound = [](const State &s) -> int {
            return s.depth;
        };

        auto objective = [](const State &s) -> int {
            return s.depth;
        };

        auto isSolution = [](const State &) {
            return false;
        };

        auto res = parallel_branch_and_bound<State, int>(
            *pool,
            root,
            branch,
            bound,
            objective,
            isSolution
            );

        // Never found a valid solution
        REQUIRE_FALSE(res.found);
        REQUIRE(res.bestCost == std::numeric_limits<int>::max());
        REQUIRE(res.nodesExpanded > 0);
    }

    SECTION("initialBest lower than any achievable cost prunes everything")
    {
        struct State {
            int cost{10};
        };

        State root;

        auto branch = [](const State &, std::vector<State> &) {
            // No children .... must still be sterile
        };

        auto bound = [](const State &s) -> int {
            return s.cost;
        };

        auto objective = [](const State &s) -> int {
            return s.cost;
        };

        auto isSolution = [](const State &) {
            return true;
        };

        // Big mouth "I already have a cost-1 solution"
        auto res = parallel_branch_and_bound<State, int>(
            *pool,
            root,
            branch,
            bound,
            objective,
            isSolution,
            /*initialBest=*/1
            );

        // Root is strictly worse than the initial best; should be pruned and never accepted.
        REQUIRE_FALSE(res.found);
        REQUIRE(res.bestCost == 1);
        REQUIRE(res.nodesPruned == 1);
    }

    pool->shutdown();
}

// // Benchmarks
TEST_CASE("parallel_branch_and_bound handles moderately deep binary tree", "[threading][bnb][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 8);

    struct StressState {
        int depth{0};
        int cost{0};
    };

    constexpr int kMaxDepth      = 12;       // 2^12 leaves if fully expanded
    constexpr int kCheapStepCost = 1;
    constexpr int kExpensive     = 100;

    StressState root;

    auto branch = [](const StressState &s, std::vector<StressState> &out) {
        if (s.depth >= kMaxDepth)
            return;

        // left: expensive path
        StressState left = s;
        left.depth += 1;
        left.cost  += kExpensive;

        // right: cheap path
        StressState right = s;
        right.depth += 1;
        right.cost  += kCheapStepCost;

        out.push_back(std::move(left));
        out.push_back(std::move(right));
    };

    auto bound = [](const StressState &s) -> int {
        // Simple: cost so far. Once we find the all-cheap path, most expensive branches... Jim get the chainsaw -> the bound.
        return s.cost;
    };

    auto objective = [](const StressState &s) -> int {
        return s.cost;
    };

    auto isSolution = [](const StressState &s) {
        return s.depth == kMaxDepth;
    };

    auto res = parallel_branch_and_bound<StressState, int>(
        *pool,
        root,
        branch,
        bound,
        objective,
        isSolution
        );

    pool->shutdown();

    // Best path: always choose the cheap child... this felt odd tying...
    REQUIRE(res.found);
    REQUIRE(res.bestCost == kMaxDepth * kCheapStepCost);
    REQUIRE(res.nodesExpanded > 0);

    // If pruning *never* happened on this tree, nodesExpanded would be ~= 2^(kMaxDepth+1)-1. ... I think
    // so don't assert an exact number, but expect at least some pruning, I mean Jim did get the chainsaw after all.
    REQUIRE(res.nodesPruned > 0);
}


TEST_CASE("parallel_branch_and_bound handles zero timeout gracefully",  "[threading][bnb][edge][timeout]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);

    struct State { int depth{0}; };
    State root;

    auto branch = [](const State &s, std::vector<State> &out) {
        if (s.depth < 5) {
            State child = s;
            child.depth += 1;
            out.push_back(child);
        }
    };

    auto bound = [](const State &s) -> int {
        return s.depth;
    };

    auto objective = [](const State &s) -> int {
        return s.depth;
    };

    auto isSolution = [](const State &s) {
        return s.depth == 5;
    };

    // Timeout of 0ms - should abort almost immediately
    auto res = parallel_branch_and_bound<State, int>(
        *pool,
        root,
        branch,
        bound,
        objective,
        isSolution,
        std::numeric_limits<int>::max(),
        std::chrono::milliseconds(0)  // Zero timeout!
        );

    pool->shutdown();

    // Should not find solution (no time to search)
    REQUIRE_FALSE(res.found);

    // Should still report valid timing (not negative or NaN)
    REQUIRE(res.elapsedSeconds >= 0.0);
    REQUIRE(std::isfinite(res.elapsedSeconds));

    INFO("Zero-timeout search expanded " << res.nodesExpanded << " nodes in " << res.elapsedSeconds << " seconds");
}

// CHECKPOINT: v1.3
