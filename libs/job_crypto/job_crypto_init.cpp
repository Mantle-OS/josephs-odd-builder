#include "job_crypto_init.h"
#include <iostream>

namespace job::crypto {

bool JobCryptoInit::initialize() noexcept
{
    if (s_initialized)
        return true;

    if (sodium_init() < 0) {
        std::cerr << "[JobCryptoInit] libsodium initialization failed!\n";
        return false;
    }

    s_initialized = true;
    std::cout << "[JobCryptoInit] libsodium initialized.\n";
    return true;
}

bool JobCryptoInit::isInitialized() noexcept
{
    return s_initialized;
}

} // namespace job::crypto
