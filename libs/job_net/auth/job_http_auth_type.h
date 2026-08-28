#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace job::net {

enum class JobHttpAuthType : std::uint8_t
{
    Bearer = 0,
    Token,
    ApiKey,
    Basic,
    Aws4HmacSha256,
    Custom,

    Unknown
};

[[nodiscard]] constexpr std::string_view toString(JobHttpAuthType type) noexcept
{
    switch (type) {
    case JobHttpAuthType::Bearer:
        return "Bearer";
    case JobHttpAuthType::Token:
        return "Token";
    case JobHttpAuthType::ApiKey:
        return "ApiKey";
    case JobHttpAuthType::Basic:
        return "Basic";
    case JobHttpAuthType::Aws4HmacSha256:
        return "AWS4-HMAC-SHA256";
    case JobHttpAuthType::Custom:
        return "Custom";
    case JobHttpAuthType::Unknown:
        return {};
    }

    return {};
}

[[nodiscard]] inline std::string toStdString(JobHttpAuthType type)
{
    return std::string{toString(type)};
}

[[nodiscard]] constexpr JobHttpAuthType toAuthType(std::string_view type) noexcept
{
    if (type == "Bearer")
        return JobHttpAuthType::Bearer;
    if (type == "Token")
        return JobHttpAuthType::Token;
    if (type == "ApiKey")
        return JobHttpAuthType::ApiKey;
    if (type == "Basic")
        return JobHttpAuthType::Basic;
    if (type == "AWS4-HMAC-SHA256")
        return JobHttpAuthType::Aws4HmacSha256;
    if (type == "Custom")
        return JobHttpAuthType::Custom;

    return JobHttpAuthType::Unknown;
}

[[nodiscard]] constexpr JobHttpAuthType toAuthType(const std::string &type) noexcept
{
    return toAuthType(std::string_view{type});
}

} // namespace job::net