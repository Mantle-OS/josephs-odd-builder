#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <utility>
#include <io/job_io_async_thread.h>
#include <job_thread_pool.h>
#include <job_logger.h>
#include "job_ipaddr.h"
#include "job_proxy_config.h"
#include "job_url.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobResolver : public std::enable_shared_from_this<JobResolver> {
public:
    using Ptr = std::shared_ptr<JobResolver>;
    using ResolveCallback = std::function<void(int errorCode, const std::vector<JobIpAddr> &addresses)>;

    struct Impl {};

    [[nodiscard]] static Ptr create(threads::JobIoAsyncThread::Ptr loop,
                                    threads::ThreadPool::Ptr workerPool,
                                    JobProxyConfig proxyConfig = {})
    {
        // std::shared_ptr<JobResolver> can't call a private ctor directly via make_shared;
        // a small pass-key or a public-but-documented ctor + factory-only convention is
        // the usual workaround here. Sketching the intent — exact mechanics TBD.
        return Ptr(new JobResolver(std::move(loop), std::move(workerPool), std::move(proxyConfig)));
    }

    ~JobResolver() = default;

    JobResolver(const JobResolver &) = delete;
    JobResolver &operator=(const JobResolver &) = delete;

    inline bool resolveAsync(const std::string &host, uint16_t port, ResolveCallback callback)
    {
        if (!m_loop.lock()) {
            JOB_LOG_ERROR("[JobResolver] Cannot resolve async: Event loop is null");
            return false;
        }
        if (!m_workerPool) {
            JOB_LOG_ERROR("[JobResolver] Cannot resolve async: no worker pool configured");
            return false;
        }

        auto self = shared_from_this();
        JobProxyConfig proxySnapshot = m_proxyConfig; // copied here, on the caller's thread

        m_workerPool->submit([self, host, port, proxySnapshot, cb = std::move(callback)]() mutable {
            int errorCode = 0;
            std::vector<JobIpAddr> addresses = self->resolveSync(host, port, proxySnapshot, errorCode);

            if (auto loop = self->m_loop.lock()) {
                loop->post([cb = std::move(cb), errorCode, addrs = std::move(addresses)]() mutable {
                    if (cb)
                        cb(errorCode, addrs);
                });
            } else {
                JOB_LOG_WARN("[JobResolver] Loop gone before resolve callback could be posted");
            }
        });
        return true;
    }

    inline bool resolveAsync(const JobUrl &url, ResolveCallback callback)
    {
        uint16_t targetPort = url.port();
        if (targetPort == 0) {
            if (url.scheme() == JobUrl::Scheme::Http)
                targetPort = 80;
            else if (url.scheme() == JobUrl::Scheme::Https)
                targetPort = 443;
        }
        return resolveAsync(url.host(), targetPort, std::move(callback));
    }

    inline void setProxyConfig(const JobProxyConfig &config)
    {
        m_proxyConfig = config;
    }

    [[nodiscard]] inline JobProxyConfig proxyConfig() const
    {
        return m_proxyConfig;
    }

    // HANDLED IN IMPL (per-OS, unchanged by this pass)
    [[nodiscard]] std::vector<JobIpAddr> resolveSync(const std::string &host, uint16_t port, const JobProxyConfig &proxyConfig, int &errorCode);

private:
    inline JobResolver(threads::JobIoAsyncThread::Ptr loop,
                       threads::ThreadPool::Ptr workerPool,
                       JobProxyConfig proxyConfig) :
        m_loop(std::move(loop)),
        m_workerPool(std::move(workerPool)),
        m_proxyConfig(std::move(proxyConfig)),
        m_impl(std::make_unique<Impl>())
    {
    }

    std::weak_ptr<threads::JobIoAsyncThread>  m_loop;
    threads::ThreadPool::Ptr                  m_workerPool;
    JobProxyConfig                            m_proxyConfig;
    std::unique_ptr<Impl>                     m_impl;
};

} // namespace job::net