#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace job::net {

enum class HttpMethod : std::uint8_t
{
    Get = 0,
    Head,
    Post,
    Put,
    Delete,
    Patch,
    Options,
    Connect,
    Trace,
    Custom
};

class JobHttpMethod
{
public:
    JobHttpMethod() = delete;
    ~JobHttpMethod() = delete;

    JobHttpMethod(const JobHttpMethod &) = delete;
    JobHttpMethod &operator=(const JobHttpMethod &) = delete;
    JobHttpMethod(JobHttpMethod &&) = delete;
    JobHttpMethod &operator=(JobHttpMethod &&) = delete;

    [[nodiscard]] static constexpr std::string_view toString(HttpMethod method) noexcept
    {
        switch (method) {
        case HttpMethod::Get:
            return "GET";
        case HttpMethod::Head:
            return "HEAD";
        case HttpMethod::Post:
            return "POST";
        case HttpMethod::Put:
            return "PUT";
        case HttpMethod::Delete:
            return "DELETE";
        case HttpMethod::Patch:
            return "PATCH";
        case HttpMethod::Options:
            return "OPTIONS";
        case HttpMethod::Connect:
            return "CONNECT";
        case HttpMethod::Trace:
            return "TRACE";
        case HttpMethod::Custom:
            return {};
        }

        return {};
    }

    [[nodiscard]] static std::string toStdString(HttpMethod method)
    {
        return std::string{toString(method)};
    }

    [[nodiscard]] static constexpr HttpMethod toMethod(std::string_view method) noexcept
    {
        if (method == "GET")
            return HttpMethod::Get;
        if (method == "HEAD")
            return HttpMethod::Head;
        if (method == "POST")
            return HttpMethod::Post;
        if (method == "PUT")
            return HttpMethod::Put;
        if (method == "DELETE")
            return HttpMethod::Delete;
        if (method == "PATCH")
            return HttpMethod::Patch;
        if (method == "OPTIONS")
            return HttpMethod::Options;
        if (method == "CONNECT")
            return HttpMethod::Connect;
        if (method == "TRACE")
            return HttpMethod::Trace;

        return HttpMethod::Custom;
    }

    [[nodiscard]] static constexpr HttpMethod toMethodStd(const std::string &method) noexcept
    {
        return toMethod(std::string_view{method});
    }
};

} // namespace job::net