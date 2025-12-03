#pragma once

#include "isocket_io.h"
#include "tcp_socket.h"
#include "ssl_error.h"

// Very very broken just boiler plate. There are other files that being developed
// JobSslCert etc This file is not used at all
namespace job::net::ssl {

class SSLSocket : public ISocketIO {
public:
    SSLSocket();
    explicit SSLSocket(bool serverMode);
    ~SSLSocket() override;

    enum class PeerVerifyMode { None, Query, VerifyPeer, Auto };
    enum class SslSocketMode { Unencrypted, SslClient, SslServer};

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &addr) override;
    bool bind(const std::string &address, uint16_t port) override;

    bool listen(int backlog = 5) override;
    std::unique_ptr<ISocketIO> accept() override;
    void disconnect() override;

    ssize_t read(void *buffer, size_t size) override;
    ssize_t write(const void *buffer, size_t size) override;

    SocketState state() const noexcept override;
    SocketErrors::SocketErrNo lastError() const noexcept override;
    SocketType type() const noexcept override { return SocketType::Tcp; }

    void setOption(SocketOption option, bool enable) override;
    bool option(SocketOption option) const override;

    std::string peerAddress() const override;
    uint16_t peerPort() const override;
    std::string localAddress() const override;
    uint16_t localPort() const override;

    void dumpState() const override;


    [[nodiscard]] bool isEncrypted() const;
    void setCertificate(const JobSslCert &certificate)
    void setLocalCertificate(const std::filesystem::path &path, CertType::EncodingFormat format = CertType::Pem)
    void setCertificateChain(const std::forward_list<JobSslCert> &chain)

private:
    bool initContext();
    bool performHandshake();
    void cleanup();

    SSL_CTX *m_ctx{nullptr};
    SSL *m_ssl{nullptr};
    std::unique_ptr<TcpSocket> m_tcp;
    SocketErrors m_errors;

    bool m_serverMode{false};
    std::atomic<SocketState> m_state{SocketState::Unconnected};
};

} // namespace job::net::ssl
// CHECKPOINT: v1
