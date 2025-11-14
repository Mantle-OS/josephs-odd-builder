#include "job_thread_graph.h"
#include <job_logger.h>

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

    // reset all depsLeft cnts
    for (auto& [name, node] : m_nodes)
        node.depsLeft.store(0, std::memory_order_relaxed);

    for (const auto& [name, node] : m_nodes) {
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
        JOB_LOG_ERROR("[JobGraph] Cannot run, ThreadPool is null.");
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }

    auto promise = std::make_shared<std::promise<bool>>();
    std::future<bool> future = promise->get_future();
    auto cnt = std::make_shared<std::atomic<int>>(0);

    const int tasks = m_nodes.size();

    if (tasks == 0) {
        promise->set_value(true);
        return future;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    int submittedTasks = 0;
    for (auto& [name, node] : m_nodes) {
        if (node.depsLeft.load(std::memory_order_relaxed) == 0) {
            submittedTasks++;
            m_pool->submit([this, name, promise, cnt, tasks]() {
                try {
                    std::function<void()> task_to_run;
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        task_to_run = m_nodes.at(name).task;
                    }

                    if(task_to_run)
                        task_to_run(); // Run the task *outside* the lock

                    onTaskFinished(name, promise, cnt, tasks);

                } catch (const std::exception &e) {
                    JOB_LOG_ERROR("[JobGraph] Task '{}' failed with exception: {}", name, e.what());
                    promise->set_value(false);
                } catch (...) {
                    JOB_LOG_ERROR("[JobGraph] Task '{}' failed with unknown exception.", name);
                    promise->set_value(false);
                }

                if (cnt->fetch_add(1) + 1 == tasks)
                    promise->set_value(true);
            });
        }
    }

    if (submittedTasks == 0) {
        JOB_LOG_ERROR("[JobGraph] No root tasks found. Check for circular dependencies.");
        promise->set_value(false);
    }

    return future;
}

void JobThreadGraph::onTaskFinished(const std::string &node_name, std::shared_ptr<std::promise<bool>> promise,
                                    std::shared_ptr<std::atomic<int>> cnt, int tasks)
{
    std::vector<std::string> deps_ck;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_nodes.find(node_name);
        if (it == m_nodes.end()) return;

        // Find all deps check if they are ready
        for (const auto &dep_name : it->second.depsList) {
            auto dep_it = m_nodes.find(dep_name);
            if (dep_it == m_nodes.end())
                continue;

            int remaining = dep_it->second.depsLeft.fetch_sub(1) - 1;
            if (remaining == 0)
                deps_ck.push_back(dep_name);
        }
    }

    for (const auto &name : deps_ck) {
        m_pool->submit([this, name, promise, cnt, tasks]() {
            try {
                std::function<void()> task_left;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    task_left = m_nodes.at(name).task;
                }

                if(task_left)
                    task_left();

                onTaskFinished(name, promise, cnt, tasks);

            } catch (const std::exception& e) {
                JOB_LOG_ERROR("[JobGraph] Task '{}' failed with exception: {}", name, e.what());
                promise->set_value(false);
            } catch (...) {
                JOB_LOG_ERROR("[JobGraph] Task '{}' failed with unknown exception.", name);
                promise->set_value(false);
            }

            if (cnt->fetch_add(1) + 1 == tasks)
                promise->set_value(true);

        });
    }
}

} // namespace job::threads

// CHECKPOINT: v1.0
