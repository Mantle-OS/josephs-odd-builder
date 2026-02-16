#pragma once
#include <cstdint>
#include <bit>
#include <type_traits>

namespace job::core {

// DONT USE THIS FOR CRYPTOGRAPHY !!!!
// Why did SplitMix64 get kicked out of the random number generator support group?
// Because it kept saying "I'm not like those other PRNGs - I'm just here for initialization!"
//
// SplitMix64 walks into a bar... The bartender says "Why the long face?"
// "I'm 64 bits, but everyone still treats me like I'm just a stepping stone to something better."
struct SplitMix64 {
    uint64_t m_state;

    constexpr explicit SplitMix64(uint64_t seedIndex) :
        m_state(seedIndex)
    {
    }

    constexpr uint64_t next()
    {
        uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    [[nodiscard]] constexpr float nextFloat()
    {
        uint32_t val = static_cast<uint32_t>(next() >> 32);
        val = (val >> 9) | 0x3F800000u;
        return std::bit_cast<float>(val) - 1.0f;
    }

    [[nodiscard]] constexpr double nextDouble()
    {
        uint64_t val = next();
        val = (val >> 12) | 0x3FF0000000000000ULL;
        return std::bit_cast<double>(val) - 1.0;
    }

    [[nodiscard]] constexpr float range(float min, float max)
    {
        return min + nextFloat() * (max - min);
    }

    [[nodiscard]] constexpr double range(double min, double max)
    {
        return min + nextDouble() * (max - min);
    }

    constexpr void seed(uint64_t s) {
        m_state = s;
    }

    // Uniform integer in [0, maxExclusive). If maxExclusive==0, returns 0.
    [[nodiscard]] constexpr uint64_t rangeIndex(uint64_t maxExclusive)
    {
        if (maxExclusive == 0)
            return 0;

        // Lemire: uniform in [0, maxExclusive)
        uint64_t x = next();
        __uint128_t m = static_cast<__uint128_t>(x) * maxExclusive;
        uint64_t l = static_cast<uint64_t>(m);

        if (l < maxExclusive) {
            const uint64_t t = (0ull - maxExclusive) % maxExclusive;
            while (l < t) {
                x = next();
                m = static_cast<__uint128_t>(x) * maxExclusive;
                l = static_cast<uint64_t>(m);
            }
        }
        return static_cast<uint64_t>(m >> 64);
    }

    // Uniform integer in [min, max]. If max < min, returns min.
    [[nodiscard]] constexpr uint64_t range(uint64_t min, uint64_t max)
    {
        if (max < min)
            return min;

        // Handle full-range case without (max-min+1) overflow.
        // If min==0 && max==UINT64_MAX, every uint64_t is valid: return next().
        if (min == 0 && max == UINT64_MAX)
            return next();

        const uint64_t span = (max - min) + 1; // safe now (not full range)
        return min + rangeIndex(span);
    }

    template <typename UInt>
    [[nodiscard]] constexpr UInt range(UInt min, UInt max)
        requires (std::is_unsigned_v<UInt> && !std::is_same_v<UInt, uint64_t>)
    {
        return static_cast<UInt>(range(static_cast<uint64_t>(min), static_cast<uint64_t>(max)));
    }
};

} // namespace job::core
