#include "job_sha.h"

#include <job_logger.h>

#include "job_sha224.h"
#include "job_sha256.h"
#include "job_sha384.h"
#include "job_sha512.h"
#include "job_sha3.h"
#include "job_sha_blake2b.h"
#include "job_sha_generic.h"

namespace job::crypto {

JobSha::Hash JobSha::compute(JobShaType type, const void *data, std::size_t size) noexcept
{
    if (!data && size > 0) {
        JOB_LOG_ERROR("[JobSha] Invalid data pointer with non-zero size");
        return {};
    }

    switch (type) {
    case JobShaType::Sha224: {
        auto const hash = JobSha224::compute(data, size);
        return Hash{hash.begin(), hash.end()};
    }

    case JobShaType::Sha256: {
        auto const hash = JobSha256::compute(data, size);
        return Hash{hash.begin(), hash.end()};
    }

    case JobShaType::Sha384: {
        auto const hash = JobSha384::compute(data, size);
        return Hash{hash.begin(), hash.end()};
    }

    case JobShaType::Sha512: {
        auto const hash = JobSha512::compute(data, size);
        return Hash{hash.begin(), hash.end()};
    }

    case JobShaType::Sha3_224:
    case JobShaType::Sha3_256:
    case JobShaType::Sha3_384:
    case JobShaType::Sha3_512:
        return JobSha3::compute(type, data, size);

    case JobShaType::ShaGeneric:
        return JobShaGeneric::compute(data, size);

    case JobShaType::ShaBlake2b:
        return JobShaBlake2b::compute(data, size);

    case JobShaType::ShaHmac256:
        JOB_LOG_ERROR("[JobSha] HMAC-SHA256 requires a key; use JobHmacSha256");
        return {};

    case JobShaType::Unknown:
    default:
        JOB_LOG_ERROR("[JobSha] Unsupported SHA type: {}", toString(type));
        return {};
    }
}

JobSha::Hash JobSha::compute(JobShaType type, std::string_view data) noexcept
{
    return compute(type, data.data(), data.size());
}

std::string JobSha::computeHex(JobShaType type, const void *data, std::size_t size)
{
    if (!data && size > 0) {
        JOB_LOG_ERROR("[JobSha] Invalid data pointer with non-zero size");
        return {};
    }

    switch (type) {
    case JobShaType::Sha224:
        return JobSha224::computeHex(data, size);

    case JobShaType::Sha256:
        return JobSha256::computeHex(data, size);

    case JobShaType::Sha384:
        return JobSha384::computeHex(data, size);

    case JobShaType::Sha512:
        return JobSha512::computeHex(data, size);

    case JobShaType::Sha3_224:
    case JobShaType::Sha3_256:
    case JobShaType::Sha3_384:
    case JobShaType::Sha3_512:
        return JobSha3::computeHex(type, data, size);

    case JobShaType::ShaGeneric:
        return JobShaGeneric::computeHex(data, size);

    case JobShaType::ShaBlake2b:
        return JobShaBlake2b::computeHex(data, size);

    case JobShaType::ShaHmac256:
        JOB_LOG_ERROR("[JobSha] HMAC-SHA256 requires a key; use JobHmacSha256");
        return {};

    case JobShaType::Unknown:
    default:
        JOB_LOG_ERROR("[JobSha] Unsupported SHA type: {}", toString(type));
        return {};
    }
}

std::string JobSha::computeHex(JobShaType type, std::string_view data)
{
    return computeHex(type, data.data(), data.size());
}

} // namespace job::crypto