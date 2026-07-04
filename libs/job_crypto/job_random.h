#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace job::crypto {

class JobRandom {
public:
    JobRandom() = delete;
    ~JobRandom() = delete;

    [[nodiscard]] static std::uint64_t secureU64();
    static void secureBytes(void *buf, std::size_t size);

    static void setGlobalSeed(std::uint64_t seed);
    static void disableGlobalSeed();

    static std::mt19937_64 &engine(bool initSodium = false);

    [[nodiscard]] static float uniformReal(float a, float b);
    [[nodiscard]] static std::uint32_t uniformU32(std::uint32_t lo, std::uint32_t hi);
    [[nodiscard]] static float normal(float mean, float stddev);
    [[nodiscard]] static bool bernoulli(float p);

    [[nodiscard]] static std::vector<std::uint8_t> randomSalt() noexcept;

private:
    static void initRandom();
    [[nodiscard]] static std::uint64_t initThreadSeed();
};

} // namespace job::crypto