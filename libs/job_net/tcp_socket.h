#pragma once

#include <functional>
#include <memory>
#include <atomic>

#include <async_event_loop.h>

#include "isocket_io.h"

namespace job::net {

class TcpSocket : public ISocketIO, public std::enable_shared_from_this<TcpSocket>
{
public:
    using Ptr = std::shared_ptr<TcpSocket>;

    explicit TcpSocket(std::shared_ptr<threads::AsyncEventLoop> loop = nullptr);
    ~TcpSocket() override;

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &address, uint16_t port) override;
    bool listen(int backlog = 5) override;
    std::shared_ptr<ISocketIO> accept() override;
    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    [[nodiscard]] SocketState state() const noexcept override;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept override;

    [[nodiscard]] SocketType type() const noexcept override;

    void setOption(SocketOption option, bool enable) override;
    [[nodiscard]] bool option(SocketOption option) const override;

    [[nodiscard]] std::string peerAddress() const override;
    [[nodiscard]] uint16_t peerPort() const override;
    [[nodiscard]] std::string localAddress() const override;
    [[nodiscard]] uint16_t localPort() const override;

    void dumpState() const override;

    std::function<void()> onConnect;
    std::function<void(const char *data, size_t len)> onRead;
    std::function<void()> onDisconnect;
    std::function<void(int err)> onError;


    [[nodiscard]] int fd() const noexcept;



    void registerEvents();
protected:
    SocketErrors m_errors;
    std::weak_ptr<threads::AsyncEventLoop> m_loop;

private:
    void handleEvents(uint32_t events);
    void handleRead();
    void handleWrite();

    void postToLoop(std::function<void()> fn);

    int m_fd{-1};
    std::atomic<SocketState> m_state{SocketState::Unconnected};

    std::atomic<bool> m_nonBlocking{true};

    std::string m_peerAddr;
    uint16_t m_peerPort{0};
};

} // namespace job::net
