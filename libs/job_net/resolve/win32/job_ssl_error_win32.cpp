#include "resolve/job_ssl_error.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <security.h>
#include <schannel.h>
namespace job::net {
std::string JobSslError::toString(SslErrNo code)
{
    switch (code) {
    case SslErrNo::None:                    return "No SSL error";
    case SslErrNo::WantRead:                return "Schannel requires more network data";
    case SslErrNo::WantWrite:               return "Schannel buffer pending write";
    case SslErrNo::Syscall:                 return "System call error during Schannel operation";
    case SslErrNo::ZeroReturn:              return "Schannel connection closed cleanly";
    case SslErrNo::HandshakeFailed:         return "Schannel handshake failed";
    case SslErrNo::CertificateVerifyFailed: return "Schannel certificate verification failed";
    case SslErrNo::InvalidState:            return "Invalid SSL state transition";
    case SslErrNo::InternalError:           return "Internal SSL library error";
    case SslErrNo::OperationNotSupported:   return "Operation not supported";
    default:                                return "Unknown Schannel error";
    }
}
void JobSslError::recordNativeError(int errCode) noexcept
{
    SECURITY_STATUS secStatus = static_cast<SECURITY_STATUS>(errCode);
    switch (secStatus) {
    case SEC_E_OK:
        m_lastError = SslErrNo::None;
        break;
    case SEC_I_CONTINUE_NEEDED:
    case SEC_E_INCOMPLETE_MESSAGE:
        m_lastError = SslErrNo::WantRead;
        break;
    case SEC_E_WRONG_PRINCIPAL:
    case SEC_E_UNTRUSTED_ROOT:
    case SEC_E_CERT_EXPIRED:
        m_lastError = SslErrNo::CertificateVerifyFailed;
        break;
    case SEC_E_INTERNAL_ERROR:
        m_lastError = SslErrNo::InternalError;
        break;
    default:
        m_lastError = SslErrNo::HandshakeFailed;
        break;
    }
    char buf[256]{};
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(errCode), 0, buf, sizeof(buf), nullptr);
    if (len > 0) {
        m_lastErrorString = buf;
        while (!m_lastErrorString.empty() && (m_lastErrorString.back() == '\r' || m_lastErrorString.back() == '\n'))
            m_lastErrorString.pop_back();
    } else {
        m_lastErrorString = toString(m_lastError);
    }
    if (m_callback)
        m_callback(m_lastError, m_lastErrorString);
}

} // namespace job::net