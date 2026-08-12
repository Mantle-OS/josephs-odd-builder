#include <cstring>
#include <bit>

#include <catch2/catch_all.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

// job::core
#include <real_type.h>

#include <simd_provider.h>

#include "test_job_simd_utils.h"

using namespace job::simd;
using namespace job::simd::test;

TEST_CASE("zero produces all-zero lanes", "[simd][avx][construct]")
{
    float out[SIMD::width()];
    readAllLanes(SIMD::zero(), out);
    for (float v : out)
        REQUIRE(v == 0.0f);
}

TEST_CASE("set1 broadcasts a scalar to every lane", "[simd][avx][construct]")
{
    float out[SIMD::width()];
    readAllLanes(SIMD::set1(4.25f), out);
    for (float v : out)
        REQUIRE(v == 4.25f);
}

TEST_CASE("set1_i32 broadcasts an int and round-trips through cast_to_float/cast_to_int", "[simd][avx][construct]")
{
    // set1_i32 -> cast_to_float -> cast_to_int -> round trip ->  not a numeric conversion.
    const i32 original  = SIMD::set1_i32(0x40490FDBu); // bits of pi as float
    const f32 asFloat   = SIMD::cast_to_float(original);
    const i32 back      = SIMD::cast_to_int(asFloat);

    alignas(32) std::int32_t out[SIMD::width()];
    SIMD::mov(reinterpret_cast<float *>(out), SIMD::cast_to_float(back));
    for (auto v : out)
        REQUIRE(static_cast<std::uint32_t>(v) == 0x40490FDBu);

    REQUIRE(extractLane0(asFloat) == Catch::Approx(3.14159274f));
}

TEST_CASE("pull/mov round-trips a full lane of floats", "[simd][avx][load][store]")
{
    alignas(32) float src[SIMD::width()];
    alignas(32) float dst[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i)
        src[i] = static_cast<float>(i) * 1.5f - 3.0f;

    SIMD::mov(dst, SIMD::pull(src));

    for (int i = 0; i < SIMD::width(); ++i)
        REQUIRE(dst[i] == src[i]);
}


TEST_CASE("add/sub/mul/div match scalar arithmetic lane-for-lane", "[simd][avx][arith]")
{
    alignas(32) float aVals[SIMD::width()];
    alignas(32) float bVals[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i) {
        aVals[i] = static_cast<float>(i + 1);
        bVals[i] = static_cast<float>(i + 2) * 0.5f;
    }
    const f32 a = SIMD::pull(aVals);
    const f32 b = SIMD::pull(bVals);

    float addOut[SIMD::width()], subOut[SIMD::width()], mulOut[SIMD::width()], divOut[SIMD::width()];
    readAllLanes(SIMD::add(a, b), addOut);
    readAllLanes(SIMD::sub(a, b), subOut);
    readAllLanes(SIMD::mul(a, b), mulOut);
    readAllLanes(SIMD::div(a, b), divOut);

    for (int i = 0; i < SIMD::width(); ++i) {
        REQUIRE(addOut[i] == Catch::Approx(aVals[i] + bVals[i]));
        REQUIRE(subOut[i] == Catch::Approx(aVals[i] - bVals[i]));
        REQUIRE(mulOut[i] == Catch::Approx(aVals[i] * bVals[i]));
        REQUIRE(divOut[i] == Catch::Approx(aVals[i] / bVals[i]));
    }
}

TEST_CASE("addsub alternates subtract/add across adjacent lanes", "[simd][avx][arith]")
{
    // _mm256_addsub_ps: even lanes = a-b, odd lanes = a+b. Complex-number
    const f32 a = SIMD::set1(5.0f);
    const f32 b = SIMD::set1(2.0f);

    float out[SIMD::width()];
    readAllLanes(SIMD::addsub(a, b), out);

    for (int i = 0; i < SIMD::width(); ++i) {
        if (i % 2 == 0)
            REQUIRE(out[i] == Catch::Approx(3.0f));  // a - b
        else
            REQUIRE(out[i] == Catch::Approx(7.0f));  // a + b
    }
}

