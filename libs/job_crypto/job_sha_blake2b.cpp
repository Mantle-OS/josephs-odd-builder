#include "job_sha_blake2b.h"

#include <sodium.h>

#include "job_crypto_init.h"
#include "job_crypto_utils.h"

namespace job::crypto {

JobShaBlake2b::Hash JobShaBlake2b::compute(const void *data,
                                           std::size_t size,
                                           std::size_t hashSize) noexcept
{
    if (!JobCryptoInit::initialize())
        return {};

    if (!data && size > 0)
        return {};

    if (hashSize < kMinHashSize || hashSize > kMaxHashSize)
        return {};

    Hash hash(hashSize);

    if (crypto_generichash_blake2b(
            hash.data(),
            hash.size(),
            static_cast<const unsigned char *>(data),
            size,
            nullptr,
            0) != 0) {
        return {};
    }

    return hash;
}

JobShaBlake2b::Hash JobShaBlake2b::compute(std::string_view data,
                                           std::size_t hashSize) noexcept
{
    return compute(data.data(), data.size(), hashSize);
}

std::string JobShaBlake2b::computeHex(const void *data,
                                      std::size_t size,
                                      std::size_t hashSize)
{
    Hash const hash = compute(data, size, hashSize);

    if (hash.empty())
        return {};

    return utils::toHex(hash);
}

std::string JobShaBlake2b::computeHex(std::string_view data,
                                      std::size_t hashSize)
{
    return computeHex(data.data(), data.size(), hashSize);
}

} // namespace job::crypto