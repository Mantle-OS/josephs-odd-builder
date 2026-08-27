#include "ssl_socket.h"

#include <cerrno>
#include <utility>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

#include <job_logger.h>

namespace job::net {

struct SslSocket::Impl {
    SSL *ssl{nullptr};
};

SslSocket::SslSocket(PrivateTag, ISocketIO::Ptr socket, JobSslContext::Ptr context) :
    m_socket(std::move(socket)),
    m_context(std::move(context)),
    m_impl(std::make_unique<Impl>())
{
    m_errors.onLastError([this](JobSslError::SslErrNo error, const std::string &message) {
        if (onSslError)
            onSslError(error, message);
    });

    if (!m_socket) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL socket transport is invalid");
        return;
    }

    if (!m_context || !m_context->isValid()) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL context is invalid");
        return;
    }

    m_state.store(State::WaitingForTransport);
}

SslSocket::~SslSocket()
{
    /*
     * Destruction cannot wait for another readable or writable event to
     * complete an orderly TLS shutdown. Break the callback path first,
     * release the native SSL session, then close the transport directly.
     */
    detachSocketCallbacks();
    releaseSsl();

    if (m_socket)
        m_socket->disconnect();
}

bool SslSocket::setupSsl()
{
    if (!m_socket || m_socket->fd() < 0) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL transport descriptor is invalid");
        return false;
    }

    if (!m_context || !m_context->isValid()) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL context is invalid");
        return false;
    }

    if (m_impl->ssl)
        return true;

    auto *nativeContext = static_cast<SSL_CTX *>(m_context->nativeHandle());

    if (!nativeContext) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "Native SSL context is invalid");
        return false;
    }

    ::ERR_clear_error();

    m_impl->ssl = ::SSL_new(nativeContext);

    if (!m_impl->ssl) {
        recordSslError(JobSslError::SslErrNo::InternalError, "Failed to create SSL session");
        return false;
    }

    if (::SSL_set_fd(m_impl->ssl, m_socket->fd()) != 1) {
        recordSslError(JobSslError::SslErrNo::InternalError, "Failed to bind SSL session to transport descriptor");
        releaseSsl();
        return false;
    }

    if (m_context->mode() == JobSslContext::SslMode::Client) {
        ::SSL_set_connect_state(m_impl->ssl);

        if (!m_peerName.empty()) {
            if (::SSL_set_tlsext_host_name(m_impl->ssl, m_peerName.c_str()) != 1) {
                recordSslError(JobSslError::SslErrNo::InternalError, "Failed to configure TLS server name");
                releaseSsl();
                return false;
            }

            if (m_context->verifyMode() != JobSslContext::VerifyMode::None) {
                if (::SSL_set1_host(m_impl->ssl, m_peerName.c_str()) != 1) {
                    recordSslError(JobSslError::SslErrNo::InternalError, "Failed to configure TLS hostname verification");
                    releaseSsl();
                    return false;
                }
            }
        }
    } else {
        ::SSL_set_accept_state(m_impl->ssl);
    }

    return true;
}

bool SslSocket::driveHandshake()
{
    if (!m_impl || !m_impl->ssl) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL session is not initialized");
        return false;
    }

    ::ERR_clear_error();

    const int result = ::SSL_do_handshake(m_impl->ssl);

    if (result == 1) {
        if (!verifyPeer())
            return false;

        const threads::IOEvent events =
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered;

        if (!m_socket->setEvents(events)) {
            recordSslError(JobSslError::SslErrNo::InternalError, "Failed to update socket events after TLS handshake");
            return false;
        }

        m_state.store(State::Encrypted);

        if (onEncrypted)
            onEncrypted();

        return true;
    }

    return processSslError(result);
}



bool SslSocket::processSslError(int result)
{
    if (!m_impl || !m_impl->ssl) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL session is not initialized");
        return false;
    }

    const int nativeError = ::SSL_get_error(m_impl->ssl, result);

    switch (nativeError) {
    case SSL_ERROR_WANT_READ:
        m_errors.recordNativeError(nativeError);
        return updateEvents(JobSslError::SslErrNo::WantRead);

    case SSL_ERROR_WANT_WRITE:
        m_errors.recordNativeError(nativeError);
        return updateEvents(JobSslError::SslErrNo::WantWrite);

    case SSL_ERROR_WANT_CONNECT:
        m_errors.recordNativeError(nativeError);
        return updateEvents(JobSslError::SslErrNo::WantConnect);

    case SSL_ERROR_WANT_ACCEPT:
        m_errors.recordNativeError(nativeError);
        return updateEvents(JobSslError::SslErrNo::WantAccept);

    case SSL_ERROR_ZERO_RETURN:
        m_errors.recordNativeError(nativeError);

        if (tryBeginShutdown())
            shutdownSsl();

        return false;

    case SSL_ERROR_SYSCALL:
        m_errors.recordNativeError(nativeError);
        m_state.store(State::Error);

        if (onSocketError)
            onSocketError(errno);

        return false;

    case SSL_ERROR_SSL: {
        const long verifyResult = ::SSL_get_verify_result(m_impl->ssl);

        if (verifyResult != X509_V_OK) {
            recordSslError(
                JobSslError::SslErrNo::CertificateVerifyFailed,
                ::X509_verify_cert_error_string(verifyResult)
                );

            return false;
        }

        m_errors.recordNativeError(nativeError);
        m_state.store(State::Error);
        return false;
    }

    default:
        m_errors.recordNativeError(nativeError);
        m_state.store(State::Error);
        return false;
    }
}

