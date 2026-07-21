#include "job_crypto_init.h"

#include <sodium.h>

#ifndef NDEBUG
#include <job_logger.h>
#endif

namespace job::crypto {

bool JobCryptoInit::initialize() noexcept
{
    if (s_initialized)
        return true;

    if (sodium_init() < 0) {

#ifndef NDEBUG
        JOB_LOG_ERROR("[JobCryptoInit] libsodium initialization failed!");
#endif
        return false;
    }

    s_initialized = true;
#ifndef NDEBUG
    JOB_LOG_DEBUG("[JobCryptoInit] libsodium initialized.");
#endif
    return true;
}

bool JobCryptoInit::isInitialized() noexcept
{
    return s_initialized;
}

} // namespace job::crypto

