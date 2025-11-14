#pragma once

#include <cstring>
#include <string>
#include <functional>
#include <cerrno>

namespace job::net {

class SocketErrors {
public:
    enum class SocketErrNo {
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

    static SocketErrNo fromErrno(int err) noexcept
    {
        switch (err) {
        case 0: return SocketErrNo::None;
        case EAGAIN:
// #ifdef EWOULDBLOCK
//         case EWOULDBLOCK:
// #endif
            return SocketErrNo::WouldBlock;
        case ETIMEDOUT:
            return SocketErrNo::Timeout;
        case EINTR:
            return SocketErrNo::Interrupted;
        case ENOMEM:
            return SocketErrNo::NoMemory;
        case EACCES:
            return SocketErrNo::PermissionDenied;
        case EFAULT:
            return SocketErrNo::Invalid;
        case EINVAL:
            return SocketErrNo::Invalid;
        case ENOTCONN:
            return SocketErrNo::NotConnected;
        case EISCONN:
            return SocketErrNo::AlreadyConnected;
        case EADDRINUSE:
            return SocketErrNo::AddressInUse;
        case EADDRNOTAVAIL:
            return SocketErrNo::AddressNotAvailable;
        case ENETDOWN:
            return SocketErrNo::NetworkDown;
        case ENETUNREACH:
            return SocketErrNo::NetworkUnreachable;
        case ECONNRESET:
            return SocketErrNo::ConnectionReset;
        case ECONNREFUSED:
            return SocketErrNo::ConnectionRefused;
        case ECONNABORTED:
            return SocketErrNo::ConnectionAborted;
        case EMFILE:
            return SocketErrNo::TooManyOpenFiles;
        case EPIPE:
            return SocketErrNo::BrokenPipe;
        case EHOSTUNREACH:
            return SocketErrNo::HostUnreachable;
#ifdef EAI_FAIL
        case EAI_FAIL: return SocketErrNo::DNSFailure;
#endif
        case EOPNOTSUPP:
            return SocketErrNo::OperationNotSupported;
        default:
            return SocketErrNo::Unknown;
        }
    }

    static std::string toString(SocketErrNo code)
    {
        switch (code) {
        case SocketErrNo::None:
            return "No error";
        case SocketErrNo::WouldBlock:
            return "Operation would block";
        case SocketErrNo::Timeout:
            return "Operation timed out";
        case SocketErrNo::Interrupted:
            return "Interrupted system call";
        case SocketErrNo::Invalid:
            return "Invalid argument";
        case SocketErrNo::NoMemory:
            return "Out of memory";
        case SocketErrNo::PermissionDenied:
            return "Permission denied";
        case SocketErrNo::IOError:
            return "I/O error";
        case SocketErrNo::AddressInUse:
            return "Address already in use";
        case SocketErrNo::AddressNotAvailable:
            return "Address not available";
        case SocketErrNo::NetworkDown:
            return "Network is down";
        case SocketErrNo::NetworkUnreachable:
            return "Network unreachable";
        case SocketErrNo::ConnectionReset:
            return "Connection reset by peer";
        case SocketErrNo::ConnectionRefused:
            return "Connection refused";
        case SocketErrNo::ConnectionAborted:
            return "Connection aborted";
        case SocketErrNo::NotConnected:
            return "Socket not connected";
        case SocketErrNo::AlreadyConnected:
            return "Socket already connected";
        case SocketErrNo::TooManyOpenFiles:
            return "Too many open files";
        case SocketErrNo::BrokenPipe:
            return "Broken pipe";
        case SocketErrNo::HostUnreachable:
            return "Host unreachable";
        case SocketErrNo::DNSFailure:
            return "DNS resolution failed";
        case SocketErrNo::OperationNotSupported:
            return "Operation not supported";
        default:
            return "Unknown socket error";
        }
    }

    void setError(int err)
    {
        recordError(err);

    }

    void inline recordError(int err) noexcept
    {
        m_lastError = fromErrno(err);

#ifdef _GNU_SOURCE
        char buf[128];
        if (strerror_r(err, buf, sizeof(buf)) == 0)
            m_lastErrorString = buf;
        else
            m_lastErrorString = "Unknown";
#else
        const char *msg = std::strerror(err);
        m_lastErrorString = msg ? msg : "Unknown";
#endif

        if (m_callback)
            m_callback(m_lastError, m_lastErrorString);
    }

    SocketErrNo lastError() const noexcept
    {
        return m_lastError;
    }

    std::string lastErrorString() const noexcept
    {
        return m_lastErrorString;
    }

    void onLastError(ErrorCallback cb) noexcept
    {
        m_callback = std::move(cb);
    }

private:
    SocketErrNo m_lastError{SocketErrNo::Unknown};
    std::string m_lastErrorString{"Unknown"};
    ErrorCallback m_callback = nullptr;
};

} // namespace job::net
// CHECKPOINT: v1
