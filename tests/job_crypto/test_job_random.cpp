// test_job_random.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <thread>
#include <set>
#include <numeric>
#include <cmath>

#include <real_type.h>
#include <job_random.h>

using namespace job;
using job::crypto::JobRandom;

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

    // Not all zeros
    bool any_non_zero = false;
    for (auto b : buf1) {
        if (b != 0) {
            any_non_zero = true;
            break;
        }
    }
    REQUIRE(any_non_zero);

    // And extremely unlikely they are identical twice in a row
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
    const float a = float(-3.5);
    const float b = float(7.25);

    float min_seen = b;
    float max_seen = a;

    for (std::size_t i = 0; i < N; ++i) {
        float v = JobRandom::uniformReal(a, b);
        REQUIRE(v >= a);
        REQUIRE(v <  b); // std::uniform_real is [a, b) by spec

        if (v < min_seen) min_seen = v;
        if (v > max_seen) max_seen = v;
    }

    // We should have explored at least a decent chunk of the interval
    REQUIRE(min_seen <= a + (b - a) * float(0.1));
    REQUIRE(max_seen >= b - (b - a) * float(0.1));
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
    const float mean = float(1.5);
    const float stddev = float(2.0);

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

    // Very rough sanity band; we're not doing a real stats test here.
    REQUIRE(emp_mu == Catch::Approx(mean).margin(0.1));
    REQUIRE(emp_var == Catch::Approx(stddev * stddev).margin(0.3));
}

TEST_CASE("JobRandom global seed influences deterministic sequences per-thread", "[job_random][seed]")
{
    JobRandom::setGlobalSeed(123456789ULL);

    // Grab a small sequence
    constexpr std::size_t N = 8;
    std::array<float, N> seq1{};
    for (std::size_t i = 0; i < N; ++i)
        seq1[i] = JobRandom::uniformReal(0.0f, 1.0f);

    // Reset global seed and force a new thread to get its own engine
    JobRandom::setGlobalSeed(123456789ULL);

    std::array<float, N> seq2{};
    std::thread t([&](){
        for (std::size_t i = 0; i < N; ++i)
            seq2[i] = JobRandom::uniformReal(0.0f, 1.0f);
    });
    t.join();

    // We *don't* expect seq1 == seq2 (different threads get different derived seeds),
    // but we at least expect the sequences to differ from a "no global seed" run.
    JobRandom::disableGlobalSeed();
    std::array<float, N> seq3{};
    for (std::size_t i = 0; i < N; ++i)
        seq3[i] = JobRandom::uniformReal(0.0f, 1.0f);

    // Very weak sanity: seq2 should differ from seq3 in at least one position.
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

    std::vector<float> sums(num_threads, float(0));

    for (std::size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &sums]() {
            float local_sum = 0;
            for (std::size_t i = 0; i < samples_per_thread; ++i) {
                local_sum += JobRandom::uniformReal(float(0), float(1));
            }
            sums[t] = local_sum;
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Sanity: all sums should be finite, non-zero, and not all identical.
    std::set<float> uniq;
    for (auto s : sums) {
        REQUIRE(job::core::isSafeFinite(static_cast<double>(s)));
        REQUIRE(s > float(0));
        uniq.insert(s);
    }
    REQUIRE(uniq.size() > 1);
}
