#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <optional>
#include <chrono>

namespace job::threads {

class TaskQueue {
    friend class ThreadPool;
public:
    TaskQueue() = default;
    ~TaskQueue() = default;

    void post(std::function<void()> task, int priority = 0);

    std::function<void()> take();
    std::optional<std::function<void()>> take(std::chrono::milliseconds timeout);

    void stop();

    [[nodiscard]] size_t size() const;
    void setMaxSize(size_t maxSize);
    [[nodiscard]] bool isEmpty() const;


    [[nodiscard]] bool stopped() const noexcept;

    void clear();

    void emplace(std::function<void()> task, int priority);

    std::optional<std::function<void()>> front();
    std::optional<std::function<void()>> back();

    [[nodiscard]] bool pop();
    void push(std::function<void()> task, int priority);

    template <typename InputIt>
    void push_range(InputIt first, InputIt last);

    void swap(TaskQueue &other);


private:
    using TaskItem = std::pair<int, std::function<void()>>;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

    struct TaskCompare {
        bool operator()(const TaskItem &a, const TaskItem &b) const {
            return a.first > b.first; // lower priority number = higher priority
        }
    };

    std::priority_queue<TaskItem,
                        std::vector<TaskItem>,
                        TaskCompare> m_queue;

    std::atomic<bool>       m_stopped{false};
    std::atomic<size_t>     m_count{0};
    size_t                  m_maxSize{0}; // 0 = unbounded

};

} // job::threads
