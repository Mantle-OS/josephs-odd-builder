#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include "job_url.h"
#include "job_ipaddr.h"
#include "socket_error.h"

namespace job::net {

class ISocketIO {
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

    virtual ~ISocketIO() = default;

    virtual bool connectToHost(const JobUrl &url) = 0;
    virtual bool bind(const JobIpAddr &addr) = 0;
    virtual bool bind(const std::string &address, uint16_t port) = 0;
    virtual bool listen(int backlog = 5) = 0;
    virtual std::unique_ptr<ISocketIO> accept() = 0;
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
};

} // namespace job::net
