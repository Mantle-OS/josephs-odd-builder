#include <cstdint>
#include <limits>
#include <stdexcept>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <real_type.h>

#include <ggml-opt.h>

#include <job_ggml_opt_adamw_params.h>
#include <job_ggml_opt_optimizer_params.h>
#include <job_ggml_opt_optimizer_schedule.h>
#include <job_ggml_opt_params.h>
#include <job_ggml_opt_sgd_params.h>

#include "test_ggml_utils.h"

using Catch::Approx;

// Block one: usage / examples
TEST_CASE( "AdamW optimizer parameters expose upstream defaults", "[ggml][opt][params][adamw][usage]" )
{
    JobGgmlOptAdamWParams params;

    REQUIRE(params.isValid());

    REQUIRE(params.alpha()  == Approx(0.001f));
    REQUIRE(params.beta1()  == Approx(0.9f));
    REQUIRE(params.beta2()  == Approx(0.999f));
    REQUIRE(params.eps()    == Approx(1.0e-8f));
    REQUIRE(params.wd()     == Approx(0.0f));
}

TEST_CASE( "AdamW optimizer parameters support validated mutation", "[ggml][opt][params][adamw][usage][mutation]" )
{
    JobGgmlOptAdamWParams params;

    params.setAlpha(5.0e-4f);
    params.setBeta1(0.85f);
    params.setBeta2(0.995f);
    params.setEps(1.0e-7f);
    params.setWd(0.01f);

    REQUIRE(params.isValid());

    REQUIRE(params.alpha()  == Approx(5.0e-4f));
    REQUIRE(params.beta1()  == Approx(0.85f));
    REQUIRE(params.beta2()  == Approx(0.995f));
    REQUIRE(params.eps()    == Approx(1.0e-7f));
    REQUIRE(params.wd()     == Approx(0.01f));
}

TEST_CASE( "AdamW optimizer parameters reset to defaults", "[ggml][opt][params][adamw][usage][reset]" )
{
    JobGgmlOptAdamWParams params{
        5.0e-4f,
        0.8f,
        0.95f,
        1.0e-6f,
        0.1f
    };

    REQUIRE(params.isValid());
    params.reset();
    REQUIRE(params.alpha() == Approx(0.001f));
    REQUIRE(params.beta1() == Approx(0.9f));
    REQUIRE(params.beta2() == Approx(0.999f));
    REQUIRE(params.eps() == Approx(1.0e-8f));
    REQUIRE(params.wd() == Approx(0.0f));
}

TEST_CASE( "SGD optimizer parameters expose upstream defaults", "[ggml][opt][params][sgd][usage]" )
{
    JobGgmlOptSgdParams params;
    REQUIRE(params.isValid());
    REQUIRE(params.alpha() == Approx(1.0e-3f));
    REQUIRE(params.wd() == Approx(0.0f));
}

TEST_CASE( "SGD optimizer parameters support validated mutation", "[ggml][opt][params][sgd][usage][mutation]" )
{
    JobGgmlOptSgdParams params;

    params.setAlpha(2.5e-3f);
    params.setWd(0.025f);
    REQUIRE(params.isValid());
    REQUIRE(params.alpha() == Approx(2.5e-3f));
    REQUIRE(params.wd() == Approx(0.025f));
}

TEST_CASE( "SGD optimizer parameters reset to defaults", "[ggml][opt][params][sgd][usage][reset]" )
{
    JobGgmlOptSgdParams params{ 0.5f, 0.2f };
    REQUIRE(params.isValid());

    params.reset();
    REQUIRE(params.alpha() == Approx(1.0e-3f));
    REQUIRE(params.wd() == Approx(0.0f));
}

TEST_CASE( "Optimizer parameter aggregate owns AdamW and SGD groups", "[ggml][opt][params][aggregate][usage]" )
{
    JobGgmlOptOptimizerParams params;
    REQUIRE(params.isValid());

    REQUIRE(params.adamw() != nullptr);
    REQUIRE(params.sgd() != nullptr);
    REQUIRE(params.adamw()->isValid());
    REQUIRE(params.sgd()->isValid());
    REQUIRE(params.adamw()->alpha()     == Approx(0.001f));
    REQUIRE(params.adamw()->beta1()     == Approx(0.9f));
    REQUIRE(params.adamw()->beta2()     == Approx(0.999f));
    REQUIRE(params.adamw()->eps()       == Approx(1.0e-8f));
    REQUIRE(params.adamw()->wd()        == Approx(0.0f));
    REQUIRE(params.sgd()->alpha()       == Approx(1.0e-3f));
    REQUIRE(params.sgd()->wd()          == Approx(0.0f));
}

