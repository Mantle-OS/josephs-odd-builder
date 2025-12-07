#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ctx/job_stealing_ctx.h>

#include <runner.h>
#include <portal_evaluator.h>
#include <layer_factory.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::infer;
using namespace job::ai::evo;

Genome buildBrain(float weightVal)
{
    Genome g;
    LayerGene dense{};
    dense.type = layers::LayerType::Dense;
    dense.inputs = 1;
    dense.outputs = 1;
    dense.activation = comp::ActivationType::Identity;
    dense.weight_count = 2; // 1 weight + 1 bias = two things kissing lol

    g.architecture.push_back(dense);

    g.weights.push_back(weightVal);
    g.weights.push_back(0.0f);      // bias
    return g;
}

TEST_CASE("Portal Evaluator: The Geometry of Error", "[trainer][evaluator]")
{
    job::threads::JobStealerCtx ctx(4);
    PortalEvaluator evaluator;

    // goal: identity function f(x) = x * 2.0
    std::vector<TrainingSample> dataset = {
        { {1.0f}, {2.0f} },
        { {2.0f}, {4.0f} },
        { {-1.0f}, {-2.0f} }
    };

    // manifold a: weights = 2.0 (perfect alignment)
    Genome gPerfect = buildBrain(2.0f);
    Runner rPerfect(gPerfect, ctx.pool);

    // manifold b: weights = 0.5 (high curvature / distortion)
    Genome gBad = buildBrain(0.5f);
    Runner rBad(gBad, ctx.pool);

    float fitPerfect = evaluator.evaluate(rPerfect, dataset);
    float fitBad = evaluator.evaluate(rBad, dataset);
    JOB_LOG_INFO("[Portal Evaluator:] fitPerfect: {} fitBad: {}", fitPerfect, fitBad);
    REQUIRE(fitPerfect == Catch::Approx(1.0f));

    REQUIRE(fitBad < fitPerfect);
    REQUIRE(fitBad > 0.0f); // "should" still be valid
}