TEST_CASE("mul_plus computes a*b+c (FMA contract)", "[simd][avx][fma]")
{
    const f32 a = SIMD::set1(2.0f);
    const f32 b = SIMD::set1(3.0f);
    const f32 c = SIMD::set1(1.0f);
    REQUIRE(extractLane0(SIMD::mul_plus(a, b, c)) == Catch::Approx(7.0f));
}

TEST_CASE("sqrt and rsqrt are (approximate) inverses of each other", "[simd][avx][arith]")
{
    const f32 x = SIMD::set1(16.0f);
    REQUIRE(extractLane0(SIMD::sqrt(x)) == Catch::Approx(4.0f));
    REQUIRE(extractLane0(SIMD::rsqrt(x)) == Catch::Approx(0.25f).epsilon(1e-3));
}


TEST_CASE("add_i32/sub_i32 match scalar integer arithmetic", "[simd][avx][int]")
{
    const i32 a = SIMD::set1_i32(100);
    const i32 b = SIMD::set1_i32(37);

    alignas(32) float addOut[SIMD::width()];
    alignas(32) float subOut[SIMD::width()];
    SIMD::mov(addOut, SIMD::cast_to_float(SIMD::add_i32(a, b)));
    SIMD::mov(subOut, SIMD::cast_to_float(SIMD::sub_i32(a, b)));

    std::int32_t addLane, subLane;
    std::memcpy(&addLane, &addOut[0], sizeof(std::int32_t));
    std::memcpy(&subLane, &subOut[0], sizeof(std::int32_t));

    REQUIRE(addLane == 137);
    REQUIRE(subLane == 63);
}

TEST_CASE("and_si/or_si behave as bitwise AND/OR on raw lanes", "[simd][avx][int]")
{
    const i32 a = SIMD::set1_i32(0x0F0F0F0F);
    const i32 b = SIMD::set1_i32(0x00FF00FF);

    float andOut[SIMD::width()], orOut[SIMD::width()];
    SIMD::mov(andOut, SIMD::cast_to_float(SIMD::and_si(a, b)));
    SIMD::mov(orOut, SIMD::cast_to_float(SIMD::or_si(a, b)));

    std::uint32_t andLane, orLane;
    std::memcpy(&andLane, &andOut[0], sizeof(std::uint32_t));
    std::memcpy(&orLane, &orOut[0], sizeof(std::uint32_t));

    REQUIRE(andLane == 0x000F000Fu);
    REQUIRE(orLane == 0x0FFF0FFFu);
}

TEST_CASE("slli_epi32/srli_epi32 shift bits by the given amount", "[simd][avx][int]")
{
    const i32 v = SIMD::set1_i32(1);

    float leftOut[SIMD::width()], rightOut[SIMD::width()];
    SIMD::mov(leftOut, SIMD::cast_to_float(SIMD::slli_epi32(v, 4)));

    const i32 shifted = SIMD::set1_i32(0x100);
    SIMD::mov(rightOut, SIMD::cast_to_float(SIMD::srli_epi32(shifted, 4)));

    std::uint32_t leftLane, rightLane;
    std::memcpy(&leftLane, &leftOut[0], sizeof(std::uint32_t));
    std::memcpy(&rightLane, &rightOut[0], sizeof(std::uint32_t));

    REQUIRE(leftLane == 0x10u);
    REQUIRE(rightLane == 0x10u);
}

TEST_CASE("slli_epi32 into the exponent field doubles a float per shift, scalbn-style", "[simd][avx][int]")
{
    const f32 one = SIMD::set1(1.0f);
    const i32 exponentBump = SIMD::slli_epi32(SIMD::set1_i32(3), 23); // k=3 -> *2^3
    const i32 bumped = SIMD::add_i32(SIMD::cast_to_int(one), exponentBump);
    REQUIRE(extractLane0(SIMD::cast_to_float(bumped)) == Catch::Approx(8.0f));
}

TEST_CASE("qnan produces the canonical quiet-NaN bit pattern", "[simd][avx][cast]")
{
    float out[SIMD::width()];
    readAllLanes(SIMD::qnan(), out);
    std::uint32_t bits;
    std::memcpy(&bits, &out[0], sizeof(std::uint32_t));
    REQUIRE(bits == 0x7FC00000u);
}

