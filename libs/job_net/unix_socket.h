#pragma once

#include "isocket_io.h"
#include "socket_error.h"

#include <sys/un.h>
#include <atomic>
#include <string>

namespace job::net {

class UnixSocket : public ISocketIO {
public:
    explicit UnixSocket(const std::shared_ptr<threads::AsyncEventLoop> &loop = nullptr);
    ~UnixSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &path, uint16_t port = 0) override;
    bool listen(int backlog = 5) override;
    std::shared_ptr<ISocketIO> accept() override;
    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    [[nodiscard]] SocketState state() const noexcept override;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept override;
    [[nodiscard]] SocketType type() const noexcept override { return SocketType::Unix; }

    void setOption(SocketOption option, bool enable) override;
    [[nodiscard]] bool option(SocketOption option) const override;

    [[nodiscard]] std::string peerAddress() const override;
    [[nodiscard]] std::string localAddress() const override;

    void dumpState() const override;
    uint16_t peerPort() const noexcept override { return 0; }
    uint16_t localPort() const noexcept override { return 0; }
protected:
    void pollEvents() override;
    void handleEvents(uint32_t events) override;

private:
    void closeSocket();
    void unlinkPath();

    std::string m_path;
    std::string m_peerPath;
    SocketErrors m_errors;
    std::atomic<SocketState> m_state{SocketState::Unconnected};
};

} // namespace job::net
