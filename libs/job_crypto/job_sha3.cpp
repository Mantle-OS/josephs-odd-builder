#include "job_sha3.h"

#include <openssl/evp.h>

#include <job_logger.h>

#include "job_crypto_utils.h"

namespace job::crypto {

JobSha3::Hash JobSha3::compute(JobShaType type, const void *data, std::size_t size) noexcept
{
    if (!data && size > 0) {
        JOB_LOG_ERROR("[JobSha3] Invalid data pointer with non-zero size");
        return {};
    }

    if (!supports(type)) {
        JOB_LOG_ERROR("[JobSha3] Unsupported SHA type: {}", toString(type));
        return {};
    }

    const EVP_MD *md = nullptr;

    switch (type) {
    case JobShaType::Sha3_224:
        md = EVP_sha3_224();
        break;
    case JobShaType::Sha3_256:
        md = EVP_sha3_256();
        break;
    case JobShaType::Sha3_384:
        md = EVP_sha3_384();
        break;
    case JobShaType::Sha3_512:
        md = EVP_sha3_512();
        break;

    case JobShaType::Sha224:
    case JobShaType::Sha256:
    case JobShaType::Sha384:
    case JobShaType::Sha512:
    case JobShaType::ShaGeneric:
    case JobShaType::ShaBlake2b:
    case JobShaType::ShaHmac256:
    case JobShaType::Unknown:
    default:
        JOB_LOG_ERROR("[JobSha3] Unsupported SHA type: {}", toString(type));
        return {};
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        JOB_LOG_ERROR("[JobSha3] Failed to create OpenSSL digest context");
        return {};
    }

    Hash hash(hashSize(type));
    unsigned int hashLength = 0;

    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data, size) != 1 ||
        EVP_DigestFinal_ex(ctx, hash.data(), &hashLength) != 1) {
        JOB_LOG_ERROR("[JobSha3] SHA3 digest operation failed for type: {}", toString(type));
        EVP_MD_CTX_free(ctx);
        return {};
    }

    EVP_MD_CTX_free(ctx);

    if (hashLength != hash.size()) {
        JOB_LOG_ERROR("[JobSha3] Unexpected digest size for type '{}': expected {}, received {}",
                      toString(type), hash.size(), hashLength);
        return {};
    }

    return hash;
}

JobSha3::Hash JobSha3::compute(JobShaType type, std::string_view data) noexcept
{
    return compute(type, data.data(), data.size());
}

std::string JobSha3::computeHex(JobShaType type, const void *data, std::size_t size)
{
    Hash const hash = compute(type, data, size);

    if (hash.empty())
        return {};

    return utils::toHex(hash);
}

std::string JobSha3::computeHex(JobShaType type, std::string_view data)
{
    return computeHex(type, data.data(), data.size());
}

} // namespace job::crypto