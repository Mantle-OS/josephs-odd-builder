#include "job_ipaddr.h"

#include <sstream>
#include <array>
#include <cctype>
#include <cstddef>
#include <vector>

#include <windows.h>

namespace job::net {

bool JobIpAddr::setAddress(const std::string &addr, uint16_t port)
{
    clear();

    // --- UNIX domain socket ---
    if (isUnixPath(addr)) {
        auto *un = reinterpret_cast<sockaddr_un *>(&m_storage);
        const size_t maxPathLen = sizeof(un->sun_path);

        if (addr.size() >= maxPathLen)
            return false;

        un->sun_family = AF_UNIX;
        std::memcpy(un->sun_path, addr.c_str(), addr.size() + 1);

        m_family = Family::Unix;

        m_len = static_cast<JobSockLen>(offsetof(sockaddr_un, sun_path) + addr.size() + 1);
        m_valid = true;
        return true;
    }

    // --- IPv4 ---
    if (isIPv4(addr)) {
        sockaddr_in ipv4Addr{};
        ipv4Addr.sin_family = AF_INET;
        ipv4Addr.sin_port = htons(port);

        if (::InetPtonA(AF_INET, addr.c_str(), &ipv4Addr.sin_addr) == 1) {
            std::memcpy(&m_storage, &ipv4Addr, sizeof(ipv4Addr));

            m_family = Family::IPv4;
            m_len = static_cast<JobSockLen>(sizeof(sockaddr_in));
            m_port = port;
            m_valid = true;
            return true;
        }
    }

    // --- IPv6 ---
    if (isIPv6(addr)) {
        sockaddr_in6 ipv6Addr{};
        ipv6Addr.sin6_family = AF_INET6;
        ipv6Addr.sin6_port = htons(port);

        if (::InetPtonA(AF_INET6, addr.c_str(), &ipv6Addr.sin6_addr) == 1) {
            std::memcpy(&m_storage, &ipv6Addr, sizeof(ipv6Addr));

            m_family = Family::IPv6;
            m_len = static_cast<JobSockLen>(sizeof(sockaddr_in6));
            m_port = port;
            m_valid = true;
            return true;
        }
    }

    m_family = Family::Unknown;
    m_valid = false;
    return false;
}

std::string JobIpAddr::toString(bool includePort) const
{
    std::array<char, INET6_ADDRSTRLEN> buf{};
    std::ostringstream oss;

    switch (m_family) {
    case Family::IPv4: {
        const auto *ipv4 = reinterpret_cast<const sockaddr_in *>(&m_storage);

        if (!::InetNtopA(
                AF_INET,
                const_cast<in_addr *>(&ipv4->sin_addr),
                buf.data(), static_cast<DWORD>(buf.size()))) {
            return "(invalid)";
        }

        oss << buf.data();
        if (includePort && ipv4->sin_port)
            oss << ':' << ntohs(ipv4->sin_port);
        break;
    }

    case Family::IPv6: {
        const auto *ipv6 = reinterpret_cast<const sockaddr_in6 *>(&m_storage);

        if (!::InetNtopA(
                AF_INET6,
                const_cast<in6_addr *>(&ipv6->sin6_addr),
                buf.data(), static_cast<DWORD>(buf.size()))) {
            return "(invalid)";
        }

        if (includePort && ipv6->sin6_port)
            oss << '[' << buf.data() << "]:" << ntohs(ipv6->sin6_port);
        else
            oss << buf.data();
        break;
    }

    case Family::Unix: {
        const auto *un = reinterpret_cast<const sockaddr_un *>(&m_storage);
        oss << un->sun_path;
        break;
    }

    default:
        oss << "(invalid)";
        break;
    }

    return oss.str();
}

bool JobIpAddr::isUnixPath(const std::string &path)
{
    if (path.empty())
        return false;

    if (path.length() >= 5 && path.compare(path.length() - 5, 5, ".sock") == 0)
        return true;

    if (path[0] == '/')
        return true;

    if (path[0] == '\\')
        return true;

    if (path.rfind("./", 0) == 0 || path.rfind(".\\", 0) == 0)
        return true;

    return path.size() >= 3 &&
           std::isalpha(static_cast<unsigned char>(path[0])) &&
           path[1] == ':' &&
           (path[2] == '\\' || path[2] == '/');
}

bool JobIpAddr::isLocal() const noexcept
{
    if (m_family == Family::Unix)
        return true;

    if (m_family == Family::IPv4) {
        const uint32_t addr = ntohl(reinterpret_cast<const sockaddr_in *>(&m_storage)->sin_addr.s_addr);
        return (addr >> 24) == 127; // 127.x.x.x
    }

    if (m_family == Family::IPv6) {
        const auto *addr = &reinterpret_cast<const sockaddr_in6 *>(&m_storage)->sin6_addr;
        return IN6_IS_ADDR_LOOPBACK(addr) || IN6_IS_ADDR_LINKLOCAL(addr);
    }

    return false;
}

bool JobIpAddr::isLoopback() const noexcept
{
    if (m_family == Family::IPv4)
        return ntohl(reinterpret_cast<const sockaddr_in *>(&m_storage)->sin_addr.s_addr ) == 0x7F000001;

    if (m_family == Family::IPv6)
        return IN6_IS_ADDR_LOOPBACK( &reinterpret_cast<const sockaddr_in6 *>(&m_storage)->sin6_addr);

    return false;
}

bool JobIpAddr::isMulticast() const noexcept
{
    if (m_family == Family::IPv4) {
        const uint32_t addr = ntohl(
            reinterpret_cast<const sockaddr_in *>(&m_storage)->sin_addr.s_addr
            );
        return (addr & 0xF0000000) == 0xE0000000;
    }

    if (m_family == Family::IPv6) {
        return IN6_IS_ADDR_MULTICAST(
            &reinterpret_cast<const sockaddr_in6 *>(&m_storage)->sin6_addr
            );
    }

    return false;
}

bool JobIpAddr::isNull() const noexcept
{
    if (m_family == Family::IPv4) {
        return reinterpret_cast<const sockaddr_in *>(&m_storage)->sin_addr.s_addr == INADDR_ANY;
    }

    if (m_family == Family::IPv6) {
        return IN6_IS_ADDR_UNSPECIFIED(
            &reinterpret_cast<const sockaddr_in6 *>(&m_storage)->sin6_addr
            );
    }

    return false;
}

bool JobIpAddr::isGlobal() const noexcept
{
    if (m_family == Family::IPv4) {
        const auto *saddr = reinterpret_cast<const sockaddr_in *>(&m_storage);
        const uint32_t addr = ntohl(saddr->sin_addr.s_addr);

        if (addr == 0)
            return false;

        const uint8_t first = static_cast<uint8_t>((addr >> 24) & 0xFF);
        const uint8_t second = static_cast<uint8_t>((addr >> 16) & 0xFF);

        if (first == 10)
            return false; // 10.0.0.0/8

        if (first == 127)
            return false; // 127.0.0.0/8

        if (first == 172 && second >= 16 && second <= 31)
            return false; // 172.16.0.0/12

        if (first == 192 && second == 168)
            return false; // 192.168.0.0/16

        if (first == 169 && second == 254)
            return false; // 169.254.0.0/16

        if (first >= 224)
            return false; // Multicast/Reserved

        return true;
    } else if (m_family == Family::IPv6) {
        const auto *addr = &reinterpret_cast<const sockaddr_in6 *>(&m_storage)->sin6_addr;

        if (IN6_IS_ADDR_UNSPECIFIED(addr) ||
            IN6_IS_ADDR_LOOPBACK(addr)    ||
            IN6_IS_ADDR_LINKLOCAL(addr)   ||
            IN6_IS_ADDR_MULTICAST(addr)) {
            return false;
        }

        return true;
    }

    return false;
}

bool JobIpAddr::isBroadcast() const noexcept
{
    if (m_family != Family::IPv4)
        return false;

    return reinterpret_cast<const sockaddr_in *>(&m_storage)->sin_addr.s_addr == INADDR_BROADCAST;
}

bool JobIpAddr::isUnixPermitted() const noexcept
{
    if (m_family != Family::Unix || !m_valid)
        return false;

    const auto *un = reinterpret_cast<const sockaddr_un *>(&m_storage);
    const char *path = un->sun_path;

    // Verify socket file existence using Windows File Attributes
    DWORD dwAttrib = GetFileAttributesA(path);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        return false; // File does not exist or access is denied

    return true;
}

bool JobIpAddr::isIPv4(const std::string &ip) noexcept
{
    in_addr addr4{};
    return ::InetPtonA(AF_INET, ip.c_str(), &addr4) == 1;
}

bool JobIpAddr::isIPv6(const std::string &ip) noexcept
{
    in6_addr addr6{};
    return ::InetPtonA(AF_INET6, ip.c_str(), &addr6) == 1;
}

bool JobIpAddr::fromSockAddr(const sockaddr *sa, JobSockLen len)
{
    if (!sa || len <= 0)
        return false;

    if (static_cast<std::size_t>(len) > sizeof(m_storage))
        return false;

    switch (sa->sa_family) {
    case AF_INET:
        if (len < static_cast<JobSockLen>(sizeof(sockaddr_in)))
            return false;
        break;

    case AF_INET6:
        if (len < static_cast<JobSockLen>(sizeof(sockaddr_in6)))
            return false;
        break;

    case AF_UNIX:
        if (len < static_cast<JobSockLen>(offsetof(sockaddr_un, sun_path)))
            return false;
        break;

    default:
        return false;
    }

    clear();

    std::memcpy(&m_storage, sa, static_cast<std::size_t>(len));

    m_len = len;
    m_valid = true;

    switch (sa->sa_family) {
    case AF_INET:
        m_family = Family::IPv4;
        m_port = ntohs(reinterpret_cast<const sockaddr_in *>(sa)->sin_port);
        break;

    case AF_INET6:
        m_family = Family::IPv6;
        m_port = ntohs(reinterpret_cast<const sockaddr_in6 *>(sa)->sin6_port);
        break;

    case AF_UNIX:
        m_family = Family::Unix;
        m_port = 0;
        break;

    default:
        clear();
        return false;
    }

    return true;
}

} // namespace job::net