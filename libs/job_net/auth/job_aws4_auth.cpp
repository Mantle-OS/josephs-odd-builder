#include "job_aws4_auth.h"

#include <utility>

namespace job::net {

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   std::string_view secretAccessKey,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", secretAccessKey),
    m_accessKeyId{accessKeyId},
    m_region{region},
    m_service{service}
{
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   job::crypto::JobSecureMem::Ptr secretAccessKey,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", std::move(secretAccessKey)),
    m_accessKeyId{accessKeyId},
    m_region{region},
    m_service{service}
{
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   std::string_view secretAccessKey,
                   std::string_view sessionToken,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", secretAccessKey),
    m_accessKeyId{accessKeyId},
    m_region{region},
    m_service{service}
{
    setSessionToken(sessionToken);
}

Aws4Auth::Aws4Auth(std::string_view accessKeyId,
                   job::crypto::JobSecureMem::Ptr secretAccessKey,
                   job::crypto::JobSecureMem::Ptr sessionToken,
                   std::string_view region,
                   std::string_view service) :
    IJobHttpAuth("Authorization", "AWS4-HMAC-SHA256", std::move(secretAccessKey)),
    m_accessKeyId{accessKeyId},
    m_region{region},
    m_service{service},
    m_sessionToken{std::move(sessionToken)}
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

job::crypto::JobSecureMem::Ptr Aws4Auth::sessionToken() const noexcept
{
    return m_sessionToken;
}

bool Aws4Auth::hasSessionToken() const noexcept
{
    return m_sessionToken && !m_sessionToken->empty();
}

void Aws4Auth::clear() noexcept
{
    IJobHttpAuth::clear();

    m_accessKeyId.clear();
    m_region.clear();
    m_service.clear();

    if (m_sessionToken)
        m_sessionToken->clear();

    m_sessionToken.reset();
}

void Aws4Auth::setSessionToken(std::string_view token)
{
    if (token.empty()) {
        if (m_sessionToken)
            m_sessionToken->clear();

        m_sessionToken.reset();
        return;
    }

    auto secureToken = job::crypto::JobSecureMem::createShared(token.size());

    if (!secureToken || secureToken->empty()) {
        m_sessionToken.reset();
        return;
    }

    secureToken->copyFrom(token.data(), token.size());
    m_sessionToken = std::move(secureToken);
}

void Aws4Auth::setSessionToken(job::crypto::JobSecureMem::Ptr token) noexcept
{
    m_sessionToken = std::move(token);
}

} // namespace job::net