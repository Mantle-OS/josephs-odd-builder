#include "sockets/job_socket_error.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

namespace job::net {

SocketErrors::SocketErrNo SocketErrors::fromErrno(int err) noexcept
{
    switch (err) {
    case 0:                     return SocketErrNo::None;
    case WSAEWOULDBLOCK:        return SocketErrNo::WouldBlock;
    case WSAETIMEDOUT:          return SocketErrNo::Timeout;
    case WSAEINTR:              return SocketErrNo::Interrupted;
    case WSA_NOT_ENOUGH_MEMORY: return SocketErrNo::NoMemory;
    case WSAEACCES:             return SocketErrNo::PermissionDenied;
    case WSAEINVAL:             return SocketErrNo::Invalid;
    case WSAENOTCONN:           return SocketErrNo::NotConnected;
    case WSAEISCONN:            return SocketErrNo::AlreadyConnected;
    case WSAEADDRINUSE:         return SocketErrNo::AddressInUse;
    case WSAEADDRNOTAVAIL:      return SocketErrNo::AddressNotAvailable;
    case WSAENETDOWN:           return SocketErrNo::NetworkDown;
    case WSAENETUNREACH:        return SocketErrNo::NetworkUnreachable;
    case WSAECONNRESET:         return SocketErrNo::ConnectionReset;
    case WSAECONNREFUSED:       return SocketErrNo::ConnectionRefused;
    case WSAECONNABORTED:       return SocketErrNo::ConnectionAborted;
    case WSAEMFILE:             return SocketErrNo::TooManyOpenFiles;
    case WSAESHUTDOWN:          return SocketErrNo::BrokenPipe;
    case WSAEHOSTUNREACH:       return SocketErrNo::HostUnreachable;
    case WSAHOST_NOT_FOUND:     return SocketErrNo::DNSFailure;
    case WSAEOPNOTSUPP:         return SocketErrNo::OperationNotSupported;
    default:                    return SocketErrNo::Unknown;
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
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(err), 0, buf, sizeof(buf), nullptr);

    if (len > 0) {
        m_lastErrorString = buf;
        while (!m_lastErrorString.empty() && (m_lastErrorString.back() == '\r' || m_lastErrorString.back() == '\n')) {
            m_lastErrorString.pop_back();
        }
    } else {
        m_lastErrorString = toString(m_lastError);
    }

    if (m_callback) {
        m_callback(m_lastError, m_lastErrorString);
    }
}

} // namespace job::net