#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

#include <real_type.h>
#include <job_logger.h>

#include "activation_math.h"
#include "activation_types.h"
#include "gdn_params.h"

using Catch::Approx;
using job::ai::base::ActivationType;
using namespace job;
using namespace job::ai::base;

static Approx approx(core::real_t value, core::real_t tol = core::real_t(1e-5))
{
    // Absolute-tolerance helper so we don't abuse relative epsilon near zero.
    return Approx(static_cast<double>(value))
    .margin(static_cast<double>(tol));
}

TEST_CASE("activate() usage: basic scalar activations", "[activation][usage]")
{
    const core::real_t xPos  = core::real_t(2.0);
    const core::real_t xNeg  = core::real_t(-2.0);
    const core::real_t xZero = core::real_t(0);

    // Identity stays itself
    REQUIRE(activate(ActivationType::Identity, xPos)  == xPos);
    REQUIRE(activate(ActivationType::Identity, xZero) == xZero);

    // ReLU: zero out the grumps
    REQUIRE(activate(ActivationType::ReLU, xPos)  == xPos);
    REQUIRE(activate(ActivationType::ReLU, xZero) == xZero);
    REQUIRE(activate(ActivationType::ReLU, xNeg)  == core::real_t(0));

    // LeakyReLU: negative side is scaled by alpha
    const core::real_t alpha = core::real_t(0.1);
    REQUIRE(activate(ActivationType::LeakyReLU, xPos, alpha) == xPos);
    REQUIRE(activate(ActivationType::LeakyReLU, xNeg, alpha) == xNeg * alpha);

    // PReLU behaves like LeakyReLU when you pass alpha
    REQUIRE(activate(ActivationType::PReLU, xNeg, alpha) == xNeg * alpha);

    // HardTanh clamps to [-1, 1]
    REQUIRE(activate(ActivationType::HardTanh, core::real_t(2.5))  == core::real_t(1));
    REQUIRE(activate(ActivationType::HardTanh, core::real_t(-3.0)) == core::real_t(-1));
    REQUIRE(activate(ActivationType::HardTanh, core::real_t(0.5))  == core::real_t(0.5));
}

TEST_CASE("Sigmoid and Tanh behave as expected", "[activation][sigmoid][tanh]")
{
    const core::real_t xZero    = core::real_t(0);
    const core::real_t sigZero  = activate(ActivationType::Sigmoid, xZero);
    const core::real_t tanhZero = activate(ActivationType::Tanh, xZero);

    // Midpoint of sigmoid and tanh
    REQUIRE(sigZero  == approx(core::real_t(0.5)));
    REQUIRE(tanhZero == approx(core::real_t(0)));

    // Sigmoid squashes into (0, 1)
    const core::real_t sigLargePos = activate(ActivationType::Sigmoid, core::real_t(10));
    const core::real_t sigLargeNeg = activate(ActivationType::Sigmoid, core::real_t(-10));
    REQUIRE(sigLargePos == approx(core::real_t(1), core::real_t(1e-4)));
    // Near zero in absolute terms
    REQUIRE(sigLargeNeg == approx(core::real_t(0), core::real_t(1e-4)));

    // tanh squashes into (-1, 1)
    const core::real_t tLargePos = activate(ActivationType::Tanh, core::real_t(10));
    const core::real_t tLargeNeg = activate(ActivationType::Tanh, core::real_t(-10));
    REQUIRE(tLargePos == approx(core::real_t(1), core::real_t(1e-4)));
    REQUIRE(tLargeNeg == approx(core::real_t(-1), core::real_t(1e-4)));
}

TEST_CASE("Softplus and ReLU6 are sane", "[activation][softplus][relu6]")
{
    // Softplus approximates ReLU for large positive x, and ~0 for large negative x
    const core::real_t xLargePos = core::real_t(20);
    const core::real_t xLargeNeg = core::real_t(-20);

    const core::real_t spPos = activate(ActivationType::Softplus, xLargePos);
    const core::real_t spNeg = activate(ActivationType::Softplus, xLargeNeg);

    REQUIRE(spPos == approx(xLargePos, core::real_t(1e-4))); // ln(1+e^x) ~ x
    REQUIRE(spNeg == approx(core::real_t(0), core::real_t(1e-4)));

    // ReLU6 clamps to [0, 6]
    REQUIRE(relu6(core::real_t(-1)) == core::real_t(0));
    REQUIRE(relu6(core::real_t(2))  == core::real_t(2));
    REQUIRE(relu6(core::real_t(10)) == core::real_t(6));
}

