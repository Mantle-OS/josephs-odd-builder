#pragma once

#include <regex>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

namespace job::net {

class JobIpAddr {
public:
    enum class Family : uint8_t {
        Unknown = 0,
        IPv4,
        IPv6,
        Unix
    };

    constexpr JobIpAddr() noexcept = default;
    JobIpAddr(const std::string &addr, uint16_t port = 0);
    JobIpAddr(const JobIpAddr &other);
    JobIpAddr &operator=(const JobIpAddr &other);

    bool setAddress(const std::string &addr, uint16_t port = 0);
    void clear() noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] Family family() const noexcept;
    [[nodiscard]] std::string toString(bool includePort = true) const;
    [[nodiscard]] uint16_t port() const noexcept;

    [[nodiscard]] const sockaddr *sockAddr() const noexcept;
    [[nodiscard]] socklen_t sockAddrLen() const noexcept;

    [[nodiscard]] bool isLocal() const noexcept;
    [[nodiscard]] bool isLoopback() const noexcept;
    [[nodiscard]] bool isMulticast() const noexcept;
    [[nodiscard]] bool isNull() const noexcept;
    [[nodiscard]] bool isGlobal() const noexcept;
    [[nodiscard]] bool isBroadcast() const noexcept;
    [[nodiscard]] bool isUnixPermitted() const noexcept;

    [[nodiscard]] static bool isIPv4(const std::string &ip) noexcept
    {
        in_addr addr4;
        return inet_pton(AF_INET, ip.c_str(), &addr4) == 1;
    }

    [[nodiscard]] static bool isIPv6(const std::string &ip) noexcept
    {
        in6_addr addr6;
        return inet_pton(AF_INET6, ip.c_str(), &addr6) == 1;
    }
    [[nodiscard]] static bool isUnixPath(const std::string &path);
    [[nodiscard]] static constexpr bool isValidPort(int32_t port) noexcept
    {
        return port >= 0 && port <= 65535;
    }

    [[nodiscard]] static constexpr std::string_view versionString(Family f) noexcept {
        switch (f) {
        case Family::IPv4: return "IPv4";
        case Family::IPv6: return "IPv6";
        case Family::Unix: return "Unix";
        default: return "Unknown";
        }
    }

    [[nodiscard]] bool fromSockAddr(const sockaddr *sa, socklen_t len);
    [[nodiscard]] const std::regex &ipv4Pattern() noexcept;
    [[nodiscard]] const std::regex &ipv6Pattern() noexcept;


    [[nodiscard]] bool operator==(const JobIpAddr &o) const noexcept;
    [[nodiscard]] bool operator!=(const JobIpAddr &o) const noexcept;

private:
    Family m_family{Family::Unknown};
    uint16_t m_port{0};
    sockaddr_storage m_storage{};
    socklen_t m_len{0};
    bool m_valid{false};
};

} // namespace job::net
// CHECKPOINT: v1
