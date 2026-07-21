#include "job_io_async_thread.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstring>
#include <algorithm>

#include <job_logger.h>

namespace job::threads {

using namespace std::chrono_literals;

static SHORT toPollEvents(IOEvent events) noexcept
{
    SHORT out = 0;
    if (hasEvent(events, IOEvent::Read))
        out |= POLLRDNORM;
    if (hasEvent(events, IOEvent::Write))
        out |= POLLWRNORM;
    return out;
}

static IOEvent fromPollRevents(SHORT revents) noexcept
{
    IOEvent out = IOEvent::None;
    if (revents & (POLLRDNORM | POLLIN))
        out |= IOEvent::Read;
    if (revents & (POLLWRNORM | POLLOUT))
        out |= IOEvent::Write;
    if (revents & POLLERR)
        out |= IOEvent::Error;
    if (revents & (POLLHUP | POLLNVAL))
        out |= IOEvent::HangUp;
    return out;
}

// WSAPoll only polls SOCKETs, so the eventfd-style "wake the loop" trick
// needs a real connected loopback TCP socket pair -- Windows has no
// socketpair(). Standard technique: listen on 127.0.0.1:ephemeral,
// connect to it, accept the connection, discard the listener.
static bool createLoopbackPair(SOCKET &outRead, SOCKET &outWrite)
{
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // ephemeral

    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listener);
        return false;
    }

    int addrLen = sizeof(addr);
    if (getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &addrLen) == SOCKET_ERROR) {
        closesocket(listener);
        return false;
    }

    if (listen(listener, 1) == SOCKET_ERROR) {
        closesocket(listener);
        return false;
    }

    SOCKET writeSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (writeSock == INVALID_SOCKET) {
        closesocket(listener);
        return false;
    }

    if (connect(writeSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listener);
        closesocket(writeSock);
        return false;
    }

    SOCKET readSock = accept(listener, nullptr, nullptr);
    closesocket(listener);

    if (readSock == INVALID_SOCKET) {
        closesocket(writeSock);
        return false;
    }

    u_long nonBlocking = 1;
    ioctlsocket(readSock, FIONBIO, &nonBlocking);
    ioctlsocket(writeSock, FIONBIO, &nonBlocking);

    outRead  = readSock;
    outWrite = writeSock;
    return true;
}

struct JobIoAsyncThread::Backend {
    bool                    wsaInitialized{false};
    SOCKET                  wakeReadSock{INVALID_SOCKET};
    SOCKET                  wakeWriteSock{INVALID_SOCKET};
    std::vector<WSAPOLLFD>  pollFds;        // live registration set; only registerFD/unregisterFD touch this
    std::vector<WSAPOLLFD>  lastPollResult; // snapshot handed to WSAPoll; only the loop thread touches this
};

JobIoAsyncThread::JobIoAsyncThread() :
    m_backend(std::make_unique<Backend>())
{
    // Winsock is internally reference-counted -- each WSAStartup() call
    // needs a matching WSACleanup(), and it's explicitly documented as
    // safe to call from multiple instances/threads without a separate
    // global once-only registry.
    WSADATA wsaData{};
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        JOB_LOG_ERROR("[JobIoAsyncThread] WSAStartup failed: {}", wsaResult);
        return;
    }
    m_backend->wsaInitialized = true;

    if (!createLoopbackPair(m_backend->wakeReadSock, m_backend->wakeWriteSock)) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Failed to create wake socket pair: {}", WSAGetLastError());
        WSACleanup();
        m_backend->wsaInitialized = false;
        return;
    }

    WSAPOLLFD wakeEntry{};
    wakeEntry.fd = m_backend->wakeReadSock;
    wakeEntry.events = POLLRDNORM;
    m_backend->pollFds.push_back(wakeEntry);
}

