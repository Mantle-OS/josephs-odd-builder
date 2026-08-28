#include "job_hmac_sha256.h"

#include <cstring>

#include <sodium.h>

#include <job_logger.h>

#include "job_crypto_init.h"

namespace job::crypto {

JobSecureMem JobHmacSha256::generateKey() noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] Key generation stopped because the crypto runtime is unavailable.");
#endif
        return {};
    }

    JobSecureMem key{kKeySize};
    if (key.empty()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] Failed to allocate secure memory for generated HMAC-SHA256 key.");
#endif
        return {};
    }

    crypto_auth_hmacsha256_keygen(key.data());
    return key;
}

JobHmacSha256::Mac JobHmacSha256::compute(const void *data,
                                          std::size_t dataSize,
                                          const void *key,
                                          std::size_t keySize) noexcept
{
    Mac mac{};

    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] HMAC computation stopped because the crypto runtime is unavailable.");
#endif
        return mac;
    }

    if (!key && keySize > 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] HMAC computation rejected a null key with a non-zero key size.");
#endif
        return mac;
    }

    if (!data && dataSize > 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] HMAC computation rejected null data with a non-zero data size.");
#endif
        return mac;
    }

    crypto_auth_hmacsha256_state state;

    if (crypto_auth_hmacsha256_init(
            &state,
            static_cast<const unsigned char *>(key),
            keySize) != 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] Failed to initialize HMAC-SHA256 state.");
#endif
        sodium_memzero(&state, sizeof(state));
        return mac;
    }

    if (dataSize > 0) {
        if (crypto_auth_hmacsha256_update(
                &state,
                static_cast<const unsigned char *>(data),
                static_cast<unsigned long long>(dataSize)) != 0) {
#ifndef NDEBUG
            JOB_LOG_ERROR("[JobHmacSha256] Failed to update HMAC-SHA256 state.");
#endif
            sodium_memzero(&state, sizeof(state));
            return {};
        }
    }

    if (crypto_auth_hmacsha256_final(&state, mac.data()) != 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] Failed to finalize HMAC-SHA256 state.");
#endif
        sodium_memzero(&state, sizeof(state));
        mac.fill(0);
        return mac;
    }

    sodium_memzero(&state, sizeof(state));
    return mac;
}

JobHmacSha256::Mac JobHmacSha256::compute(const void *data,
                                          std::size_t dataSize,
                                          const JobSecureMem &key) noexcept
{
    return compute(data, dataSize, key.data(), key.size());
}

JobHmacSha256::Mac JobHmacSha256::compute(std::string_view data,
                                          const JobSecureMem &key) noexcept
{
    return compute(data.data(), data.size(), key);
}

JobSecureMem JobHmacSha256::computeSecure(const void *data,
                                          std::size_t dataSize,
                                          const void *key,
                                          std::size_t keySize) noexcept
{
    Mac mac = compute(data, dataSize, key, keySize);

    JobSecureMem secureMac{kMacSize};
    if (secureMac.empty()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHmacSha256] Failed to allocate secure memory for HMAC-SHA256 result.");
#endif
        sodium_memzero(mac.data(), mac.size());
        return {};
    }

    std::memcpy(secureMac.data(), mac.data(), mac.size());
    sodium_memzero(mac.data(), mac.size());

    return secureMac;
}

JobSecureMem JobHmacSha256::computeSecure(const void *data,
                                          std::size_t dataSize,
                                          const JobSecureMem &key) noexcept
{
    return computeSecure(data, dataSize, key.data(), key.size());
}

JobSecureMem JobHmacSha256::computeSecure(std::string_view data,
                                          const JobSecureMem &key) noexcept
{
    return computeSecure(data.data(), data.size(), key);
}

bool JobHmacSha256::verify(const Mac &mac,
                           const void *data,
                           std::size_t dataSize,
                           const void *key,
                           std::size_t keySize) noexcept
{
    Mac const computed = compute(data, dataSize, key, keySize);
    return sodium_memcmp(mac.data(), computed.data(), kMacSize) == 0;
}

bool JobHmacSha256::verify(const Mac &mac,
                           const void *data,
                           std::size_t dataSize,
                           const JobSecureMem &key) noexcept
{
    return verify(mac, data, dataSize, key.data(), key.size());
}

bool JobHmacSha256::verify(const Mac &mac,
                           std::string_view data,
                           const JobSecureMem &key) noexcept
{
    return verify(mac, data.data(), data.size(), key);
}

} // namespace job::crypto