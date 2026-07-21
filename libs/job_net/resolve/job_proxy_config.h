#pragma once

#include <cstdint>

#include <job_secure_mem.h>

namespace job::net {

enum class ProxyType : uint8_t
{
    None,
    Socks4,
    Socks5,
    HttpConnect,
    HttpCaching,
    Ftp
};

enum class ProxyCapability : uint32_t
{
    None                   = 0,
    TunnelingCapability    = 1 << 0,
    ListeningCapability    = 1 << 1,
    UdpTunnelingCapability = 1 << 2,
    CachingCapability      = 1 << 3,
    HostNameLookup         = 1 << 4,
    SctpTunneling          = 1 << 5,
    SslTunneling           = 1 << 6
};


inline ProxyCapability operator|(ProxyCapability a, ProxyCapability b)
{
    return static_cast<ProxyCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool hasCapability(ProxyCapability mask, ProxyCapability cap)
{
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(cap)) != 0;
}

struct JobProxyConfig
{
    std::string host;
    uint16_t port{0};

    ProxyType proxyType{ProxyType::None};
    ProxyCapability capabilities{ProxyCapability::None};

    crypto::JobSecureMem username;
    crypto::JobSecureMem password;

    // Checks if the configuration meets requirements (e.g., must resolve remotely)
    [[nodiscard]] bool supports(ProxyCapability required) const noexcept
    {
        return hasCapability(capabilities, required);
    }
};

} // namespace job::net
