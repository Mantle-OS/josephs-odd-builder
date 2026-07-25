#include "job_ipaddr.h"

#include <sstream>
#include <array>
#include <cstddef>
#include <sys/stat.h>
#include <unistd.h>

namespace job::net {

bool JobIpAddr::setAddress(const std::string &addr, uint16_t port) {
    clear();

    // --- UNIX socket ---
    if (isUnixPath(addr)) {
        auto *un = reinterpret_cast<sockaddr_un*>(&m_storage);
        const size_t maxPathLen = sizeof(un->sun_path);

        if (addr.size() >= maxPathLen)
            return false; // Path exceeds sockaddr_un buffer limits

        m_family = Family::Unix;
        un->sun_family = AF_UNIX;

        // Use memcpy to safely support Linux abstract namespace sockets (starting with '\0')
        std::memcpy(un->sun_path, addr.data(), addr.size());

        // Null-terminate non-abstract sockets
        if (!addr.empty() && addr[0] != '\0' && addr.size() < maxPathLen)
            un->sun_path[addr.size()] = '\0';

        // Calculate exact structure length for kernel functions (bind/connect)
        if (!addr.empty() && addr[0] == '\0')
            m_len = static_cast<JobSockLen>(offsetof(sockaddr_un, sun_path) + addr.size());
        else
            m_len = static_cast<JobSockLen>(offsetof(sockaddr_un, sun_path) + addr.size() + 1);

        m_valid = true;
        return true;
    }

    // --- IPv4 ---
    if (isIPv4(addr)) {
        sockaddr_in ipv4_addr{};
        ipv4_addr.sin_family = AF_INET;
        ipv4_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, addr.c_str(), &ipv4_addr.sin_addr) == 1) {
            std::memcpy(&m_storage, &ipv4_addr, sizeof(sockaddr_in));
            m_family = Family::IPv4;
            m_len = sizeof(sockaddr_in);
            m_port = port;
            m_valid = true;
            return true;
        }
    }

    // --- IPv6 ---
    if (isIPv6(addr)) {
        sockaddr_in6 ipv6_addr{};
        ipv6_addr.sin6_family = AF_INET6;
        ipv6_addr.sin6_port = htons(port);
        if (inet_pton(AF_INET6, addr.c_str(), &ipv6_addr.sin6_addr) == 1) {
            std::memcpy(&m_storage, &ipv6_addr, sizeof(sockaddr_in6));
            m_family = Family::IPv6;
            m_len = sizeof(sockaddr_in6);
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
        auto *ipv4 = reinterpret_cast<const sockaddr_in*>(&m_storage);
        inet_ntop(AF_INET, &ipv4->sin_addr, buf.data(), buf.size());
        oss << buf.data();
        if (includePort && ipv4->sin_port)
            oss << ":" << ntohs(ipv4->sin_port);
        break;
    }
    case Family::IPv6: {
        auto *ipv6 = reinterpret_cast<const sockaddr_in6*>(&m_storage);
        inet_ntop(AF_INET6, &ipv6->sin6_addr, buf.data(), buf.size());
        if (includePort && ipv6->sin6_port)
            oss << "[" << buf.data() << "]:" << ntohs(ipv6->sin6_port);
        else
            oss << buf.data();
        break;
    }
    case Family::Unix: {
        const auto *un = reinterpret_cast<const sockaddr_un*>(&m_storage);

        // Print abstract sockets safely
        if (un->sun_path[0] == '\0')
            oss << "@" << (un->sun_path + 1);
        else
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

    if (path[0] == '\0')
        return true;

    if (path.length() >= 5 && path.compare(path.length() - 5, 5, ".sock") == 0)
        return true;

    if (path[0] == '/' || path.rfind("./", 0) == 0 || path.rfind("../", 0) == 0)
        return true;

    return false;
}

bool JobIpAddr::isLocal() const noexcept
{
    if (m_family == Family::Unix)
        return true;

    if (m_family == Family::IPv4) {
        auto addr = ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr);
        return (addr >> 24) == 127; // 127.x.x.x
    }

    if (m_family == Family::IPv6) {
        const auto *a6 = &reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr;
        return IN6_IS_ADDR_LOOPBACK(a6) || IN6_IS_ADDR_LINKLOCAL(a6);
    }

    return false;
}

