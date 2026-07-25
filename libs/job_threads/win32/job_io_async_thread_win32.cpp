#include "job_io_async_thread.h"

#include <cstring>
#include <algorithm>
#include <unordered_map>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <job_logger.h>

#include "win_fd_reg.h"

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

// WSAPoll only polls SOCKETs, so the eventfd-style "wake the loop" trick needs a real connected loopback TCP socket pair
static bool createLoopbackPair(SOCKET &outRead, SOCKET &outWrite)
{
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // ephemeral port

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
    bool                            wsaInitialized{false};
    SOCKET                          wakeReadSock{INVALID_SOCKET};
    SOCKET                          wakeWriteSock{INVALID_SOCKET};
    std::vector<WSAPOLLFD>          pollFds;
    std::vector<WSAPOLLFD>          lastPollResult;
    std::unordered_map<SOCKET, int> socketToFd;
};

JobIoAsyncThread::JobIoAsyncThread() :
    m_backend(std::make_unique<Backend>())
{
    WSADATA wsaData{};

    int const wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
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
    wakeEntry.revents = 0;

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
        m_backend->socketToFd.clear();
    }

    m_queue.stop();

    if (m_backend->wakeWriteSock != INVALID_SOCKET) {
        char const val = 1;
        int const result = send(m_backend->wakeWriteSock, &val, 1, 0);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
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
    if (fd < 0 || !m_backend->wsaInitialized) [[unlikely]]
        return false;

    SOCKET const socket = WinFdReg::instance().lookup(fd);
    if (socket == INVALID_SOCKET) [[unlikely]] {
        JOB_LOG_ERROR("[JobIoAsyncThread] registerFD received invalid Win32 fd {}", fd);
        return false;
    }

    {
        std::scoped_lock lock(m_ioMutex);

        auto const existingSocket = m_backend->socketToFd.find(socket);

        if (existingSocket != m_backend->socketToFd.end() && existingSocket->second != fd) [[unlikely]] {
            JOB_LOG_ERROR("[JobIoAsyncThread] native socket is already registered under fd {} instead of fd {}", existingSocket->second, fd);
            return false;
        }

        auto const pollEntry = std::find_if(m_backend->pollFds.begin(), m_backend->pollFds.end(),
                                            [socket](WSAPOLLFD const &entry) {
                                                return entry.fd == socket;
                                            });

        if (pollEntry != m_backend->pollFds.end()) {
            pollEntry->events = toPollEvents(events);
            pollEntry->revents = 0;
        } else {
            WSAPOLLFD entry{};
            entry.fd = socket;
            entry.events = toPollEvents(events);
            entry.revents = 0;

            try {
                m_backend->pollFds.push_back(entry);
                m_backend->socketToFd.emplace(socket, fd);
            } catch (...) {
                auto const insertedEntry = std::find_if(m_backend->pollFds.begin(), m_backend->pollFds.end(),
                                                        [socket](WSAPOLLFD const &candidate) {
                                                            return candidate.fd == socket;
                                                        });

                if (insertedEntry != m_backend->pollFds.end())
                    m_backend->pollFds.erase(insertedEntry);

                JOB_LOG_ERROR("[JobIoAsyncThread] failed to store Win32 fd {}", fd);
                return false;
            }
        }

        m_fdCallbacks[fd] = std::move(callback);
    }

    post([] {});
    return true;
}

bool JobIoAsyncThread::modifyFD(int fd, IOEvent events)
{
    if (fd < 0 || !m_backend->wsaInitialized)
        return false;

    SOCKET socket = WinFdReg::instance().lookup(fd);
    if (socket == INVALID_SOCKET) {
        JOB_LOG_WARN("[JobIoAsyncThread] modifyFD received invalid Win32 fd {}", fd);
        return false;
    }

    {
        std::scoped_lock lock(m_ioMutex);
        auto const socketEntry =
            m_backend->socketToFd.find(socket);

        if (socketEntry == m_backend->socketToFd.end() ||
            socketEntry->second != fd) {
            JOB_LOG_WARN( "[JobIoAsyncThread] modifyFD called for unregistered fd {}", fd);
            return false;
        }

        auto const pollEntry = std::find_if( m_backend->pollFds.begin(), m_backend->pollFds.end(),
                                            [socket](WSAPOLLFD const &entry) {
                                                return entry.fd == socket;
                                            });

        if (pollEntry == m_backend->pollFds.end()) {
            JOB_LOG_WARN("[JobIoAsyncThread] no poll entry for fd {}", fd );
            return false;
        }

        pollEntry->events = toPollEvents(events);
        pollEntry->revents = 0;
    }

    post([] {});
    return true;
}

