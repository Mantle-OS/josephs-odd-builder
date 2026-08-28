#include "job_http_status.h"

namespace job::net {

JobHttpStatus::JobHttpStatus(std::uint16_t code) noexcept :
    m_code(code)
{
}

JobHttpStatus::JobHttpStatus(JobHttpStatusCode status) noexcept :
    m_code(toStatusCode(status))
{
}

std::uint16_t JobHttpStatus::code() const noexcept
{
    return m_code;
}

JobHttpStatusCode JobHttpStatus::status() const noexcept
{
    return toHttpStatusCode(m_code);
}

std::string_view JobHttpStatus::statusString() const noexcept
{
    return toString(status());
}

void JobHttpStatus::setCode(std::uint16_t code) noexcept
{
    m_code = code;
}

void JobHttpStatus::setStatus(JobHttpStatusCode status) noexcept
{
    m_code = toStatusCode(status);
}

bool JobHttpStatus::isKnown() const noexcept
{
    return status() != JobHttpStatusCode::Unknown;
}

bool JobHttpStatus::isInformational() const noexcept
{
    return m_code >= 100 && m_code < 200;
}

bool JobHttpStatus::isSuccess() const noexcept
{
    return m_code >= 200 && m_code < 300;
}

bool JobHttpStatus::isRedirection() const noexcept
{
    return m_code >= 300 && m_code < 400;
}

bool JobHttpStatus::isClientError() const noexcept
{
    return m_code >= 400 && m_code < 500;
}

bool JobHttpStatus::isServerError() const noexcept
{
    return m_code >= 500 && m_code < 600;
}

bool JobHttpStatus::operator==(JobHttpStatusCode status) const noexcept
{
    return this->status() == status;
}

bool JobHttpStatus::operator==(std::uint16_t code) const noexcept
{
    return m_code == code;
}

} // namespace job::net