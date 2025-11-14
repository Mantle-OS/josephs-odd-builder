#pragma once

#include <string>

#include <functional>
#include <future>
#include <mutex>

#include "job_thread_pool.h"
#include "job_thread_node.h"

namespace job::threads {
class JobThreadGraph {
public:
    explicit JobThreadGraph(ThreadPool::Ptr pool);
    [[nodiscard]] bool addNode(const std::string &name, std::function<void()> task);
    [[nodiscard]] bool addEdge(const std::string &from, const std::string &to);
    [[nodiscard]] std::future<bool> run();
    void reset();
    void clear();

private:
    void onTaskFinished(const std::string &node_name, std::shared_ptr<std::promise<bool>> promise,
                        std::shared_ptr<std::atomic<int>> cnt, int tasks);

    ThreadPool::Ptr m_pool;
    std::unordered_map<std::string, JobThreadNode> m_nodes;
    std::mutex m_mutex;
};

} // namespace job::threads

//CHECKPOINT: v1.0
