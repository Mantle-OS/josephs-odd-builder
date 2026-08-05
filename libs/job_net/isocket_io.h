#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
#include <type_traits>
// threads
#include <job_io_async_thread.h>
#include <job_logger.h>
#include <job_assert.h>
#include "job_url.h"
#include "resolve/job_ipaddr.h"
#include "resolve/job_resolver.h"
#include "sockets/job_socket_error.h"
#include "jobnet_export.h"
namespace job::net {
class JOBNET_EXPORT ISocketIO : public std::enable_shared_from_this<ISocketIO> {
public:
    using Ptr = std::shared_ptr<ISocketIO>;
    enum class SocketType : uint8_t {
        Unknown = 0,
        Tcp,
        Udp,
        Unix,
        SSL
    };
    enum class SocketState : uint8_t {
        Unconnected = 0,
        Connecting,
        Connected,
        Closing,
        Closed,
        Listening,
        Bound,
        Error
    };
    enum class SocketOption : uint8_t {
        ReuseAddress = 1 << 0,
        KeepAlive    = 1 << 1,
        Linger       = 1 << 2,
        TcpNoDelay   = 1 << 3,
        Broadcast    = 1 << 4,
        NonBlocking  = 1 << 5
    };
    explicit ISocketIO(threads::JobIoAsyncThread::Ptr loop);
    virtual ~ISocketIO() = default;

    virtual bool connectToHost(const JobIpAddr &ipaddr) = 0;
    virtual bool connectToHost(const JobUrl &url)
    {
        if (!m_resolver) {
            JOB_LOG_ERROR("[ISocketIO] connectToHost(url) requires a resolver — call setResolver() first");
            return false;
        }

        auto weakSelf = weak_from_this();
        return m_resolver->resolveAsync(url, [weakSelf](int errorCode, const std::vector<JobIpAddr> &addresses) {
            auto self = weakSelf.lock();
            if (!self)
                return; // socket destroyed before resolution completed

            if (errorCode != 0 || addresses.empty()) {
                if (self->onError)
                    self->onError(errorCode != 0 ? errorCode : -1);
                return;
            }

            for (const auto &addr : addresses) {
                if (self->connectToHost(addr))
                    return;
            }

            if (self->onError)
                self->onError(-1); // every candidate rejected synchronously
        });
    }

    // The one bind primitive every transport implements.
    virtual bool bind(const JobIpAddr &addr) = 0;

    // Shared, non-virtual convenience wrapper — was duplicated per-socket-type before.
    bool bind(const std::string &address, uint16_t port = 0)
    {
        JobIpAddr addr(address, port);
        return bind(addr);
    }

    virtual bool bind(const JobUrl &url) = 0;

    virtual bool listen(int backlog = 5) = 0;
    virtual ISocketIO::Ptr accept() = 0;
    virtual void disconnect() = 0;
    virtual int64_t read(void *buffer, size_t size) = 0;
    virtual int64_t write(const void *buffer, size_t size) = 0;
    virtual SocketState state() const noexcept = 0;
    virtual SocketErrors::SocketErrNo lastError() const noexcept = 0;
    virtual SocketType type() const noexcept = 0;
    virtual void setOption(SocketOption option, bool enable) = 0;
    virtual bool option(SocketOption option) const = 0;
    virtual std::string peerAddress() const = 0;
    virtual uint16_t peerPort() const = 0;
    virtual std::string localAddress() const = 0;
    virtual uint16_t localPort() const = 0;
    virtual void dumpState() const = 0;
    int fd() const noexcept;
    void setLoop(const threads::JobIoAsyncThread::Ptr &loop);

    void setResolver(JobResolver::Ptr resolver) { m_resolver = std::move(resolver); }
    [[nodiscard]] JobResolver::Ptr resolver() const noexcept { return m_resolver; }

    [[nodiscard]] virtual bool setEvents(threads::IOEvent events) noexcept
    {
        if (m_fd < 0) {
            JOB_LOG_ERROR("[ISocketIO] fd is invalid");
            return false;
        }

        const auto loop = m_loop.lock();

        if (!loop) {
            JOB_LOG_ERROR("[ISocketIO] event loop is invalid");
            return false;
        }

        if (!loop->modifyFD(m_fd, events)) [[unlikely]] {
            JOB_LOG_ERROR("[ISocketIO] Failed to modify events for fd {}", m_fd);
            return false;
        } else {
            return true;
        }
    }

    std::function<void()> onConnect;
    std::function<void(const char*, size_t)> onRead;
    std::function<void(const char*, size_t)> onWrite;
    std::function<void()> onReady; // For UDP
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;
    std::function<void(std::shared_ptr<ISocketIO>)> onAccept;
protected:
    virtual void onEvents(threads::IOEvent events) = 0;
    virtual void registerEvents(threads::IOEvent events);
    virtual void modifyEvents(threads::IOEvent events);

    std::weak_ptr<threads::JobIoAsyncThread> m_loop;
    JobResolver::Ptr m_resolver{nullptr};
    int m_fd{-1};
};
constexpr ISocketIO::SocketOption operator|(ISocketIO::SocketOption a, ISocketIO::SocketOption b) noexcept
{
    using T = std::underlying_type_t<ISocketIO::SocketOption>;
    return static_cast<ISocketIO::SocketOption>(static_cast<T>(a) | static_cast<T>(b));
}
constexpr ISocketIO::SocketOption operator&( ISocketIO::SocketOption a, ISocketIO::SocketOption b) noexcept
{
    using T = std::underlying_type_t<ISocketIO::SocketOption>;
    return static_cast<ISocketIO::SocketOption>(static_cast<T>(a) & static_cast<T>(b));
}
} // namespace job::net