#pragma once

#include <string>
#include <functional>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace job::net::ssl {

class SslErrors {
public:
    enum class SslErrNo {
        None = 0,
        WantRead,
        WantWrite,
        WantConnect,
        WantAccept,
        Syscall,
        ZeroReturn,
        HandshakeFailed,
        CertificateVerifyFailed,
        InvalidState,
        InternalError,
        Unknown = 255
    };

    using ErrorCallback = std::function<void(SslErrNo, const std::string&)>;

    SslErrors() = default;

    static SslErrNo fromSSLError(int sslError) noexcept
    {
        switch (sslError) {
        case SSL_ERROR_NONE:
            return SslErrNo::None;
        case SSL_ERROR_WANT_READ:
            return SslErrNo::WantRead;
        case SSL_ERROR_WANT_WRITE:
            return SslErrNo::WantWrite;
        case SSL_ERROR_WANT_CONNECT:
            return SslErrNo::WantConnect;
        case SSL_ERROR_WANT_ACCEPT:
            return SslErrNo::WantAccept;
        case SSL_ERROR_SYSCALL:
            return SslErrNo::Syscall;
        case SSL_ERROR_ZERO_RETURN:
            return SslErrNo::ZeroReturn;
        case SSL_ERROR_SSL:
            return SslErrNo::HandshakeFailed;
        default:
            return SslErrNo::Unknown;
        }
    }

    static SslErrNo fromQueuedError(unsigned long code) noexcept
    {
        if (code == 0)
            return SslErrNo::None;

        const unsigned int lib = ERR_GET_LIB(code);
        const unsigned int reason = ERR_GET_REASON(code);

        if (lib == ERR_LIB_SSL) {
            switch (reason) {
            case SSL_R_CERTIFICATE_VERIFY_FAILED:
                return SslErrNo::CertificateVerifyFailed;
            case SSL_R_WRONG_VERSION_NUMBER:
            case SSL_R_NO_PROTOCOLS_AVAILABLE:
            case SSL_R_NO_CIPHERS_AVAILABLE:
                return SslErrNo::HandshakeFailed;
            default:
                return SslErrNo::InternalError;
            }
        }

        if (lib == ERR_LIB_SYS)
            return SslErrNo::Syscall;

        return SslErrNo::Unknown;
    }

    static std::string toString(SslErrNo code)
    {
        switch (code) {
        case SslErrNo::None:
            return "No SSL error";
        case SslErrNo::WantRead:
            return "SSL wants to read more data";
        case SslErrNo::WantWrite:
            return "SSL wants to write more data";
        case SslErrNo::WantConnect:
            return "SSL wants to connect";
        case SslErrNo::WantAccept:
            return "SSL wants to accept connection";
        case SslErrNo::Syscall:
            return "System call failure during SSL operation";
        case SslErrNo::ZeroReturn:
            return "SSL connection closed cleanly";
        case SslErrNo::HandshakeFailed:
            return "SSL handshake failed";
        case SslErrNo::CertificateVerifyFailed:
            return "Certificate verification failed";
        case SslErrNo::InvalidState:
            return "Invalid SSL state transition";
        case SslErrNo::InternalError:
            return "Internal SSL library error";
        case SslErrNo::Unknown: default:
            return "Unknown SSL error";
        }
    }

    void recordError(int sslError, [[maybe_unused]]SSL *ssl = nullptr)
    {
        m_lastError = fromSSLError(sslError);

        if (m_lastError == SslErrNo::HandshakeFailed ||
            m_lastError == SslErrNo::Syscall         ||
            m_lastError == SslErrNo::InternalError)
        {
            unsigned long queued = ERR_get_error();
            if (queued != 0) {
                m_lastError = fromQueuedError(queued);

                char buf[256];
                ERR_error_string_n(queued, buf, sizeof(buf));
                m_lastErrorString = buf;
            }
        } else {
            m_lastErrorString = toString(m_lastError);
        }

        if (m_callback)
            m_callback(m_lastError, m_lastErrorString);
    }

    [[nodiscard]] SslErrNo lastError() const noexcept
    {
        return m_lastError;
    }

    [[nodiscard]] const std::string &lastErrorString() const noexcept
    {
        return m_lastErrorString;
    }

    void onLastError(ErrorCallback cb) noexcept
    {
        m_callback = std::move(cb);
    }

private:
    SslErrNo m_lastError{SslErrNo::Unknown};
    std::string m_lastErrorString{"Unknown"};
    ErrorCallback m_callback;
};

} // namespace job::net::ssl

