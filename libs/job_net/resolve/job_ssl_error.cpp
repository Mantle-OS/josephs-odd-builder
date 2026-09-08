#include "resolve/job_ssl_error.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace job::net {

std::string JobSslError::toString(SslErrNo code)
{
    switch (code) {
    case SslErrNo::None:                    return "No SSL error";
    case SslErrNo::WantRead:                return "SSL wants read operation";
    case SslErrNo::WantWrite:               return "SSL wants write operation";
    case SslErrNo::WantConnect:             return "SSL wants connect";
    case SslErrNo::WantAccept:              return "SSL wants accept";
    case SslErrNo::Syscall:                 return "System call error during SSL operation";
    case SslErrNo::ZeroReturn:              return "SSL connection closed cleanly";
    case SslErrNo::HandshakeFailed:         return "SSL handshake failed";
    case SslErrNo::CertificateVerifyFailed: return "Certificate verification failed";
    case SslErrNo::InvalidState:            return "Invalid SSL state transition";
    case SslErrNo::InternalError:           return "Internal SSL library error";
    case SslErrNo::OperationNotSupported:   return "Operation not supported";
    default:                                return "Unknown SSL error";
    }
}

void JobSslError::recordNativeError(int errCode) noexcept
{
    switch (errCode) {
    case SSL_ERROR_NONE:         m_lastError = SslErrNo::None; break;
    case SSL_ERROR_WANT_READ:    m_lastError = SslErrNo::WantRead; break;
    case SSL_ERROR_WANT_WRITE:   m_lastError = SslErrNo::WantWrite; break;
    case SSL_ERROR_WANT_CONNECT: m_lastError = SslErrNo::WantConnect; break;
    case SSL_ERROR_WANT_ACCEPT:  m_lastError = SslErrNo::WantAccept; break;
    case SSL_ERROR_SYSCALL:      m_lastError = SslErrNo::Syscall; break;
    case SSL_ERROR_ZERO_RETURN:  m_lastError = SslErrNo::ZeroReturn; break;
    case SSL_ERROR_SSL:          m_lastError = SslErrNo::HandshakeFailed; break;
    default:                     m_lastError = SslErrNo::Unknown; break;
    }
    unsigned long queued = ERR_get_error();
    if (queued != 0) {
        char buf[256]{};
        ERR_error_string_n(queued, buf, sizeof(buf));
        m_lastErrorString = buf;
    } else {
        m_lastErrorString = toString(m_lastError);
    }
    if (m_callback)
        m_callback(m_lastError, m_lastErrorString);
}
} // namespace job::net