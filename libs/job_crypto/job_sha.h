#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "job_sha_types.h"
#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha
{
public:
    using Hash = std::vector<unsigned char>;

    JobSha() = delete;
    ~JobSha() = delete;

    JobSha(const JobSha &) = delete;
    JobSha &operator=(const JobSha &) = delete;
    JobSha(JobSha &&) = delete;
    JobSha &operator=(JobSha &&) = delete;

    [[nodiscard]] static Hash compute(JobShaType type, const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(JobShaType type, std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(JobShaType type, const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(JobShaType type, std::string_view data);

    [[nodiscard]] static constexpr bool supports(JobShaType type) noexcept
    {
        switch (type) {
        case JobShaType::Sha224:
        case JobShaType::Sha256:
        case JobShaType::Sha384:
        case JobShaType::Sha512:
        case JobShaType::Sha3_224:
        case JobShaType::Sha3_256:
        case JobShaType::Sha3_384:
        case JobShaType::Sha3_512:
        case JobShaType::ShaGeneric:
        case JobShaType::ShaBlake2b:
            return true;

        case JobShaType::ShaHmac256:
        case JobShaType::Unknown:
        default:
            return false;
        }
    }

    [[nodiscard]] static constexpr bool requiresKey(JobShaType type) noexcept
    {
        return type == JobShaType::ShaHmac256;
    }

    [[nodiscard]] static constexpr bool variableSize(JobShaType type) noexcept
    {
        return type == JobShaType::ShaGeneric ||
               type == JobShaType::ShaBlake2b;
    }

    [[nodiscard]] static constexpr std::size_t hashSize(JobShaType type) noexcept
    {
        switch (type) {
        case JobShaType::Sha224:
        case JobShaType::Sha3_224:
            return 28;

        case JobShaType::Sha256:
        case JobShaType::Sha3_256:
        case JobShaType::ShaGeneric:
        case JobShaType::ShaBlake2b:
            return 32;

        case JobShaType::Sha384:
        case JobShaType::Sha3_384:
            return 48;

        case JobShaType::Sha512:
        case JobShaType::Sha3_512:
            return 64;

        case JobShaType::ShaHmac256:
            return 32;

        case JobShaType::Unknown:
        default:
            return 0;
        }
    }
};

} // namespace job::crypto