#include "job_hash.h"

#include <fstream>
#include <iostream>

#include <sodium.h>

#include "job_crypto_init.h"

namespace job::crypto {

std::vector<unsigned char> JobHash::hashBuffer(const std::vector<unsigned char> &data,
                                               std::size_t hashSize,
                                               const unsigned char *key,
                                               std::size_t keylen) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return {};

    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
#ifndef NDEBUG
        std::cerr << "[JobHash] Invalid requested hash byte dimensions: " << hashSize << "\n";
#endif
        return {};
    }

    // Validate libsodium key boundaries if a key context is provided
    if (key != nullptr) {
        if (keylen < crypto_generichash_KEYBYTES_MIN || keylen > crypto_generichash_KEYBYTES_MAX) {
#ifndef NDEBUG
            std::cerr << "[JobHash] Keyed MAC error: Provided key length (" << keylen
                      << ") falls outside permitted boundaries.\n";
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
        std::cerr << "[JobHash] Failed to compute buffer hash pass.\n";
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
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return {};

    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
#ifndef NDEBUG
        std::cerr << "[JobHash] Invalid requested hash byte dimensions: " << hashSize << "\n";
#endif
        return {};
    }

    // Validate libsodium key boundaries if a key context is provided
    if (key != nullptr) {
        if (keylen < crypto_generichash_KEYBYTES_MIN || keylen > crypto_generichash_KEYBYTES_MAX) {
#ifndef NDEBUG
            std::cerr << "[JobHash] Keyed MAC error: Provided key length (" << keylen
                      << ") falls outside permitted boundaries.\n";
#endif
            return {};
        }
    } else if (keylen > 0) {
        return {};
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
#ifndef NDEBUG
        std::cerr << "[JobHash] Cannot open file source for hashing loops: " << filePath << "\n";
#endif
        return {};
    }

    crypto_generichash_state state;
    crypto_generichash_init(&state, key, keylen, hashSize);

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
        std::cerr << "[JobHash] Disk read error while calculating signature hash for: " << filePath << "\n";
#endif
        return {};
    }

    std::vector<unsigned char> outHash(hashSize);
    crypto_generichash_final(&state, outHash.data(), outHash.size());

    return outHash;
}

} // namespace job::crypto