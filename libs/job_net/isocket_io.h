#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include "async_event_loop.h"
#include "job_url.h"
#include "job_ipaddr.h"
#include "socket_error.h"

namespace job::net {

class ISocketIO : public std::enable_shared_from_this<ISocketIO> {

public:
    enum class SocketType : uint8_t {
        Unknown = 0,
        Tcp,
        Udp,
        Unix
    };

    enum class SocketState : uint8_t {
        Unconnected = 0,
        Connecting,
        Connected,
        Closing,
        Closed,
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
    explicit ISocketIO(std::shared_ptr<threads::AsyncEventLoop> loop = nullptr):
        m_loop{loop}
    {
    }
    virtual ~ISocketIO() = default;

    virtual bool connectToHost(const JobUrl &url) = 0;
    virtual bool bind(const JobIpAddr &addr) = 0;
    virtual bool bind(const std::string &address, uint16_t port) = 0;
    virtual bool listen(int backlog = 5) = 0;
    virtual std::shared_ptr<ISocketIO> accept() = 0;
    virtual void disconnect() = 0;

    virtual ssize_t read(void *buffer, size_t size) = 0;
    virtual ssize_t write(const void *buffer, size_t size) = 0;

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

    int fd() const noexcept
    {
        return m_fd;
    }

    void registerEvents()
    {
        if (auto loop = m_loop.lock()) {
            loop->post([self = shared_from_this()]{
                self->pollEvents();
            });
        }
    }
    void setLoop(const std::shared_ptr<threads::AsyncEventLoop> &loop)
    {
        m_loop = loop;
    }
    static constexpr bool hasFlag(ISocketIO::SocketOption value, ISocketIO::SocketOption flag) noexcept {
        using T = std::underlying_type_t<ISocketIO::SocketOption>;
        return (static_cast<T>(value) & static_cast<T>(flag)) != 0;
    }

    // Event callbacks (common to all socket types)
    std::function<void()> onConnect;
    std::function<void(const char*, size_t)> onRead;
    std::function<void()> onReady;

    std::function<void()> onDisconnect;
    std::function<void(int)> onError;

    std::atomic<bool> running{true};


protected:
    virtual void pollEvents() = 0;
    virtual void handleEvents(uint32_t events) = 0;

    std::weak_ptr<threads::AsyncEventLoop> m_loop;
    int m_fd{-1};
};

constexpr ISocketIO::SocketOption operator|(
    ISocketIO::SocketOption a,
    ISocketIO::SocketOption b) noexcept
{
    using T = std::underlying_type_t<ISocketIO::SocketOption>;
    return static_cast<ISocketIO::SocketOption>(
        static_cast<T>(a) | static_cast<T>(b)
        );
}

constexpr ISocketIO::SocketOption operator&(
    ISocketIO::SocketOption a,
    ISocketIO::SocketOption b) noexcept
{
    using T = std::underlying_type_t<ISocketIO::SocketOption>;
    return static_cast<ISocketIO::SocketOption>(
        static_cast<T>(a) & static_cast<T>(b)
        );
}

} // namespace job::net
