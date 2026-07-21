// test_job_simd_math.cpp
#include <cmath>
#include <cstring>

#include <catch2/catch_all.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <real_type.h>

#include <simd_provider.h>

#include "test_job_simd_utils.h"

using namespace job::simd;
using namespace job::simd::test;

TEST_CASE("exp_poly5 matches std::exp across a normal range", "[simd][math][exp]")
{
    // Worst-case relative error ~2.2e-5 near the edge of the reduced
    sweepAndCheckMaxRelativeError(exp_poly5, [](float x) { return std::exp(x); },
                                  -10.0f, 10.0f, 2000, 5e-5f);
}

TEST_CASE("exp_estrin matches std::exp across a normal range", "[simd][math][exp]")
{
    // Worst-case relative error ~2.0e-5 at x~9.8
    sweepAndCheckMaxRelativeError(exp_estrin, [](float x) { return std::exp(x); },
                                  -10.0f, 10.0f, 2000, 5e-5f);
}

TEST_CASE("exp_poly5 and exp_estrin agree with each other", "[simd][math][exp]")
{
    // Worst-case relative error ~2.2e-5 at x~-9.36
    sweepAndCheckMaxRelativeError(exp_estrin, [](float x) { return extractLane0(exp_poly5(SIMD::set1(x))); },
                                  -10.0f, 10.0f, 2000, 5e-5f);
}

TEST_CASE("exp_schraudolph tracks std::exp within the bit-hack's known error budget", "[simd][math][exp]")
{
    // Worst-case relative error ~3.0e-2 at x~-4.5
    sweepAndCheckMaxRelativeError(exp_schraudolph, [](float x) { return std::exp(x); },
                                  -5.0f, 5.0f, 2000, 0.05f);
}

TEST_CASE("exp functions agree that exp(x) * exp(-x) == 1", "[simd][math][exp]")
{
    for (float x : {0.1f, 1.0f, 3.7f, 8.25f, 15.0f}) {
        const float pos = extractLane0(exp_poly5(SIMD::set1(x)));
        const float neg = extractLane0(exp_poly5(SIMD::set1(-x)));
        REQUIRE(pos * neg == Catch::Approx(1.0f).epsilon(1e-3));
    }
}

TEST_CASE("avx_log matches std::log across a positive range", "[simd][math][log]")
{
    sweepAndCheckMaxRelativeError(avx_log, [](float x) { return std::log(x); },
                                  0.01f, 1000.0f, 2000, 1e-4f);
}

TEST_CASE("avx_log(1) is 0 and avx_log(e) is 1", "[simd][math][log]")
{
    REQUIRE(extractLane0(avx_log(SIMD::set1(1.0f))) == Catch::Approx(0.0f).margin(1e-5));
    REQUIRE(extractLane0(avx_log(SIMD::set1(2.718281828f))) == Catch::Approx(1.0f).epsilon(1e-4));
}

TEST_CASE("exp and log are inverses of each other", "[simd][math][exp][log]")
{
    for (float x : {0.5f, 1.0f, 4.0f, 20.0f, 500.0f}) {
        const float logged  = extractLane0(avx_log(SIMD::set1(x)));
        const float roundtripped = extractLane0(exp_estrin(SIMD::set1(logged)));
        REQUIRE(roundtripped == Catch::Approx(x).epsilon(1e-3));
    }
}

TEST_CASE("hsum reduces a SIMD register to its scalar sum", "[simd][math][hsum]")
{
    alignas(32) float vals[SIMD::width()];
    float expected = 0.0f;
    for (int i = 0; i < SIMD::width(); ++i) {
        vals[i] = static_cast<float>(i + 1);
        expected += vals[i];
    }
    REQUIRE(hsum(SIMD::pull(vals)) == Catch::Approx(expected));
}

// 2
TEST_CASE("exp_poly5/exp_estrin clamp at +-87.5 instead of overflowing", "[simd][math][exp][edge]")
{
    const float poly5High  = extractLane0(exp_poly5(SIMD::set1(1000.0f)));
    const float poly5Low   = extractLane0(exp_poly5(SIMD::set1(-1000.0f)));
    const float estrinHigh = extractLane0(exp_estrin(SIMD::set1(1000.0f)));
    const float estrinLow  = extractLane0(exp_estrin(SIMD::set1(-1000.0f)));

    REQUIRE(job::core::isSafeFinite(poly5High));
    REQUIRE(job::core::isSafeFinite(estrinHigh));
    REQUIRE(poly5Low >= 0.0f);
    REQUIRE(estrinLow >= 0.0f);
    REQUIRE(poly5Low < 1e-30f);
    REQUIRE(estrinLow < 1e-30f);
}

TEST_CASE("exp_schraudolph clamps at +-87 instead of overflowing", "[simd][math][exp][edge]")
{
    // Note the narrower clamp range here versus exp_poly5/exp_estrin's
    const float high = extractLane0(exp_schraudolph(SIMD::set1(1000.0f)));
    const float low  = extractLane0(exp_schraudolph(SIMD::set1(-1000.0f)));

    REQUIRE(job::core::isSafeFinite(high));
    REQUIRE(low >= 0.0f);
}

