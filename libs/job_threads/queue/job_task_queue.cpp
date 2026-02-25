#include "job_task_queue.h"
#include <algorithm>

namespace job::threads {

void TaskQueue::resizePriorityVector(size_t newSize)
{
    // IMPORANT: m_mutex MUST be locked
    if (newSize > m_byPriority.size())
        m_byPriority.resize(newSize, 0);
}

void TaskQueue::post(std::function<void()> task, int priority) {
    std::unique_lock lock(m_mutex);

    if (m_stopped.load())
        return;

    if (m_maxSize > 0) {
        m_condition.wait(lock, [&]() { return m_queue.size() < m_maxSize || m_stopped.load(); });
        if (m_stopped.load())
            return;
    }

    if (priority >= 0) {
        if (static_cast<size_t>(priority) >= m_byPriority.size())
            resizePriorityVector(priority + 1);
        m_byPriority[priority]++;
    }

    m_queue.emplace(priority, std::move(task));
    m_count.fetch_add(1, std::memory_order_relaxed);
    m_condition.notify_one();
}

void TaskQueue::emplace(std::function<void ()> task, int priority)
{
    post(std::move(task), priority);
}

void TaskQueue::push(std::function<void ()> task, int priority)
{
    post(std::move(task), priority);
}

std::function<void()> TaskQueue::take() {
    std::unique_lock lock(m_mutex);

    m_condition.wait(lock, [&]() {return !m_queue.empty() || m_stopped.load();});

    if (m_queue.empty())
        return [] {};

    int prio = m_queue.top().first;
    if (prio >= 0 && static_cast<size_t>(prio) < m_byPriority.size())
        m_byPriority[prio]--;

    auto task = std::move(m_queue.top().second);
    m_queue.pop();
    m_count.fetch_sub(1, std::memory_order_relaxed);
    if (m_maxSize > 0)
        m_condition.notify_one();

    return task;
}

std::optional<std::function<void()>> TaskQueue::take(std::chrono::milliseconds timeout) {
    std::unique_lock lock(m_mutex);
    if (!m_condition.wait_for(lock, timeout, [&]() {
            return !m_queue.empty() || m_stopped.load();
        }))
        return std::nullopt;

    if (m_queue.empty())
        return std::nullopt;

    int prio = m_queue.top().first;
    if (prio >= 0 && static_cast<size_t>(prio) < m_byPriority.size())
        m_byPriority[prio]--;

    auto task = std::move(m_queue.top().second);
    m_queue.pop();
    m_count.fetch_sub(1, std::memory_order_relaxed);

    if (m_maxSize > 0)
        m_condition.notify_one();

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
    return m_count.load(std::memory_order_relaxed) == 0;
}

void TaskQueue::clear()
{
    std::scoped_lock lock(m_mutex);
    while (!m_queue.empty())
        m_queue.pop();

    std::fill(m_byPriority.begin(), m_byPriority.end(), 0);

    m_count.store(0, std::memory_order_relaxed);
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
        int prio = m_queue.top().first;
        if (prio >= 0 && static_cast<size_t>(prio) < m_byPriority.size())
            m_byPriority[prio]--;

        m_queue.pop();
        m_count.fetch_sub(1, std::memory_order_relaxed);
        if (m_maxSize > 0)
            m_condition.notify_one();
        ret = true;
    }

    return ret;
}

JobThreadMetrics TaskQueue::snapshotMetrics() const
{
    std::scoped_lock lock(m_mutex);
    JobThreadMetrics metrics;
    metrics.byPriority = m_byPriority;
    metrics.total = m_count.load(std::memory_order_relaxed);
    return metrics;
}

void TaskQueue::swap(TaskQueue &other)
{
    if (this == &other)
        return;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    std::swap(m_queue, other.m_queue);
    std::swap(m_maxSize, other.m_maxSize);
    std::swap(m_byPriority, other.m_byPriority); // Swap metrics

    // Swap atomic values manually :(
    const bool thisStopped  = m_stopped.load(std::memory_order_relaxed);
    const bool otherStopped = other.m_stopped.load(std::memory_order_relaxed);
    m_stopped.store(otherStopped, std::memory_order_relaxed);
    other.m_stopped.store(thisStopped, std::memory_order_relaxed);

    const size_t thisCount  = m_count.load(std::memory_order_relaxed);
    const size_t otherCount = other.m_count.load(std::memory_order_relaxed);
    m_count.store(otherCount, std::memory_order_relaxed);
    other.m_count.store(thisCount, std::memory_order_relaxed);
}

bool TaskQueue::stopped() const noexcept
{
    bool ret = m_stopped.load();
    return ret;
}

} // job::threads