TEST_CASE( "Optimizer parameter aggregate converts to native GGML values", "[ggml][opt][params][aggregate][usage][native]" )
{
    JobGgmlOptOptimizerParams params;

    params.adamw()->setAlpha(4.0e-4f);
    params.adamw()->setBeta1(0.8f);
    params.adamw()->setBeta2(0.98f);
    params.adamw()->setEps(1.0e-6f);
    params.adamw()->setWd(0.05f);
    params.sgd()->setAlpha(7.5e-3f);
    params.sgd()->setWd(0.15f);

    const struct ggml_opt_optimizer_params native = params.optimizerParams();
    REQUIRE(native.adamw.alpha == Approx(4.0e-4f));
    REQUIRE(native.adamw.beta1 == Approx(0.8f));
    REQUIRE(native.adamw.beta2 == Approx(0.98f));
    REQUIRE(native.adamw.eps == Approx(1.0e-6f));
    REQUIRE(native.adamw.wd == Approx(0.05f));
    REQUIRE(native.sgd.alpha == Approx(7.5e-3f));
    REQUIRE(native.sgd.wd == Approx(0.15f));
}

TEST_CASE( "Optimizer parameter aggregate imports native GGML values", "[ggml][opt][params][aggregate][usage][native]" )
{
    const struct ggml_opt_optimizer_params native{
        {
            2.0e-4f,
            0.75f,
            0.97f,
            1.0e-5f,
            0.025f
        },
        {
            3.0e-3f,
            0.075f
        }
    };

    JobGgmlOptOptimizerParams params{ native };
    REQUIRE(params.isValid());

    REQUIRE(params.adamw()->alpha() == Approx(native.adamw.alpha));
    REQUIRE(params.adamw()->beta1() == Approx(native.adamw.beta1));
    REQUIRE(params.adamw()->beta2() == Approx(native.adamw.beta2));
    REQUIRE(params.adamw()->eps() == Approx(native.adamw.eps));
    REQUIRE(params.adamw()->wd() == Approx(native.adamw.wd));

    REQUIRE(params.sgd()->alpha() == Approx(native.sgd.alpha));
    REQUIRE(params.sgd()->wd() == Approx(native.sgd.wd));
}

TEST_CASE( "Optimizer parameter aggregate replaces native values atomically", "[ggml][opt][params][aggregate][usage][mutation]" )
{
    JobGgmlOptOptimizerParams params;
    const struct ggml_opt_optimizer_params replacement{
        {
            8.0e-4f,
            0.88f,
            0.996f,
            1.0e-7f,
            0.02f
        },
        {
            6.0e-3f,
            0.04f
        }
    };

    params.setOptimizerParams( replacement );
    REQUIRE(params.isValid());

    const struct ggml_opt_optimizer_params native = params.optimizerParams();
    REQUIRE(native.adamw.alpha  == Approx(replacement.adamw.alpha));
    REQUIRE(native.adamw.beta1  == Approx(replacement.adamw.beta1));
    REQUIRE(native.adamw.beta2  == Approx(replacement.adamw.beta2));
    REQUIRE(native.adamw.eps    == Approx(replacement.adamw.eps));
    REQUIRE(native.adamw.wd     == Approx(replacement.adamw.wd));
    REQUIRE(native.sgd.alpha    == Approx(replacement.sgd.alpha));
    REQUIRE(native.sgd.wd       == Approx(replacement.sgd.wd));
}

TEST_CASE( "Optimizer parameter aggregate reset restores upstream defaults", "[ggml][opt][params][aggregate][usage][reset]" )
{
    JobGgmlOptOptimizerParams params;

    params.adamw()->setAlpha(0.25f);
    params.adamw()->setBeta1(0.5f);
    params.adamw()->setBeta2(0.75f);
    params.adamw()->setEps(0.125f);
    params.adamw()->setWd(0.5f);
    params.sgd()->setAlpha(0.5f);
    params.sgd()->setWd(0.25f);
    params.reset();

    const struct ggml_opt_optimizer_params native = params.optimizerParams();
    REQUIRE(native.adamw.alpha  == Approx(0.001f));
    REQUIRE(native.adamw.beta1  == Approx(0.9f));
    REQUIRE(native.adamw.beta2  == Approx(0.999f));
    REQUIRE(native.adamw.eps    == Approx(1.0e-8f));
    REQUIRE(native.adamw.wd     == Approx(0.0f));
    REQUIRE(native.sgd.alpha    == Approx(1.0e-3f));
    REQUIRE(native.sgd.wd       == Approx(0.0f));
}

