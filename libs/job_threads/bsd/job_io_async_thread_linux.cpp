#include "job_io_async_thread.h"

#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <job_logger.h>

namespace job::threads {

constexpr int kDefaultEpollEvents = 64;
using namespace std::chrono_literals;

struct JobIoAsyncThread::Backend {
    int                       epollFd{-1};
    int                       eventFd{-1};
    std::vector<epoll_event>  epollEvents;
};

// posix backend
static uint32_t toEpollBits(IOEvent events) noexcept
{
    uint32_t bits = 0;
    if (hasEvent(events, IOEvent::Read))
        bits |= EPOLLIN;
    if (hasEvent(events, IOEvent::Write))
        bits |= EPOLLOUT;
    if (hasEvent(events, IOEvent::EdgeTriggered))
        bits |= EPOLLET;
    return bits;
}

static IOEvent fromEpollBits(uint32_t bits) noexcept
{
    IOEvent out = IOEvent::None;
    if (bits & EPOLLIN)
        out |= IOEvent::Read;
    if (bits & EPOLLOUT)
        out |= IOEvent::Write;
    if (bits & EPOLLERR)
        out |= IOEvent::Error;
    if (bits & EPOLLHUP)
        out |= IOEvent::HangUp;
    return out;
}

JobIoAsyncThread::JobIoAsyncThread() :
    m_backend(std::make_unique<Backend>())
{
    m_backend->epollFd = epoll_create1(0);
    if (m_backend->epollFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to create epoll FD: {}", strerror(errno));
        return;
    }

    m_backend->eventFd = eventfd(0, EFD_NONBLOCK);
    if (m_backend->eventFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to create eventfd: {}", strerror(errno));
        ::close(m_backend->epollFd);
        m_backend->epollFd = -1;
        return;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_backend->eventFd;
    if (epoll_ctl(m_backend->epollFd, EPOLL_CTL_ADD, m_backend->eventFd, &ev) == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to add eventfd to epoll: {}", strerror(errno));
        ::close(m_backend->epollFd);
        ::close(m_backend->eventFd);
        m_backend->epollFd = -1;
        m_backend->eventFd = -1;
        return;
    }
    m_backend->epollEvents.resize(kDefaultEpollEvents);
}

JobIoAsyncThread::~JobIoAsyncThread() noexcept
{
    stop();

    if (m_backend->epollFd != -1)
        ::close(m_backend->epollFd);

    if (m_backend->eventFd != -1)
        ::close(m_backend->eventFd);
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
    if (m_backend->eventFd != -1) {
        uint64_t val = 1;
        ssize_t s = write(m_backend->eventFd, &val, sizeof(uint64_t));
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
    if (m_backend->eventFd != -1) {
        uint64_t val = 1;
        ssize_t s = write(m_backend->eventFd, &val, sizeof(uint64_t));
        if (s != sizeof(uint64_t) && errno != EAGAIN)
            JOB_LOG_WARN("[JobIoAsyncThread] Failed to write to eventfd: {}", strerror(errno));
    }
}

bool JobIoAsyncThread::registerFD(int fd, IOEvent events, IOEventCallback callback)
{
    if (fd < 0 || m_backend->epollFd == -1)
        return false;

    epoll_event ev{};
    ev.events = toEpollBits(events);
    ev.data.fd = fd;
    if (epoll_ctl(m_backend->epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] epoll_ctl ADD failed for fd {}: {}", fd, strerror(errno));
        return false;
    }
    {
        std::scoped_lock lock(m_ioMutex);
        m_fdCallbacks[fd] = std::move(callback);
    }
    post([]{});
    return true;
}
bool JobIoAsyncThread::modifyFD(int fd, IOEvent events)
{
    if (fd < 0 || m_backend->epollFd == -1)
        return false;

    {
        std::scoped_lock lock(m_ioMutex);
        if (!m_fdCallbacks.contains(fd)) {
            JOB_LOG_WARN("[JobIoAsyncThread] modifyFD called for unregistered fd {}", fd);
            return false;
        }
    }

    epoll_event ev{};
    ev.events = toEpollBits(events);
    ev.data.fd = fd;

    if (::epoll_ctl( m_backend->epollFd, EPOLL_CTL_MOD, fd, &ev ) == -1) {
        JOB_LOG_ERROR( "[JobIoAsyncThread] epoll_ctl MOD failed for fd {}: {}", fd, std::strerror(errno));
        return false;
    }

    post([] {});
    return true;
}
bool JobIoAsyncThread::unregisterFD(int fd)
{
    if (fd < 0 || m_backend->epollFd == -1)
        return false;

    epoll_event ev{};
    if (epoll_ctl(m_backend->epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
        if (errno != ENOENT)
            JOB_LOG_WARN("[JobIoAsyncThread] epoll_ctl DEL failed for fd {}: {}", fd, strerror(errno));

    {
        std::scoped_lock lock(m_ioMutex);
        m_fdCallbacks.erase(fd);
    }

    post([]{});
    return true;
}

void JobIoAsyncThread::processIOEvents(int event_count)
{
    Event_Callback callbacksToRun;
    callbacksToRun.reserve(event_count);

    {
        std::scoped_lock lock(m_ioMutex);
        for (int i = 0; i < event_count; ++i) {
            int fd = m_backend->epollEvents[i].data.fd;
            auto ev = m_backend->epollEvents[i].events;
            if (fd == m_backend->eventFd) {
                for (;;) {
                    uint64_t val;
                    ssize_t s = read(m_backend->eventFd, &val, sizeof(val));
                    if (s == -1 && errno == EAGAIN)
                        break;
                    if (s != sizeof(val)) {
                        JOB_LOG_WARN("[JobIoAsyncThread] short read on eventfd");
                        break;
                    }
                }
                continue;
            }

            auto it = m_fdCallbacks.find(fd);
            if (it != m_fdCallbacks.end()) {
                callbacksToRun.push_back({it->second, fromEpollBits(ev)});
            } else if (ev & (EPOLLERR | EPOLLHUP)) {
                JOB_LOG_WARN("[JobIoAsyncThread] ERR/HUP on unknown/unregistered fd {}", fd);
            }
        }
    }

    for (auto& [callback, events] : callbacksToRun)
        post([=]() { callback(events); });
}

void JobIoAsyncThread::loop(std::stop_token token, [[maybe_unused]] std::chrono::milliseconds idle_heartbeat)
{
    if (m_backend->epollFd == -1) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Loop starting with invalid epoll FD. Aborting.");
        return;
    }

    while (!token.stop_requested()) {
        processTimers();
        processTasks();

        auto next_wakeup = calculateNextWakeup();
        int timeout_ms = -1;

        if (!m_queue.isEmpty()) {
            timeout_ms = 0;
        } else if (next_wakeup != std::chrono::milliseconds::max()) {
            using rep = std::chrono::milliseconds::rep;
            timeout_ms = static_cast<int>(std::max(static_cast<rep>(0), next_wakeup.count()));
        }

        int event_count = epoll_wait(m_backend->epollFd, m_backend->epollEvents.data(), m_backend->epollEvents.size(), timeout_ms);

        if (token.stop_requested())
            break;

        if (event_count < 0) {
            if (errno == EINTR)
                continue;
            JOB_LOG_ERROR("[JobIoAsyncThread] epoll_wait error: {}", strerror(errno));
            continue;
        }

        if (event_count == static_cast<int>(m_backend->epollEvents.size())) {
            m_backend->epollEvents.resize(m_backend->epollEvents.size() * 2);
            JOB_LOG_INFO("[JobIoAsyncThread] Epoll event buffer grew to {}", m_backend->epollEvents.size());
        }

        if (event_count > 0)
            processIOEvents(event_count);
    }
}

} // namespace job::threads