TEST_CASE("cvt_f32_i32 truncates toward zero, cvt_i32_f32 converts back", "[simd][avx][cast]")
{
    REQUIRE(extractLane0(SIMD::cvt_i32_f32(SIMD::cvt_f32_i32(SIMD::set1(3.9f))))    == 3.0f );
    REQUIRE(extractLane0(SIMD::cvt_i32_f32(SIMD::cvt_f32_i32(SIMD::set1(-3.9f))))   == -3.0f);
}

TEST_CASE("cast_f32_f16/ext_f16 pull the low/high 128 bits without converting values", "[simd][avx][cast]")
{
    alignas(32) float src[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i)
        src[i] = static_cast<float>(i);
    const f32 v = SIMD::pull(src);

    // lanes 0-3.
    const f16 low = SIMD::cast_f32_f16(v);
    alignas(16) float lowOut[4];
    _mm_storeu_ps(lowOut, low);
    for (int i = 0; i < 4; ++i)
        REQUIRE(lowOut[i] == src[i]);

    // lanes 4-7
    const f16 high = SIMD::ext_f16(v, 1);
    alignas(16) float highOut[4];
    _mm_storeu_ps(highOut, high);
    for (int i = 0; i < 4; ++i)
        REQUIRE(highOut[i] == src[i + 4]);
}

TEST_CASE("add_f16 adds two 128-bit halves lane-for-lane", "[simd][avx][cast]")
{
    const f16 a = _mm_set1_ps(1.0f);
    const f16 b = _mm_set1_ps(2.5f);
    alignas(16) float out[4];
    _mm_storeu_ps(out, SIMD::add_f16(a, b));
    for (float v : out)
        REQUIRE(v == Catch::Approx(3.5f));
}

TEST_CASE("max/min pick the larger/smaller value lane-for-lane", "[simd][avx][logic]")
{
    const f32 a = SIMD::set1(3.0f);
    const f32 b = SIMD::set1(7.0f);
    REQUIRE(extractLane0(SIMD::max(a, b)) == 7.0f);
    REQUIRE(extractLane0(SIMD::min(a, b)) == 3.0f);
}

TEST_CASE("eq reports true only for exactly equal lanes", "[simd][avx][compare]")
{
    const f32 same = SIMD::set1(3.0f);
    const f32 diff = SIMD::set1(4.0f);

    std::uint32_t equalBits, notEqualBits;
    const float equalLane = extractLane0(SIMD::eq(same, same));
    const float notEqualLane = extractLane0(SIMD::eq(same, diff));
    std::memcpy(&equalBits, &equalLane, sizeof(float));
    std::memcpy(&notEqualBits, &notEqualLane, sizeof(float));

    REQUIRE(equalBits == 0xFFFFFFFFu);
    REQUIRE(notEqualBits == 0x00000000u);
}

TEST_CASE("and_ps/or_ps/xor_ps/is_not behave as bitwise ops on float lanes", "[simd][avx][logic]")
{
    const f32 allOnes = SIMD::cast_to_float(SIMD::set1_i32(-1));
    const f32 pattern = SIMD::cast_to_float(SIMD::set1_i32(0x0F0F0F0F));

    auto bitsOf = [](f32 v) {
        std::uint32_t b;
        const float lane = extractLane0(v);
        std::memcpy(&b, &lane, sizeof(std::uint32_t));
        return b;
    };

    REQUIRE(bitsOf(SIMD::and_ps(allOnes, pattern)) == 0x0F0F0F0Fu);
    REQUIRE(bitsOf(SIMD::or_ps(allOnes, pattern)) == 0xFFFFFFFFu);
    REQUIRE(bitsOf(SIMD::xor_ps(allOnes, pattern)) == 0xF0F0F0F0u);
    // is_not(a, b) == andnot(a, b) == (~a) & b
    REQUIRE(bitsOf(SIMD::is_not(allOnes, pattern)) == 0x00000000u);
}

