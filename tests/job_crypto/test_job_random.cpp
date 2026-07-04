#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

#include <sodium.h>

#include "job_random.h"

using namespace job::crypto;

namespace job::crypto::tests {

[[nodiscard]] constexpr bool isSafeFinite(float f) noexcept
{
    std::uint32_t u = std::bit_cast<std::uint32_t>(f);
    std::uint32_t exponent = (u & 0x7F800000) >> 23;
    return exponent != 255;
}

} // namespace job::crypto::tests

TEST_CASE("JobRandom::secureU64 produces non-trivial randomness", "[job_random][secure]")
{
    constexpr std::size_t N = 16;
    std::vector<std::uint64_t> values;
    values.reserve(N);

    for (std::size_t i = 0; i < N; ++i) {
        values.push_back(JobRandom::secureU64());
    }

    std::set<std::uint64_t> unique(values.begin(), values.end());
    REQUIRE(unique.size() > 1);
}

TEST_CASE("JobRandom::secureBytes fills a buffer", "[job_random][secure]")
{
    constexpr std::size_t N = 32;
    std::array<std::uint8_t, N> buf1{};
    std::array<std::uint8_t, N> buf2{};

    JobRandom::secureBytes(buf1.data(), buf1.size());
    JobRandom::secureBytes(buf2.data(), buf2.size());

    bool any_non_zero = false;
    for (auto b : buf1) {
        if (b != 0) {
            any_non_zero = true;
            break;
        }
    }
    REQUIRE(any_non_zero);

    bool all_equal = true;
    for (std::size_t i = 0; i < N; ++i) {
        if (buf1[i] != buf2[i]) {
            all_equal = false;
            break;
        }
    }
    REQUIRE_FALSE(all_equal);
}

TEST_CASE("JobRandom::uniformReal stays within bounds", "[job_random][prng]")
{
    constexpr std::size_t N = 10'000;
    const float a = -3.5f;
    const float b = 7.25f;

    float min_seen = b;
    float max_seen = a;

    for (std::size_t i = 0; i < N; ++i) {
        float v = JobRandom::uniformReal(a, b);
        REQUIRE(v >= a);
        REQUIRE(v <  b);

        if (v < min_seen) min_seen = v;
        if (v > max_seen) max_seen = v;
    }

    REQUIRE(min_seen <= a + (b - a) * 0.1f);
    REQUIRE(max_seen >= b - (b - a) * 0.1f);
}

TEST_CASE("JobRandom::uniformU32 stays within bounds", "[job_random][prng]")
{
    constexpr std::size_t N = 10'000;
    const std::uint32_t lo = 5;
    const std::uint32_t hi = 27;

    std::uint32_t min_seen = hi;
    std::uint32_t max_seen = lo;

    for (std::size_t i = 0; i < N; ++i) {
        auto v = JobRandom::uniformU32(lo, hi);
        REQUIRE(v >= lo);
        REQUIRE(v <= hi);

        if (v < min_seen) min_seen = v;
        if (v > max_seen) max_seen = v;
    }

    REQUIRE(min_seen == lo);
    REQUIRE(max_seen == hi);
}

TEST_CASE("JobRandom::normal looks roughly sane", "[job_random][prng]")
{
    constexpr std::size_t N = 50'000;
    const float mean = 1.5f;
    const float stddev = 2.0f;

    float sum = 0;
    float sum_sq = 0;

    for (std::size_t i = 0; i < N; ++i) {
        float v = JobRandom::normal(mean, stddev);
        sum    += v;
        sum_sq += v * v;
    }

    const float n      = static_cast<float>(N);
    const float emp_mu = sum / n;
    const float emp_var = sum_sq / n - emp_mu * emp_mu;

    REQUIRE(emp_mu == Catch::Approx(mean).margin(0.1));
    REQUIRE(emp_var == Catch::Approx(stddev * stddev).margin(0.3));
}

TEST_CASE("JobRandom global seed influences deterministic sequences per-thread", "[job_random][seed]")
{
    JobRandom::setGlobalSeed(123456789ULL);

    constexpr std::size_t N = 8;
    std::array<float, N> seq1{};
    for (std::size_t i = 0; i < N; ++i)
        seq1[i] = JobRandom::uniformReal(0.0f, 1.0f);

    JobRandom::setGlobalSeed(123456789ULL);

    std::array<float, N> seq2{};
    std::thread t([&](){
        for (std::size_t i = 0; i < N; ++i)
            seq2[i] = JobRandom::uniformReal(0.0f, 1.0f);
    });
    t.join();

    JobRandom::disableGlobalSeed();
    std::array<float, N> seq3{};
    for (std::size_t i = 0; i < N; ++i)
        seq3[i] = JobRandom::uniformReal(0.0f, 1.0f);

    bool any_diff = false;
    for (std::size_t i = 0; i < N; ++i) {
        if (seq2[i] != seq3[i]) {
            any_diff = true;
            break;
        }
    }
    REQUIRE(any_diff);
}

TEST_CASE("JobRandom is usable safely from many threads", "[job_random][threads]")
{
    constexpr std::size_t num_threads = 8;
    constexpr std::size_t samples_per_thread = 10'000;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::vector<float> sums(num_threads, 0.0f);

    for (std::size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &sums]() {
            float local_sum = 0;
            for (std::size_t i = 0; i < samples_per_thread; ++i) {
                local_sum += JobRandom::uniformReal(0.0f, 1.0f);
            }
            sums[t] = local_sum;
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    std::set<float> uniq;
    for (auto s : sums) {
        REQUIRE(job::crypto::tests::isSafeFinite(s));
        REQUIRE(s > 0.0f);
        uniq.insert(s);
    }
    REQUIRE(uniq.size() > 1);
}

TEST_CASE("JobRandom::randomSalt size and uniqueness verification", "[job_random][salt]")
{
    auto salt1 = JobRandom::randomSalt();
    auto salt2 = JobRandom::randomSalt();

    REQUIRE(salt1.size() == crypto_pwhash_SALTBYTES);
    REQUIRE(salt2.size() == crypto_pwhash_SALTBYTES);
    REQUIRE(salt1 != salt2);
}