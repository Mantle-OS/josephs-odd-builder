#pragma once
#include <sodium.h>
class QSodiumInit {
public:
    static bool initialize() noexcept;
    static bool isInitialized() noexcept;
private:
    inline static bool s_initialized = false;
};
