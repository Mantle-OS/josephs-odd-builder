#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <job_io_async_thread.h>
#include "isocket_io.h"
#include "job_socket_error.h"
#include "resolve/job_ipaddr.h"
#include "jobnet_export.h"

namespace job::net {
class JOBNET_EXPORT UnixSocket : public ISocketIO {
public:
    using Ptr = std::shared_ptr<UnixSocket>;
    using ISocketIO::bind;

    struct PrivateTag {
    private:
        PrivateTag() = default;
        friend class UnixSocket;
    };

    UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop);
    UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop, int existing_fd, const std::string &peerPath);

    [[nodiscard]] static Ptr create(threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_shared<UnixSocket>(PrivateTag{}, std::move(loop));
    }
    [[nodiscard]] static Ptr create(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const std::string &peerPath)
    {
        return std::make_shared<UnixSocket>(PrivateTag{}, std::move(loop), existing_fd, peerPath);
    }

    ~UnixSocket() override;

    bool connectToHost(const JobIpAddr &ipaddr) override;
    bool connectToHost(const JobUrl &url) override;

    bool bind(const JobIpAddr &addr) override;
    bool bind(const JobUrl &url) override;

    bool listen(int backlog = 5) override;
    ISocketIO::Ptr accept() override;
    void disconnect() override;
    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;
    [[nodiscard]] ISocketIO::SocketState state() const noexcept override;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept override;
    std::string lastErrorString() const noexcept;
    [[nodiscard]] ISocketIO::SocketType type() const noexcept override;
    void setOption(SocketOption option, bool enable) override;
    [[nodiscard]] bool option(SocketOption option) const override;
    [[nodiscard]] std::string peerAddress() const override;
    [[nodiscard]] std::string localAddress() const override;
    // You don't matter and are ...... dumb mr unix socket..
    [[nodiscard]] uint16_t peerPort() const override;
    [[nodiscard]] uint16_t localPort() const override;
    void dumpState() const override;
    void updateLocalInfo();
    [[nodiscard]] bool isOpen() const noexcept;
    void triggerReadIfDataAvailable();
protected:
    void onEvents(threads::IOEvent events) override;
private:
    void closeSocket();
    void unlinkPath();
    std::string m_path;
    std::string m_peerPath;
    // True only for a socket that itself successfully bind()'d m_path
    bool m_ownsPath{false};
    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
};
} // namespace job::net