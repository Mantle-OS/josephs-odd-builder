#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <job_logger.h>

#include "isocket_io.h"
#include "jobnet_export.h"
#include "resolve/job_ssl_context.h"
#include "resolve/job_ssl_error.h"

namespace job::net {

class JOBNET_EXPORT SslSocket final : public std::enable_shared_from_this<SslSocket> {
public:
    using Ptr  = std::shared_ptr<SslSocket>;
    using WPtr = std::weak_ptr<SslSocket>;

    enum class State : uint8_t {
        Uninitialized = 0,
        WaitingForTransport,
        Handshaking,
        Encrypted,
        ShuttingDown,
        Closed,
        Error
    };

    struct PrivateTag {
    private:
        PrivateTag() = default;
        friend class SslSocket;
    };

    SslSocket(PrivateTag, ISocketIO::Ptr socket, JobSslContext::Ptr context);
    ~SslSocket();

    SslSocket(const SslSocket &) = delete;
    SslSocket &operator=(const SslSocket &) = delete;
    SslSocket(SslSocket &&) = delete;
    SslSocket &operator=(SslSocket &&) = delete;

    [[nodiscard]] static Ptr create(ISocketIO::Ptr socket, JobSslContext::Ptr context)
    {
        auto sslSocket = std::make_shared<SslSocket>(PrivateTag{}, std::move(socket), std::move(context));

        if (sslSocket->state() != State::Error)
            sslSocket->attachSocketCallbacks();

        return sslSocket;
    }

    [[nodiscard]] bool connectToHost(const JobIpAddr &ipaddr)
    {
        if (!prepareForConnection())
            return false;

        m_peerName.clear();

        if (!m_socket->connectToHost(ipaddr)) {
            m_state.store(State::Error);
            return false;
        }

        if (m_socket->state() == ISocketIO::SocketState::Connected)
            return startHandshake();

        return true;
    }

    [[nodiscard]] bool connectToHost(const JobUrl &url)
    {
        if (!prepareForConnection())
            return false;

        m_peerName = url.host();

        if (!m_socket->connectToHost(url)) {
            m_state.store(State::Error);
            return false;
        }

        if (m_socket->state() == ISocketIO::SocketState::Connected)
            return startHandshake();

        return true;
    }

    [[nodiscard]] bool startHandshake()
    {
        if (!m_socket || m_socket->state() != ISocketIO::SocketState::Connected) {
            recordSslError(JobSslError::SslErrNo::InvalidState, "SSL handshake requires a connected transport");
            return false;
        }

        const State currentState = m_state.load();

        if (currentState == State::Encrypted)
            return true;

        if (currentState == State::Handshaking)
            return driveHandshake();

        if (currentState == State::ShuttingDown || currentState == State::Closed) {
            recordSslError(JobSslError::SslErrNo::InvalidState, "SSL handshake cannot start while the socket is closing");
            return false;
        }

        if (!setupSsl()) {
            m_state.store(State::Error);
            return false;
        }

        m_state.store(State::Handshaking);
        return driveHandshake();
    }

    void disconnect();

    [[nodiscard]] int64_t read(void *buffer, size_t size);
    [[nodiscard]] int64_t write(const void *buffer, size_t size);

    [[nodiscard]] State state() const noexcept
    {
        return m_state.load();
    }

    [[nodiscard]] ISocketIO::SocketState socketState() const noexcept
    {
        return m_socket ? m_socket->state() : ISocketIO::SocketState::Error;
    }

    [[nodiscard]] bool isEncrypted() const noexcept
    {
        return state() == State::Encrypted;
    }

    [[nodiscard]] bool isOpen() const noexcept
    {
        switch (state()) {
        case State::Handshaking:
        case State::Encrypted:
        case State::ShuttingDown:
            return true;

        default:
            return false;
        }
    }

    [[nodiscard]] JobSslError::SslErrNo lastError() const noexcept
    {
        return m_errors.lastError();
    }

    [[nodiscard]] std::string lastErrorString() const
    {
        return m_errors.lastErrorString();
    }

    void setOption(ISocketIO::SocketOption option, bool enable)
    {
        if (m_socket)
            m_socket->setOption(option, enable);
    }

    [[nodiscard]] bool option(ISocketIO::SocketOption option) const
    {
        return m_socket && m_socket->option(option);
    }

    [[nodiscard]] std::string peerAddress() const
    {
        return m_socket ? m_socket->peerAddress() : std::string{};
    }

    [[nodiscard]] uint16_t peerPort() const
    {
        return m_socket ? m_socket->peerPort() : 0;
    }

    [[nodiscard]] std::string localAddress() const
    {
        return m_socket ? m_socket->localAddress() : std::string{};
    }

    [[nodiscard]] uint16_t localPort() const
    {
        return m_socket ? m_socket->localPort() : 0;
    }

    [[nodiscard]] ISocketIO::Ptr socket() const noexcept
    {
        return m_socket;
    }

    [[nodiscard]] JobSslContext::Ptr context() const noexcept
    {
        return m_context;
    }

    void dumpState() const
    {
        JOB_LOG_DEBUG(
            "[SslSocket] state={} socketState={} encrypted={} peer={}:{} local={}:{} sslError={} sslErrorString={}",
            static_cast<int>(state()),
            static_cast<int>(socketState()),
            isEncrypted(),
            peerAddress(),
            peerPort(),
            localAddress(),
            localPort(),
            static_cast<int>(lastError()),
            lastErrorString()
            );
    }

    std::function<void()>                     onEncrypted;
    std::function<void(const char *, size_t)> onRead;
    std::function<void(const char *, size_t)> onWrite;
    std::function<void()>                     onDisconnect;
    std::function<void(int)>                  onSocketError;
    JobSslError::ErrorCallback                onSslError;

private:
    struct Impl;

