#pragma once
#include <random>
#include <cmath>
#include <catch2/catch_all.hpp>
#include <simd_provider.h>

namespace job::simd::test {

inline int threadCountForTests()
{
#ifdef JOB_CI_BUILD
    return 4;
#else
    return 8;
#endif
}

inline float extractLane0(f32 v)
{
    alignas(sizeof(float) * SIMD::width()) float tmp[SIMD::width()];
    SIMD::mov(tmp, v);
    return tmp[0];
}

inline void readAllLanes(f32 v, float (&out)[SIMD::width()])
{
    SIMD::mov(out, v);
}

template <typename ApproxFn, typename ReferenceFn>
inline void sweepAndCheckMaxError(ApproxFn approxFn, ReferenceFn refFn,
                                  float lo, float hi, int steps, float tolerance)
{
    float worstError = 0.0f;
    float worstX      = lo;

    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps - 1);
        const float x = lo + t * (hi - lo);

        const float got      = extractLane0(approxFn(SIMD::set1(x)));
        const float expected = refFn(x);

        const float err = std::fabs(got - expected);
        if (err > worstError) {
            worstError = err;
            worstX      = x;
        }
    }

    INFO("worst x = " << worstX << ", error = " << worstError);
    REQUIRE(worstError < tolerance);
}


// #ifdef JOB_TEST_BENCHMARKS


template <typename ApproxFn, typename ReferenceFn>
inline void sweepAndCheckMaxRelativeError(ApproxFn approxFn, ReferenceFn refFn,
                                          float lo, float hi, int steps, float tolerance)
{
    float worstRelError = 0.0f;
    float worstX          = lo;

    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps - 1);
        const float x = lo + t * (hi - lo);

        const float got      = extractLane0(approxFn(SIMD::set1(x)));
        const float expected = refFn(x);

        const float relErr = std::fabs(got - expected) / std::fabs(expected);
        if (relErr > worstRelError) {
            worstRelError = relErr;
            worstX         = x;
        }
    }

    INFO("worst x = " << worstX << ", relative error = " << worstRelError);
    REQUIRE(worstRelError < tolerance);
}


//////////////// BENCH
// Data
inline std::vector<float> makeData(std::size_t n)
{
    std::vector<float> v(n);
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-5.0f, 5.0f);
    for (auto &x : v)
        x = dist(gen);
    return v;
}

// Same purpose as makeData, restricted to a positive domain for log.
inline std::vector<float> makePositiveData(std::size_t n)
{
    std::vector<float> v(n);
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(0.001f, 100.0f);
    for (auto &x : v)
        x = dist(gen);
    return v;
}

inline std::vector<float> makeTileData(std::size_t tileCount)
{
    return makeData(tileCount * SIMD::width() * SIMD::width());
}

inline std::vector<float> makePositiveTileData(std::size_t tileCount)
{
    return makePositiveData(tileCount * SIMD::width() * SIMD::width());
}

////////////////////////////////////
// Multiple Plus BASE AVX CLASS
////////////////////////////////////

// Good ....
inline float benchScalarMulPlus(const std::vector<float> &src, float b, float c)
{
    std::vector<float> v = src;
    for (auto &x : v)
        x = x * b + c;
    float sum = 0.0f;
    for (auto x : v)
        sum += x;
    return sum;
}

// No std::vector<f32> here, f32 (__m256) never becomes a template
// argument, walks the plain float buffer width() at a time instead.
// Caller guarantees src.size() is a multiple of SIMD::width().
inline float benchVectorMulPlus(const std::vector<float> &src, f32 b, f32 c)
{
    std::vector<float> v = src;
    const std::size_t w = SIMD::width();
    for (std::size_t i = 0; i < v.size(); i += w)
        SIMD::mov(&v[i], SIMD::mul_plus(SIMD::pull(&v[i]), b, c));

    f32 sum = SIMD::zero();
    for (std::size_t i = 0; i < v.size(); i += w)
        sum = SIMD::add(sum, SIMD::pull(&v[i]));
    return hsum(sum);
}
// Caller guarantees data points at a writable buffer sized for a full
// width()xwidth() tile (row * stride reaches width() valid floats
// per row). Results are stored back into data, matching Vector's
// mutate-in-place shape rather than staying register-only.
inline f32 benchTileMulPlus(float* __restrict__ data, size_t stride, f32 b, f32 c)
{
    constexpr int sz = SIMD::width();
    f32 rows[sz];
    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::pull(data + r * stride);

    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::mul_plus(rows[r], b, c);

    for (int r = 0; r < sz; ++r)
        SIMD::mov(data + r * stride, rows[r]);

    f32 sum = SIMD::zero();
    for (int r = 0; r < sz; ++r)
        sum = SIMD::add(sum, rows[r]);

    return sum;
}

////////////////////////////////////
// MASK SELECT  BASE AVX CLASS
////////////////////////////////////
inline float benchScalarMaskSelect(const std::vector<float> &src, float eps)
{
    std::vector<float> v = src;
    for (auto &x : v)
        x = (x > eps) ? x : 0.0f;
    float sum = 0.0f;
    for (auto x : v)
        sum += x;
    return sum;
}

