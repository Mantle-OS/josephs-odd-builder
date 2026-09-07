#include "job_http_request.h"

#include <utility>

namespace job::net {

JobHttpRequest::JobHttpRequest(const JobUrl &url) :
    m_url{url}
{
}

JobHttpRequest::JobHttpRequest(HttpMethod method, const JobUrl &url) :
    m_method{method},
    m_url{url}
{
}

JobHttpRequest::JobHttpRequest(std::string_view customMethod, const JobUrl &url) :
    m_method{HttpMethod::Custom},
    m_customMethod{customMethod},
    m_url{url}
{
}

HttpMethod JobHttpRequest::method() const noexcept
{
    return m_method;
}

std::string_view JobHttpRequest::methodString() const noexcept
{
    if (m_method == HttpMethod::Custom)
        return m_customMethod;

    return JobHttpMethod::toString(m_method);
}

const std::string &JobHttpRequest::customMethod() const noexcept
{
    return m_customMethod;
}

void JobHttpRequest::setMethod(HttpMethod method) noexcept
{
    m_method = method;
    if (m_method != HttpMethod::Custom)
        m_customMethod.clear();
}

void JobHttpRequest::setCustomMethod(std::string_view method)
{
    m_method = HttpMethod::Custom;
    m_customMethod.assign(method);
}

void JobHttpRequest::clearCustomMethod() noexcept
{
    m_customMethod.clear();

    if (m_method == HttpMethod::Custom)
        m_method = HttpMethod::Get;
}

const JobUrl &JobHttpRequest::url() const noexcept
{
    return m_url;
}

void JobHttpRequest::setUrl(const JobUrl &url)
{
    m_url = url;
}

void JobHttpRequest::setUrl(JobUrl &&url)
{
    m_url = std::move(url);
}

const JobHttpHeader &JobHttpRequest::headers() const noexcept
{
    return m_headers;
}

JobHttpHeader &JobHttpRequest::headers() noexcept
{
    return m_headers;
}

void JobHttpRequest::setHeaders(const JobHttpHeader &headers)
{
    m_headers = headers;
}

void JobHttpRequest::setHeaders(JobHttpHeader &&headers)
{
    m_headers = std::move(headers);
}

void JobHttpRequest::clearHeaders()
{
    m_headers.clear();
}

const JobHttpRequest::Body &JobHttpRequest::body() const noexcept
{
    return m_body;
}

std::span<const std::byte> JobHttpRequest::bodyView() const noexcept
{
    return m_body;
}

bool JobHttpRequest::hasBody() const noexcept
{
    return !m_body.empty();
}

std::size_t JobHttpRequest::bodySize() const noexcept
{
    return m_body.size();
}

void JobHttpRequest::setBody(const Body &body)
{
    m_body = body;
}

void JobHttpRequest::setBody(Body &&body)
{
    m_body = std::move(body);
}

void JobHttpRequest::setBody(std::span<const std::byte> body)
{
    m_body.assign(body.begin(), body.end());
}

void JobHttpRequest::clearBody() noexcept
{
    m_body.clear();
}

void JobHttpRequest::clear()
{
    m_method = HttpMethod::Get;
    m_customMethod.clear();
    m_url.clear();
    m_headers.clear();
    m_body.clear();
}

bool JobHttpRequest::isValid() const noexcept
{
    if (!m_url.isValid())
        return false;

    if (m_method == HttpMethod::Custom)
        return !m_customMethod.empty();

    return !methodString().empty();
}

} // namespace job::net