TEST_CASE( "Optimizer schedule bridges a C callback to a C++ callable", "[ggml][opt][params][schedule][usage]" )
{
    auto schedule = JobGgmlOptOptimizerSchedule::createUniq( []( JobGgmlOptOptimizerParams &params, std::int64_t callCount ) {
        params.adamw()->setAlpha(static_cast<float>(callCount) * 1.0e-4f);
        params.sgd()->setAlpha(static_cast<float>(callCount) * 1.0e-3f);
    });

    REQUIRE(schedule != nullptr);
    REQUIRE(schedule->isValid());
    REQUIRE(schedule->callCount() == 0);

    const ggml_opt_get_optimizer_params callback = schedule->nativeCallback();

    REQUIRE(callback != nullptr);
    REQUIRE(schedule->nativeUserData() == schedule.get());

    const struct ggml_opt_optimizer_params first = callback( schedule->nativeUserData() );
    REQUIRE(schedule->callCount() == 1);
    REQUIRE(first.adamw.alpha == Approx(1.0e-4f));
    REQUIRE(first.sgd.alpha == Approx(1.0e-3f));

    const struct ggml_opt_optimizer_params second = callback( schedule->nativeUserData() );
    REQUIRE(schedule->callCount() == 2);
    REQUIRE(second.adamw.alpha == Approx(2.0e-4f));
    REQUIRE(second.sgd.alpha == Approx(2.0e-3f));
}

TEST_CASE( "Optimizer schedule callback may capture application state", "[ggml][opt][params][schedule][usage][capture]" )
{
    float learningRate = 5.0e-4f;
    JobGgmlOptOptimizerSchedule schedule{ [&learningRate]( JobGgmlOptOptimizerParams &params, std::int64_t ) {
            params.adamw()->setAlpha( learningRate );
        }
    };

    const auto callback = schedule.nativeCallback();
    const struct ggml_opt_optimizer_params first = callback( schedule.nativeUserData() );
    REQUIRE(first.adamw.alpha == Approx(5.0e-4f));

    learningRate = 1.0e-4f;
    const struct ggml_opt_optimizer_params second = callback( schedule.nativeUserData() );
    REQUIRE(second.adamw.alpha == Approx(1.0e-4f));
}

TEST_CASE( "Optimizer schedule reset restarts its callback count", "[ggml][opt][params][schedule][usage][reset]" )
{
    JobGgmlOptOptimizerSchedule schedule{ []( JobGgmlOptOptimizerParams &, std::int64_t ) { } };

    const auto callback = schedule.nativeCallback();
    callback( schedule.nativeUserData() );
    callback( schedule.nativeUserData() );
    REQUIRE(schedule.callCount() == 2);

    schedule.resetCallCount();
    REQUIRE(schedule.callCount() == 0);

    callback(schedule.nativeUserData() );
    REQUIRE(schedule.callCount() == 1);
}

TEST_CASE( "Optimization parameters expose GGML defaults", "[ggml][opt][params][context_params][usage]" )
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams( fixture );
    REQUIRE(params != nullptr);
    REQUIRE(params->isValid());

    REQUIRE(params->backendSched() == fixture.scheduler().get());
    REQUIRE(params->computeContext() == nullptr);
    REQUIRE(params->inputs() == nullptr);
    REQUIRE(params->outputs() == nullptr);

    REQUIRE_FALSE(params->usesStaticGraphs());

    REQUIRE(params->lossType() == JobGgmlOptLossType::Mean);
    REQUIRE(params->ggmlLossType() == GGML_OPT_LOSS_TYPE_MEAN);

    REQUIRE(params->buildType() == JobGgmlOptBuildType::Opt);
    REQUIRE(params->ggmlBuildType() == GGML_OPT_BUILD_TYPE_OPT);

    REQUIRE(params->optPeriod() == 1);

    REQUIRE(params->getOptimizerParams() == ggml_opt_get_default_optimizer_params);
    REQUIRE(params->getOptimizerParamsUserData() == nullptr);
    REQUIRE(params->ggmlOptimizer() == GGML_OPT_OPTIMIZER_TYPE_ADAMW);
}

