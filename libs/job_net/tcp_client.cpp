#include "tcp_client.h"
#include <job_logger.h>

namespace job::net {

TcpClient::TcpClient(const JobUrl &url, bool auto_connect) {
    if (auto_connect)
        if(connectTo(url, true)){
            //debug
        }
}

TcpClient::TcpClient(const JobIpAddr &address, bool auto_connect) {
    if (auto_connect)
        if(connectTo(address, true)){

        }
}

TcpClient::~TcpClient() {
    disconnect();
}

bool TcpClient::connectTo(const JobUrl &url, bool autoReconnect) {
    m_autoReconnect.store(autoReconnect, std::memory_order_relaxed);

    const std::string host = url.host();
    const uint16_t port = url.port();
    return connectTo(host, port, autoReconnect);
}

bool TcpClient::connectTo(const JobIpAddr &address, bool autoReconnect) {
    m_autoReconnect.store(autoReconnect, std::memory_order_relaxed);

    std::string host;
    uint16_t port = address.port();

    // Determine address string
    switch (address.family()) {
    case JobIpAddr::Family::IPv4:
    case JobIpAddr::Family::IPv6:
        host = address.toString(false);
        break;
    default:
        m_errors.setError(EINVAL);
        return false;
    }

    return connectTo(host, port, autoReconnect);
}

bool TcpClient::connectTo(const std::string &host, uint16_t port, bool autoReconnect) {
    if (isConnected())
        return true;

    m_host = host;
    m_port = port;
    m_autoReconnect.store(autoReconnect, std::memory_order_relaxed);

    JobUrl url(std::string("tcp://") + host + ":" + std::to_string(port));
    bool ret = TcpSocket::connectToHost(url);

    if (!ret) {
        m_connected.store(false, std::memory_order_relaxed);
        if (onConnectError)
            onConnectError(static_cast<int>(lastError()));

        if (m_autoReconnect.load())
            handleReconnect();
        return false;
    }

    m_connected.store(true, std::memory_order_relaxed);
    registerSocketEvents();

    if (onConnectSuccess)
        onConnectSuccess();

    return true;
}

void TcpClient::disconnect() noexcept {
    if (!isConnected())
        return;

    TcpSocket::disconnect();
    m_connected.store(false, std::memory_order_relaxed);

    if (onDisconnectEvent)
        onDisconnectEvent();

    if (m_autoReconnect.load())
        handleReconnect();
}

bool TcpClient::isConnected() const noexcept {
    return m_connected.load(std::memory_order_relaxed);
}

void TcpClient::setReconnectDelay(core::JobTimer::Duration delay) noexcept {
    m_reconnectDelay = delay;
}

core::JobTimer::Duration TcpClient::reconnectDelay() const noexcept {
    return m_reconnectDelay;
}

void TcpClient::handleReconnect() {
    if (m_host.empty() || m_port == 0)
        return;

    // Capture weak_ptr to avoid dangling shared_from_this()
    std::weak_ptr<TcpClient> weakSelf = std::dynamic_pointer_cast<TcpClient>(shared_from_this());

    // Launch lightweight thread for reconnection delay
    std::thread([weakSelf, delay = m_reconnectDelay]() {
        std::this_thread::sleep_for(delay);
        if (auto self = weakSelf.lock()) {
            if (!self->isConnected()) {
                // JobLogger::warn("TcpClient", "Attempting reconnect to {}:{}", self->m_host, self->m_port);
                self->connectTo(self->m_host, self->m_port, true);
            }
        }
    }).detach();
}

void TcpClient::registerSocketEvents() {
    onConnect = [this]() {
        m_connected.store(true, std::memory_order_relaxed);
        if (onConnectSuccess)
            onConnectSuccess();
    };

    onDisconnect = [this]() {
        m_connected.store(false, std::memory_order_relaxed);
        if (onDisconnectEvent)
            onDisconnectEvent();
        if (m_autoReconnect.load())
            handleReconnect();
    };

    onRead = [this](const char *data, size_t len) {
        if (onData)
            onData(data, len);
    };

    onError = [this](int err) {
        m_connected.store(false, std::memory_order_relaxed);
        if (onConnectError)
            onConnectError(err);
        if (m_autoReconnect.load())
            handleReconnect();
    };

    registerEvents();
}

} // namespace job::net
