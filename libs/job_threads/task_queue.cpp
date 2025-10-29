#include "task_queue.h"

namespace job::threads {

void TaskQueue::post(std::function<void()> task, int priority) {
    std::unique_lock lock(m_mutex);

    if (m_stopped.load())
        return;

    if (m_maxSize > 0) {
        m_condition.wait(lock, [&]() {
            return m_queue.size() < m_maxSize || m_stopped.load();
        });

        if (m_stopped.load())
            return;
    }

    m_queue.emplace(priority, std::move(task));
    m_count.fetch_add(1, std::memory_order_relaxed);
    m_condition.notify_one();
}

std::function<void()> TaskQueue::take() {
    std::unique_lock lock(m_mutex);

    m_condition.wait(lock, [&]() {
        return !m_queue.empty() || m_stopped.load();
    });

    if (m_queue.empty())
        return [] {};

    auto task = std::move(m_queue.top().second);
    m_queue.pop();
    m_count.fetch_sub(1, std::memory_order_relaxed);
    return task;
}

std::optional<std::function<void()>> TaskQueue::take(std::chrono::milliseconds timeout) {
    std::unique_lock lock(m_mutex);
    if (!m_condition.wait_for(lock, timeout, [&]() {return !m_queue.empty() || m_stopped.load();}))
        return std::nullopt;

    if (m_queue.empty())
        return std::nullopt;

    auto task = std::move(m_queue.top().second);
    m_queue.pop();
    m_count.fetch_sub(1, std::memory_order_relaxed);
    return task;
}

void TaskQueue::stop() {
    std::scoped_lock lock(m_mutex);
    m_stopped.store(true);
    m_condition.notify_all();
}

size_t TaskQueue::size() const {
    return m_count.load(std::memory_order_relaxed);
}

void TaskQueue::setMaxSize(size_t maxSize) {
    std::scoped_lock lock(m_mutex);
    m_maxSize = maxSize;
}

bool TaskQueue::isEmpty() const
{
    std::scoped_lock lock(m_mutex);
    bool ret = m_queue.empty();
    return ret;
}

void TaskQueue::clear()
{
    std::scoped_lock lock(m_mutex);
    while (!m_queue.empty())
        m_queue.pop();

    m_count.store(0, std::memory_order_relaxed);
}

void TaskQueue::emplace(std::function<void ()> task, int priority)
{
    std::unique_lock lock(m_mutex);

    if (m_stopped.load())
        return;

    if (m_maxSize > 0) {
        m_condition.wait(lock, [&]() {
            return m_queue.size() < m_maxSize || m_stopped.load();
        });

        if (m_stopped.load())
            return;
    }

    m_queue.emplace(priority, std::move(task));
    m_count.fetch_add(1, std::memory_order_relaxed);
    m_condition.notify_one();
}

std::optional<std::function<void ()> > TaskQueue::front()
{
    std::scoped_lock lock(m_mutex);
    std::optional<std::function<void()>> ret;

    if (!m_queue.empty())
        ret = m_queue.top().second;

    return ret;
}

bool TaskQueue::pop()
{
    bool ret = false;
    std::scoped_lock lock(m_mutex);

    if (!m_queue.empty()) {
        m_queue.pop();
        m_count.fetch_sub(1, std::memory_order_relaxed);
        ret = true;
    }

    return ret;
}

void TaskQueue::push(std::function<void ()> task, int priority)
{
    emplace(std::move(task), priority);
}

void TaskQueue::swap(TaskQueue &other)
{
    if (this == &other)
        return;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    std::swap(m_queue, other.m_queue);
    std::swap(m_maxSize, other.m_maxSize);

    // Swap atomic values manually
    const bool thisStopped  = m_stopped.load(std::memory_order_relaxed);
    const bool otherStopped = other.m_stopped.load(std::memory_order_relaxed);
    m_stopped.store(otherStopped, std::memory_order_relaxed);
    other.m_stopped.store(thisStopped, std::memory_order_relaxed);

    const size_t thisCount  = m_count.load(std::memory_order_relaxed);
    const size_t otherCount = other.m_count.load(std::memory_order_relaxed);
    m_count.store(otherCount, std::memory_order_relaxed);
    other.m_count.store(thisCount, std::memory_order_relaxed);
}

std::optional<std::function<void ()> > TaskQueue::back()
{
    std::scoped_lock lock(m_mutex);
    std::optional<std::function<void()>> ret;

    // priority_queue has no direct "back" access — this is the lowest-priority item
    if (!m_queue.empty()) {
        auto tmp = m_queue;

        while (tmp.size() > 1)
            tmp.pop();

        ret = tmp.top().second;
    }

    return ret;
}

bool TaskQueue::stopped() const noexcept
{
    bool ret = m_stopped.load();
    return ret;
}

} // job::threads
