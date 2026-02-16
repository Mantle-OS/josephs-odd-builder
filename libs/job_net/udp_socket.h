#pragma once

#include <atomic>
#include <memory>

#include <job_io_async_thread.h>

#include "socket_error.h"
#include "isocket_io.h"
#include "job_url.h"
#include "job_ipaddr.h"

namespace job::net {

class UdpSocket : public ISocketIO {

public:
    using Ptr = std::shared_ptr<UdpSocket>;
    explicit UdpSocket(threads::JobIoAsyncThread::Ptr loop);
    ~UdpSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &address, uint16_t port) override;

    bool listen([[maybe_unused]]int backlog = 0) override;
    ISocketIO::Ptr accept() override;

    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    ssize_t sendTo(const void *buffer, size_t size, const JobIpAddr &dest);
    ssize_t recvFrom(void *buffer, size_t size, JobIpAddr &sender);

    ISocketIO::SocketState state() const noexcept override;
    SocketErrors::SocketErrNo lastError() const noexcept override;
    std::string lastErrorString() const noexcept
    {
        return m_errors.lastErrorString();
    }
    ISocketIO::SocketType type() const noexcept override;

    void setOption(SocketOption option, bool enable) override;
    bool option(SocketOption option) const override;

    std::string peerAddress() const override;
    uint16_t peerPort() const override;
    std::string localAddress() const override;
    uint16_t localPort() const override;

    void dumpState() const override;
    [[nodiscard]] bool isOpen() const noexcept;

protected:
    void onEvents(uint32_t events) override;

private:
    void closeSocket();
    void updateLocalInfo();

    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
    JobIpAddr m_boundAddr;
    JobIpAddr m_peerAddr;
};

} // namespace job::net
// CHECKPOINT: v2.1
