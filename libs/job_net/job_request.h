#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <memory>


#include "job_http_header.h"
#include "job_ssl_context.h"
#include "job_url.h"
#include "resolve/job_ipaddr.h"
#include "jobnet_export.h"

namespace job::net {


enum class JobNetMethod : std::uint8_t {
    Get,
    Post,
    Put,
    Delete,
    Custom
};

enum class Priority : std::uint8_t {
    High,
    Medium, // Normal
    Low
};



class JOBNET_EXPORT JobNetRequest  {

public:
    using Ptr = std::shared_ptr<JobNetRequest>;

    JobNetRequest()
    {

    }
    JobNetRequest(const JobUrl &url):
        m_url{url}
    {

    }
    ~JobNetRequest() = default;

    JobNetRequest(const JobNetRequest &other) = delete;
    JobNetRequest &operator=(const JobNetRequest &) = delete;
    JobNetRequest(JobNetRequest &&) = delete;
    JobNetRequest &operator=(JobNetRequest &&) = delete;

    [[nodiscard]] bool operator!=(const JobNetRequest &other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool operator==(const JobNetRequest &other) const noexcept
    {
        return m_url == other.m_url
               && m_numberOfRedirects == other.m_numberOfRedirects
               && m_peerName == other.m_peerName
               && m_priority == other.m_priority
               && m_headers == other.m_headers
               && m_ip == other.m_ip
               && m_sslContext == other.m_sslContext;
    }

    void swap(JobNetRequest &other) noexcept
    {
        using std::swap;

        swap(m_url, other.m_url);
        swap(m_numberOfRedirects, other.m_numberOfRedirects);
        swap(m_peerName, other.m_peerName);
        swap(m_priority, other.m_priority);
        swap(m_headers, other.m_headers);
        swap(m_ip, other.m_ip);
        swap(m_sslContext, other.m_sslContext);
    }

    [[nodiscard]] JobUrl url() const noexcept
    {
        return m_url;
    }
    void setUrl(const JobUrl &url) noexcept
    {
        if(m_url != url)
            m_url = url;
    }


    [[nodiscard]] int numberOfRedirects() const noexcept
    {
        return m_numberOfRedirects;
    }
    void setNumberOfRedirects(int numberOfRedirects) noexcept
    {
        if(m_numberOfRedirects != numberOfRedirects)
            m_numberOfRedirects = numberOfRedirects;
    }

    [[nodiscard]] std::string peerName() const noexcept
    {
        return m_peerName;
    }

    void setPeerName(const std::string &peerName)
    {
        if(m_peerName != peerName && !peerName.empty())
            m_peerName = peerName;
    }

    [[nodiscard]] Priority priority() const noexcept;
    void setPriority(Priority priority) noexcept
    {
        if(m_priority != priority)
            m_priority = priority;
    }

    [[nodiscard]] JobHttpHeader headers() const noexcept
    {
        return m_headers;
    }
    void setHeaders(const JobHttpHeader &headers)
    {
        m_headers = headers;
    }
    void appendHeader(JobIana::IanaHeaders header, std::string_view val)
    {
        if(!m_headers.append(header, val)){
            // LOG
        }
    }

    [[nodiscard]] std::string_view header(JobIana::IanaHeaders header) const
    {
        std::string_view ret;
        if(m_headers.contains(header))
            ret = m_headers.value(header, "");
        return ret;
    }

    [[nodiscard]] bool hasRawHeader(std::string_view name, std::string_view value)
    {
        return m_headers.append(name, value);
    }
    [[nodiscard]] std::string_view rawHeader(std::string_view name) const
    {
        std::string_view ret;
        ret = m_headers.value(name, "");
        return ret;
    }

    std::vector<std::string_view> rawHeaderList() const
    {
        std::vector<std::string_view> ret;
        if(!m_headers.isEmpty()){
            for(auto i = m_headers.cbegin() ; i != m_headers.cend(); ++i)
                ret.push_back(i->first);
        }
        return ret;
    }

    bool setRawHeader(const std::string &name, const std::string &value)
    {
        return m_headers.append(name, value);
    }

    void setSslContext(JobSslContext::Ptr config)
    {
        m_sslContext = config;
    }
    [[nodiscard]] JobSslContext::Ptr sslContext() const
    {
        return m_sslContext;
    }


private:
    JobUrl              m_url;
    int                 m_numberOfRedirects = 1;
    std::string         m_peerName;
    Priority            m_priority;
    JobHttpHeader       m_headers;

    JobIpAddr           m_ip;
    JobSslContext::Ptr  m_sslContext; // borrowed

};
}