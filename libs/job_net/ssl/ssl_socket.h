#pragma once

#include "isocket_io.h"
#include "ssl/job_ssl_cert.h"
#include "ssl_error.h"

// Very very broken just boiler plate. There are other files that being developed
// JobSslCert etc This file is not used at all
// This could be inherit of a tcpSocket if need be.
namespace job::net::ssl {

class SSLSocket : public ISocketIO {
public:

    explicit SSLSocket(threads::JobIoAsyncThread::Ptr loop);
    // explicit SSLSocket(bool serverMode);
    SSLSocket(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const JobIpAddr &peerAddr);
    ~SSLSocket() override;

    enum class PeerVerifyMode { None, Query, VerifyPeer, Auto };
    enum class SslSocketMode { Unencrypted, SslClient, SslServer};

    bool connectToHost(const JobUrl &url) override;
    bool bind(const JobIpAddr &address) override;
    bool bind(const std::string &address, uint16_t port) override;

    bool listen(int backlog = 5) override;
    ISocketIO::Ptr accept() override;

    void disconnect() override;
    void closeSocket();

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
    void updateLocalInfo() ;

    void dumpState() const override;


    [[nodiscard]] bool isEncrypted() const;
    void setCertificate(JobSslCert::Ptr certificate);
    // void setLocalCertificate(const std::filesystem::path &path, CertType::EncodingFormat format = CertType::Pem);
    // void setCertificateChain(const std::forward_list<JobSslCert> &chain)

private:
    enum class HandshakeState : uint8_t {
        None,
        Handshaking,
        Ready
    };
    bool setupSSL();
    void performHandshake();
    void cleanup();
    void onEvents(uint32_t events) override;

    JobSslCert::Ptr             m_certContext; // Identity/Policy Wrapper
    SSL                         *m_ssl{nullptr};

    SocketErrors                m_errors;      // Standard Socket errors
    SslErrors                   m_sslErrors;   // OpenSSL specific errors

    std::string                 m_targetHost;
    JobIpAddr                   m_peerAddr;
    JobIpAddr                   m_localAddr;

    bool                        m_serverMode{false};
    std::atomic<SocketState>    m_state{SocketState::Unconnected};
    HandshakeState              m_handshakeState{HandshakeState::None};



    std::mutex                  m_writeMutex;

};

} // namespace job::net::ssl
// CHECKPOINT: v1