TEST_CASE("gt_ps/le_ps produce all-true or all-false lane masks", "[simd][avx][compare]")
{
    auto bitsOf = [](f32 v) {
        std::uint32_t b;
        const float lane = extractLane0(v);
        std::memcpy(&b, &lane, sizeof(std::uint32_t));
        return b;
    };

    REQUIRE(bitsOf(SIMD::gt_ps(SIMD::set1(5.0f), SIMD::set1(1.0f))) == 0xFFFFFFFFu);
    REQUIRE(bitsOf(SIMD::gt_ps(SIMD::set1(1.0f), SIMD::set1(5.0f))) == 0x00000000u);
    REQUIRE(bitsOf(SIMD::le_ps(SIMD::set1(1.0f), SIMD::set1(1.0f))) == 0xFFFFFFFFu);
}

TEST_CASE("blendv selects b where mask is set, a otherwise, like the star-forces guard", "[simd][avx][mask]")
{
    const f32 a = SIMD::set1(1.0f);
    const f32 b = SIMD::set1(2.0f);
    const f32 allTrue  = SIMD::gt_ps(SIMD::set1(5.0f), SIMD::set1(0.0f));
    const f32 allFalse = SIMD::gt_ps(SIMD::set1(0.0f), SIMD::set1(5.0f));

    REQUIRE(extractLane0(SIMD::blendv(a, b, allTrue)) == 2.0f);
    REQUIRE(extractLane0(SIMD::blendv(a, b, allFalse)) == 1.0f);
}

TEST_CASE("clamp_f32 pins values outside [floor, ceil]", "[simd][avx][arith]")
{
    REQUIRE(extractLane0(SIMD::clamp_f32(SIMD::set1(100.0f), -1.0f, 1.0f)) == Catch::Approx(1.0f));
    REQUIRE(extractLane0(SIMD::clamp_f32(SIMD::set1(-100.0f), -1.0f, 1.0f)) == Catch::Approx(-1.0f));
    REQUIRE(extractLane0(SIMD::clamp_f32(SIMD::set1(0.5f), -1.0f, 1.0f)) == Catch::Approx(0.5f));
}

TEST_CASE("ceil/floor round toward +inf/-inf", "[simd][avx][round]")
{
    REQUIRE(extractLane0(SIMD::ceil(SIMD::set1(1.2f))) == 2.0f);
    REQUIRE(extractLane0(SIMD::ceil(SIMD::set1(-1.2f))) == -1.0f);
    REQUIRE(extractLane0(SIMD::floor(SIMD::set1(1.8f))) == 1.0f);
    REQUIRE(extractLane0(SIMD::floor(SIMD::set1(-1.8f))) == -2.0f);
}

TEST_CASE("round<Mode> matches each RoundingMode's documented behavior", "[simd][avx][round]")
{
    const f32 posVal = SIMD::set1(2.5f);
    const f32 negVal = SIMD::set1(-2.5f);

    REQUIRE(extractLane0(SIMD::round<RoundingMode::Nearest>(posVal)) == 2.0f);  // horse shoes and hand....
    REQUIRE(extractLane0(SIMD::round<RoundingMode::Down>(posVal)) == 2.0f);
    REQUIRE(extractLane0(SIMD::round<RoundingMode::Up>(posVal)) == 3.0f);
    REQUIRE(extractLane0(SIMD::round<RoundingMode::Truncate>(negVal)) == -2.0f);
}

TEST_CASE("unpack_lo/unpack_hi interleave two vectors' low/high halves", "[simd][avx][shuffle]")
{
    alignas(32) float aVals[SIMD::width()];
    alignas(32) float bVals[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i) {
        aVals[i] = static_cast<float>(i);
        bVals[i] = static_cast<float>(i + 100);
    }
    const f32 a = SIMD::pull(aVals);
    const f32 b = SIMD::pull(bVals);

    float loOut[SIMD::width()], hiOut[SIMD::width()];
    readAllLanes(SIMD::unpack_lo(a, b), loOut);
    readAllLanes(SIMD::unpack_hi(a, b), hiOut);

    // [a0 b0 a1 b1 | a4 b4 a5 b5] for lo.
    REQUIRE(loOut[0] == 0.0f);
    REQUIRE(loOut[1] == 100.0f);
    REQUIRE(loOut[2] == 1.0f);
    REQUIRE(loOut[3] == 101.0f);
}

