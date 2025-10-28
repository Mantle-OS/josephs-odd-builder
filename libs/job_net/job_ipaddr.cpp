#include "job_ipaddr.h"

#include <sstream>
#include <cstring>
#include <regex>

#include <netdb.h>
#include <unistd.h>
namespace job::net {

namespace {
    constexpr std::string_view kIPv4Regex = R"(^(\d{1,3}\.){3}\d{1,3}$)";
    constexpr std::string_view kIPv6Regex = R"(^([0-9A-Fa-f]{0,4}:){1,7}[0-9A-Fa-f]{0,4}$)";
    const std::regex kIPv4Pattern(kIPv4Regex.data());
    const std::regex kIPv6Pattern(kIPv6Regex.data());
} // namespace


JobIpAddr::JobIpAddr() = default;
JobIpAddr::JobIpAddr(const std::string &addr, uint16_t port) { setAddress(addr, port); }
JobIpAddr::JobIpAddr(const JobIpAddr &other) = default;
JobIpAddr &JobIpAddr::operator=(const JobIpAddr &other) = default;

void JobIpAddr::clear() noexcept {
    std::memset(&m_storage, 0, sizeof(m_storage));
    m_len = 0;
    m_port = 0;
    m_valid = false;
    m_family = Family::Unknown;
}


bool JobIpAddr::setAddress(const std::string &addr, uint16_t port) {
    clear();

    // --- UNIX socket ---
    if (isUnixPath(addr)) {
        m_family = Family::Unix;
        sockaddr_un *un = reinterpret_cast<sockaddr_un*>(&m_storage);
        un->sun_family = AF_UNIX;
        std::snprintf(un->sun_path, sizeof(un->sun_path), "%s", addr.c_str());
        m_len = sizeof(sockaddr_un);
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


bool JobIpAddr::isValid() const noexcept
{
    return m_valid;
}

JobIpAddr::Family JobIpAddr::family() const noexcept
{
    return m_family;
}
uint16_t JobIpAddr::port() const noexcept
{
    return m_port;
}

std::string JobIpAddr::toString(bool includePort) const
{
    char buf[INET6_ADDRSTRLEN] = {0};
    std::ostringstream oss;

    switch (m_family) {
    case Family::IPv4: {
        auto *ipv4 = reinterpret_cast<const sockaddr_in*>(&m_storage);
        inet_ntop(AF_INET, &ipv4->sin_addr, buf, sizeof(buf));
        oss << buf;
        if (includePort && ipv4->sin_port)
            oss << ":" << ntohs(ipv4->sin_port);
        break;
    }
    case Family::IPv6: {
        auto *ipv6 = reinterpret_cast<const sockaddr_in6*>(&m_storage);
        inet_ntop(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf));
        if (includePort && ipv6->sin6_port)
            oss << "[" << buf << "]:" << ntohs(ipv6->sin6_port);
        else
            oss << buf;
        break;
    }
    case Family::Unix: {
        const auto *un = reinterpret_cast<const sockaddr_un*>(&m_storage);
        oss << un->sun_path;
        break;
    }
    default:
        oss << "(invalid)";
        break;
    }

    return oss.str();
}

const sockaddr *JobIpAddr::sockAddr() const noexcept
{
    return reinterpret_cast<const sockaddr*>(&m_storage);
}

socklen_t JobIpAddr::sockAddrLen() const noexcept
{
    return m_len;
}


bool JobIpAddr::isIPv4(const std::string &ip)
{
    return std::regex_match(ip, kIPv4Pattern);
}

bool JobIpAddr::isIPv6(const std::string &ip)
{
    return std::regex_match(ip, kIPv6Pattern);
}

bool JobIpAddr::isUnixPath(const std::string &path) {
    return !path.empty() && path[0] == '/';
}


bool JobIpAddr::isLocal() const noexcept
{
    if (m_family == Family::Unix)
        return true;

    if (m_family == Family::IPv4) {
        auto addr = ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr);
        // 127.x.x.x
        return (addr >> 24) == 127;
    }

    if (m_family == Family::IPv6) {
        const auto *a6 = &reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr;
        return IN6_IS_ADDR_LOOPBACK(a6) || IN6_IS_ADDR_LINKLOCAL(a6);
    }

    return false;
}

bool JobIpAddr::isLoopback() const noexcept
{
    bool ret = false;

    if (m_family == Family::IPv4)
        ret = ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr) == 0x7F000001;

