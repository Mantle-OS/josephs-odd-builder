#pragma once

#include <QByteArray>
#include <cstdint>
#include <cstddef>
#include <job_random.h>

class QExtraRandom {
public:
    QExtraRandom() = delete;
    ~QExtraRandom() = delete;

    [[nodiscard]] static std::uint64_t secureU64() { return job::crypto::JobRandom::secureU64(); }
    static void secureBytes(void *buf, std::size_t size) { job::crypto::JobRandom::secureBytes(buf, size); }

    static void setGlobalSeed(std::uint64_t seed) { job::crypto::JobRandom::setGlobalSeed(seed); }
    static void disableGlobalSeed() { job::crypto::JobRandom::disableGlobalSeed(); }

    [[nodiscard]] static float uniformReal(float a, float b) { return job::crypto::JobRandom::uniformReal(a, b); }
    [[nodiscard]] static std::uint32_t uniformU32(std::uint32_t lo, std::uint32_t hi) { return job::crypto::JobRandom::uniformU32(lo, hi); }
    [[nodiscard]] static float normal(float mean, float stddev) { return job::crypto::JobRandom::normal(mean, stddev); }
    [[nodiscard]] static bool bernoulli(float p) { return job::crypto::JobRandom::bernoulli(p); }

    [[nodiscard]] static QByteArray randomSalt() noexcept
    {
        auto const nativeSalt = job::crypto::JobRandom::randomSalt();
        return QByteArray(reinterpret_cast<const char*>(nativeSalt.data()), static_cast<int>(nativeSalt.size()));
    }
};