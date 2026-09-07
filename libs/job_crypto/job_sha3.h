#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "job_sha_types.h"
#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha3
{
public:
    using Hash = std::vector<unsigned char>;

    JobSha3() = delete;
    ~JobSha3() = delete;

    JobSha3(const JobSha3 &) = delete;
    JobSha3 &operator=(const JobSha3 &) = delete;
    JobSha3(JobSha3 &&) = delete;
    JobSha3 &operator=(JobSha3 &&) = delete;

    [[nodiscard]] static Hash compute(JobShaType type, const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(JobShaType type, std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(JobShaType type, const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(JobShaType type, std::string_view data);

    [[nodiscard]] static constexpr std::size_t hashSize(JobShaType type) noexcept
    {
        switch (type) {
        case JobShaType::Sha3_224:
            return 28;
        case JobShaType::Sha3_256:
            return 32;
        case JobShaType::Sha3_384:
            return 48;
        case JobShaType::Sha3_512:
            return 64;

        case JobShaType::Sha224:
        case JobShaType::Sha256:
        case JobShaType::Sha384:
        case JobShaType::Sha512:
        case JobShaType::ShaGeneric:
        case JobShaType::ShaBlake2b:
        case JobShaType::ShaHmac256:
        case JobShaType::Unknown:
        default:
            return 0;
        }
    }

    [[nodiscard]] static constexpr bool supports(JobShaType type) noexcept
    {
        return hashSize(type) != 0;
    }
};

} // namespace job::crypto