bool JobIpAddr::isLoopback() const noexcept
{
    if (m_family == Family::IPv4)
        return ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr) == 0x7F000001;

    if (m_family == Family::IPv6)
        return IN6_IS_ADDR_LOOPBACK(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return false;
}

bool JobIpAddr::isMulticast() const noexcept
{
    if (m_family == Family::IPv4) {
        auto addr = ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr);
        return (addr & 0xF0000000) == 0xE0000000;
    }

    if (m_family == Family::IPv6)
        return IN6_IS_ADDR_MULTICAST(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return false;
}

bool JobIpAddr::isNull() const noexcept
{
    if (m_family == Family::IPv4)
        return reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr == INADDR_ANY;

    if (m_family == Family::IPv6)
        return IN6_IS_ADDR_UNSPECIFIED(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return false;
}

bool JobIpAddr::isGlobal() const noexcept
{
    if (m_family == Family::IPv4) {
        auto *saddr = reinterpret_cast<const sockaddr_in*>(&m_storage);
        if (!saddr)
            return false;

        uint32_t addr = ntohl(saddr->sin_addr.s_addr);

        if (addr == 0)
            return false;

        uint8_t first  = (addr >> 24) & 0xFF;
        uint8_t second = (addr >> 16) & 0xFF;

        if (first == 10)
            return false; // 10.0.0.0/8
        if (first == 127)
            return false; // 127.0.0.0/8
        if (first == 172 && (second >= 16 && second <= 31))
            return false; // 172.16.0.0/12
        if (first == 192 && second == 168)
            return false; // 192.168.0.0/16
        if (first == 169 && second == 254)
            return false; // 169.254.0.0/16
        if (first >= 224)
            return false; // Multicast / Reserved

        return true;
    }
    else if (m_family == Family::IPv6) {
        const auto *a6 = &reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr;
        return !(IN6_IS_ADDR_UNSPECIFIED(a6) ||
                 IN6_IS_ADDR_LOOPBACK(a6)    ||
                 IN6_IS_ADDR_LINKLOCAL(a6)   ||
                 IN6_IS_ADDR_MULTICAST(a6));
    }

    return false;
}

bool JobIpAddr::isBroadcast() const noexcept
{
    if (m_family != Family::IPv4)
        return false;
    return reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr == INADDR_BROADCAST;
}

bool JobIpAddr::isUnixPermitted() const noexcept
{
    if (m_family != Family::Unix || !m_valid)
        return false;

    const auto *un = reinterpret_cast<const sockaddr_un*>(&m_storage);
    const char *path = un->sun_path;

    // Abstract namespace sockets don't exist on the filesystem
    if (path[0] == '\0')
        return true;

    struct stat st{};
    if (::stat(path, &st) != 0)
        return false;

    if (!S_ISSOCK(st.st_mode))
        return false;

    if (::access(path, R_OK | W_OK) != 0)
        return false;

    return true;
}

bool JobIpAddr::isIPv4(const std::string &ip) noexcept
{
    in_addr addr4;
    return inet_pton(AF_INET, ip.c_str(), &addr4) == 1;
}

bool JobIpAddr::isIPv6(const std::string &ip) noexcept
{
    in6_addr addr6;
    return inet_pton(AF_INET6, ip.c_str(), &addr6) == 1;
}

bool JobIpAddr::fromSockAddr(const sockaddr *sa, job::net::JobSockLen len)
{
    if (!sa || len <= 0)
        return false;

    if (static_cast<std::size_t>(len) > sizeof(m_storage))
        return false;

    // Minimum size validation per family
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
        m_port = ntohs(reinterpret_cast<const sockaddr_in*>(sa)->sin_port);
        break;
    case AF_INET6:
        m_family = Family::IPv6;
        m_port = ntohs(reinterpret_cast<const sockaddr_in6*>(sa)->sin6_port);
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