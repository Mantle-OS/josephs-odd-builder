#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <sys/un.h>

#include <job_io_async_thread.h>

#include "isocket_io.h"
#include "socket_error.h"

namespace job::net {

class UnixSocket : public ISocketIO {
public:
    using UnixSocketPtr = std::shared_ptr<UnixSocket>;
    explicit UnixSocket(std::shared_ptr<threads::JobIoAsyncThread> loop);

    UnixSocket(std::shared_ptr<threads::JobIoAsyncThread> loop, int existing_fd, const std::string &peerPath);
    ~UnixSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &path, uint16_t port = 0) override;
    bool listen(int backlog = 5) override;
    std::shared_ptr<ISocketIO> accept() override;
    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    [[nodiscard]] ISocketIO::SocketState state() const noexcept override;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept override;
    std::string lastErrorString() const noexcept
    {
        return m_errors.lastErrorString();
    }
    [[nodiscard]] ISocketIO::SocketType type() const noexcept override;

    void setOption(SocketOption option, bool enable) override;
    [[nodiscard]] bool option(SocketOption option) const override;

    [[nodiscard]] std::string peerAddress() const override;
    [[nodiscard]] std::string localAddress() const override;

    // You don't matter and are ...... dumb mr unix socket..
    [[nodiscard]] uint16_t peerPort() const override;
    [[nodiscard]] uint16_t localPort() const override;
    // JK I love you you have held soild for so many years !

    void dumpState() const override;
    void updateLocalInfo();
    [[nodiscard]] bool isOpen() const noexcept;


    // std::function<void()> onWritable;
    // std::function<void(std::shared_ptr<ISocketIO>)> onAccept;
    void triggerReadIfDataAvailable() {
        if (m_fd < 0 || m_state.load() != SocketState::Connected)
            return;

        // Peek at the socket to see if data is available without consuming it
        char probe;
        ssize_t n = ::recv(m_fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);

        if (n > 0 && onRead) {
            // Data is available, trigger the callback
            onRead(nullptr, 0);
        }
    }

protected:
    void onEvents(uint32_t events) override;

private:
    void closeSocket();
    void unlinkPath();

    std::string m_path;
    std::string m_peerPath;
    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
};

} // namespace job::net
// CHECKPOINT: v2.3
