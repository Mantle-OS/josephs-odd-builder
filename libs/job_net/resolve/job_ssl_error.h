#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include "jobnet_export.h"
namespace job::net {
class JOBNET_EXPORT JobSslError {
public:
    enum class SslErrNo : uint8_t {
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
        OperationNotSupported,
        Unknown = 255
    };
    using ErrorCallback = std::function<void(SslErrNo, const std::string&)>;
    JobSslError() = default;
    ~JobSslError() = default;
    [[nodiscard]] static std::string toString(SslErrNo code);

    // Backend-internal only: errCode is in the native error domain for whichever
    // TLS engine is compiled in (OpenSSL's SSL_get_error() on Linux, SECURITY_STATUS
    // on Windows). Portable code (ssl_socket.h and friends) must never call this
    // directly, since it can't know which domain "int" means without an #ifdef —
    // use recordError(SslErrNo, message) instead.
    void recordNativeError(int errCode) noexcept;

    void recordError(SslErrNo err, const std::string &message = {}) noexcept
    {
        m_lastError = err;
        m_lastErrorString = message.empty() ? toString(err) : message;
        if (m_callback)
            m_callback(m_lastError, m_lastErrorString);
    }

    [[nodiscard]] SslErrNo lastError() const noexcept { return m_lastError; }
    [[nodiscard]] std::string lastErrorString() const noexcept { return m_lastErrorString; }
    void onLastError(ErrorCallback cb) noexcept { m_callback = std::move(cb); }
private:
    SslErrNo m_lastError{SslErrNo::None};
    std::string m_lastErrorString{"No SSL error"};
    ErrorCallback m_callback{nullptr};
};
} // namespace job::net