    if (m_family == Family::IPv6)
        ret = IN6_IS_ADDR_LOOPBACK(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return ret;
}

bool JobIpAddr::isMulticast() const noexcept
{
    bool ret = false;

    if (m_family == Family::IPv4) {
        auto addr = ntohl(reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr);
        // 224.0.0.0–239.255.255.255
        ret = (addr & 0xF0000000) == 0xE0000000;
    }

    if (m_family == Family::IPv6)
        ret = IN6_IS_ADDR_MULTICAST(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return ret;
}

bool JobIpAddr::isNull() const noexcept
{
    bool ret = false;

    if (m_family == Family::IPv4)
        ret = reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr == 0;

    if (m_family == Family::IPv6)
        return IN6_IS_ADDR_UNSPECIFIED(&reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr);

    return ret;
}

bool JobIpAddr::isGlobal() const noexcept
{
    bool ret = false;

    if (m_family == Family::IPv4) {
        auto *saddr = reinterpret_cast<const sockaddr_in*>(&m_storage);
        if (!saddr)
            return false;

        uint32_t addr = ntohl(saddr->sin_addr.s_addr);

        // unspec address
        if (addr == 0)
            return false;

        uint8_t first  = (addr >> 24) & 0xFF;
        uint8_t second = (addr >> 16) & 0xFF;

        // Private / reserved IPv4 ranges (RFC1918, loopback, link-local, etc.)

        // 10.0.0.0/8
        if (first == 10)
            return false;

        // 127.0.0.0/8
        if (first == 127)
            return false;

        // 172.16.0.0/12
        if (first == 172 && (second >= 16 && second <= 31))
            return false;

        // 192.168.0.0/16
        if (first == 192 && second == 168)
            return false;

        // 169.254.0.0/16
        if (first == 169 && second == 254)
            return false;

        // Multicast || reserved
        if (first >= 224)
            return false;

        ret = true;
    }
    else if (m_family == Family::IPv6) {
        const auto *a6 = &reinterpret_cast<const sockaddr_in6*>(&m_storage)->sin6_addr;
        if (IN6_IS_ADDR_UNSPECIFIED(a6) ||
            IN6_IS_ADDR_LOOPBACK(a6)    ||
            IN6_IS_ADDR_LINKLOCAL(a6)   ||
            IN6_IS_ADDR_MULTICAST(a6))
            return false;
        ret = true;
    }

    return ret;
}
bool JobIpAddr::isBroadcast() const noexcept
{
    if (m_family != Family::IPv4)
        return false;
    return reinterpret_cast<const sockaddr_in*>(&m_storage)->sin_addr.s_addr == INADDR_BROADCAST;
}

// FIXME
bool JobIpAddr::isUnixPermitted() const noexcept
{
    // Placeholder for future permission checks (e.g. stat() or ACL logic)
    return true;
}

std::string JobIpAddr::versionString(Family f) {
    switch (f) {
    case Family::IPv4:
        return "IPv4";
    case Family::IPv6:
        return "IPv6";
    case Family::Unix:
        return "Unix";
    default:
        return "Unknown";
    }
}

bool JobIpAddr::fromSockAddr(const sockaddr *sa, socklen_t len)
{
    if (!sa || len == 0)
        return false;

    clear();

    std::memcpy(&m_storage, sa, len);
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
        m_family = Family::Unknown;
        m_valid = false;
        return false;
    }

    return true;
}


} // namespace job::net