inline float benchVectorMaskSelect(const std::vector<float> &src, f32 eps)
{
    std::vector<float> v = src;
    const std::size_t w = SIMD::width();
    for (std::size_t i = 0; i < v.size(); i += w) {
        const f32 x = SIMD::pull(&v[i]);
        SIMD::mov(&v[i], SIMD::and_ps(SIMD::gt_ps(x, eps), x));
    }

    f32 sum = SIMD::zero();
    for (std::size_t i = 0; i < v.size(); i += w)
        sum = SIMD::add(sum, SIMD::pull(&v[i]));
    return hsum(sum);
}

inline f32 benchTileMaskSelect(float* __restrict__ data, size_t stride, f32 eps)
{
    constexpr int sz = SIMD::width();
    f32 rows[sz];
    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::pull(data + r * stride);

    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::and_ps(SIMD::gt_ps(rows[r], eps), rows[r]);

    for (int r = 0; r < sz; ++r)
        SIMD::mov(data + r * stride, rows[r]);

    f32 sum = SIMD::zero();
    for (int r = 0; r < sz; ++r)
        sum = SIMD::add(sum, rows[r]);

    return sum;
}

// START MATH END BASE AVX

///////////////////////////////////////////////////////////////////////////
// EXP (MATH)
//////////////////////////////////////////////////////////////////////////

inline float benchScalarExp(const std::vector<float> &src)
{
    std::vector<float> v = src;
    for (auto &x : v)
        x = std::exp(x);
    float sum = 0.0f;
    for (auto x : v)
        sum += x;
    return sum;
}

inline float benchVectorExp(const std::vector<float> &src)
{
    std::vector<float> v = src;
    const std::size_t w = SIMD::width();
    for (std::size_t i = 0; i < v.size(); i += w)
        SIMD::mov(&v[i], SIMD::exp(SIMD::pull(&v[i])));

    f32 sum = SIMD::zero();
    for (std::size_t i = 0; i < v.size(); i += w)
        sum = SIMD::add(sum, SIMD::pull(&v[i]));
    return hsum(sum);
}

inline float benchVectorExpFast(const std::vector<float> &src)
{
    std::vector<float> v = src;
    const std::size_t w = SIMD::width();
    for (std::size_t i = 0; i < v.size(); i += w)
        SIMD::mov(&v[i], SIMD::exp_fast(SIMD::pull(&v[i])));

    f32 sum = SIMD::zero();
    for (std::size_t i = 0; i < v.size(); i += w)
        sum = SIMD::add(sum, SIMD::pull(&v[i]));
    return hsum(sum);
}

// Caller guarantees data points at a writable buffer sized for a full
// width()xwidth() tile.
inline f32 benchTileExp(float* __restrict__ data, size_t stride)
{
    constexpr int sz = SIMD::width();
    f32 rows[sz];
    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::pull(data + r * stride);

    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::exp(rows[r]);

    for (int r = 0; r < sz; ++r)
        SIMD::mov(data + r * stride, rows[r]);

    f32 sum = SIMD::zero();
    for (int r = 0; r < sz; ++r)
        sum = SIMD::add(sum, rows[r]);

    return sum;
}

inline f32 benchTileExpFast(float* __restrict__ data, size_t stride)
{
    constexpr int sz = SIMD::width();
    f32 rows[sz];
    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::pull(data + r * stride);

    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::exp_fast(rows[r]);

    for (int r = 0; r < sz; ++r)
        SIMD::mov(data + r * stride, rows[r]);

    f32 sum = SIMD::zero();
    for (int r = 0; r < sz; ++r)
        sum = SIMD::add(sum, rows[r]);

    return sum;
}

////////////////////////////
/// LOG (MATH)
///////////////////////////
inline float benchScalarLog(const std::vector<float> &src)
{
    std::vector<float> v = src;
    for (auto &x : v)
        x = std::log(x);
    float sum = 0.0f;
    for (auto x : v)
        sum += x;
    return sum;
}

inline float benchVectorLog(const std::vector<float> &src)
{
    std::vector<float> v = src;
    const std::size_t w = SIMD::width();
    for (std::size_t i = 0; i < v.size(); i += w)
        SIMD::mov(&v[i], SIMD::log(SIMD::pull(&v[i])));

    f32 sum = SIMD::zero();
    for (std::size_t i = 0; i < v.size(); i += w)
        sum = SIMD::add(sum, SIMD::pull(&v[i]));
    return hsum(sum);
}

inline f32 benchTileLog(float* __restrict__ data, size_t stride)
{
    constexpr int sz = SIMD::width();
    f32 rows[sz];
    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::pull(data + r * stride);

    for (int r = 0; r < sz; ++r)
        rows[r] = SIMD::log(rows[r]);

    for (int r = 0; r < sz; ++r)
        SIMD::mov(data + r * stride, rows[r]);

    f32 sum = SIMD::zero();
    for (int r = 0; r < sz; ++r)
        sum = SIMD::add(sum, rows[r]);

    return sum;
}
// #endif // JOB_TEST_BENCHMARKS




} // namespace job::simd::test