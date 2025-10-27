#pragma once

#include <atomic>

#include "isocket_io.h"
#include "job_url.h"
#include "job_ipaddr.h"
#include "socket_error.h"

namespace job::net {

class UdpSocket : public ISocketIO {
public:
    UdpSocket();
    ~UdpSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &address, uint16_t port) override;
    bool listen([[maybe_unused]]int backlog = 0) override
    {
        return false;
    }
    std::unique_ptr<ISocketIO> accept() override
    {
        return nullptr;
    }

    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    // Datagram variants
    ssize_t sendTo(const void *buffer, size_t size, const JobIpAddr &dest);
    ssize_t recvFrom(void *buffer, size_t size, JobIpAddr &sender);

    SocketState state() const noexcept override;
    SocketErrors::SocketErrNo lastError() const noexcept override;
    SocketType type() const noexcept override
    {
        return SocketType::Udp;
    }

    void setOption(SocketOption option, bool enable) override;
    bool option(SocketOption option) const override;

    std::string peerAddress() const override;
    uint16_t peerPort() const override;
    std::string localAddress() const override;
    uint16_t localPort() const override;

    void dumpState() const override;

    [[nodiscard]] int fd() const noexcept
    {
        return m_fd;
    }

private:
    void closeSocket();
    int m_fd{-1};
    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
    JobIpAddr m_boundAddr;
    JobIpAddr m_peerAddr;
};

} // namespace job::net
