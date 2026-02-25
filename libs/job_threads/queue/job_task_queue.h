#pragma once
// This class is deperciated and should not be used now that there are better options.
// This is left around just in case someone wants to use this or we need a one off Queue
#include <optional>
#include <chrono>
#include <vector>
#include <functional>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "job_thread_metrics.h"

namespace job::threads {

class TaskQueue {
    friend class ThreadPool;

public:
    using Ptr = std::shared_ptr<TaskQueue>;
    using WPtr = std::weak_ptr<TaskQueue>;

    TaskQueue() = default;
    ~TaskQueue() = default;

    void post(std::function<void()> task, int priority = 0);
    void emplace(std::function<void()> task, int priority);
    void push(std::function<void()> task, int priority);

    std::function<void()> take();
    std::optional<std::function<void()>> take(std::chrono::milliseconds timeout);

    void stop();
    [[nodiscard]] bool stopped() const noexcept;
    void clear();

    [[nodiscard]] size_t size() const;
    void setMaxSize(size_t maxSize);
    [[nodiscard]] bool isEmpty() const;


    std::optional<std::function<void()>> front();
    [[nodiscard]] bool pop();

    [[nodiscard]] JobThreadMetrics snapshotMetrics() const;

    template<typename InputIt>
    void push_range(InputIt first, InputIt last)
    {
        size_t count_added = 0;
        std::unique_lock lock(m_mutex);

        for (; first != last; first++) {
            if (m_stopped.load())
                break;

            const auto &[priority, func] = *first;
            if (priority >= 0) {
                if (static_cast<size_t>(priority) >= m_byPriority.size())
                    resizePriorityVector(priority + 1);
                m_byPriority[priority]++;
            }

            m_queue.emplace(priority, func);
            m_count.fetch_add(1, std::memory_order_relaxed);
            count_added++;
        }

        lock.unlock();

        if (count_added > 0)
            m_condition.notify_all();
    }

    void swap(TaskQueue &other);

private:
    using TaskItem = std::pair<int, std::function<void()>>;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

    void resizePriorityVector(size_t newSize);

    struct TaskCompare {
        // lower priority number = higher priority
        bool operator()(const TaskItem &a, const TaskItem &b) const {
            return a.first > b.first;
        }
    };

    std::priority_queue<TaskItem, std::vector<TaskItem>, TaskCompare> m_queue;
    std::atomic<bool>       m_stopped{false};
    std::atomic<size_t>     m_count{0};
    size_t                  m_maxSize{0}; // 0 = unbounded

    mutable std::vector<size_t> m_byPriority{std::vector<size_t>(JobThreadMetrics::kDefaultPriorityLevels, 0)};

};

} // job::threads


