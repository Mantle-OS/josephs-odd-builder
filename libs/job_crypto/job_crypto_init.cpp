#include "job_crypto_init.h"

#ifndef NDEBUG
#include <iostream>
#endif

namespace job::crypto {

bool JobCryptoInit::initialize() noexcept
{
    if (s_initialized)
        return true;

    if (sodium_init() < 0) {

#ifndef NDEBUG
        std::cerr << "[JobCryptoInit] libsodium initialization failed!\n";
#endif
        return false;
    }

    s_initialized = true;
#ifndef NDEBUG
    std::cout << "[JobCryptoInit] libsodium initialized.\n";
#endif
    return true;
}

bool JobCryptoInit::isInitialized() noexcept
{
    return s_initialized;
}

} // namespace job::crypto

