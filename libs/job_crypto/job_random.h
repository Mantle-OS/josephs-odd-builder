#pragma once

#include <cstdint>
#include <random>
#include <mutex>
#include <atomic>

#include <sodium.h>

#include <real_type.h>
#include <job_logger.h>

#include "job_crypto_init.h"

namespace job::crypto {

class JobRandom {
public:
    JobRandom() = delete;
    ~JobRandom() = delete;

    [[nodiscard]] static std::uint64_t secureU64()
    {
        initRandom();
        std::uint64_t x;
        randombytes_buf(&x, sizeof(x));
        return x;
    }

    static void secureBytes(void *buf, std::size_t size)
    {
        initRandom();
        randombytes_buf(buf, size);
    }

    static void setGlobalSeed(std::uint64_t seed)
    {
        m_globalRandomSeed.store(seed, std::memory_order_relaxed);
        m_useRandomGlobalSeed.store(true, std::memory_order_release);
    }

    static void disableGlobalSeed()
    {
        m_useRandomGlobalSeed.store(false, std::memory_order_release);
    }

    static std::mt19937_64 &engine(bool initSodium = false)
    {
        if(initSodium)
            initRandom();

        thread_local std::mt19937_64 eng(initThreadSeed());
        return eng;
    }

    [[nodiscard]] static float uniformReal(float a, float b)
    {
        std::uniform_real_distribution<float> dist(a, b);
        return dist(engine());
    }

    [[nodiscard]] static std::uint32_t uniformU32(std::uint32_t lo, std::uint32_t hi)
    {
        std::uniform_int_distribution<std::uint32_t> dist(lo, hi);
        return dist(engine());
    }

    [[nodiscard]] static float normal(float mean, float stddev)
    {
        std::normal_distribution<float> dist(mean, stddev);
        return dist(engine());
    }

    [[nodiscard]] static bool bernoulli(float p)
    {
        std::bernoulli_distribution dist(static_cast<double>(p));
        return dist(engine());
    }




private:
    inline static std::atomic<bool>          m_useRandomGlobalSeed;
    inline static std::atomic<std::uint64_t> m_globalRandomSeed;

    static void initRandom()
    {
        static std::once_flag flag;
        std::call_once(flag, []{
            if (!JobCryptoInit::initialize()) {
                JOB_LOG_ERROR("[JobRandom] libsodium initialization failed");
                std::abort();
            }
        });
    }

    static std::uint64_t initThreadSeed()
    {
        if (m_useRandomGlobalSeed.load(std::memory_order_acquire)) {
            std::uint64_t base = m_globalRandomSeed.load(std::memory_order_relaxed);
            auto tid = reinterpret_cast<std::uintptr_t>(&base);
            // ≈ 2⁶⁴ / φ, where φ = (1+√5)/2
            return base ^ (0x9e3779b97f4a7c15ULL + (tid << 6) + (tid >> 2));
        }

        // no global seed -> secure seed
        return secureU64();
    }

};

} // namespace job::crypto

