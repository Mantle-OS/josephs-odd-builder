#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include <random>
#include <vector>
#include <cmath>

#include <job_logger.h>

// Link against your new header
#include <activation.h>

using Catch::Approx;
using namespace job::ai::comp;

// Standard precision (for ReLU, Linear, etc.)
static Approx approx(float value, float tol = 1e-5f)
{
    return Approx(static_cast<double>(value)).margin(static_cast<double>(tol));
}

// "Painter's" precision (for GELU, Sigmoid, Tanh via fast_exp)
// Allows ~1% deviation, which is acceptable for Neural Networks
// static Approx approx_sloppy(float value, float tol = 0.01f)
// {
//     return Approx(static_cast<double>(value)).margin(static_cast<double>(tol));
// }

// =========================================================
//  Logic Verification (Scalar / Naive Path)
// =========================================================

TEST_CASE("Activation Logic: Basic ReLU/Leaky", "[activation][scalar]")
{
    const float xPos  = 2.0f;
    const float xNeg  = -2.0f;
    const float xZero = 0.0f;

    // Use <false> for T_ESTRIN (standard precision mode for testing)
    REQUIRE(activateNaive<false>(xPos, ActivationType::Identity)  == xPos);

    REQUIRE(activateNaive<false>(xPos, ActivationType::ReLU)      == xPos);
    REQUIRE(activateNaive<false>(xZero, ActivationType::ReLU)     == xZero);
    REQUIRE(activateNaive<false>(xNeg, ActivationType::ReLU)      == 0.0f);

    const float alpha = 0.1f;
    REQUIRE(activateNaive<false>(xPos, ActivationType::LeakyReLU, alpha) == xPos);
    REQUIRE(activateNaive<false>(xNeg, ActivationType::LeakyReLU, alpha) == xNeg * alpha);
}

TEST_CASE("Activation Logic: Sigmoid and Tanh", "[activation][scalar]")
{
    // Sigmoid(0) = 0.5
    REQUIRE(activateNaive<false>(0.0f, ActivationType::Sigmoid) == approx(0.5f));


    // Tanh(0) = 0.0
    REQUIRE(activateNaive<false>(0.0f, ActivationType::Tanh)    == approx(0.0f));

    // Limits
    REQUIRE(activateNaive<true>(10.0f, ActivationType::Sigmoid) == approx(1.0f, 1e-4f));
    REQUIRE(activateNaive<true>(20.0f, ActivationType::Sigmoid) == approx(1.0f));
    REQUIRE(activateNaive<true>(-10.0f, ActivationType::Sigmoid) == approx(0.0f, 1e-4f));
}

TEST_CASE("Activation Logic: Softplus", "[activation][scalar]")
{
    const float xLargePos = 20.0f;
    const float xLargeNeg = -20.0f;

    const float spPos = activateNaive<false>(xLargePos, ActivationType::Softplus);
    const float spNeg = activateNaive<false>(xLargeNeg, ActivationType::Softplus);

    // Softplus approaches Linear for large +x
    REQUIRE(spPos == approx(xLargePos, 1e-4f));

    // Softplus approaches 0 for large -x
    REQUIRE(spNeg == approx(0.0f, 1e-4f));
}

TEST_CASE("Activation Logic: HardTanh", "[activation][scalar]")
{
    // Replaces the old ReLU6 test. HardTanh clamps to [-1, 1] usually.
    // If your implementation uses [-1, 1]:
    REQUIRE(activateNaive<false>(-2.0f, ActivationType::HardTanh) == -1.0f);
    REQUIRE(activateNaive<false>( 0.5f, ActivationType::HardTanh) ==  0.5f);
    REQUIRE(activateNaive<false>( 2.0f, ActivationType::HardTanh) ==  1.0f);
}

TEST_CASE("Activation Logic: GELU vs ApproxGELU", "[activation][scalar]")
{
    // ApproxGELU (Tanh) vs GELU (Erf) should be reasonably close
    const float xs[] = { -3.0f, -1.0f, 0.0f, 1.0f, 3.0f };

    for (float x : xs) {
        const float stdGelu = activateNaive<false>(x, ActivationType::GELU);
        const float apxGelu = activateNaive<false>(x, ActivationType::AproxGELU);

        // Allow loose tolerance for the approximation difference
        REQUIRE(apxGelu == approx(stdGelu, 0.05f));
    }
}

