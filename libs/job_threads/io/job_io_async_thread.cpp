#include "job_io_async_thread.h"

#include <unistd.h>
#include <cstring>
#include <algorithm>

#include <sys/eventfd.h>

#include <job_logger.h>

namespace job::threads {

constexpr int kDefaultEpollEvents = 64;
using namespace std::chrono_literals;

JobIoAsyncThread::JobIoAsyncThread()
{
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to create epoll FD: {}", strerror(errno));
        return;
    }

    m_eventFd = eventfd(0, EFD_NONBLOCK);
    if (m_eventFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to create eventfd: {}", strerror(errno));
        ::close(m_epollFd);
        m_epollFd = -1;
        return;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_eventFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &ev) == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to add eventfd to epoll: {}", strerror(errno));
        ::close(m_epollFd);
        ::close(m_eventFd);
        m_epollFd = -1;
        m_eventFd = -1;
        return;
    }
    m_epollEvents.resize(kDefaultEpollEvents);
}

JobIoAsyncThread::~JobIoAsyncThread() noexcept
{
    stop();

    if (m_epollFd != -1)
        ::close(m_epollFd);

    if (m_eventFd != -1)
        ::close(m_eventFd);
}

void JobIoAsyncThread::stop()
{
    if (!m_running.exchange(false))
        return;

    if (m_thread)
        m_thread->requestStop();

    {
        std::scoped_lock lock(m_timerMutex);
        m_timers.clear();
    }

    {
        std::scoped_lock lock(m_ioMutex);
        m_fdCallbacks.clear();
    }

    m_queue.stop();
    if (m_eventFd != -1) {
        uint64_t val = 1;
        ssize_t s = write(m_eventFd, &val, sizeof(uint64_t));

        if (s != sizeof(uint64_t) && errno != EAGAIN)
            JOB_LOG_ERROR("[JobIoAsyncThread] stop() failed to write to eventfd: {}", strerror(errno));

    }
    if (m_thread) {
        (void)m_thread->join();
        m_thread.reset();
    }
}


void JobIoAsyncThread::post(std::function<void()> task, int priority)
{
    AsyncEventLoop::post(std::move(task), priority);
    if (m_eventFd != -1) {
        uint64_t val = 1;
        ssize_t s = write(m_eventFd, &val, sizeof(uint64_t));
        if (s != sizeof(uint64_t) && errno != EAGAIN)
            JOB_LOG_WARN("[JobIoAsyncThread] Failed to write to eventfd: {}", strerror(errno));
    }
}

bool JobIoAsyncThread::registerFD(int fd, uint32_t events, IOEventCallback callback)
{
    if (fd < 0 || m_epollFd == -1)
        return false;

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] epoll_ctl ADD failed for fd {}: {}", fd, strerror(errno));
        return false;
    }
    {
        std::scoped_lock lock(m_ioMutex);
        m_fdCallbacks[fd] = std::move(callback);
    }
    post([]{

    });
    return true;
}

bool JobIoAsyncThread::unregisterFD(int fd)
{
    if (fd < 0 || m_epollFd == -1) return false;
    epoll_event ev{};
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, &ev) == -1) {
        // ENOENT (No such file or directory) is fine, it means it was already removed.
        if (errno != ENOENT) {
            JOB_LOG_WARN("[JobIoAsyncThread] epoll_ctl DEL failed for fd {}: {}", fd, strerror(errno));
        }
    }

    {
        std::scoped_lock lock(m_ioMutex);
        m_fdCallbacks.erase(fd);
    }

    post([]{

    });
    return true;
}

void JobIoAsyncThread::processIOEvents(int event_count)
{
    std::vector<std::pair<IOEventCallback, uint32_t>> callbacksToRun;
    callbacksToRun.reserve(event_count);

    {
        std::scoped_lock lock(m_ioMutex);
        for (int i = 0; i < event_count; ++i) {
            int fd = m_epollEvents[i].data.fd;
            auto ev = m_epollEvents[i].events;
            if (fd == m_eventFd) {
                for (;;) {
                    uint64_t val;
                    ssize_t s = read(m_eventFd, &val, sizeof(val)); // non-blocking . . . .
                    if (s == -1 && errno == EAGAIN)
                        break; // Drained

                    if (s != sizeof(val)) {
                        JOB_LOG_WARN("[JobIoAsyncThread] short read on eventfd");
                        break;
                    }
                }
                continue;
            }

            auto it = m_fdCallbacks.find(fd);
            if (it != m_fdCallbacks.end()) {
                callbacksToRun.push_back({it->second, ev});
            } else {
                if ((ev & (EPOLLERR | EPOLLHUP))) {
                    JOB_LOG_WARN("[JobIoAsyncThread] ERR/HUP on unknown/unregistered fd {}", fd);
                }
            }
        }
    }

    for (auto& [callback, events] : callbacksToRun) {
        post([=]() {
            callback(events);
        });
    }
}

void JobIoAsyncThread::loop(std::stop_token token, [[maybe_unused]] std::chrono::milliseconds idle_heartbeat)
{
    if (m_epollFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Loop starting with invalid epoll FD. Aborting.");
        return;
    }

    while (!token.stop_requested()) {
        processTimers();
        processTasks();

        auto next_wakeup = calculateNextWakeup();
        int timeout_ms = -1;

        if (!m_queue.isEmpty()) {
            timeout_ms = 0; // Tasks are waiting, don't block
        } else if (next_wakeup != std::chrono::milliseconds::max()) {
            using rep = std::chrono::milliseconds::rep;
            timeout_ms = static_cast<int>(std::max(static_cast<rep>(0), next_wakeup.count()));
        }

        int event_count = epoll_wait(m_epollFd, m_epollEvents.data(), m_epollEvents.size(), timeout_ms);

        if (token.stop_requested())
            break;

        if (event_count < 0) {
            if (errno == EINTR)
                continue;
            JOB_LOG_ERROR("[JobIoAsyncThread] epoll_wait error: {}", strerror(errno));
            continue;
        }

        if (event_count == static_cast<int>(m_epollEvents.size())) {
            m_epollEvents.resize(m_epollEvents.size() * 2);
            JOB_LOG_INFO("[JobIoAsyncThread] Epoll event buffer grew to {}", m_epollEvents.size());
        }

        if (event_count > 0) {
            processIOEvents(event_count);
        }
    }
}

} // namespace job::threads