TEST_CASE("GELU and ApproxGELU are close friends", "[activation][gelu]")
{
    const core::real_t xs[] = {
        core::real_t(-3), core::real_t(-1),
        core::real_t(0),  core::real_t(1),
        core::real_t(3)
    };

    for (core::real_t x : xs) {
        const core::real_t exact     = gelu(x);
        const core::real_t approxVal = approxGelu(x);

        // Allow a small absolute difference for the tanh approximation
        REQUIRE(approxVal == approx(exact, core::real_t(1e-3)));
    }
}

TEST_CASE("Swish, HSwish, Mish, HMish behave roughly as advertised", "[activation][swish][mish]")
{
    const core::real_t xPos  = core::real_t(2.0);
    const core::real_t xNeg  = core::real_t(-2.0);
    const core::real_t xZero = core::real_t(0);

    // Swish: x * sigmoid(x)
    const core::real_t swishZero = swish(xZero);
    REQUIRE(swishZero == approx(core::real_t(0)));

    const core::real_t swishPos = swish(xPos);
    REQUIRE(swishPos > core::real_t(0));
    REQUIRE(swishPos < xPos); // sigmoid(x) < 1

    // Hard-Swish: x * relu6(x+3)/6
    const core::real_t hsNeg  = hSwish(core::real_t(-4));
    const core::real_t hsZero = hSwish(core::real_t(0));
    const core::real_t hsMid  = hSwish(core::real_t(1));
    const core::real_t hsPos  = hSwish(core::real_t(4));

    REQUIRE(hsNeg  == core::real_t(0)); // x + 3 <= 0 => relu6 = 0 => product = 0
    REQUIRE(hsZero == core::real_t(0));
    REQUIRE(hsMid  >  core::real_t(0)); // in the sloped positive region
    REQUIRE(hsPos  >  core::real_t(0));

    // Mish: x * tanh(softplus(x)), basic sign sanity
    const core::real_t mishNeg = mish(xNeg);
    const core::real_t mishPos = mish(xPos);
    REQUIRE(mishNeg < core::real_t(0));
    REQUIRE(mishPos > core::real_t(0));

    const core::real_t hmNeg = hMish(core::real_t(-1.0)); // stop being so dang negitive.....
    const core::real_t hmPos = hMish(xPos);
    REQUIRE(hmNeg < core::real_t(0));
    REQUIRE(hmPos > core::real_t(0));
}

TEST_CASE("SELU uses the baked-in magic numbers", "[activation][selu]")
{
    const core::real_t xPos = core::real_t(1.0);
    const core::real_t xNeg = core::real_t(-1.0);

    const core::real_t yPos = selu(xPos);
    const core::real_t yNeg = selu(xNeg);

    // Positive: scaled linear
    REQUIRE(yPos == approx(kSeluScale * xPos));

    // Negative: kScale * kAlpha * (exp(x) - 1)
    const core::real_t expectedNeg =
        kSeluScale * kSeluAlpha * (std::exp(xNeg) - core::real_t(1));
    REQUIRE(yNeg == approx(expectedNeg));
}

TEST_CASE("maxout reduces a group to its maximum", "[activation][maxout]")
{
    std::vector<core::real_t> vals = {
        core::real_t(-1), core::real_t(0),
        core::real_t(3),  core::real_t(2)
    };

    const auto maxVal = maxout(vals.begin(), vals.end());
    REQUIRE(maxVal == core::real_t(3));

    // Empty range: currently defined as T(0)
    std::vector<core::real_t> empty;
    const auto maxEmpty = maxout(empty.begin(), empty.end());
    REQUIRE(maxEmpty == core::real_t(0));
}

TEST_CASE("rRelu primitive applies a fixed negative slope", "[activation][rrelu-primitive]")
{
    const core::real_t xPos  = core::real_t(2.0);
    const core::real_t xNeg  = core::real_t(-2.0);
    const core::real_t alpha = core::real_t(0.25);

    const core::real_t yPos = rRelu(xPos, alpha);
    const core::real_t yNeg = rRelu(xNeg, alpha);

    REQUIRE(yPos == xPos);
    REQUIRE(yNeg == xNeg * alpha);
}

