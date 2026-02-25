#pragma once

#include <memory>
#include <atomic>
#include <mutex>

#include "isocket_io.h"
#include "socket_error.h"

#include <io/job_io_async_thread.h>

namespace job::net {


class TcpSocket : public ISocketIO {
public:
    using Ptr = std::shared_ptr<TcpSocket>;
    explicit TcpSocket(threads::JobIoAsyncThread::Ptr loop);

    TcpSocket(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const JobIpAddr &peerAddr);

    ~TcpSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &address, uint16_t port) override;
    bool listen(int backlog = 5) override;
    ISocketIO::Ptr accept() override;
    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    ISocketIO::SocketState state() const noexcept override;
    SocketErrors::SocketErrNo lastError() const noexcept override;
    std::string lastErrorString() const noexcept;
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
    void updatePeerInfo();
    void updateLocalInfo();

    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};

    JobIpAddr m_peerAddr;
    JobIpAddr m_localAddr;

    std::mutex m_writeMutex;
};

} // namespace job::net

