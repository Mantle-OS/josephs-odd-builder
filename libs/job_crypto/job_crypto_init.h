#pragma once

#include "jobcrypto_export.h"
namespace job::crypto {
class JOBCRYPTO_EXPORT JobCryptoInit {
public:
    static bool initialize() noexcept;
    static bool isInitialized() noexcept;
private:
    inline static bool s_initialized = false;
};
} // namespace job::crypto
