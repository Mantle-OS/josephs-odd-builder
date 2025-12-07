#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>
#include <random>
#include <vector>
#include <algorithm>
#include <cmath>

#include <ctx/job_stealing_ctx.h>

#include <real_type.h>
#include <job_logger.h>

#include "activation_math.h"
#include "activation_types.h"
#include "activate_parallel.h"
#include "gdn_params.h"

using Catch::Approx;

using namespace job::ai::comp;
using namespace job::core;
using namespace job::threads;

static Approx approx(real_t value, real_t tol = real_t(1e-5))
{
    return Approx(static_cast<double>(value)).margin(static_cast<double>(tol));
}

TEST_CASE("activate() usage: basic scalar activations", "[activation][usage]")
{
    const real_t xPos  = real_t(2.0);
    const real_t xNeg  = real_t(-2.0);
    const real_t xZero = real_t(0);

    REQUIRE(activate(ActivationType::Identity, xPos)  == xPos);
    REQUIRE(activate(ActivationType::ReLU, xPos)  == xPos);
    REQUIRE(activate(ActivationType::ReLU, xZero) == xZero);
    REQUIRE(activate(ActivationType::ReLU, xNeg)  == real_t(0));

    const real_t alpha = real_t(0.1);
    REQUIRE(activate(ActivationType::LeakyReLU, xPos, alpha) == xPos);
    REQUIRE(activate(ActivationType::LeakyReLU, xNeg, alpha) == xNeg * alpha);
}

TEST_CASE("Sigmoid and Tanh behave as expected", "[activation][sigmoid][tanh]")
{
    REQUIRE(activate(ActivationType::Sigmoid, real_t(0)) == approx(real_t(0.5)));
    REQUIRE(activate(ActivationType::Tanh, real_t(0)) == approx(real_t(0)));
}

TEST_CASE("Softplus and ReLU6 are sane", "[activation][softplus][relu6]")
{
    // Softplus approximates ReLU for large positive x, and ~0 for large negative x
    const real_t xLargePos = real_t(20);
    const real_t xLargeNeg = real_t(-20);

    const real_t spPos = activate(ActivationType::Softplus, xLargePos);
    const real_t spNeg = activate(ActivationType::Softplus, xLargeNeg);

    REQUIRE(spPos == approx(xLargePos, real_t(1e-4))); // ln(1+e^x) ~ x
    REQUIRE(spNeg == approx(real_t(0), real_t(1e-4)));

    // ReLU6 clamps to [0, 6]
    REQUIRE(relu6(real_t(-1)) == real_t(0));
    REQUIRE(relu6(real_t(2))  == real_t(2));
    REQUIRE(relu6(real_t(10)) == real_t(6));
}

TEST_CASE("GELU and ApproxGELU are close friends", "[activation][gelu]")
{
    const real_t xs[] = {
        real_t(-3), real_t(-1),
        real_t(0),  real_t(1),
        real_t(3)
    };

    for (real_t x : xs) {
        const real_t exact     = gelu(x);
        const real_t approxVal = approxGelu(x);

        // Allow a small absolute difference for the tanh approximation
        REQUIRE(approxVal == approx(exact, real_t(1e-3)));
    }
}

TEST_CASE("Swish, HSwish, Mish, HMish behave roughly as advertised", "[activation][swish][mish]")
{
    const real_t xPos  = real_t(2.0);
    const real_t xNeg  = real_t(-2.0);
    const real_t xZero = real_t(0);

    // Swish: x * sigmoid(x)
    const real_t swishZero = swish(xZero);
    REQUIRE(swishZero == approx(real_t(0)));

    const real_t swishPos = swish(xPos);
    REQUIRE(swishPos > real_t(0));
    REQUIRE(swishPos < xPos); // sigmoid(x) < 1

    // Hard-Swish: x * relu6(x+3)/6
    const real_t hsNeg  = hSwish(real_t(-4));
    const real_t hsZero = hSwish(real_t(0));
    const real_t hsMid  = hSwish(real_t(1));
    const real_t hsPos  = hSwish(real_t(4));

    REQUIRE(hsNeg  == real_t(0)); // x + 3 <= 0 => relu6 = 0 => product = 0
    REQUIRE(hsZero == real_t(0));
    REQUIRE(hsMid  >  real_t(0)); // in the sloped positive region
    REQUIRE(hsPos  >  real_t(0));

    // mish: x * tanh(softplus(x)), basic sign sanity ... I think
    const real_t mishNeg = mish(xNeg);
    const real_t mishPos = mish(xPos);
    REQUIRE(mishNeg < real_t(0));
    REQUIRE(mishPos > real_t(0));

    const real_t hmNeg = hMish(real_t(-1.0)); // stop being so damn negitive.....
    const real_t hmPos = hMish(xPos);
    REQUIRE(hmNeg < real_t(0));
    REQUIRE(hmPos > real_t(0));
}

