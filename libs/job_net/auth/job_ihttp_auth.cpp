#include "job_ihttp_auth.h"

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
                           job::crypto::JobSecureMem &&token) :
    m_headerName(headerName),
    m_scheme(scheme),
    m_token(std::move(token))
{
}

const std::string &IJobHttpAuth::headerName() const noexcept
{
    return m_headerName;
}

const std::string &IJobHttpAuth::scheme() const noexcept
{
    return m_scheme;
}

const job::crypto::JobSecureMem &IJobHttpAuth::token() const noexcept
{
    return m_token;
}

bool IJobHttpAuth::hasToken() const noexcept
{
    return !m_token.empty();
}

void IJobHttpAuth::clear() noexcept
{
    m_headerName.clear();
    m_scheme.clear();
    m_token.clear();
}

void IJobHttpAuth::setHeaderName(std::string_view headerName)
{
    m_headerName = headerName;
}

void IJobHttpAuth::setScheme(std::string_view scheme)
{
    m_scheme = scheme;
}

void IJobHttpAuth::setToken(std::string_view token)
{
    if (token.empty()) {
        m_token.clear();
        return;
    }

    if (!m_token.allocate(token.size())) {
        m_token.clear();
        return;
    }

    m_token.copyFrom(token.data(), token.size());
}

void IJobHttpAuth::setToken(job::crypto::JobSecureMem &&token) noexcept
{
    m_token = std::move(token);
}

} // namespace job::net