#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

namespace job::threads {

template <typename T>
class JobBoundedMPMCQueue {
public:
    using Lock = std::unique_lock<std::mutex>;

    explicit JobBoundedMPMCQueue(std::size_t cap):
        m_cap(cap)
    {

    }

    ////////////////////////////////////////
    // blocking:
    ////////////////////////////////////////
    [[nodiscard]] bool pop(T &out)
    {
        Lock lock(m_mutex);
        m_not_empty.wait(lock, [&]{
            return m_closed || !m_deque.empty();
        });
        if (m_deque.empty())
            return false; // closed and drained

        out = std::move(m_deque.front());
        m_deque.pop_front();
        lock.unlock();
        m_not_full.notify_one();
        return true;
    }

    [[nodiscard]] bool push(T value)
    {
        Lock lock(m_mutex);
        m_not_full.wait(lock, [&]{
            return m_closed || m_deque.size() < m_cap;
        });

        if (m_closed)
            return false;

        m_deque.push_back(std::move(value));
        lock.unlock();
        m_not_empty.notify_one();
        return true;
    }

    ////////////////////////////////////////
    // !blocking:
    ////////////////////////////////////////
    [[nodiscard]] bool tryPop(T &out)
    {
        Lock lock(m_mutex);
        if (m_deque.empty())
            return false;

        out = std::move(m_deque.front());
        m_deque.pop_front();
        lock.unlock();
        m_not_full.notify_one();
        return true;
    }
    [[nodiscard]] bool tryPush(T value)
    {
        Lock lock(m_mutex);
        if (m_closed || m_deque.size() >= m_cap)
            return false;
        m_deque.push_back(std::move(value));
        lock.unlock();
        m_not_empty.notify_one();
        return true;
    }

    // no pushes | wake up waiters. pops will drain remaining.
    void close()
    {
        Lock lock(m_mutex);
        m_closed = true;
        m_not_empty.notify_all();
        m_not_full.notify_all();
    }

    [[nodiscard]] bool closed() const noexcept
    {
        Lock lock(m_mutex);
        return m_closed;
    }

    [[nodiscard]] bool isEmpty() const noexcept
    {
        Lock lock(m_mutex);
        return m_deque.empty();
    }


private:
    const std::size_t           m_cap;
    std::deque<T>               m_deque;
    mutable std::mutex          m_mutex;
    std::condition_variable     m_not_empty;
    std::condition_variable     m_not_full;
    bool                        m_closed = false;
};

} // namespace job::threads

// CHECKPOINT: v1.2



// Minor polish that could be worth in a future pass:
// If this is truly “general utility”, you might want [[nodiscard]] on tryPush/tryPop as well, to scream when someone ignores the bool.