TEST_CASE("SELU uses the baked-in magic numbers", "[activation][selu]")
{
    const real_t xPos = real_t(1.0);
    const real_t xNeg = real_t(-1.0);

    const real_t yPos = selu(xPos);
    const real_t yNeg = selu(xNeg);

    // Positive: scaled linear
    REQUIRE(yPos == approx(kSeluScale * xPos)); // stop being so damn positive .....

    const real_t expectedNeg =
        kSeluScale * kSeluAlpha * (std::exp(xNeg) - real_t(1)); // make up your mind already !!!
    REQUIRE(yNeg == approx(expectedNeg));
}

TEST_CASE("maxout reduces a group to its maximum", "[activation][maxout]")
{
    std::vector<real_t> vals = {
        real_t(-1), real_t(0),
        real_t(3),  real_t(2)
    };

    const auto maxVal = maxout(vals.begin(), vals.end());
    REQUIRE(maxVal == real_t(3));

    // Empty range: currently defined as T(0)
    std::vector<real_t> empty;
    const auto maxEmpty = maxout(empty.begin(), empty.end());
    REQUIRE(maxEmpty == real_t(0));
}

TEST_CASE("rRelu primitive applies a fixed negative slope", "[activation][rrelu-primitive]")
{
    const real_t xPos  = real_t(2.0);
    const real_t xNeg  = real_t(-2.0);
    const real_t alpha = real_t(0.25);

    const real_t yPos = rRelu(xPos, alpha);
    const real_t yNeg = rRelu(xNeg, alpha);

    REQUIRE(yPos == xPos);
    REQUIRE(yNeg == xNeg * alpha);
}

TEST_CASE("activate() dispatches all supported activation types", "[activation][dispatch]")
{
    const real_t x = real_t(0.5);

    // Smoke-test: ensure wiring doesn't fall through or explode.
    (void)activate(ActivationType::Identity,   x);
    (void)activate(ActivationType::Sigmoid,    x);
    (void)activate(ActivationType::Tanh,       x);
    (void)activate(ActivationType::HardTanh,   x);
    (void)activate(ActivationType::ReLU,       x);
    (void)activate(ActivationType::LeakyReLU,  x, real_t(0.1));
    (void)activate(ActivationType::PReLU,      x, real_t(0.2));
    (void)activate(ActivationType::ELU,        x, real_t(1.0));
    (void)activate(ActivationType::SELU,       x);
    (void)activate(ActivationType::GELU,       x);
    (void)activate(ActivationType::AproxGELU,  x);
    (void)activate(ActivationType::Swish,      x);
    (void)activate(ActivationType::HSwish,     x);
    (void)activate(ActivationType::Mish,       x);
    (void)activate(ActivationType::HMish,      x);
    (void)activate(ActivationType::Softplus,   x);
    (void)activate(ActivationType::RReLU,      x, real_t(0.3));

    // Maxout / GDN are intentionally not implemented as pointwise in activate()
    // just make sure they don't throw or crash.
    (void)activate(ActivationType::Maxout,     x);
    (void)activate(ActivationType::GDN,        x);
}

TEST_CASE("GDN performs cross-channel normalization", "[activation][gdn]")
{
    // Tiny 3-channel example with diagonal gamma to keep math simple.
    std::vector<real_t> x = {
        real_t(1.0),
        real_t(2.0),
        real_t(3.0),
    };

    GDNParams p;
    p.beta = {
        real_t(1),
        real_t(1),
        real_t(1)
    };
    p.gamma = {
               { real_t(0.5), real_t(0.0), real_t(0.0) },
               { real_t(0.0), real_t(0.5), real_t(0.0) },
               { real_t(0.0), real_t(0.0), real_t(0.5) },
               };

    const auto y = gdn(x, p);

    REQUIRE(y.size() == x.size());

    // For this diagonal gamma, each channel:
    // y_i = x_i / sqrt(beta_i + gamma_ii * x_i^2)
    for (std::size_t i = 0; i < x.size(); ++i) {
        const real_t denom =
            p.beta[i] + p.gamma[i][i] * x[i] * x[i];
        const real_t expected = x[i] / std::sqrt(denom);
        REQUIRE(y[i] == approx(expected));
    }
}


