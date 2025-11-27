#pragma once

#include <vector>
#include <limits>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <optional>
#include <chrono>

#include <job_logger.h>
#include "job_thread_pool.h"

//
// The Exponential Tree Chainsaw. Now with multi-core teeth.
// All callbacks must be thread-safe. Each State is passed by value into tasks;
// do not mutate shared global state from callbacks without your own synchronization.
//

namespace job::threads {

template <typename C>
concept BbCost = std::is_arithmetic_v<C>;

template <typename State, BbCost Cost>
struct BranchAndBoundResult {
    bool        found{false};
    State       bestState{};
    Cost        bestCost{std::numeric_limits<Cost>::max()};
    std::size_t nodesExpanded{0};
    std::size_t nodesPruned{0};
    double      elapsedSeconds{0.0};
};

namespace detail {

#if 0
// Not used ATM maybe later
// Helper to atomic-min a float/double/int
template <typename T>
void atomicUpdateMin(std::atomic<T> &atom, T val)
{
    T prev = atom.load(std::memory_order_relaxed);
    while (prev > val && !atom.compare_exchange_weak(prev, val, std::memory_order_relaxed)) {
        // Spin until we win the race or realize we lost this game
    }
}
#endif
template <typename State, typename Cost>
struct BnbContext {
    std::atomic<Cost>                           bestCost;
    std::mutex                                  stateMutex;
    State                                       bestState;
    std::atomic<bool>                           found{false};
    std::atomic<bool>                           timedOut{false};
    // Metrics
    std::atomic<size_t>                         nodesExpanded{0};
    std::atomic<size_t>                         nodesPruned{0};
    // Synchronization: "Are we there yet?"
    std::atomic<int>                            pendingTasks{0};
    std::mutex                                  waitMutex;
    std::condition_variable                     waitCv;
};

} // namespace detail

template<
    typename State,
    BbCost Cost,
    typename BranchFunc,
    typename BoundFunc,
    typename ObjectiveFunc,
    typename IsSolutionFunc
    >
BranchAndBoundResult<State, Cost> parallel_branch_and_bound(
    ThreadPool &pool,
    const State &root,
    BranchFunc &&branch,
    BoundFunc &&bound,
    ObjectiveFunc &&objective,
    IsSolutionFunc &&isSolution,
    Cost initialBest = std::numeric_limits<Cost>::max(),
    std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
    using Context = detail::BnbContext<State, Cost>;

    auto startTime = std::chrono::steady_clock::now();

    std::optional<std::chrono::steady_clock::time_point> deadline;
    if (timeout)
        deadline = startTime + *timeout;

    Context ctx;
    ctx.bestCost.store(initialBest, std::memory_order_release);

    // Self-passing lambda: self(self, node)
    auto processNode = [&](auto &self, State node) -> void {
        // Timeout check
        if (deadline && std::chrono::steady_clock::now() > *deadline) {
            // Only one thread gets to complain loudly
            if (!ctx.timedOut.exchange(true, std::memory_order_relaxed)) {
                JOB_LOG_WARN("[parallel_branch_and_bound] Timeout reached, aborting search");
            }

            // Task done
            if (ctx.pendingTasks.fetch_sub(1, std::memory_order_release) == 1) {
                std::lock_guard<std::mutex> lock(ctx.waitMutex);
                ctx.waitCv.notify_one();
            }
            return;
        }

        // Bound / prune
        Cost lower = bound(node);
        if (lower >= ctx.bestCost.load(std::memory_order_acquire)) {
            ctx.nodesPruned.fetch_add(1, std::memory_order_relaxed);

            if (ctx.pendingTasks.fetch_sub(1, std::memory_order_release) == 1) {
                std::lock_guard<std::mutex> lock(ctx.waitMutex);
                ctx.waitCv.notify_one();
            }
            return;
        }

        ctx.nodesExpanded.fetch_add(1, std::memory_order_relaxed);

        // Solution?
        if (isSolution(node)) {
            Cost val = objective(node);

            if (val < ctx.bestCost.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(ctx.stateMutex);
                if (val < ctx.bestCost.load(std::memory_order_relaxed)) {
                    ctx.bestCost.store(val, std::memory_order_release);
                    ctx.bestState = node;
                    ctx.found.store(true, std::memory_order_release);
                }
            }

            if (ctx.pendingTasks.fetch_sub(1, std::memory_order_release) == 1) {
                std::lock_guard<std::mutex> lock(ctx.waitMutex);
                ctx.waitCv.notify_one();
            }
            return;
        }

        // Branch
        std::vector<State> children;
        branch(node, children);

        if (children.empty()) {
            if (ctx.pendingTasks.fetch_sub(1, std::memory_order_release) == 1) {
                std::lock_guard<std::mutex> lock(ctx.waitMutex);
                ctx.waitCv.notify_one();
            }
            return;
        }

        // Same accounting: N children => + (N-1) pending (this task "becomes" child 0)
        int netChange = static_cast<int>(children.size()) - 1;
        if (netChange != 0)
            ctx.pendingTasks.fetch_add(netChange, std::memory_order_relaxed);

        // Offload all but first child.
        // IMPORTANT: capture `self` BY VALUE so each task has its own lambda copy;
        // no shared std::function, no shared mutable state.
        for (std::size_t i = 1; i < children.size(); ++i) {
            State child = std::move(children[i]);
            pool.submit([&, child = std::move(child), self]() mutable {
                self(self, std::move(child));
            });
        }

        // First child stays on this thread
        self(self, std::move(children[0]));
    };

    // Kickoff
    ctx.pendingTasks.store(1, std::memory_order_release);

    // Root also gets its own copy of the lambda
    pool.submit([&, rootCopy = root, processNode]() mutable {
        processNode(processNode, rootCopy);
    });

    {
        std::unique_lock<std::mutex> lock(ctx.waitMutex);
        ctx.waitCv.wait(lock, [&]() {
            return ctx.pendingTasks.load(std::memory_order_acquire) == 0;
        });
    }

    auto endTime = std::chrono::steady_clock::now();

    BranchAndBoundResult<State, Cost> res;
    res.found          = ctx.found.load(std::memory_order_acquire);
    res.bestState      = ctx.bestState;
    res.bestCost       = ctx.bestCost.load(std::memory_order_acquire);
    res.nodesExpanded  = ctx.nodesExpanded.load(std::memory_order_relaxed);
    res.nodesPruned    = ctx.nodesPruned.load(std::memory_order_relaxed);
    res.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    return res;
}


} // namespace job::threads
// CHECKPOINT: v1.4
