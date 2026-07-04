#include "job_random.h"

#include <atomic>
#include <iostream>
#include <mutex>

#include <sodium.h>

#include "job_crypto_init.h"

namespace job::crypto {

namespace {
std::atomic<bool>           g_useRandomGlobalSeed{false};
std::atomic<std::uint64_t>  g_globalRandomSeed{0};
}

void JobRandom::initRandom()
{
    static std::once_flag flag;
    std::call_once(flag, []{
        if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize()) {
            std::cerr << "[jobcrypto::JobRandom] CRITICAL ERROR: libsodium runtime failed initialization.\n";
            std::abort();
        }
#ifndef NDEBUG
        std::clog << "[jobcrypto::JobRandom] Random sub-engine validation pass complete.\n";
#endif
    });
}

std::uint64_t JobRandom::secureU64()
{
    initRandom();
    std::uint64_t x;
    randombytes_buf(&x, sizeof(x));
    return x;
}

void JobRandom::secureBytes(void *buf, std::size_t size)
{
    initRandom();
    randombytes_buf(buf, size);
}

void JobRandom::setGlobalSeed(std::uint64_t seed)
{
    g_globalRandomSeed.store(seed, std::memory_order_relaxed);
    g_useRandomGlobalSeed.store(true, std::memory_order_release);
}

void JobRandom::disableGlobalSeed()
{
    g_useRandomGlobalSeed.store(false, std::memory_order_release);
}

std::uint64_t JobRandom::initThreadSeed()
{
    if (g_useRandomGlobalSeed.load(std::memory_order_acquire)) {
        std::uint64_t base = g_globalRandomSeed.load(std::memory_order_relaxed);
        auto tid = reinterpret_cast<std::uintptr_t>(&base);
        // ≈ 2⁶⁴ / φ, where φ = (1+√5)/2
        return base ^ (0x9e3779b97f4a7c15ULL + (tid << 6) + (tid >> 2));
    }
    return secureU64();
}

std::mt19937_64 &JobRandom::engine(bool initSodium)
{
    if (initSodium) {
        initRandom();
    }
    thread_local std::mt19937_64 eng(initThreadSeed());
    return eng;
}

float JobRandom::uniformReal(float a, float b)
{
    std::uniform_real_distribution<float> dist(a, b);
    return dist(engine());
}

std::uint32_t JobRandom::uniformU32(std::uint32_t lo, std::uint32_t hi)
{
    std::uniform_int_distribution<std::uint32_t> dist(lo, hi);
    return dist(engine());
}

float JobRandom::normal(float mean, float stddev)
{
    std::normal_distribution<float> dist(mean, stddev);
    return dist(engine());
}

bool JobRandom::bernoulli(float p)
{
    std::bernoulli_distribution dist(static_cast<double>(p));
    return dist(engine());
}

std::vector<std::uint8_t> JobRandom::randomSalt() noexcept
{
    initRandom();
    std::vector<std::uint8_t> salt(crypto_pwhash_SALTBYTES);
    randombytes_buf(salt.data(), salt.size());
    return salt;
}

} // namespace job::crypto