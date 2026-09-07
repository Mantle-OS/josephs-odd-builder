#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace job::crypto {

enum class JobShaType : uint8_t
{
    Sha224 = 0,
    Sha256,
    Sha384,
    Sha512,
    Sha3_224,
    Sha3_256,
    Sha3_384,
    Sha3_512,
    ShaGeneric,
    ShaBlake2b,
    ShaHmac256,
    Unknown
};

[[nodiscard]] constexpr std::string_view toString(JobShaType type) noexcept
{
    switch (type) {
    case JobShaType::Sha224:
        return "SHA-224";
    case JobShaType::Sha256:
        return "SHA-256";
    case JobShaType::Sha384:
        return "SHA-384";
    case JobShaType::Sha512:
        return "SHA-512";
    case JobShaType::Sha3_224:
        return "SHA3-224";
    case JobShaType::Sha3_256:
        return "SHA3-256";
    case JobShaType::Sha3_384:
        return "SHA3-384";
    case JobShaType::Sha3_512:
        return "SHA3-512";
    case JobShaType::ShaGeneric:
        return "GENERIC";
    case JobShaType::ShaBlake2b:
        return "BLAKE2B";
    case JobShaType::ShaHmac256:
        return "HMAC-SHA256";
    case JobShaType::Unknown:
    default:
        return {};
    }
}

[[nodiscard]] inline std::string toStdString(JobShaType type)
{
    return std::string{toString(type)};
}

[[nodiscard]] constexpr JobShaType toShaType(std::string_view value) noexcept
{
    if (value == "SHA-224")
        return JobShaType::Sha224;
    if (value == "SHA-256")
        return JobShaType::Sha256;
    if (value == "SHA-384")
        return JobShaType::Sha384;
    if (value == "SHA-512")
        return JobShaType::Sha512;

    if (value == "SHA3-224")
        return JobShaType::Sha3_224;
    if (value == "SHA3-256")
        return JobShaType::Sha3_256;
    if (value == "SHA3-384")
        return JobShaType::Sha3_384;
    if (value == "SHA3-512")
        return JobShaType::Sha3_512;

    if (value == "GENERIC")
        return JobShaType::ShaGeneric;
    if (value == "BLAKE2B")
        return JobShaType::ShaBlake2b;
    if (value == "HMAC-SHA256")
        return JobShaType::ShaHmac256;

    return JobShaType::Unknown;
}

[[nodiscard]] constexpr JobShaType toShaType(const char *value) noexcept
{
    return value ? toShaType(std::string_view{value}) : JobShaType::Unknown;
}

[[nodiscard]] inline JobShaType toShaType(const std::string &value) noexcept
{
    return toShaType(std::string_view{value});
}

} // namespace job::crypto