TEST_CASE("shuffle<Mask> selects lanes per the transpose kernel's 0x44/0xEE masks", "[simd][avx][shuffle]")
{
    alignas(32) float aVals[SIMD::width()];
    alignas(32) float bVals[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i) {
        aVals[i] = static_cast<float>(i);
        bVals[i] = static_cast<float>(i + 100);
    }
    const f32 a = SIMD::pull(aVals);
    const f32 b = SIMD::pull(bVals);

    // 0x44 = 01 00 01 00: per 128-bit lane, out = [a0, a1, b0, b1]
    float lowOut[SIMD::width()];
    readAllLanes(SIMD::shuffle<0x44>(a, b), lowOut);
    REQUIRE(lowOut[0] == aVals[0]);
    REQUIRE(lowOut[1] == aVals[1]);
    REQUIRE(lowOut[2] == bVals[0]);
    REQUIRE(lowOut[3] == bVals[1]);

    // 0xEE = 11 10 11 10: per 128-bit lane, out = [a2, a3, b2, b3]
    float highOut[SIMD::width()];
    readAllLanes(SIMD::shuffle<0xEE>(a, b), highOut);
    REQUIRE(highOut[0] == aVals[2]);
    REQUIRE(highOut[1] == aVals[3]);
    REQUIRE(highOut[2] == bVals[2]);
    REQUIRE(highOut[3] == bVals[3]);
}

TEST_CASE("permute_lanes<Mask> swaps 128-bit halves per the transpose kernel's final step", "[simd][avx][shuffle]")
{
    alignas(32) float aVals[SIMD::width()];
    alignas(32) float bVals[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i) {
        aVals[i] = static_cast<float>(i);
        bVals[i] = static_cast<float>(i + 100);
    }
    const f32 a = SIMD::pull(aVals);
    const f32 b = SIMD::pull(bVals);

    // 0x20: result = [low 128 of a, low 128 of b]
    float out[SIMD::width()];
    readAllLanes(SIMD::permute_lanes<0x20>(a, b), out);

    for (int i = 0; i < 4; ++i)
        REQUIRE(out[i] == aVals[i]);

    for (int i = 0; i < 4; ++i)
        REQUIRE(out[i + 4] == bVals[i]);
}


TEST_CASE("hadd/hsub combine adjacent pairs within each 128-bit half, not the whole vector", "[simd][avx][shuffle]")
{
    alignas(32) float aVals[SIMD::width()];
    for (int i = 0; i < SIMD::width(); ++i)
        aVals[i] = static_cast<float>(i + 1);

    const f32 a = SIMD::pull(aVals);

    float out[SIMD::width()];
    readAllLanes(SIMD::hadd(a, a), out);
    // _mm256_hadd_ps(a,a): lane0=a0+a1, lane1=a2+a3, lane2=a0+a1, lane3=a2+a3 (per 128-bit half)
    REQUIRE(out[0] == Catch::Approx(3.0f));  // 1+2
    REQUIRE(out[1] == Catch::Approx(7.0f));  // 3+4
}

// 2
TEST_CASE("div by zero produces a non-finite bit pattern", "[simd][avx][edge]")
{
    const float result = extractLane0(SIMD::div(SIMD::set1(1.0f), SIMD::set1(0.0f)));
    REQUIRE_FALSE(job::core::isSafeFinite(result));
}

TEST_CASE("rsqrt of zero saturates to a non-finite bit pattern", "[simd][avx][edge]")
{
    const float result = extractLane0(SIMD::rsqrt(SIMD::set1(0.0f)));
    REQUIRE_FALSE(job::core::isSafeFinite(result));
}