bool SslSocket::verifyPeer()
{
    if (!m_impl || !m_impl->ssl || !m_context)
        return false;

    if (m_context->verifyMode() == JobSslContext::VerifyMode::None)
        return true;

    const long verifyResult = ::SSL_get_verify_result(m_impl->ssl);

    if (verifyResult != X509_V_OK) {
        recordSslError(JobSslError::SslErrNo::CertificateVerifyFailed, ::X509_verify_cert_error_string(verifyResult));
        return false;
    }

    X509 *peerCertificate = ::SSL_get1_peer_certificate(m_impl->ssl);

    if (!peerCertificate) {
        recordSslError(JobSslError::SslErrNo::CertificateVerifyFailed, "TLS peer did not provide a certificate");
        return false;
    }

    ::X509_free(peerCertificate);
    return true;
}

int64_t SslSocket::read(void *buffer, size_t size)
{
    if (!buffer || size == 0)
        return 0;

    if (state() != State::Encrypted || !m_impl || !m_impl->ssl) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL read requires an encrypted connection");
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_readMutex);

    /*
     * The state and SSL pointer are checked again after taking the lock.
     * Another thread may have initiated shutdown between the first check and
     * acquiring m_readMutex.
     */
    if (state() != State::Encrypted || !m_impl->ssl)
        return -1;

    ::ERR_clear_error();

    const int result = ::SSL_read(m_impl->ssl, buffer, static_cast<int>(size));

    if (result > 0)
        return result;

    if (processSslError(result))
        return 0;

    if (m_errors.lastError() == JobSslError::SslErrNo::ZeroReturn)
        return 0;

    return -1;
}

int64_t SslSocket::write(const void *buffer, size_t size)
{
    if (!buffer || size == 0)
        return 0;

    if (state() != State::Encrypted || !m_impl || !m_impl->ssl) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL write requires an encrypted connection");
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_writeMutex);

    if (state() != State::Encrypted || !m_impl->ssl)
        return -1;

    ::ERR_clear_error();

    const int result = ::SSL_write(m_impl->ssl, buffer, static_cast<int>(size));

    if (result > 0)
        return result;

    if (processSslError(result))
        return 0;

    return -1;
}

void SslSocket::disconnect()
{
    if (!tryBeginShutdown())
        return;

    if (!m_socket) {
        releaseSsl();
        m_state.store(State::Closed);
        return;
    }

    if (!m_impl || !m_impl->ssl) {
        releaseSsl();

        if (m_socket->state() == ISocketIO::SocketState::Unconnected) {
            m_state.store(State::Closed);
            return;
        }

        m_socket->disconnect();
        return;
    }

    shutdownSsl();
}
void SslSocket::shutdownSsl() noexcept
{
    if (state() != State::ShuttingDown)
        return;

    if (!m_impl || !m_impl->ssl) {
        if (m_socket)
            m_socket->disconnect();
        else
            m_state.store(State::Closed);

        return;
    }

    ::ERR_clear_error();

    const int result = ::SSL_shutdown(m_impl->ssl);

    if (result == 1) {
        releaseSsl();

        if (m_socket)
            m_socket->disconnect();
        else
            m_state.store(State::Closed);

        return;
    }

    /*
     * The first successful SSL_shutdown() call normally returns zero after
     * sending close_notify. Wait for the peer's close_notify and call
     * SSL_shutdown() again from the next readable event.
     */
    if (result == 0) {
        if (!updateEvents(JobSslError::SslErrNo::WantRead)) {
            releaseSsl();

            if (m_socket)
                m_socket->disconnect();
            else
                m_state.store(State::Closed);
        }

        return;
    }

    const int nativeError = ::SSL_get_error(m_impl->ssl, result);

    switch (nativeError) {
    case SSL_ERROR_WANT_READ:
        m_errors.recordNativeError(nativeError);

        if (!updateEvents(JobSslError::SslErrNo::WantRead)) {
            releaseSsl();

            if (m_socket)
                m_socket->disconnect();
            else
                m_state.store(State::Closed);
        }

        return;

    case SSL_ERROR_WANT_WRITE:
        m_errors.recordNativeError(nativeError);

        if (!updateEvents(JobSslError::SslErrNo::WantWrite)) {
            releaseSsl();

            if (m_socket)
                m_socket->disconnect();
            else
                m_state.store(State::Closed);
        }

        return;

    case SSL_ERROR_ZERO_RETURN:
        m_errors.recordNativeError(nativeError);
        break;

    default:
        m_errors.recordNativeError(nativeError);
        break;
    }

    /*
     * TLS shutdown failed or the peer has already closed. The transport still
     * needs to be closed, but the native SSL session is released exactly once.
     */
    releaseSsl();

    if (m_socket)
        m_socket->disconnect();
    else
        m_state.store(State::Closed);
}

void SslSocket::releaseSsl() noexcept
{
    if (!m_impl)
        return;

    /*
     * Clear the owned pointer before entering SSL_free(). If releaseSsl() is
     * reached again through a reentrant disconnect path, the second call sees
     * nullptr and becomes a no-op.
     */
    SSL *ssl = std::exchange(m_impl->ssl, nullptr);

    if (ssl)
        ::SSL_free(ssl);
}

} // namespace job::net