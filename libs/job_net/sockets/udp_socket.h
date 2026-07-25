#pragma once
#include <atomic>
#include <memory>
#include <job_io_async_thread.h>
#include "job_socket_error.h"
#include "isocket_io.h"
#include "job_url.h"
#include "resolve/job_ipaddr.h"
namespace job::net {
class JOBNET_EXPORT UdpSocket : public ISocketIO {
public:
    using Ptr = std::shared_ptr<UdpSocket>;
    using ISocketIO::connectToHost;
    using ISocketIO::bind;

    struct PrivateTag {
    private:
        PrivateTag() = default;
        friend class UdpSocket;
    };

    UdpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop);

    [[nodiscard]] static Ptr create(threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_shared<UdpSocket>(PrivateTag{}, std::move(loop));
    }

    ~UdpSocket() override;

    bool connectToHost(const JobIpAddr &ipaddr) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const JobUrl &url) override; // UdpSocket's own call: numeric-host-only, no DNS (see .cpp)

    bool listen([[maybe_unused]] int backlog = 0) override;
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
    void onEvents(job::threads::IOEvent events) override;
private:
    void closeSocket();
    void updateLocalInfo();
    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
    JobIpAddr m_boundAddr;
    JobIpAddr m_peerAddr;
};
} // namespace job::net