TEST_CASE("Activation Logic: Swish Family", "[activation][scalar]")
{
    const float xPos  = 2.0f;
    const float xNeg  = -2.0f;
    const float xZero = 0.0f;

    // Swish(0) = 0 * 0.5 = 0
    REQUIRE(activateNaive<false>(xZero, ActivationType::Swish) == approx(0.0f));

    // Swish(2) = 2 * Sigmoid(2) ~= 2 * 0.88 = 1.76
    const float swishPos = activateNaive<false>(xPos, ActivationType::Swish);
    REQUIRE(swishPos > 1.5f);
    REQUIRE(swishPos < 2.0f);

    // Mish check
    const float mishNeg = activateNaive<false>(xNeg, ActivationType::Mish);
    const float mishPos = activateNaive<false>(xPos, ActivationType::Mish);
    REQUIRE(mishNeg < 0.0f); // Mish dips negative slightly
    REQUIRE(mishPos > 0.0f);
}

TEST_CASE("Activation Logic: SELU Magic Numbers", "[activation][scalar]")
{
    // Ensure the alpha/scale constants are applied
    const float xPos = 1.0f;
    const float yPos = activateNaive<false>(xPos, ActivationType::SELU);

    // Positive input is just linear * scale
    // kSeluScale is ~1.0507
    REQUIRE(yPos > 1.05f);
    REQUIRE(yPos < 1.06f);
}

// =========================================================
//  Integration: Vector vs Scalar Consistency
// =========================================================

TEST_CASE("Vectorized SIMD matches Scalar Reference (Comprehensive)", "[activation][vector]")
{
    // Generate random data covering a range that exercises non-linearities
    constexpr size_t N = 4096;
    std::vector<float> input(N);
    std::mt19937 gen(42);
    // Range [-5, 5] hits the active zones of Sigmoid/Tanh/GELU/Swish
    std::uniform_real_distribution<float> dist(-5.0f, 5.0f);
    for(auto& x : input) x = dist(gen);

    // Complete list of supported activations
    const std::vector<ActivationType> types = {
        ActivationType::Identity,
        ActivationType::Sigmoid,
        ActivationType::Tanh,
        ActivationType::HardTanh,
        ActivationType::ReLU,
        ActivationType::LeakyReLU,
        ActivationType::PReLU,
        ActivationType::RReLU, // Assumes fixed alpha behavior for inference
        ActivationType::ELU,
        ActivationType::SELU,
        ActivationType::GELU,
        ActivationType::AproxGELU,
        ActivationType::Swish,
        ActivationType::HSwish,
        ActivationType::Mish,
        ActivationType::HMish,
        ActivationType::Softplus
    };

    for (auto type : types) {
        std::vector<float> scalarRef = input;
        std::vector<float> vectorOut = input;

        // 1. Compute Scalar Truth
        // Uses T_ESTRIN = true to match the high-precision vector mode if available
        for(size_t i=0; i<N; ++i)
            scalarRef[i] = activateNaive<true>(scalarRef[i], type, 0.1f);

        // 2. Compute Vectorized
        activate<true>(vectorOut.data(), N, type, 0.1f);

        // 3. Determine Tolerance Strategy
        float tolerance = 1e-5f; // Default Strict

        switch(type) {
        // --- Exact / Piecewise Linear Types ---
        // These rely on simple math (+, -, *, min, max) and should match exactly.
        case ActivationType::Identity:
        case ActivationType::ReLU:
        case ActivationType::LeakyReLU:
        case ActivationType::PReLU:
        case ActivationType::RReLU:
        case ActivationType::HardTanh:
        case ActivationType::HSwish:
        case ActivationType::HMish:
            tolerance = 1e-5f;
            break;

            // --- Transcendental / Curved Types ---
            // These rely on exp/log/tanh/sigmoid.
            // Even with High Precision mode, SIMD polynomial Approx != std::exp
        case ActivationType::Sigmoid:
        case ActivationType::Tanh:
        case ActivationType::ELU:
        case ActivationType::SELU:
        case ActivationType::Softplus:
            tolerance = 0.01f; // 1% tolerance
            break;

            // --- Compound Approximations ---
            // These stack approximations (e.g. Tanh inside Mul, or Sigmoid inside Mul)
            // Error accumulates slightly more.
        case ActivationType::GELU:
        case ActivationType::AproxGELU:
        case ActivationType::Swish:
        case ActivationType::Mish:
            tolerance = 0.02f; // 2% tolerance
            break;

        default:
            tolerance = 1e-5f;
            break;
        }

        // 4. Compare
        for(size_t i=0; i<N; ++i) {
            float diff = std::abs(scalarRef[i] - vectorOut[i]);

            if (diff > tolerance) {
                // Check relative error for larger values to avoid false positives on large inputs
                float rel_err = (std::abs(scalarRef[i]) > 1e-4f)
                                    ? diff / std::abs(scalarRef[i])
                                    : diff;

                if (rel_err > tolerance) {
                    FAIL("Mismatch for Type " << (int)type << " at index " << i
                                              << "\nInput:  " << input[i]
                                              << "\nScalar: " << scalarRef[i]
                                              << "\nVector: " << vectorOut[i]
                                              << "\nDiff:   " << diff
                                              << "\nTol:    " << tolerance);
                }
            }
        }
    }
    SUCCEED("All activation kernels match scalar reference");
}

