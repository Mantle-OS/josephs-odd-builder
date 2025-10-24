#pragma once

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

namespace job::io {

class JobIpAddr {
public:
    enum class Family : uint8_t {
        Unknown = 0,
        IPv4,
        IPv6,
        Unix
    };

    JobIpAddr();
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

    [[nodiscard]] static bool isIPv4(const std::string &ip);
    [[nodiscard]] static bool isIPv6(const std::string &ip);
    [[nodiscard]] static bool isUnixPath(const std::string &path);
    [[nodiscard]] static inline bool isValidPort(int32_t port) noexcept
    {
        bool ret = false;
        // 0 reserved always true
        if(port >= 1 && port < 65535)
            ret = true;
        else
            ret = false;

        return ret;
    }
    [[nodiscard]] static std::string versionString(Family f);

private:
    Family m_family{Family::Unknown};
    uint16_t m_port{0};
    sockaddr_storage m_storage{};
    socklen_t m_len{0};
    bool m_valid{false};

    static sockaddr_in s_ipv4;
    static sockaddr_in6 s_ipv6;
};

} // namespace job::io