TEST_CASE("activate() dispatches all supported activation types", "[activation][dispatch]")
{
    const core::real_t x = core::real_t(0.5);

    // Smoke-test: ensure wiring doesn't fall through or explode.
    (void)activate(ActivationType::Identity,   x);
    (void)activate(ActivationType::Sigmoid,    x);
    (void)activate(ActivationType::Tanh,       x);
    (void)activate(ActivationType::HardTanh,   x);
    (void)activate(ActivationType::ReLU,       x);
    (void)activate(ActivationType::LeakyReLU,  x, core::real_t(0.1));
    (void)activate(ActivationType::PReLU,      x, core::real_t(0.2));
    (void)activate(ActivationType::ELU,        x, core::real_t(1.0));
    (void)activate(ActivationType::SELU,       x);
    (void)activate(ActivationType::GELU,       x);
    (void)activate(ActivationType::AproxGELU,  x);
    (void)activate(ActivationType::Swish,      x);
    (void)activate(ActivationType::HSwish,     x);
    (void)activate(ActivationType::Mish,       x);
    (void)activate(ActivationType::HMish,      x);
    (void)activate(ActivationType::Softplus,   x);
    (void)activate(ActivationType::RReLU,      x, core::real_t(0.3));

    // Maxout / GDN are intentionally not implemented as pointwise in activate()
    // We just make sure they don't throw or crash.
    (void)activate(ActivationType::Maxout,     x);
    (void)activate(ActivationType::GDN,        x);
}

TEST_CASE("GDN performs cross-channel normalization", "[activation][gdn]")
{
    // Tiny 3-channel example with diagonal gamma to keep math simple.
    std::vector<core::real_t> x = {
        core::real_t(1.0),
        core::real_t(2.0),
        core::real_t(3.0),
    };

    GDNParams p;
    p.beta = {
        core::real_t(1),
        core::real_t(1),
        core::real_t(1)
    };
    p.gamma = {
               { core::real_t(0.5), core::real_t(0.0), core::real_t(0.0) },
               { core::real_t(0.0), core::real_t(0.5), core::real_t(0.0) },
               { core::real_t(0.0), core::real_t(0.0), core::real_t(0.5) },
               };

    const auto y = gdn(x, p);

    REQUIRE(y.size() == x.size());

    // For this diagonal gamma, each channel:
    // y_i = x_i / sqrt(beta_i + gamma_ii * x_i^2)
    for (std::size_t i = 0; i < x.size(); ++i) {
        const core::real_t denom =
            p.beta[i] + p.gamma[i][i] * x[i] * x[i];
        const core::real_t expected = x[i] / std::sqrt(denom);
        REQUIRE(y[i] == approx(expected));
    }
}





TEST_CASE("Activation performance: bulk ReLU", "[activation][bench]") {
    constexpr std::size_t N = 1'000'000;
    std::vector<core::real_t> data(N);
    std::vector<core::real_t> out(N);

    // Fill with some garbage
    for (std::size_t i = 0; i < N; ++i)
        data[i] = (i % 2 == 0) ? core::real_t(i * 0.001) : core::real_t(-i * 0.001);

    BENCHMARK("ReLU loop (direct)") {
        for (std::size_t i = 0; i < N; ++i)
            out[i] = relu(data[i]);
        return out[123]; // keep compiler honest
    };
}


TEST_CASE("GDN performance: small vs medium channels", "[activation][gdn][bench]") {
    constexpr std::size_t C_small = 16;
    // constexpr std::size_t C_med   = 128; << unused

    GDNParams p_small;
    p_small.beta.assign(C_small, core::real_t(1));
    p_small.gamma.assign(C_small, std::vector<core::real_t>(C_small, core::real_t(0.0)));
    for (std::size_t i = 0; i < C_small; ++i)
        p_small.gamma[i][i] = core::real_t(0.5);

    std::vector<core::real_t> x_small(C_small, core::real_t(1.0));

    BENCHMARK("GDN C=16") {
        auto y = gdn(x_small, p_small);
        return y[0];
    };

    // Same pattern for C=128 if you want
}






