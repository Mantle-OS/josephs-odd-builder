#include "resolve/job_resolver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>
#include <job_logger.h>

namespace job::net {

std::vector<JobIpAddr> JobResolver::resolveSync( const std::string &host, uint16_t port, const JobProxyConfig &proxyConfig, int &errorCode)
{
    errorCode = 0;
    std::vector<JobIpAddr> results;
    std::string targetHost = host;
    uint16_t targetPort = port;

    if (proxyConfig.proxyType != ProxyType::None &&
        proxyConfig.supports(ProxyCapability::HostNameLookup)) {
        // The proxy resolves the final destination hostname.
        // Locally resolve only the proxy gateway used as the next hop.
        targetHost = proxyConfig.host;
        targetPort = proxyConfig.port;
        JOB_LOG_INFO(
            "[JobResolver] Proxy hostname lookup active: "
            "resolving proxy gateway '{}' instead of destination '{}'",
            targetHost,
            host
            );
    }

    if (targetHost.empty()) {
        JOB_LOG_ERROR(
            "[JobResolver] Cannot resolve empty host target"
            );
        errorCode = WSAEINVAL;
        return results;
    }

    ADDRINFOA hints{};
    PADDRINFOA resultList = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const std::string portString =
        std::to_string(targetPort);

    const int result = ::GetAddrInfoA(
        targetHost.c_str(),
        portString.c_str(),
        &hints,
        &resultList
        );

    if (result != 0) {
        errorCode = result;
        JOB_LOG_ERROR(
            "[JobResolver] GetAddrInfoA failed for '{}': "
            "Winsock error {}",
            targetHost,
            errorCode
            );
        return results;
    }

    for (PADDRINFOA current = resultList;
         current != nullptr;
         current = current->ai_next) {
        JobIpAddr address;
        if (address.fromSockAddr(
                current->ai_addr,
                static_cast<JobSockLen>(
                    current->ai_addrlen))) {
            results.push_back(std::move(address));
        }
    }

    ::FreeAddrInfoA(resultList);
    return results;
}

} // namespace job::net