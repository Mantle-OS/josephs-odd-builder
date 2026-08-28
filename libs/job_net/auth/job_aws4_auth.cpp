#include "job_aws4_auth.h"

#include <utility>

namespace job::net {

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   std::string_view secretAccessKey,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", secretAccessKey),
    m_accessKeyId(accessKeyId),
    m_region(region),
    m_service(service)
{
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   job::crypto::JobSecureMem &&secretAccessKey,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", std::move(secretAccessKey)),
    m_accessKeyId(accessKeyId),
    m_region(region),
    m_service(service)
{
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   std::string_view secretAccessKey,
                   std::string_view sessionToken,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", secretAccessKey),
    m_accessKeyId(accessKeyId),
    m_region(region),
    m_service(service)
{
    setSessionToken(sessionToken);
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   job::crypto::JobSecureMem &&secretAccessKey,
                   job::crypto::JobSecureMem &&sessionToken,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", std::move(secretAccessKey)),
    m_accessKeyId(accessKeyId),
    m_region(region),
    m_service(service),
    m_sessionToken(std::move(sessionToken))
{
}

JobHttpAuthType Aws4Auth::authType() const noexcept
{
    return JobHttpAuthType::Aws4HmacSha256;
}

bool Aws4Auth::isValid() const noexcept
{
    return !m_accessKeyId.empty() &&
           hasToken() &&
           !m_region.empty() &&
           !m_service.empty();
}

const std::string &Aws4Auth::accessKeyId() const noexcept
{
    return m_accessKeyId;
}

const std::string &Aws4Auth::region() const noexcept
{
    return m_region;
}

const std::string &Aws4Auth::service() const noexcept
{
    return m_service;
}

const job::crypto::JobSecureMem &Aws4Auth::sessionToken() const noexcept
{
    return m_sessionToken;
}

bool Aws4Auth::hasSessionToken() const noexcept
{
    return !m_sessionToken.empty();
}

void Aws4Auth::clear() noexcept
{
    IJobHttpAuth::clear();

    m_accessKeyId.clear();
    m_region.clear();
    m_service.clear();
    m_sessionToken.clear();
}

void Aws4Auth::setSessionToken(std::string_view token)
{
    if (token.empty()) {
        m_sessionToken.clear();
        return;
    }

    if (!m_sessionToken.allocate(token.size())) {
        m_sessionToken.clear();
        return;
    }

    m_sessionToken.copyFrom(token.data(), token.size());
}

void Aws4Auth::setSessionToken(job::crypto::JobSecureMem &&token) noexcept
{
    m_sessionToken = std::move(token);
}

} // namespace job::net