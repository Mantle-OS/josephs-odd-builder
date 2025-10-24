#pragma once
#include <sodium.h>

namespace job::crypto {

class JobCryptoInit {
public:
    static bool initialize() noexcept;
    static bool isInitialized() noexcept;

private:
    inline static bool s_initialized = false;
};

} // namespace job::crypto