    [[nodiscard]] bool prepareForConnection()
    {
        if (!m_socket) {
            recordSslError(JobSslError::SslErrNo::InvalidState, "SSL socket transport is invalid");
            return false;
        }

        const State currentState = state();

        if (currentState != State::WaitingForTransport && currentState != State::Closed) {
            recordSslError(JobSslError::SslErrNo::InvalidState, "SSL socket cannot connect in its current state");
            return false;
        }

        m_state.store(State::WaitingForTransport);
        return true;
    }

    void attachSocketCallbacks()
    {
        if (!m_socket)
            return;

        const WPtr weakSelf = weak_from_this();

        m_socket->onConnect = [weakSelf]() {
            if (const auto self = weakSelf.lock())
                self->handleSocketConnect();
        };

        m_socket->onRead = [weakSelf](const char *data, size_t size) {
            if (const auto self = weakSelf.lock())
                self->handleSocketRead(data, size);
        };

        m_socket->onWrite = [weakSelf](const char *data, size_t size) {
            if (const auto self = weakSelf.lock())
                self->handleSocketWrite(data, size);
        };

        m_socket->onDisconnect = [weakSelf]() {
            if (const auto self = weakSelf.lock())
                self->handleSocketDisconnect();
        };

        m_socket->onError = [weakSelf](int error) {
            if (const auto self = weakSelf.lock())
                self->handleSocketError(error);
        };
    }

    void detachSocketCallbacks() noexcept
    {
        if (!m_socket)
            return;

        m_socket->onConnect = nullptr;
        m_socket->onRead = nullptr;
        m_socket->onWrite = nullptr;
        m_socket->onDisconnect = nullptr;
        m_socket->onError = nullptr;
    }

    void handleSocketConnect()
    {
        if (state() == State::WaitingForTransport && !startHandshake())
            JOB_LOG_ERROR("[SslSocket] Failed to start TLS handshake");
    }

    void handleSocketRead(const char *data, size_t size)
    {
        static_cast<void>(data);
        static_cast<void>(size);

        switch (state()) {
        case State::Handshaking:
            if (!driveHandshake())
                JOB_LOG_ERROR("[SslSocket] Failed to advance TLS handshake from readable event");
            break;

        case State::ShuttingDown:
            shutdownSsl();
            break;

        case State::Encrypted:
            if (onRead)
                onRead(nullptr, 0);
            break;

        default:
            break;
        }
    }

    void handleSocketWrite(const char *data, size_t size)
    {
        static_cast<void>(data);
        static_cast<void>(size);

        switch (state()) {
        case State::Handshaking:
            if (!driveHandshake())
                JOB_LOG_ERROR("[SslSocket] Failed to advance TLS handshake from writable event");
            break;

        case State::ShuttingDown:
            shutdownSsl();
            break;

        case State::Encrypted:
            if (onWrite)
                onWrite(nullptr, 0);
            break;

        default:
            break;
        }
    }

    void handleSocketDisconnect()
    {
        const State previousState = m_state.exchange(State::Closed);

        if (previousState == State::Closed)
            return;

        releaseSsl();

        if (onDisconnect)
            onDisconnect();
    }

    void handleSocketError(int error)
    {
        const State previousState = m_state.exchange(State::Error);

        if (previousState == State::Closed || previousState == State::Error)
            return;

        if (onSocketError)
            onSocketError(error);
    }

    [[nodiscard]] bool tryBeginShutdown() noexcept
    {
        State expected = state();

        while (expected != State::Closed && expected != State::ShuttingDown) {
            if (m_state.compare_exchange_weak(expected, State::ShuttingDown))
                return true;
        }

        return false;
    }

    [[nodiscard]] bool setupSsl();
    [[nodiscard]] bool driveHandshake();
    [[nodiscard]] bool processSslError(int result);
    [[nodiscard]] bool verifyPeer();

    [[nodiscard]] bool updateEvents(JobSslError::SslErrNo error)
    {
        if (!m_socket)
            return false;

        threads::IOEvent events =
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered;

        switch (error) {
        case JobSslError::SslErrNo::WantRead:
        case JobSslError::SslErrNo::WantAccept:
            events |= threads::IOEvent::Read;
            break;

        case JobSslError::SslErrNo::WantWrite:
        case JobSslError::SslErrNo::WantConnect:
            events |= threads::IOEvent::Write;
            break;

        default:
            recordSslError(JobSslError::SslErrNo::InvalidState, "Invalid TLS event transition");
            return false;
        }

        if (!m_socket->setEvents(events)) {
            recordSslError(JobSslError::SslErrNo::InternalError, "Failed to update transport events");
            return false;
        }

        return true;
    }

    void recordSslError(JobSslError::SslErrNo error, const std::string &message = {})
    {
        m_errors.recordError(error, message);
        m_state.store(State::Error);
        JOB_LOG_ERROR("[SslSocket] {}", m_errors.lastErrorString());
    }

    void shutdownSsl() noexcept;

    /*
     * Releases only the platform TLS session.
     *
     * It must be idempotent and must not detach transport callbacks or
     * disconnect the underlying socket.
     */
    void releaseSsl() noexcept;

    ISocketIO::Ptr        m_socket;
    JobSslContext::Ptr    m_context;
    std::unique_ptr<Impl> m_impl;
    JobSslError           m_errors;
    std::atomic<State>    m_state{State::Uninitialized};
    std::mutex            m_readMutex;
    std::mutex            m_writeMutex;
    std::string           m_peerName;
};

} // namespace job::net