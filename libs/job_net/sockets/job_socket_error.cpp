#include "sockets/job_socket_error.h"

#include <cerrno>
#include <cstring>

namespace job::net {

SocketErrors::SocketErrNo SocketErrors::fromErrno(int err) noexcept
{
    switch (err) {
    case 0:             return SocketErrNo::None;
    case EAGAIN:        return SocketErrNo::WouldBlock;
    case ETIMEDOUT:     return SocketErrNo::Timeout;
    case EINTR:         return SocketErrNo::Interrupted;
    case ENOMEM:        return SocketErrNo::NoMemory;
    case EACCES:        return SocketErrNo::PermissionDenied;
    case EFAULT:
    case EINVAL:        return SocketErrNo::Invalid;
    case ENOTCONN:      return SocketErrNo::NotConnected;
    case EISCONN:       return SocketErrNo::AlreadyConnected;
    case EADDRINUSE:    return SocketErrNo::AddressInUse;
    case EADDRNOTAVAIL: return SocketErrNo::AddressNotAvailable;
    case ENETDOWN:      return SocketErrNo::NetworkDown;
    case ENETUNREACH:   return SocketErrNo::NetworkUnreachable;
    case ECONNRESET:    return SocketErrNo::ConnectionReset;
    case ECONNREFUSED:  return SocketErrNo::ConnectionRefused;
    case ECONNABORTED:  return SocketErrNo::ConnectionAborted;
    case EMFILE:        return SocketErrNo::TooManyOpenFiles;
    case EPIPE:         return SocketErrNo::BrokenPipe;
    case EHOSTUNREACH:  return SocketErrNo::HostUnreachable;
#ifdef EAI_FAIL
    case EAI_FAIL:      return SocketErrNo::DNSFailure;
#endif
    case EOPNOTSUPP:    return SocketErrNo::OperationNotSupported;
    default:            return SocketErrNo::Unknown;
    }
}

std::string SocketErrors::toString(SocketErrNo code)
{
    switch (code) {
    case SocketErrNo::None:                 return "No error";
    case SocketErrNo::WouldBlock:           return "Operation would block";
    case SocketErrNo::Timeout:              return "Operation timed out";
    case SocketErrNo::Interrupted:          return "Interrupted system call";
    case SocketErrNo::Invalid:              return "Invalid argument";
    case SocketErrNo::NoMemory:             return "Out of memory";
    case SocketErrNo::PermissionDenied:     return "Permission denied";
    case SocketErrNo::IOError:              return "I/O error";
    case SocketErrNo::AddressInUse:         return "Address already in use";
    case SocketErrNo::AddressNotAvailable:  return "Address not available";
    case SocketErrNo::NetworkDown:          return "Network is down";
    case SocketErrNo::NetworkUnreachable:   return "Network unreachable";
    case SocketErrNo::ConnectionReset:      return "Connection reset by peer";
    case SocketErrNo::ConnectionRefused:    return "Connection refused";
    case SocketErrNo::ConnectionAborted:    return "Connection aborted";
    case SocketErrNo::NotConnected:         return "Socket not connected";
    case SocketErrNo::AlreadyConnected:     return "Socket already connected";
    case SocketErrNo::TooManyOpenFiles:     return "Too many open files";
    case SocketErrNo::BrokenPipe:           return "Broken pipe";
    case SocketErrNo::HostUnreachable:      return "Host unreachable";
    case SocketErrNo::DNSFailure:           return "DNS resolution failed";
    case SocketErrNo::OperationNotSupported:return "Operation not supported";
    default:                                return "Unknown socket error";
    }
}

void SocketErrors::setError(int err)
{
    recordError(err);
}

void SocketErrors::recordError(SocketErrNo err) noexcept
{
    recordError(static_cast<int>(err));
}

void SocketErrors::recordError(int err) noexcept
{
    m_lastError = fromErrno(err);

    char buf[256]{};
    const char *msg = strerror_r(err, buf, sizeof(buf));
    m_lastErrorString = msg ? msg : toString(m_lastError);

    if (m_callback)
        m_callback(m_lastError, m_lastErrorString);
}

} // namespace job::net