TEST_CASE( "Optimization parameters convert to the native aggregate", "[ggml][opt][params][context_params][usage][native]" )
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture, JobGgmlOptLossType::Sum);
    REQUIRE(params != nullptr);

    params->setGgmlBuildType(GGML_OPT_BUILD_TYPE_GRAD);
    params->setOptPeriod(4);

    params->setGgmlOptimizer(GGML_OPT_OPTIMIZER_TYPE_SGD);
    const struct ggml_opt_params native = params->optParams();

    REQUIRE(native.backend_sched    == fixture.scheduler()->scheduler());
    REQUIRE(native.ctx_compute      == nullptr);
    REQUIRE(native.inputs           == nullptr);
    REQUIRE(native.outputs          == nullptr);
    REQUIRE(native.loss_type        == GGML_OPT_LOSS_TYPE_SUM);
    REQUIRE(native.build_type       == GGML_OPT_BUILD_TYPE_GRAD);
    REQUIRE(native.opt_period       == 4);
    REQUIRE(native.get_opt_pars     == ggml_opt_get_default_optimizer_params );
    REQUIRE(native.get_opt_pars_ud  == nullptr);
    REQUIRE(native.optimizer        == GGML_OPT_OPTIMIZER_TYPE_SGD);
}

TEST_CASE("Optimization parameters describe a static graph configuration", "[ggml][opt][params][context_params][usage][static_graph]")
{
    CpuSchedulerFixture fixture;
    auto context = JobGgmlContext::createUniqMetadata(2);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());
    auto inputs = context->newTensor2d(JobGgmlType::F32, 4, 2);
    auto outputs = context->newTensor2d(JobGgmlType::F32, 1, 2);
    REQUIRE(inputs != nullptr);
    REQUIRE(outputs != nullptr);

    auto params = createDefaultOptParams(fixture);
    params->setComputeContext(context.get());
    params->setInputs(inputs.get());
    params->setOutputs(outputs.get());
    REQUIRE(params->usesStaticGraphs());
    REQUIRE(params->isValid());

    const struct ggml_opt_params native = params->optParams();

    REQUIRE(native.ctx_compute == context->context());
    REQUIRE(native.inputs == inputs->tensor());
    REQUIRE(native.outputs == outputs->tensor());
}

TEST_CASE( "Optimization parameters retain an optimizer schedule", "[ggml][opt][params][context_params][usage][schedule]" )
{
    CpuSchedulerFixture fixture;
    auto params = createDefaultOptParams(fixture);
    auto schedule = JobGgmlOptOptimizerSchedule::createShared([](JobGgmlOptOptimizerParams &optimizerParams, std::int64_t callCount) {
        optimizerParams.adamw()->setAlpha( callCount == 1 ? 1.0e-3f : 5.0e-4f );
    });

    params->setOptimizerSchedule( schedule );
    REQUIRE(params->optimizerSchedule() == schedule);
    REQUIRE(params->getOptimizerParams() == schedule->nativeCallback());
    REQUIRE(params->getOptimizerParamsUserData() == schedule->nativeUserData());
    const struct ggml_opt_params native = params->optParams();

    REQUIRE(native.get_opt_pars == schedule->nativeCallback());
    REQUIRE(native.get_opt_pars_ud == schedule->nativeUserData());
    const struct ggml_opt_optimizer_params optimizerParams = native.get_opt_pars(native.get_opt_pars_ud);
    REQUIRE(schedule->callCount() == 1);
    REQUIRE(optimizerParams.adamw.alpha == Approx(1.0e-3f));
}

TEST_CASE("Optimization parameters may use a raw native callback pair", "[ggml][opt][params][context_params][usage][native_callback]")
{
    CpuSchedulerFixture fixture;
    auto params = createDefaultOptParams(fixture);
    struct ggml_opt_optimizer_params constantParams{
        {
            3.0e-4f,
            0.8f,
            0.95f,
            1.0e-7f,
            0.02f
        },
        {
            8.0e-3f,
            0.03f
        }
    };

    params->setGetOptimizerParams(ggml_opt_get_constant_optimizer_params, &constantParams);
    REQUIRE(params->getOptimizerParams() == ggml_opt_get_constant_optimizer_params);
    REQUIRE(params->getOptimizerParamsUserData() == &constantParams);
    const struct ggml_opt_params native = params->optParams();

    const struct ggml_opt_optimizer_params returned = native.get_opt_pars(native.get_opt_pars_ud);
    REQUIRE(returned.adamw.alpha    == Approx(3.0e-4f));
    REQUIRE(returned.adamw.beta1    == Approx(0.8f));
    REQUIRE(returned.adamw.beta2    == Approx(0.95f));
    REQUIRE(returned.adamw.eps      == Approx(1.0e-7f));
    REQUIRE(returned.adamw.wd       == Approx(0.02f));
    REQUIRE(returned.sgd.alpha      == Approx(8.0e-3f));
    REQUIRE(returned.sgd.wd         == Approx(0.03f));
}

