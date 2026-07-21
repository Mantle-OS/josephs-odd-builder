#pragma once

#include <cstdint>
#include <cstddef>

#include <QByteArray>

#include <job_random.h>

#include "qsodium_export.h"

class QSODIUM_EXPORT QExtraRandom {
public:
    QExtraRandom() = delete;
    ~QExtraRandom() = delete;

    [[nodiscard]] static std::uint64_t secureU64();
    static void secureBytes(void *buf, std::size_t size);

    static void setGlobalSeed(std::uint64_t seed);
    static void disableGlobalSeed();

    [[nodiscard]] static float uniformReal(float a, float b);
    [[nodiscard]] static std::uint32_t uniformU32(std::uint32_t lo, std::uint32_t hi);
    [[nodiscard]] static float normal(float mean, float stddev);
    [[nodiscard]] static bool bernoulli(float p);
    [[nodiscard]] static QByteArray randomSalt() noexcept;
};