bool JobIoAsyncThread::unregisterFD(int fd)
{
    if (fd < 0) [[unlikely]]
        return false;

    SOCKET socket = WinFdReg::instance().lookup(fd);
    bool removed = false;

    {
        std::scoped_lock lock(m_ioMutex);

        // Fallback: search socketToFd if WinFdReg lookup returned INVALID_SOCKET
        // if (socket == INVALID_SOCKET) {
        //     for (auto const &[sock, tokenFd] : m_backend->socketToFd) {
        //         if (tokenFd == fd) {
        //             socket = sock;
        //             break;
        //         }
        //     }
        // }
        // Fallback: search socketToFd if WinFdReg lookup returned INVALID_SOCKET
        if (socket == INVALID_SOCKET) {
            for (auto const &[sock, tokenFd] : m_backend->socketToFd) {
                if (tokenFd == fd) {
                    socket = sock;
                    break;
                }
            }
        }

        if (socket != INVALID_SOCKET) {
            auto const pollEntry = std::find_if(m_backend->pollFds.begin(), m_backend->pollFds.end(),
                                                [socket](WSAPOLLFD const &entry) {
                                                    return entry.fd == socket;
                                                });

            if (pollEntry != m_backend->pollFds.end()) {
                m_backend->pollFds.erase(pollEntry);
                removed = true;
            }

            m_backend->socketToFd.erase(socket);
        }

        m_fdCallbacks.erase(fd);
    }

    if (removed) {
        post([] {});
    }

    return removed;
}

void JobIoAsyncThread::processIOEvents(int event_count)
{
    Event_Callback callbacksToRun;
    callbacksToRun.reserve(static_cast<std::size_t>(event_count));

    for (auto const &entry : m_backend->lastPollResult) {
        if (entry.revents == 0)
            continue;

        if (entry.fd == m_backend->wakeReadSock) {
            char buffer[64];
            for (int iteration = 0; iteration < 4; ++iteration) {
                int const result = recv(m_backend->wakeReadSock, buffer, static_cast<int>(sizeof(buffer)), 0);
                if (result == SOCKET_ERROR) {
                    int const error = WSAGetLastError();
                    if (error != WSAEWOULDBLOCK)
                        JOB_LOG_WARN("[JobIoAsyncThread] error draining wake socket: {}", error);
                    break;
                }

                if (result == 0 || result < static_cast<int>(sizeof(buffer)))
                    break;
            }

            continue;
        }

        IOEvent const translated = fromPollRevents(entry.revents);

        std::scoped_lock lock(m_ioMutex);
        auto const fdEntry = m_backend->socketToFd.find(entry.fd);
        if (fdEntry == m_backend->socketToFd.end()) {
            if (hasEvent(translated, IOEvent::Error) || hasEvent(translated, IOEvent::HangUp))
                JOB_LOG_WARN("[JobIoAsyncThread] ERR/HUP on unknown native socket");
            continue;
        }

        int const fd = fdEntry->second;
        auto const callbackEntry = m_fdCallbacks.find(fd);
        if (callbackEntry != m_fdCallbacks.end())
            callbacksToRun.emplace_back(callbackEntry->second, translated);
        else if (hasEvent(translated, IOEvent::Error) || hasEvent(translated, IOEvent::HangUp))
            JOB_LOG_WARN("[JobIoAsyncThread] ERR/HUP on unregistered fd {}", fd);
    }

    for (auto &[callback, events] : callbacksToRun)
        post([callback = std::move(callback), events]() mutable { callback(events); });
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

        // Snapshot under lock, then release before blocking WSAPoll call
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