TEST_CASE( "Optimization parameters reset to upstream defaults", "[ggml][opt][params][context_params][usage][reset]" )
{
    CpuSchedulerFixture fixture;
    auto context = JobGgmlContext::createUniqMetadata(2);
    auto inputs  = context->newTensor2d(JobGgmlType::F32, 4, 2);
    auto outputs = context->newTensor2d(JobGgmlType::F32, 1, 2);
    auto params  = createDefaultOptParams(fixture, JobGgmlOptLossType::Sum);
    auto schedule = JobGgmlOptOptimizerSchedule::createShared( []( JobGgmlOptOptimizerParams &, std::int64_t ) { } );

    params->setComputeContext(context.get());
    params->setInputs(inputs.get());
    params->setOutputs(outputs.get());

    params->setGgmlBuildType( GGML_OPT_BUILD_TYPE_FORWARD );
    params->setOptPeriod(8);

    params->setGgmlOptimizer(GGML_OPT_OPTIMIZER_TYPE_SGD);

    params->setOptimizerSchedule(schedule );
    REQUIRE(params->usesStaticGraphs());
    REQUIRE(params->optimizerSchedule() == schedule);

    params->reset();

    REQUIRE(params->backendSched() == fixture.scheduler().get());

    // Scheduler and selected loss type are the required inputs to the upstream default-parameter
    // function and remain associated with this object.
    REQUIRE(params->lossType()          == JobGgmlOptLossType::Sum);
    REQUIRE(params->ggmlLossType()      == GGML_OPT_LOSS_TYPE_SUM);
    REQUIRE(params->computeContext()    == nullptr);
    REQUIRE(params->inputs()            == nullptr);
    REQUIRE(params->outputs()           == nullptr);

    REQUIRE_FALSE(params->usesStaticGraphs());

    REQUIRE(params->buildType()             == JobGgmlOptBuildType::Opt);
    REQUIRE(params->ggmlBuildType()         == GGML_OPT_BUILD_TYPE_OPT);
    REQUIRE(params->optPeriod()             == 1);
    REQUIRE(params->optimizerSchedule()     == nullptr);
    REQUIRE(params->getOptimizerParams()    == ggml_opt_get_default_optimizer_params );
    REQUIRE(params->getOptimizerParamsUserData() == nullptr );
    REQUIRE(params->ggmlOptimizer()         == GGML_OPT_OPTIMIZER_TYPE_ADAMW );
    REQUIRE(params->isValid());
}