TEST_CASE("exp_poly5 at zero returns 1", "[simd][math][exp][edge]")
{
    REQUIRE(extractLane0(exp_poly5(SIMD::set1(0.0f))) == Catch::Approx(1.0f).margin(1e-6));
}

TEST_CASE("avx_log rejects zero and negative input with qnan", "[simd][math][log][edge]")
{
    auto bitsOf = [](float f) {
        std::uint32_t b;
        std::memcpy(&b, &f, sizeof(std::uint32_t));
        return b;
    };

    REQUIRE(bitsOf(extractLane0(avx_log(SIMD::set1(0.0f)))) == 0x7FC00000u);
    REQUIRE(bitsOf(extractLane0(avx_log(SIMD::set1(-5.0f)))) == 0x7FC00000u);
}


TEST_CASE("benchTileExp stores exp(x) back into data and returns the correct sum", "[simd][math][tile]")
{
    const std::size_t width = SIMD::width();
    std::vector<float> data(width * width);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float>(i) * 0.1f - 3.2f; // spread across a normal range

    std::vector<float> expected(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        expected[i] = std::exp(data[i]);

    const f32 result = benchTileExp(data.data(), width);

    // Store-back: benchTileExp calls SIMD::exp, which dispatches to
    // exp_estrin -- same ~2e-5 relative-error ceiling we measured in
    // test_job_simd_math.cpp's sweep, so this uses a relative check,
    // not exact equality.
    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(data[i] == Catch::Approx(expected[i]).epsilon(1e-4));

    float expectedSum = 0.0f;
    for (float v : expected)
        expectedSum += v;

    REQUIRE(hsum(result) == Catch::Approx(expectedSum).epsilon(1e-4));
}

TEST_CASE("benchTileExpFast stores exp_fast(x) back into data and returns the correct sum", "[simd][math][tile]")
{
    const std::size_t width = SIMD::width();
    std::vector<float> data(width * width);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float>(i) * 0.05f - 1.6f; // stays within +-5, matching exp_schraudolph's sweep range

    std::vector<float> expected(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        expected[i] = std::exp(data[i]);

    const f32 result = benchTileExpFast(data.data(), width);

    // exp_schraudolph's known error budget is ~3% relative, not the
    // tight ~2e-5 the polynomial functions get -- same tolerance as
    // the sweep test in test_job_simd_math.cpp.
    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(data[i] == Catch::Approx(expected[i]).epsilon(0.05));

    float expectedSum = 0.0f;
    for (float v : expected)
        expectedSum += v;

    REQUIRE(hsum(result) == Catch::Approx(expectedSum).epsilon(0.05));
}

TEST_CASE("benchTileLog stores log(x) back into data and returns the correct sum", "[simd][math][tile]")
{
    const std::size_t width = SIMD::width();
    std::vector<float> data(width * width);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float>(i + 1) * 0.5f; // strictly positive, avx_log's domain

    std::vector<float> expected(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        expected[i] = std::log(data[i]);

    const f32 result = benchTileLog(data.data(), width);

    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(data[i] == Catch::Approx(expected[i]).epsilon(1e-4));

    float expectedSum = 0.0f;
    for (float v : expected)
        expectedSum += v;

    REQUIRE(hsum(result) == Catch::Approx(expectedSum).epsilon(1e-4));
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark: Scalar Loop vs Vectorized Kernel (exp)", "[simd][math][benchmark]")
{
    constexpr std::size_t n = 1'000'000;
    const auto scalarData = makeData(n);

    const std::size_t width     = SIMD::width();
    const std::size_t tileCount = n / (width * width);
    auto tileData = makeTileData(tileCount);

    BENCHMARK("exp (Scalar std::exp)") {
        return benchScalarExp(scalarData);
    };

    BENCHMARK("exp_estrin (Vector)") {
        return benchVectorExp(scalarData);
    };

    BENCHMARK("exp_estrin (Tile)") {
        f32 total = SIMD::zero();
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileExp(&tileData[t * width * width], width));
        return hsum(total);
    };

    BENCHMARK("exp_schraudolph (Vector)") {
        return benchVectorExpFast(scalarData);
    };

    BENCHMARK("exp_schraudolph (Tile)") {
        f32 total = SIMD::zero();
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileExpFast(&tileData[t * width * width], width));
        return hsum(total);
    };
}

TEST_CASE("Benchmark: Scalar Loop vs Vectorized Kernel (log)", "[simd][math][benchmark]")
{
    constexpr std::size_t n = 1'000'000;
    const auto scalarData = makePositiveData(n);

    const std::size_t width     = SIMD::width();
    const std::size_t tileCount = n / (width * width);
    auto tileData = makePositiveTileData(tileCount);

    BENCHMARK("log (Scalar std::log)") {
        return benchScalarLog(scalarData);
    };

    BENCHMARK("avx_log (Vector)") {
        return benchVectorLog(scalarData);
    };

    BENCHMARK("avx_log (Tile)") {
        f32 total = SIMD::zero();
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileLog(&tileData[t * width * width], width));
        return hsum(total);
    };
}
#endif