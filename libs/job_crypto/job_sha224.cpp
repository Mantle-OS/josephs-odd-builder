#include "job_sha224.h"

#include <openssl/evp.h>

#include "job_crypto_utils.h"

namespace job::crypto {

JobSha224::Hash JobSha224::compute(const void *data, std::size_t size) noexcept
{
    Hash hash{};

    if (!data && size > 0)
        return hash;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
        return hash;

    unsigned int hashLength = 0;

    if (EVP_DigestInit_ex(ctx, EVP_sha224(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data, size) != 1 ||
        EVP_DigestFinal_ex(ctx, hash.data(), &hashLength) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    EVP_MD_CTX_free(ctx);

    return hash;
}

JobSha224::Hash JobSha224::compute(std::string_view data) noexcept
{
    return compute(data.data(), data.size());
}

std::string JobSha224::computeHex(const void *data, std::size_t size)
{
    Hash const hash = compute(data, size);
    return utils::toHex(hash);
}

std::string JobSha224::computeHex(std::string_view data)
{
    return computeHex(data.data(), data.size());
}

} // namespace job::crypto