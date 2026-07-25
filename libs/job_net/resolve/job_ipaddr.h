#pragma once

#include <regex>
#include <string>
#include <memory>
#include <cstdint>
#include <cstring>
#include <string_view>

#if defined(JOB_WINDOWS)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <afunix.h>   // sockaddr_un, Win10 1803+
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/un.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

#include "jobnet_export.h"

namespace job::net {

#if defined(JOB_WINDOWS)
    using JobSockLen = int;
#else
    using JobSockLen = socklen_t;
#endif

class JOBNET_EXPORT JobIpAddr {
public:
    using Ptr = std::shared_ptr<JobIpAddr>;
    enum class Family : uint8_t {
        Unknown = 0,
        IPv4,
        IPv6,
        Unix
    };
#if defined(JOB_LINUX)
    constexpr JobIpAddr() noexcept = default;
#else
    JobIpAddr() noexcept = default;
#endif

    explicit JobIpAddr(const std::string &addr, uint16_t port = 0)
    {
        (void)setAddress(addr, port);
    }
    JobIpAddr(const JobIpAddr &other) = default;
    JobIpAddr &operator=(const JobIpAddr &other) = default;

    void clear() noexcept
    {
        std::memset(&m_storage, 0, sizeof(m_storage));
        m_len = 0;
        m_port = 0;
        m_valid = false;
        m_family = Family::Unknown;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_valid;
    }

    [[nodiscard]] Family family() const noexcept
    {
        return m_family;
    }

    [[nodiscard]] uint16_t port() const noexcept
    {
        return m_port;
    }

    [[nodiscard]] const sockaddr *sockAddr() const noexcept
    {
        return reinterpret_cast<const sockaddr*>(&m_storage);
    }

    [[nodiscard]] JobSockLen sockAddrLen() const noexcept
    {
        return m_len;
    }

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

    [[nodiscard]] static const std::regex &ipv4Pattern()
    {
        static const std::regex re(R"(^(\d{1,3}\.){3}\d{1,3}$)");
        return re;
    }

    [[nodiscard]] static const std::regex &ipv6Pattern()
    {
        static const std::regex re(R"(^([0-9A-Fa-f]{0,4}:){1,7}[0-9A-Fa-f]{0,4}$)");
        return re;
    }

    [[nodiscard]] bool operator==(const JobIpAddr &o) const noexcept
    {
        return m_family == o.m_family &&
               m_port == o.m_port &&
               std::memcmp(&m_storage, &o.m_storage, sizeof(sockaddr_storage)) == 0;
    }

    [[nodiscard]] bool operator!=(const JobIpAddr &o) const noexcept
    {
        return !(*this == o);
    }

    // HANDED PER OS IMPL
    bool setAddress(const std::string &addr, uint16_t port = 0);
    [[nodiscard]] std::string toString(bool includePort = true) const;
    [[nodiscard]] bool isLocal() const noexcept;
    [[nodiscard]] bool isLoopback() const noexcept;
    [[nodiscard]] bool isMulticast() const noexcept;
    [[nodiscard]] bool isNull() const noexcept;
    [[nodiscard]] bool isGlobal() const noexcept;
    [[nodiscard]] bool isBroadcast() const noexcept;

    [[nodiscard]] static bool isIPv4(const std::string &ip) noexcept;
    [[nodiscard]] static bool isIPv6(const std::string &ip) noexcept;
    [[nodiscard]] static bool isUnixPath(const std::string &path);
    [[nodiscard]] bool isUnixPermitted() const noexcept;

    [[nodiscard]] bool fromSockAddr(const sockaddr *sa, JobSockLen len);
private:
    Family              m_family{Family::Unknown};
    uint16_t            m_port{0};
    sockaddr_storage    m_storage{};
    JobSockLen          m_len{0};
    bool                m_valid{false};
};

} // namespace job::net

