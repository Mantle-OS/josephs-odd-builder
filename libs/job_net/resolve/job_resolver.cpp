#include "resolve/job_resolver.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <job_logger.h>

namespace job::net {

std::vector<JobIpAddr> JobResolver::resolveSync(const std::string &host, uint16_t port, const JobProxyConfig &proxyConfig, int &errorCode)
{
    errorCode = 0;
    std::vector<JobIpAddr> results;
    std::string targetHost = host;
    uint16_t targetPort = port;

    if (proxyConfig.proxyType != ProxyType::None &&
        proxyConfig.supports(ProxyCapability::HostNameLookup)) {
        targetHost = proxyConfig.host;
        targetPort = proxyConfig.port;
        JOB_LOG_INFO("[JobResolver] Proxy hostname lookup active: resolving proxy gateway '{}' instead of destination '{}'", targetHost, host);
    }

    if (targetHost.empty()) {
        JOB_LOG_ERROR( "[JobResolver] Cannot resolve empty host target" );
        errorCode = EINVAL;
        return results;
    }

    addrinfo hints{};
    addrinfo *resultList = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const std::string portString = std::to_string(targetPort);
    const int result = ::getaddrinfo(
        targetHost.c_str(),
        portString.c_str(),
        &hints,
        &resultList
        );

    if (result != 0) {
        errorCode = result;
        JOB_LOG_ERROR("[JobResolver] getaddrinfo failed for '{}': {}", targetHost, ::gai_strerror(result));
        return results;
    }

    for (addrinfo *current = resultList; current != nullptr; current = current->ai_next) {
        JobIpAddr address;
        if (address.fromSockAddr( current->ai_addr, static_cast<JobSockLen>( current->ai_addrlen)))
            results.push_back(std::move(address));
    }

    ::freeaddrinfo(resultList);
    return results;
}

} // namespace job::net