TEST_CASE("max/min with a NaN lane follow the hardware's second-operand rule", "[simd][avx][edge]")
{
    const f32 nanReg = SIMD::cast_to_float(SIMD::set1_i32(0x7FC00000));
    const f32 oneReg = SIMD::set1(1.0f);

    REQUIRE(extractLane0(SIMD::max(nanReg, oneReg)) == 1.0f);
    REQUIRE(extractLane0(SIMD::min(nanReg, oneReg)) == 1.0f);
}

TEST_CASE("permute_lanes/shuffle/unpack are no-ops when both inputs are identical zero vectors", "[simd][avx][edge]")
{
    const f32 z = SIMD::zero();
    float out[SIMD::width()];
    readAllLanes(SIMD::permute_lanes<0x20>(z, z), out);
    for (float v : out)
        REQUIRE(v == 0.0f);
}

TEST_CASE("benchTileMulPlus stores a*b+c back into data and returns the correct sum", "[simd][avx][tile]")
{
    const std::size_t width = SIMD::width();
    std::vector<float> data(width * width);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float>(i + 1); // 1, 2, 3, ... width*width

    const float b = 2.0f;
    const float c = 1.0f;

    std::vector<float> expected(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        expected[i] = data[i] * b + c;

    const f32 bReg = SIMD::set1(b);
    const f32 cReg = SIMD::set1(c);
    const f32 result = benchTileMulPlus(data.data(), width, bReg, cReg);

    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(data[i] == Catch::Approx(expected[i]));

    float expectedSum = 0.0f;
    for (float v : expected)
        expectedSum += v;

    REQUIRE(hsum(result) == Catch::Approx(expectedSum));
}


TEST_CASE("benchTileMaskSelect stores the eps-guarded value back into data and returns the correct sum", "[simd][avx][tile]")
{
    const std::size_t width = SIMD::width();
    std::vector<float> data(width * width);

    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = (i % 2 == 0) ? 1e-12f : static_cast<float>(i + 1);

    const float eps = 1e-9f;

    std::vector<float> expected(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        expected[i] = (data[i] > eps) ? data[i] : 0.0f;

    const f32 epsReg = SIMD::set1(eps);
    const f32 result = benchTileMaskSelect(data.data(), width, epsReg);

    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(data[i] == Catch::Approx(expected[i]));

    float expectedSum = 0.0f;
    for (float v : expected)
        expectedSum += v;

    REQUIRE(hsum(result) == Catch::Approx(expectedSum));
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark: Scalar Loop vs Vectorized Kernel (mul_plus, mask-select)", "[simd][avx][benchmark]")
{
    constexpr std::size_t n = 1'000'000;
    const auto scalarData = makeData(n);

    const std::size_t width     = SIMD::width();
    const std::size_t tileCount = n / (width * width);
    auto tileData = makeTileData(tileCount);

    const f32 mulB    = SIMD::set1(3.0f);
    const f32 mulC     = SIMD::set1(1.0f);
    const f32 maskEps  = SIMD::set1(1e-9f);

    BENCHMARK("mul_plus (Scalar)") {
        return benchScalarMulPlus(scalarData, 3.0f, 1.0f);
    };

    BENCHMARK("mul_plus (Vector)") {
        return benchVectorMulPlus(scalarData, mulB, mulC);
    };

    BENCHMARK("mul_plus (Tile)") {
        f32 total = SIMD::zero();
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileMulPlus(&tileData[t * width * width], width, mulB, mulC));
        return hsum(total);
    };

    BENCHMARK("mask-select (Scalar)") {
        return benchScalarMaskSelect(scalarData, 1e-9f);
    };

    BENCHMARK("mask-select (Vector)") {
        return benchVectorMaskSelect(scalarData, maskEps);
    };

    BENCHMARK("mask-select (Tile)") {
        f32 total = SIMD::zero(); // cost here
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileMaskSelect(&tileData[t * width * width], width, maskEps));
        return hsum(total);
    };


    BENCHMARK("mask-select (Tile)") {
        f32 total = SIMD::zero(); // cost here
        for (std::size_t t = 0; t < tileCount; ++t)
            total = SIMD::add(total, benchTileMaskSelect(&tileData[t * width * width], width, maskEps));
        return hsum(total);
    };





}
#endif