#include "job_thread_graph.h"
#include <job_logger.h>
#include <string_view>

namespace job::threads {

JobThreadGraph::JobThreadGraph(ThreadPool::Ptr pool) :
    m_pool(std::move(pool))
{
    if (!m_pool)
        JOB_LOG_ERROR("[JobThreadGraph] ThreadPool is null!");
}

bool JobThreadGraph::addNode(const std::string &name, std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nodes.contains(name)) {
        JOB_LOG_WARN("[JobThreadGraph] JobThreadNode '{}' already exists.", name);
        return false;
    }

    m_nodes.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(name, std::move(task)));
    return true;
}

bool JobThreadGraph::addEdge(const std::string &from, const std::string &to)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto from_it = m_nodes.find(from);
    auto to_it = m_nodes.find(to);

    if (from_it == m_nodes.end()) {
        JOB_LOG_WARN("[JobThreadGraph] Prerequisite node '{}' not found.", from);
        return false;
    }
    if (to_it == m_nodes.end()) {
        JOB_LOG_WARN("[JobThreadGraph] Dependent node '{}' not found.", to);
        return false;
    }

    from_it->second.depsList.push_back(to);
    to_it->second.depsLeft.fetch_add(1, std::memory_order_relaxed);

    return true;
}

void JobThreadGraph::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failed.store(false);
    for (auto &[name, node] : m_nodes)
        node.depsLeft.store(0, std::memory_order_relaxed);

    for (const auto &[name, node] : m_nodes) {
        for (const auto &dep_name : node.depsList) {
            auto it = m_nodes.find(dep_name);
            if (it != m_nodes.end())
                it->second.depsLeft.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void JobThreadGraph::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodes.clear();
}

std::future<bool> JobThreadGraph::run()
{
    if (!m_pool) {
        JOB_LOG_ERROR("[JobThreadGraph] Cannot run, ThreadPool is null.");
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    auto promise = std::make_shared<std::promise<bool>>();
    auto future  = promise->get_future();
    auto cnt     = std::make_shared<std::atomic<int>>(0);
    m_failed.store(false, std::memory_order_relaxed);

    int tasks = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        tasks = static_cast<int>(m_nodes.size());
        if (tasks == 0) {
            promise->set_value(true);
            return future;
        }

        // DFS
        if (hasCycleLocked()) {
            JOB_LOG_ERROR("[JobThreadGraph] Graph has cycles; cannot run.");
            promise->set_value(false);
            return future;
        }

        for (auto &[name, node] : m_nodes)
            node.depsLeft.store(0, std::memory_order_relaxed);

        for (const auto &[name, node] : m_nodes) {
            for (const auto &dep_name : node.depsList) {
                auto it = m_nodes.find(dep_name);
                if (it != m_nodes.end())
                    it->second.depsLeft.fetch_add(1, std::memory_order_relaxed);
            }
        }
    } // unlock m_mutex

    if (tasks == 0) {
        promise->set_value(true);
        return future;
    }

    int submittedTasks = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &[name, node] : m_nodes) {
            if (node.depsLeft.load(std::memory_order_relaxed) == 0) {
                ++submittedTasks;
                m_pool->submit([this, name, promise, cnt, tasks]() {
                    bool success = true;
                    try {
                        std::function<void()> task_to_run;
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            task_to_run = m_nodes.at(name).task;
                        }

                        if (task_to_run && !m_failed.load(std::memory_order_relaxed))
                            task_to_run();

                    } catch (const std::exception &e) {
                        JOB_LOG_ERROR("[JobThreadGraph] Task '{}' failed with exception: {}", name, e.what());
                        success = false;
                    } catch (...) {
                        JOB_LOG_ERROR("[JobThreadGraph] Task '{}' failed with unknown exception.", name);
                        success = false;
                    }

                    if (!success) {
                        m_failed.store(true, std::memory_order_relaxed);
                        try { promise->set_value(false); } catch (...) {}
                    } else {
                        onTaskFinished(name, promise, cnt, tasks);
                    }

                    if (cnt->fetch_add(1, std::memory_order_relaxed) + 1 == tasks) {
                        if (!m_failed.load(std::memory_order_relaxed)) {
                            try { promise->set_value(true); } catch (...) {}
                        }
                    }
                });
            }
        }
    }

    // This "should" never happen
    if (submittedTasks == 0) {
        JOB_LOG_ERROR("[JobThreadGraph] No root tasks found after cycle check.");
        promise->set_value(false);
    }

    return future;
}


