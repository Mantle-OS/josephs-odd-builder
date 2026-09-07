#include "job_sha512.h"

#include <sodium.h>

#include "job_crypto_init.h"
#include "job_crypto_utils.h"

namespace job::crypto {

JobSha512::Hash JobSha512::compute(const void *data, std::size_t size) noexcept
{
    Hash hash{};

    if (!JobCryptoInit::initialize())
        return hash;

    if (!data && size > 0)
        return hash;

    crypto_hash_sha512(hash.data(), static_cast<const unsigned char *>(data), size);

    return hash;
}

JobSha512::Hash JobSha512::compute(std::string_view data) noexcept
{
    return compute(data.data(), data.size());
}

std::string JobSha512::computeHex(const void *data, std::size_t size)
{
    Hash const hash = compute(data, size);
    return utils::toHex(hash);
}

std::string JobSha512::computeHex(std::string_view data)
{
    return computeHex(data.data(), data.size());
}

} // namespace job::crypto