// =========================================================
//  Benchmarks
// =========================================================

#ifdef JOB_TEST_BENCHMARKS

constexpr bool kHighPrecision = false; // set true to benchmark Estrin paths
constexpr float kAlpha = 0.1f;         // used by Leaky/PReLU/RReLU/ELU/Swish

// Generate deterministic input once
static std::vector<float> make_data(std::size_t N) {
    std::vector<float> v(N);
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-5.0f, 5.0f);
    for (auto& x : v) x = dist(gen);
    return v;
}

// Run scalar loop over a fresh copy; return sum to avoid DCE
template<bool T_ESTRIN>
static float bench_scalar(const std::vector<float>& src, ActivationType type, float alpha) {
    std::vector<float> v = src; // fresh copy per run
    for (auto& x : v)
        x = activateNaive<T_ESTRIN>(x, type, alpha);
    // lightweight reduction prevents the compiler from dead-storing
    float sum = 0.f;
    for (auto x : v) sum += x;
    return sum;
}

// Run vector kernel over a fresh copy; return sum to avoid DCE
template<bool T_ESTRIN>
static float bench_vector(const std::vector<float>& src, ActivationType type, float alpha) {
    std::vector<float> v = src; // fresh copy per run
    activate<T_ESTRIN>(v.data(), v.size(), type, alpha);
    float sum = 0.f;
    for (auto x : v) sum += x;
    return sum;
}

template<bool T_ESTRIN>
static float bench_vector_threads(const std::vector<float>& src, ActivationType type, float alpha) {
    std::vector<float> v = src; // fresh copy per run
    activateDenseParallel<T_ESTRIN>(v.data(), v.size(), type, alpha);
    float sum = 0.f;
    for (auto x : v) sum += x;
    return sum;
}


// Helper macro to emit two benchmarks per activation (Scalar / Vector)
#define BENCH_ACT(NAME, TYPE)                                                                \
BENCHMARK(#NAME " (Scalar)") { return bench_scalar<kHighPrecision>(data, TYPE, kAlpha); }; \
    BENCHMARK(#NAME " (Vector)") { return bench_vector<kHighPrecision>(data, TYPE, kAlpha); }; \

TEST_CASE("Benchmark: Scalar Loop vs Vectorized Kernel (all activations)", "[activation][bench]") {
    constexpr std::size_t N = 1'000'000;
    const auto data = make_data(N);

    // Exact / piecewise / curved — the whole enum:
    BENCH_ACT(Identity,     ActivationType::Identity)
    BENCH_ACT(Sigmoid,      ActivationType::Sigmoid)
    BENCH_ACT(Tanh,         ActivationType::Tanh)
    BENCH_ACT(HardTanh,     ActivationType::HardTanh)
    BENCH_ACT(ReLU,         ActivationType::ReLU)
    BENCH_ACT(LeakyReLU,    ActivationType::LeakyReLU)
    BENCH_ACT(PReLU,        ActivationType::PReLU)
    BENCH_ACT(RReLU,        ActivationType::RReLU)
    BENCH_ACT(ELU,          ActivationType::ELU)
    BENCH_ACT(SELU,         ActivationType::SELU)
    BENCH_ACT(GELU,         ActivationType::GELU)
    BENCH_ACT(ApproxGELU,   ActivationType::AproxGELU)
    BENCH_ACT(Swish,        ActivationType::Swish)
    BENCH_ACT(HSwish,       ActivationType::HSwish)
    BENCH_ACT(Mish,         ActivationType::Mish)
    BENCH_ACT(HMish,        ActivationType::HMish)
    BENCH_ACT(Softplus,     ActivationType::Softplus)
}

#endif
