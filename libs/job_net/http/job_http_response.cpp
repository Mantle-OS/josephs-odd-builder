#include "job_http_response.h"

#include <utility>

namespace job::net {

JobHttpResponse::JobHttpResponse(std::uint16_t code) :
    m_status{code}
{
}

JobHttpResponse::JobHttpResponse(JobHttpStatusCode statusCode) :
    m_status{statusCode}
{
}

JobHttpResponse::JobHttpResponse(std::uint16_t code, std::string_view reasonPhrase) :
    m_status{code},
    m_reasonPhrase{reasonPhrase}
{
}

JobHttpResponse::JobHttpResponse(JobHttpStatusCode statusCode, std::string_view reasonPhrase) :
    m_status{statusCode},
    m_reasonPhrase{reasonPhrase}
{
}

const JobHttpStatus &JobHttpResponse::status() const noexcept
{
    return m_status;
}

JobHttpStatus &JobHttpResponse::status() noexcept
{
    return m_status;
}

std::uint16_t JobHttpResponse::code() const noexcept
{
    return m_status.code();
}

JobHttpStatusCode JobHttpResponse::statusCode() const noexcept
{
    return m_status.status();
}

void JobHttpResponse::setCode(std::uint16_t code) noexcept
{
    m_status.setCode(code);
}

void JobHttpResponse::setStatusCode(JobHttpStatusCode statusCode) noexcept
{
    m_status.setStatus(statusCode);
}

const std::string &JobHttpResponse::reasonPhrase() const noexcept
{
    return m_reasonPhrase;
}

void JobHttpResponse::setReasonPhrase(std::string_view reasonPhrase)
{
    m_reasonPhrase.assign(reasonPhrase);
}

void JobHttpResponse::clearReasonPhrase() noexcept
{
    m_reasonPhrase.clear();
}

const JobHttpHeader &JobHttpResponse::headers() const noexcept
{
    return m_headers;
}

JobHttpHeader &JobHttpResponse::headers() noexcept
{
    return m_headers;
}

void JobHttpResponse::setHeaders(const JobHttpHeader &headers)
{
    m_headers = headers;
}

void JobHttpResponse::setHeaders(JobHttpHeader &&headers)
{
    m_headers = std::move(headers);
}

void JobHttpResponse::clearHeaders()
{
    m_headers.clear();
}

const JobHttpHeader &JobHttpResponse::trailers() const noexcept
{
    return m_trailers;
}

JobHttpHeader &JobHttpResponse::trailers() noexcept
{
    return m_trailers;
}

bool JobHttpResponse::hasTrailers() const noexcept
{
    return !m_trailers.isEmpty();
}

void JobHttpResponse::setTrailers(const JobHttpHeader &trailers)
{
    m_trailers = trailers;
}

void JobHttpResponse::setTrailers(JobHttpHeader &&trailers)
{
    m_trailers = std::move(trailers);
}

void JobHttpResponse::clearTrailers()
{
    m_trailers.clear();
}

const JobHttpResponse::Body &JobHttpResponse::body() const noexcept
{
    return m_body;
}

std::span<const std::byte> JobHttpResponse::bodyView() const noexcept
{
    return m_body;
}

bool JobHttpResponse::hasBody() const noexcept
{
    return !m_body.empty();
}

std::size_t JobHttpResponse::bodySize() const noexcept
{
    return m_body.size();
}

void JobHttpResponse::setBody(const Body &body)
{
    m_body = body;
}

void JobHttpResponse::setBody(Body &&body)
{
    m_body = std::move(body);
}

void JobHttpResponse::setBody(std::span<const std::byte> body)
{
    m_body.assign(body.begin(), body.end());
}

void JobHttpResponse::appendBody(std::span<const std::byte> data)
{
    m_body.insert(m_body.end(), data.begin(), data.end());
}

void JobHttpResponse::clearBody() noexcept
{
    m_body.clear();
}

bool JobHttpResponse::isInformational() const noexcept
{
    return m_status.isInformational();
}

bool JobHttpResponse::isSuccessful() const noexcept
{
    return m_status.isSuccess();
}

bool JobHttpResponse::isRedirect() const noexcept
{
    return m_status.isRedirection();
}

bool JobHttpResponse::isClientError() const noexcept
{
    return m_status.isClientError();
}

bool JobHttpResponse::isServerError() const noexcept
{
    return m_status.isServerError();
}

void JobHttpResponse::clear()
{
    m_status.setCode(0);
    m_reasonPhrase.clear();
    m_headers.clear();
    m_trailers.clear();
    m_body.clear();
}

bool JobHttpResponse::isValid() const noexcept
{
    const std::uint16_t statusCode = m_status.code();
    return statusCode >= 100 && statusCode < 600;
}

} // namespace job::net