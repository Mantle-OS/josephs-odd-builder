#pragma once

#include <string>
#include <functional>
#include <cstdint>

#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT SocketErrors {
public:
    enum class SocketErrNo : uint8_t {
        None = 0,
        WouldBlock,
        Timeout,
        Interrupted,
        Invalid,
        NoMemory,
        PermissionDenied,
        IOError,
        AddressInUse,
        AddressNotAvailable,
        NetworkDown,
        NetworkUnreachable,
        ConnectionReset,
        ConnectionRefused,
        ConnectionAborted,
        NotConnected,
        AlreadyConnected,
        TooManyOpenFiles,
        BrokenPipe,
        HostUnreachable,
        DNSFailure,
        OperationNotSupported,
        Unknown = 255
    };

    using ErrorCallback = std::function<void(SocketErrNo, const std::string&)>;

    SocketErrors() = default;
    ~SocketErrors() = default;

    [[nodiscard]] static SocketErrNo fromErrno(int err) noexcept;
    [[nodiscard]] static std::string toString(SocketErrNo code);

    void setError(int err);
    void recordError(SocketErrNo err) noexcept;
    void recordError(int err) noexcept;

    [[nodiscard]] SocketErrNo lastError() const noexcept { return m_lastError; }
    [[nodiscard]] std::string lastErrorString() const noexcept { return m_lastErrorString; }
    void onLastError(ErrorCallback cb) noexcept { m_callback = std::move(cb); }

private:
    SocketErrNo   m_lastError{SocketErrNo::None};
    std::string   m_lastErrorString{"No error"};
    ErrorCallback m_callback{nullptr};
};

} // namespace job::net