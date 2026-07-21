#include "job_hash.h"

#include <fstream>
#include <iostream>

#include <sodium.h>

#include "job_crypto_init.h"

#include <job_logger.h>

namespace job::crypto {

std::vector<unsigned char> JobHash::hashBuffer(const std::vector<unsigned char> &data,
                                               std::size_t hashSize,
                                               const unsigned char *key,
                                               std::size_t keylen) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize()){
        JOB_LOG_DEBUG(
            "[JobHash] Buffer hashing stopped because the crypto runtime is unavailable."
            );
        return {};
    }

    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHash] Invalid requested hash byte dimensions: {}", hashSize);
#endif
        return {};
    }

    // Validate libsodium key boundaries if a key context is provided
    if (key != nullptr) {
        if (keylen < crypto_generichash_KEYBYTES_MIN || keylen > crypto_generichash_KEYBYTES_MAX) {
#ifndef NDEBUG
            JOB_LOG_ERROR(
                "[JobHash] Keyed MAC error: Provided key length ({}) "
                "falls outside permitted boundaries.",
                keylen
                );
#endif
            return {};
        }
    } else if (keylen > 0) {
        return {}; // Safety bounce: Size declared with a null pointer handle
    }

    std::vector<unsigned char> outHash(hashSize);

    int const result = crypto_generichash(
        outHash.data(), outHash.size(),
        data.data(), data.size(),
        key, keylen
        );

    if (result != 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHash] Failed to compute buffer hash pass.");
#endif
        return {};
    }

    return outHash;
}

std::vector<unsigned char> JobHash::hashFile(const std::string &filePath,
                                             std::size_t hashSize,
                                             const unsigned char *key,
                                             std::size_t keylen) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize()){
        JOB_LOG_DEBUG(
            "[JobHash] File hashing stopped because the crypto runtime is unavailable: {}",
            filePath
            );
        return {};
    }

    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobHash] Invalid requested hash byte dimensions: {}", hashSize );
#endif
        return {};
    }

    // Validate libsodium key boundaries if a key context is provided
    if (key != nullptr) {
        if (keylen < crypto_generichash_KEYBYTES_MIN || keylen > crypto_generichash_KEYBYTES_MAX) {
#ifndef NDEBUG
            JOB_LOG_ERROR(
                "[JobHash] Keyed MAC error: Provided key length ({}) "
                "falls outside permitted boundaries.",
                keylen
                );
#endif
            return {};
        }
    } else if (keylen > 0) {
        return {};
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
#ifndef NDEBUG
        JOB_LOG_ERROR(
            "[JobHash] Cannot open file source for hashing loops: {}",
            filePath
            );
#endif
        return {};
    }

    crypto_generichash_state state;

    if (crypto_generichash_init( &state, key, keylen, hashSize ) != 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR(
            "[JobHash] Failed to initialize file hashing state."
            );
#endif
        return {};
    }

    std::vector<char> buffer(kChunkSize);
    while (file.good()) {
        file.read(buffer.data(), kChunkSize);
        std::streamsize const bytesRead = file.gcount();
        if (bytesRead > 0) {
            crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(buffer.data()), bytesRead);
        }
    }

    if (file.bad()) {
#ifndef NDEBUG
        JOB_LOG_ERROR(
            "[JobHash] Disk read error while calculating signature hash for: {}",
            filePath
            );
#endif
        return {};
    }

    std::vector<unsigned char> outHash(hashSize);
    crypto_generichash_final(&state, outHash.data(), outHash.size());

    return outHash;
}

} // namespace job::crypto