void JobThreadGraph::onTaskFinished(const std::string &node_name,
                                    std::shared_ptr<std::promise<bool>> promise,
                                    std::shared_ptr<std::atomic<int>> cnt,
                                    int tasks)
{
    if (m_failed.load())
        return;

    std::vector<std::string> ready_nodes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_nodes.find(node_name);
        if (it == m_nodes.end())
            return;

        for (const auto &dep_name : it->second.depsList) {
            auto dep_it = m_nodes.find(dep_name);
            if (dep_it == m_nodes.end())
                continue;

            int remaining = dep_it->second.depsLeft.fetch_sub(1) - 1;
            if (remaining == 0)
                ready_nodes.push_back(dep_name);
        }
    }

    for (const auto &name : ready_nodes) {
        m_pool->submit([this, name, promise, cnt, tasks]() {
            bool success = true;
            try {
                std::function<void()> task_left;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    task_left = m_nodes.at(name).task;
                }

                if (task_left && !m_failed.load())
                    task_left();

            } catch (const std::exception &e) {
                JOB_LOG_ERROR("[JobGraph] Task '{}' failed with exception: {}", name, e.what());
                success = false;
            } catch (...) {
                JOB_LOG_ERROR("[JobGraph] Task '{}' failed with unknown exception.", name);
                success = false;
            }

            if (!success) {
                m_failed.store(true);
                try {
                    promise->set_value(false);
                } catch (...) {
                }
            } else {
                onTaskFinished(name, promise, cnt, tasks);
            }

            if (cnt->fetch_add(1) + 1 == tasks) {
                if (!m_failed.load()) {
                    try {
                        promise->set_value(true);
                    } catch (...) {

                    }
                }
            }
        });
    }
}

bool JobThreadGraph::hasCycleLocked() const
{
    const size_t n = m_nodes.size();
    if (n == 0)
        return false;

    // node name -> index
    std::unordered_map<std::string_view, size_t> nameToIndex;
    nameToIndex.reserve(n);

    std::vector<const std::string*> indexToName;
    indexToName.reserve(n);

    size_t idx = 0;
    for (const auto &kv : m_nodes) {
        const auto &name = kv.first;
        nameToIndex.emplace(std::string_view{name}, idx);
        indexToName.push_back(&name);
        ++idx;
    }

    std::vector<std::vector<size_t>> adj(n);
    idx = 0;
    for (const auto &kv : m_nodes) {
        const auto &node = kv.second;
        for (const auto &dep_name : node.depsList) {
            auto it = nameToIndex.find(std::string_view{dep_name});
            if (it != nameToIndex.end()) {
                adj[idx].push_back(it->second);
            }
        }
        ++idx;
    }

    // 0 = unvisited, 1 = visiting, 2 = done
    std::vector<uint8_t> state(n, 0);
    auto dfs = [&](auto &&self, size_t u) -> bool {
        // visiting
        state[u] = 1;
        for (auto v : adj[u]) {
            // back edge too cycle
            if (state[v] == 1)
                return true;

            if (state[v] == 0 && self(self, v))
                return true;

        }
        state[u] = 2;
        return false;
    };

    for (size_t i = 0; i < n; ++i) {
        if (state[i] == 0 && dfs(dfs, i))
            return true;
    }
    return false;
}

} // namespace job::threads
// CHECKPOINT: v1.2