JobIoAsyncThread::~JobIoAsyncThread() noexcept
{
    stop();

    if (m_backend->wakeReadSock != INVALID_SOCKET)
        closesocket(m_backend->wakeReadSock);
    if (m_backend->wakeWriteSock != INVALID_SOCKET)
        closesocket(m_backend->wakeWriteSock);
    if (m_backend->wsaInitialized)
        WSACleanup();
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
    if (m_backend->wakeWriteSock != INVALID_SOCKET) {
        char val = 1;
        int s = send(m_backend->wakeWriteSock, &val, 1, 0);
        if (s == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            JOB_LOG_ERROR("[JobIoAsyncThread] stop() failed to write to wake socket: {}", WSAGetLastError());
    }
    if (m_thread) {
        (void)m_thread->join();
        m_thread.reset();
    }
}

void JobIoAsyncThread::post(std::function<void()> task, int priority)
{
    AsyncEventLoop::post(std::move(task), priority);
    if (m_backend->wakeWriteSock != INVALID_SOCKET) {
        char val = 1;
        int s = send(m_backend->wakeWriteSock, &val, 1, 0);
        if (s == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            JOB_LOG_WARN("[JobIoAsyncThread] Failed to write to wake socket: {}", WSAGetLastError());
    }
}

bool JobIoAsyncThread::registerFD(int fd, IOEvent events, IOEventCallback callback)
{
    if (fd < 0 || !m_backend->wsaInitialized)
        return false;

    {
        std::scoped_lock lock(m_ioMutex);
        auto it = std::find_if(m_backend->pollFds.begin(), m_backend->pollFds.end(),
                               [fd](const WSAPOLLFD &e) { return e.fd == static_cast<SOCKET>(fd); });
        if (it != m_backend->pollFds.end()) {
            it->events = toPollEvents(events);
        } else {
            WSAPOLLFD entry{};
            entry.fd = static_cast<SOCKET>(fd);
            entry.events = toPollEvents(events);
            m_backend->pollFds.push_back(entry);
        }
        m_fdCallbacks[fd] = std::move(callback);
    }
    post([]{});
    return true;
}

bool JobIoAsyncThread::unregisterFD(int fd)
{
    if (fd < 0)
        return false;

    {
        std::scoped_lock lock(m_ioMutex);
        auto it = std::find_if(m_backend->pollFds.begin(), m_backend->pollFds.end(),
                               [fd](const WSAPOLLFD &e) { return e.fd == static_cast<SOCKET>(fd); });
        if (it != m_backend->pollFds.end())
            m_backend->pollFds.erase(it);
        m_fdCallbacks.erase(fd);
    }
    post([]{});
    return true;
}

void JobIoAsyncThread::processIOEvents(int event_count)
{
    // lastPollResult is only ever touched by this (the loop) thread safe to read without m_ioMutex.
    // Only the m_fdCallbacks lookups need it.
    std::vector<std::pair<IOEventCallback, IOEvent>> callbacksToRun;
    callbacksToRun.reserve(event_count);

    for (auto &entry : m_backend->lastPollResult) {
        if (entry.revents == 0)
            continue;

        if (entry.fd == m_backend->wakeReadSock) {
            char buf[64];
            // Limit the maximum number of drain cycles per event loop iteration
            // to prevent loop starvation during severe thread wake-up races.
            for (int iterations = 0; iterations < 4; ++iterations) {
                int s = recv(m_backend->wakeReadSock, buf, sizeof(buf), 0);

                if (s == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (err != WSAEWOULDBLOCK) {
                        JOB_LOG_WARN("[JobIoAsyncThread] error draining wake socket: {}", err);
                    }
                    break; // Buffer is dry or faulted
                }

                if (s == 0) {
                    break; // Socket closed gracefully
                }

                // If we read less than the full buffer size, the socket is guaranteed
                // to be empty right now. Break early to eliminate a costly WSAEWOULDBLOCK syscall.
                if (s < static_cast<int>(sizeof(buf))) {
                    break;
                }
            }
            continue;
        }

        int fd = static_cast<int>(entry.fd);
        IOEvent translated = fromPollRevents(entry.revents);

        std::scoped_lock lock(m_ioMutex);
        auto it = m_fdCallbacks.find(fd);
        if (it != m_fdCallbacks.end()) {
            callbacksToRun.push_back({it->second, translated});
        } else if (hasEvent(translated, IOEvent::Error) || hasEvent(translated, IOEvent::HangUp)) {
            JOB_LOG_WARN("[JobIoAsyncThread] ERR/HUP on unknown/unregistered fd {}", fd);
        }
    }

    // events is already IOEvent -- no re-conversion, same fix as the posix side.
    for (auto &[callback, events] : callbacksToRun)
        post([=]() { callback(events); });
}

void JobIoAsyncThread::loop(std::stop_token token, [[maybe_unused]] std::chrono::milliseconds idle_heartbeat)
{
    if (!m_backend->wsaInitialized) {
        JOB_LOG_ERROR("[JobIoAsyncThread] Loop starting with uninitialized Winsock. Aborting.");
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

        // Snapshot under the lock, then release before the blocking call --
        // registerFD/unregisterFD only ever touch pollFds, never this
        // snapshot, so they're never stuck waiting behind WSAPoll.
        {
            std::scoped_lock lock(m_ioMutex);
            m_backend->lastPollResult = m_backend->pollFds;
        }

        int event_count = WSAPoll(m_backend->lastPollResult.data(),
                                  static_cast<ULONG>(m_backend->lastPollResult.size()),
                                  timeout_ms);

        if (token.stop_requested())
            break;

        if (event_count == SOCKET_ERROR) {
            JOB_LOG_ERROR("[JobIoAsyncThread] WSAPoll error: {}", WSAGetLastError());
            continue;
        }

        if (event_count > 0)
            processIOEvents(event_count);
    }
}

} // namespace job::threads