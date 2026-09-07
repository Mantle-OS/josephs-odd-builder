#include "job_ihttp_auth.h"

#include <utility>

namespace job::net {

IJobHttpAuth::IJobHttpAuth(std::string_view headerName,
                           std::string_view scheme,
                           std::string_view token) :
    m_headerName(headerName),
    m_scheme(scheme)
{
    setToken(token);
}

IJobHttpAuth::IJobHttpAuth(std::string_view headerName,
                           std::string_view scheme,
                           job::crypto::JobSecureMem::Ptr token) :
    m_headerName(headerName),
    m_scheme(scheme),
    m_token(std::move(token))
{
}

const std::string &IJobHttpAuth::headerName() const noexcept
{
    return m_headerName;
}

void IJobHttpAuth::setHeaderName(std::string_view headerName)
{
    m_headerName = headerName;
}

const std::string &IJobHttpAuth::scheme() const noexcept
{
    return m_scheme;
}

void IJobHttpAuth::setScheme(std::string_view scheme)
{
    m_scheme = scheme;
}

bool IJobHttpAuth::hasToken() const noexcept
{
    return m_token && !m_token->empty();
}

job::crypto::JobSecureMem::Ptr IJobHttpAuth::token() const noexcept
{
    return m_token;
}

void IJobHttpAuth::setToken(std::string_view token)
{
    if (token.empty()) {
        m_token.reset();
        return;
    }

    auto secureToken = job::crypto::JobSecureMem::createShared(token.size());

    if (!secureToken || secureToken->empty()) {
        m_token.reset();
        return;
    }

    secureToken->copyFrom(token.data(), token.size());
    m_token = std::move(secureToken);
}

void IJobHttpAuth::setToken(job::crypto::JobSecureMem::Ptr token) noexcept
{
    m_token = std::move(token);
}

void IJobHttpAuth::clear() noexcept
{
    m_headerName.clear();
    m_scheme.clear();

    if (m_token)
        m_token->clear();

    m_token.reset();
}

} // namespace job::net