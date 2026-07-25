#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <type_traits>

#include <job_secure_mem.h>
#include "jobnet_export.h"

namespace job::net {

enum class ProxyType : uint8_t {
    None = 0,
    Socks4,
    Socks5,
    HttpConnect,
    HttpCaching,
    Ftp
};

enum class ProxyCapability : uint32_t {
    None                   = 0,
    TunnelingCapability    = 1 << 0,
    ListeningCapability    = 1 << 1,
    UdpTunnelingCapability = 1 << 2,
    CachingCapability      = 1 << 3,
    HostNameLookup         = 1 << 4,
    SctpTunneling          = 1 << 5,
    SslTunneling           = 1 << 6
};

constexpr ProxyCapability operator|(ProxyCapability a, ProxyCapability b) noexcept
{
    using T = std::underlying_type_t<ProxyCapability>;
    return static_cast<ProxyCapability>(static_cast<T>(a) | static_cast<T>(b));
}

constexpr ProxyCapability operator&(ProxyCapability a, ProxyCapability b) noexcept
{
    using T = std::underlying_type_t<ProxyCapability>;
    return static_cast<ProxyCapability>(static_cast<T>(a) & static_cast<T>(b));
}

constexpr ProxyCapability operator~(ProxyCapability a) noexcept
{
    using T = std::underlying_type_t<ProxyCapability>;
    return static_cast<ProxyCapability>(~static_cast<T>(a));
}

inline ProxyCapability &operator|=(ProxyCapability &a, ProxyCapability b) noexcept
{
    a = a | b;
    return a;
}

inline bool hasCapability(ProxyCapability mask, ProxyCapability cap) noexcept
{
    return (mask & cap) != ProxyCapability::None;
}


struct JOBNET_EXPORT JobProxyConfig {
    std::string host;
    uint16_t port{0};

    ProxyType proxyType{ProxyType::None};
    ProxyCapability capabilities{ProxyCapability::None};

    std::shared_ptr<crypto::JobSecureMem> username;
    std::shared_ptr<crypto::JobSecureMem> password;

    [[nodiscard]] bool isValid() const noexcept
    {
        if (proxyType == ProxyType::None)
            return true;
        return !host.empty() && port > 0;
    }

    [[nodiscard]] bool supports(ProxyCapability required) const noexcept
    {
        return hasCapability(capabilities, required);
    }

    void setCredentials(const char *user, size_t userLen, const char *pass, size_t passLen) {
        if (user && userLen > 0) {
            username = std::make_shared<crypto::JobSecureMem>(userLen);
            username->copyFrom(user, userLen);
        } else {
            username.reset();
        }

        if (pass && passLen > 0) {
            password = std::make_shared<crypto::JobSecureMem>(passLen);
            password->copyFrom(pass, passLen);
        } else {
            password.reset();
        }
    }

    void clearCredentials() noexcept {
        username.reset();
        password.reset();
    }
};

} // namespace job::net