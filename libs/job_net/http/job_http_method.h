#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace job::net {

enum class JobHttpMethod : std::uint8_t
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

[[nodiscard]] constexpr std::string_view toString(JobHttpMethod method) noexcept
{
    switch (method) {
    case JobHttpMethod::Get:
        return "GET";
    case JobHttpMethod::Head:
        return "HEAD";
    case JobHttpMethod::Post:
        return "POST";
    case JobHttpMethod::Put:
        return "PUT";
    case JobHttpMethod::Delete:
        return "DELETE";
    case JobHttpMethod::Patch:
        return "PATCH";
    case JobHttpMethod::Options:
        return "OPTIONS";
    case JobHttpMethod::Connect:
        return "CONNECT";
    case JobHttpMethod::Trace:
        return "TRACE";
    case JobHttpMethod::Custom:
        return {};
    }

    return {};
}

[[nodiscard]] inline std::string toStdString(JobHttpMethod method)
{
    return std::string{toString(method)};
}

[[nodiscard]] constexpr JobHttpMethod toMethod(std::string_view method) noexcept
{
    if (method == "GET")
        return JobHttpMethod::Get;
    if (method == "HEAD")
        return JobHttpMethod::Head;
    if (method == "POST")
        return JobHttpMethod::Post;
    if (method == "PUT")
        return JobHttpMethod::Put;
    if (method == "DELETE")
        return JobHttpMethod::Delete;
    if (method == "PATCH")
        return JobHttpMethod::Patch;
    if (method == "OPTIONS")
        return JobHttpMethod::Options;
    if (method == "CONNECT")
        return JobHttpMethod::Connect;
    if (method == "TRACE")
        return JobHttpMethod::Trace;

    return JobHttpMethod::Custom;
}

[[nodiscard]] constexpr JobHttpMethod toMethod(const std::string &method) noexcept
{
    return toMethod(std::string_view{method});
}

} // namespace job::net