TEST_CASE("AdamW optimizer parameters reject non-finite values", "[ggml][opt][params][adamw][edge][finite]")
{
    const float infinity = job::core::safeInfinity();
    const float quietNaN = std::numeric_limits<float>::quiet_NaN();

    JobGgmlOptAdamWParams params;
    REQUIRE_THROWS_AS(params.setAlpha(infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setAlpha(quietNaN), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta1(infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta2(quietNaN), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setEps(infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setWd(quietNaN), std::invalid_argument);
    REQUIRE(params.isValid());
}

TEST_CASE("AdamW optimizer parameters reject invalid ranges", "[ggml][opt][params][adamw][edge][range]")
{
    JobGgmlOptAdamWParams params;
    REQUIRE_THROWS_AS(params.setAlpha(0.0f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setAlpha(-1.0e-3f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta1(-0.1f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta1(1.0f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta2(-0.1f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setBeta2(1.0f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setEps(0.0f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setEps(-1.0e-8f), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setWd(-0.01f), std::invalid_argument);

    REQUIRE(params.isValid());
}

TEST_CASE("SGD optimizer parameters reject non-finite values", "[ggml][opt][params][sgd][edge][finite]")
{
    const float infinity = job::core::safeInfinity();
    const float quietNaN = std::numeric_limits<float>::quiet_NaN();
    JobGgmlOptSgdParams params;

    REQUIRE_THROWS_AS(params.setAlpha(infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setAlpha(quietNaN), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setWd(infinity), std::invalid_argument);
    REQUIRE_THROWS_AS(params.setWd(quietNaN), std::invalid_argument);
    REQUIRE(params.isValid());
}

TEST_CASE( "SGD optimizer parameters reject invalid ranges", "[ggml][opt][params][sgd][edge][range]" )
{
    JobGgmlOptSgdParams params;

    REQUIRE_THROWS_AS( params.setAlpha(0.0f), std::invalid_argument );
    REQUIRE_THROWS_AS( params.setAlpha(-1.0e-3f), std::invalid_argument );
    REQUIRE_THROWS_AS( params.setWd(-0.01f), std::invalid_argument );
    REQUIRE(params.isValid());
}

TEST_CASE( "Optimizer aggregate preserves its previous state after invalid replacement", "[ggml][opt][params][aggregate][edge][transaction]" )
{
    JobGgmlOptOptimizerParams params;

    params.adamw()->setAlpha(4.0e-4f);
    params.sgd()->setAlpha(7.0e-3f);

    const struct ggml_opt_optimizer_params before = params.optimizerParams();
    struct ggml_opt_optimizer_params invalid = before;
    invalid.adamw.alpha = std::numeric_limits<float>::quiet_NaN();
    REQUIRE_THROWS_AS( params.setOptimizerParams(invalid), std::invalid_argument );

    const struct ggml_opt_optimizer_params after = params.optimizerParams();
    REQUIRE(after.adamw.alpha   == Approx(before.adamw.alpha));
    REQUIRE(after.adamw.beta1   == Approx(before.adamw.beta1));
    REQUIRE(after.adamw.beta2   == Approx(before.adamw.beta2));
    REQUIRE(after.adamw.eps     == Approx(before.adamw.eps));
    REQUIRE(after.adamw.wd      == Approx(before.adamw.wd));
    REQUIRE(after.sgd.alpha     == Approx(before.sgd.alpha));
    REQUIRE(after.sgd.wd        == Approx(before.sgd.wd));
}

TEST_CASE( "Optimizer schedule rejects an empty callback", "[ggml][opt][params][schedule][edge][callback]" )
{
    JobGgmlOptOptimizerSchedule::Callback callback;
    REQUIRE_FALSE(static_cast<bool>(callback));
    REQUIRE_THROWS_AS(JobGgmlOptOptimizerSchedule{ callback }, std::invalid_argument);
}

TEST_CASE("Optimizer schedule supports replacing its callback", "[ggml][opt][params][schedule][edge][callback]")
{
    JobGgmlOptOptimizerSchedule schedule {[](JobGgmlOptOptimizerParams &params, std::int64_t) {
            params.adamw()->setAlpha( 1.0e-3f );
        }
    };

    const auto callback = schedule.nativeCallback();
    const struct ggml_opt_optimizer_params first = callback( schedule.nativeUserData() );
    REQUIRE(first.adamw.alpha == Approx(1.0e-3f));
    schedule.setCallback([](JobGgmlOptOptimizerParams &params, std::int64_t) {
        params.adamw()->setAlpha( 2.5e-4f );
    });

    const struct ggml_opt_optimizer_params second = callback( schedule.nativeUserData() );
    REQUIRE(second.adamw.alpha == Approx(2.5e-4f));
}

TEST_CASE( "Optimizer schedule rejects an empty replacement callback", "[ggml][opt][params][schedule][edge][callback]" )
{
    JobGgmlOptOptimizerSchedule schedule {[](JobGgmlOptOptimizerParams &, std::int64_t){}};
    JobGgmlOptOptimizerSchedule::Callback callback;

    REQUIRE_THROWS_AS(schedule.setCallback(callback), std::invalid_argument);
    REQUIRE(schedule.isValid());
}

TEST_CASE( "Optimizer schedule catches callback exceptions", "[ggml][opt][params][schedule][edge][exception]" )
{
    JobGgmlOptOptimizerSchedule schedule {[](JobGgmlOptOptimizerParams &params, std::int64_t callCount) {
            if (callCount == 1) {
                params.adamw()->setAlpha( 2.5e-4f );
                return;
            }
            params.adamw()->setAlpha( 7.5e-4f );
            throw std::runtime_error{ "expected test exception" };
        }
    };

    const auto callback = schedule.nativeCallback();
    const struct ggml_opt_optimizer_params first = callback( schedule.nativeUserData() );
    REQUIRE(schedule.callCount() == 1);
    REQUIRE(first.adamw.alpha == Approx(2.5e-4f));


    // The second invocation mutates the temporary C++ parameter object and then throws.
    // The C boundary must receive the previous committed value.
    const struct ggml_opt_optimizer_params second = callback(schedule.nativeUserData());
    REQUIRE(schedule.callCount() == 2);
    REQUIRE(second.adamw.alpha == Approx(2.5e-4f));
    REQUIRE(schedule.optimizerParams()->adamw()->alpha() == Approx(2.5e-4f));
}

TEST_CASE( "Optimizer schedule null userdata returns upstream defaults", "[ggml][opt][params][schedule][edge][userdata]" )
{
    JobGgmlOptOptimizerSchedule schedule{[](JobGgmlOptOptimizerParams &params, std::int64_t) {
            params.adamw()->setAlpha( 0.5f );
        }
    };

    const struct ggml_opt_optimizer_params native = schedule.nativeCallback()( nullptr );
    REQUIRE(native.adamw.alpha  == Approx(0.001f));
    REQUIRE(native.adamw.beta1  == Approx(0.9f));
    REQUIRE(native.adamw.beta2  == Approx(0.999f));
    REQUIRE(native.adamw.eps    == Approx(1.0e-8f));
    REQUIRE(native.adamw.wd     == Approx(0.0f));
    REQUIRE(native.sgd.alpha    == Approx(1.0e-3f));
    REQUIRE(native.sgd.wd       == Approx(0.0f));
    REQUIRE(schedule.callCount() == 0);
}

TEST_CASE("Optimization parameters reject a null scheduler", "[ggml][opt][params][context_params][edge][scheduler]")
{
    REQUIRE_THROWS_AS((JobGgmlOptParams{nullptr, JobGgmlOptLossType::Mean}), std::invalid_argument);
}

TEST_CASE( "Optimization parameters reject invalid native enum values", "[ggml][opt][params][context_params][edge][enum]" )
{
    CpuSchedulerFixture fixture;
    auto params = createDefaultOptParams( fixture );
    REQUIRE_THROWS_AS( params->setGgmlLossType(static_cast<enum ggml_opt_loss_type>(-1)), std::invalid_argument );
    REQUIRE_THROWS_AS( params->setGgmlBuildType(static_cast<enum ggml_opt_build_type>(-1)), std::invalid_argument );
    REQUIRE_THROWS_AS( params->setGgmlOptimizer(GGML_OPT_OPTIMIZER_TYPE_COUNT), std::invalid_argument );
    REQUIRE(params->isValid());
}

TEST_CASE( "Optimization parameters reject invalid optimizer periods", "[ggml][opt][params][context_params][edge][period]" )
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);
    REQUIRE_THROWS_AS(params->setOptPeriod(0), std::invalid_argument);
    REQUIRE_THROWS_AS(params->setOptPeriod(-1), std::invalid_argument);

    REQUIRE(params->optPeriod() == 1);
    REQUIRE(params->isValid());
}

TEST_CASE( "Optimization parameters identify incomplete static graph configuration", "[ggml][opt][params][context_params][edge][static_graph]" )
{
    CpuSchedulerFixture fixture;
    auto context = JobGgmlContext::createUniqMetadata(2);
    auto inputs  = context->newTensor2d(JobGgmlType::F32, 4, 2);
    auto outputs = context->newTensor2d(JobGgmlType::F32, 1, 2);
    auto params  = createDefaultOptParams(fixture);

    SECTION("compute context only")
    {
        params->setComputeContext(context.get());
        REQUIRE_FALSE(params->usesStaticGraphs());
        REQUIRE_FALSE(params->isValid());
    }

    SECTION("compute context and inputs")
    {
        params->setComputeContext(context.get());
        params->setInputs(inputs.get());
        REQUIRE_FALSE(params->usesStaticGraphs());
        REQUIRE_FALSE(params->isValid());
    }

    SECTION("inputs and outputs without compute context")
    {
        params->setInputs(inputs.get());
        params->setOutputs(outputs.get());
        REQUIRE_FALSE(params->usesStaticGraphs());
        REQUIRE_FALSE(params->isValid());
    }

    SECTION("complete static graph configuration")
    {
        params->setComputeContext(context.get());
        params->setInputs(inputs.get());
        params->setOutputs(outputs.get());
        REQUIRE(params->usesStaticGraphs());
        REQUIRE(params->isValid());
    }
}

TEST_CASE("Optimization parameters may become invalid with a null callback", "[ggml][opt][params][context_params][edge][callback]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);
    REQUIRE(params->isValid());

    params->setGetOptimizerParams(nullptr);
    REQUIRE( params->getOptimizerParams() == nullptr);
    REQUIRE_FALSE(params->isValid());

    params->setGetOptimizerParams(ggml_opt_get_default_optimizer_params);
    REQUIRE(params->isValid());
}

TEST_CASE("Clearing an optimizer schedule restores the default native callback", "[ggml][opt][params][context_params][edge][schedule]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);
    auto schedule = JobGgmlOptOptimizerSchedule::createShared([](JobGgmlOptOptimizerParams &, std::int64_t){});
    params->setOptimizerSchedule(schedule);
    REQUIRE(params->optimizerSchedule() == schedule);

    params->setOptimizerSchedule( nullptr );
    REQUIRE(params->optimizerSchedule() == nullptr);

    REQUIRE(params->getOptimizerParams() == ggml_opt_get_default_optimizer_params);
    REQUIRE(params->getOptimizerParamsUserData() == nullptr);
    REQUIRE(params->isValid());
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE( "Optimizer parameter native conversion performance", "[ggml][opt][params][benchmark][native]" )
{
    JobGgmlOptOptimizerParams params;
    params.adamw()->setAlpha(5.0e-4f);
    params.adamw()->setWd(0.01f);
    params.sgd()->setAlpha(2.0e-3f);
    params.sgd()->setWd(0.02f);
    BENCHMARK("convert optimizer parameter aggregate to native") {
        return params.optimizerParams();
    };
}

TEST_CASE( "Optimizer schedule callback dispatch performance", "[ggml][opt][params][benchmark][schedule]" )
{
    JobGgmlOptOptimizerSchedule schedule{ []( JobGgmlOptOptimizerParams &params, std::int64_t callCount ) {
            const float alpha = callCount % 2 == 0 ? 5.0e-4f : 1.0e-3f;
            params.adamw()->setAlpha( alpha );
        }
    };
    const auto callback = schedule.nativeCallback();
    void *userData = schedule.nativeUserData();
    BENCHMARK("dispatch optimizer schedule through C callback") {
        return callback( userData );
    };
}

TEST_CASE("Optimization parameter native aggregate performance", "[ggml][opt][params][benchmark][context_params]")
{
    CpuSchedulerFixture fixture;
    auto params = createDefaultOptParams(fixture);
    params->setOptPeriod(4);

    BENCHMARK("convert optimization initialization parameters to native"){
        return params->optParams();
    };
}

// TEST_CASE("Optimizer schedule repeated callback stress", "[ggml][opt][params][stress][schedule]")
// {
//     constexpr std::int64_t iterationCount = 10000;
//     JobGgmlOptOptimizerSchedule schedule{[]( JobGgmlOptOptimizerParams &params, std::int64_t callCount) {
//             params.adamw()->setAlpha(callCount % 2 == 0 ? 5.0e-4f : 1.0e-3f);
//         }
//     };

//     const auto callback = schedule.nativeCallback();
//     for (std::int64_t index = 0; index < iterationCount; ++index) {
//         const struct ggml_opt_optimizer_params native = callback(schedule.nativeUserData());
//         REQUIRE(native.adamw.alpha > 0.0f);
//     }

//     REQUIRE(schedule.callCount() == iterationCount);
// }
TEST_CASE("Optimizer schedule repeated callback stress", "[ggml][opt][params][benchmark][schedule][stress]")
{
    constexpr std::int64_t iterationCount = 10000;

    JobGgmlOptOptimizerSchedule schedule{[](JobGgmlOptOptimizerParams &params, std::int64_t callCount) {
        params.adamw()->setAlpha(callCount % 2 == 0 ? 5.0e-4f : 1.0e-3f);
    }};

    const auto callback = schedule.nativeCallback();
    void *userData = schedule.nativeUserData();

    BENCHMARK("dispatch 10000 optimizer schedule callbacks") {
        schedule.resetCallCount();
        for (std::int64_t index = 0; index < iterationCount; ++index)
            callback(userData);

        return schedule.callCount();
    };
    REQUIRE(schedule.callCount() == iterationCount);
}
#endif