TEST_CASE("Vectorized Dispatcher matches Scalar Reference", "[activation][vector]")
{
    JobStealerCtx ctx(4);

    constexpr size_t N = 65536;
    std::vector<float> input(N);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
    for(auto& x : input)
        x = dist(gen);

    std::vector<float> scalarRef = input;
    std::vector<float> vectorOut = input;

    for(size_t i=0; i<N; ++i)
        scalarRef[i] = activate(ActivationType::ReLU, scalarRef[i]);

    activate_buffer(*ctx.pool, vectorOut.data(), N, ActivationType::ReLU);

    for(size_t i=0; i<N; ++i)
        if (std::abs(scalarRef[i] - vectorOut[i]) > 1e-5f)
            FAIL("Mismatch at index " << i << ": Scalar=" << scalarRef[i] << " Vector=" << vectorOut[i]);

    SUCCEED("ReLU Vectorization matches Scalar");
}

TEST_CASE("Vectorized Swish (Approx) vs Scalar (Exact)", "[activation][vector][swish]")
{
    JobStealerCtx ctx(4);
    constexpr size_t N = 1024;
    std::vector<float> input(N);
    for(size_t i=0; i<N; ++i)
        input[i] = (float)i * 0.01f - 5.0f; // -5 to 5 range

    std::vector<float> vecOut = input;

    // meh whatever
    activate_buffer(*ctx.pool, vecOut.data(), N, ActivationType::Swish);

    // run scalar (std exp)
    for(size_t i=0; i<N; ++i) {
        float scalar = activate(ActivationType::Swish, input[i]);
        float diff = std::abs(scalar - vecOut[i]);
        if (std::abs(scalar) > 1e-3f) {
            float rel_err = diff / std::abs(scalar);
            REQUIRE(rel_err < 0.05f); // Make sure the painter(Johann von Schraudolph) paints in the lines
        }
    }
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Activation performance: bulk ReLU", "[activation][bench]")
{
    constexpr std::size_t N = 1'000'000;
    std::vector<real_t> data(N);
    std::vector<real_t> out(N);

    // Fill with some garbage
    for (std::size_t i = 0; i < N; ++i)
        data[i] = (i % 2 == 0) ? real_t(i * 0.001) : real_t(-i * 0.001);

    BENCHMARK("ReLU loop (direct)") {
        for (std::size_t i = 0; i < N; ++i)
            out[i] = relu(data[i]);
        return out[123]; // keep compiler honest
    };
}

TEST_CASE("GDN performance: small vs medium channels", "[activation][gdn][bench]")
{
    constexpr std::size_t C_small = 16;
    // constexpr std::size_t C_med   = 128;

    GDNParams p_small;
    p_small.beta.assign(C_small, real_t(1));
    p_small.gamma.assign(C_small, std::vector<real_t>(C_small, real_t(0.0)));
    for (std::size_t i = 0; i < C_small; ++i)
        p_small.gamma[i][i] = real_t(0.5);

    std::vector<real_t> x_small(C_small, real_t(1.0));
    BENCHMARK("GDN C=16") {
        auto y = gdn(x_small, p_small);
        return y[0];
    };
}

TEST_CASE("Activation Benchmark: Scalar vs Vector", "[activation][bench]")
{
    JobStealerCtx ctx(8);

    constexpr std::size_t N = 1'000'000; // 1M floats = 4MB data ... math silly math
    std::vector<real_t> data(N);

    // get out of here homunculus
    for (std::size_t homunculus = 0; homunculus < N; ++homunculus)
        data[homunculus] = (homunculus % 2 == 0) ?
                      real_t(homunculus * 0.001) :
                      real_t(-homunculus * 0.001);

    // destructive benchmarks !!
    std::vector<real_t> bufA = data;
    std::vector<real_t> bufB = data;

    BENCHMARK("ReLU (Scalar Loop)") {
        for (std::size_t homunculus = 0; homunculus < N; ++homunculus)
            bufA[homunculus] = activate(ActivationType::ReLU, bufA[homunculus]);
        return bufA[0];
    };

    BENCHMARK("ReLU (Vectorized Dispatch)") {
        activate_buffer(*ctx.pool, bufB.data(), N, ActivationType::ReLU);
        return bufB[0];
    };

    BENCHMARK("Swish (Scalar Loop)") {
        for (std::size_t i = 0; i < N; ++i)
            bufA[i] = activate(ActivationType::Swish, bufA[i]);
        return bufA[0];
    };

    BENCHMARK("Swish (Vectorized (By the painter))") {
        activate_buffer(*ctx.pool, bufB.data(), N, ActivationType::Swish);
        return bufB[0];
    };
}



#endif
