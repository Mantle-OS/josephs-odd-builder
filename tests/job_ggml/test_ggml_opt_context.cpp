#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ggml.h>

#include <job_ggml_context.h>
#include <job_ggml_opt_context.h>
#include <job_ggml_opt_optimizer_params.h>
#include <job_ggml_opt_optimizer_schedule.h>
#include <job_ggml_opt_params.h>
#include <job_ggml_tensor.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

namespace {

/*
 * A small static optimization graph:
 *
 *     weights [2, 1] × inputs [2, 4]
 *                    |
 *               outputs [1, 4]
 *
 * The weights tensor is marked as a model parameter.
 *
 * Static optimizer construction creates the forward, gradient, and optimizer
 * graphs inside the supplied compute context. The dedicated static optimizer
 * context helper reserves metadata for all three graph structures.
 */
struct CpuStaticOptContextFixture
{
    static constexpr std::int64_t NeDatapoint = 2;
    static constexpr std::int64_t NdataBatch  = 4;
    static constexpr std::int64_t NeOutput    = 1;

    static constexpr std::size_t ComputeTensorCount  = 128;
    static constexpr std::size_t ComputePayloadBytes = 16 * 1024;

    CpuSchedulerFixture schedulerFixture;

    JobGgmlContext::UPtr computeContext;

    JobGgmlTensor::UPtr inputs;
    JobGgmlTensor::UPtr weights;
    JobGgmlTensor::UPtr outputs;

    JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule;
    JobGgmlOptParams::UPtr           params;
    JobGgmlOptContext::UPtr          optContext;
    JobGgmlTensorOp::UPtr            tensorOp; // Owned

    explicit CpuStaticOptContextFixture(JobGgmlOptLossType lossType = JobGgmlOptLossType::MeanSquaredError,
                                        JobGgmlOptOptimizerType optimizer = JobGgmlOptOptimizerType::AdamW,
                                        std::int32_t optimizerPeriod = 2,
                                        bool useSchedule = true)
    {
        computeContext = createAllocatedStaticOptContext(ComputeTensorCount, ComputePayloadBytes);

        if (!computeContext || !computeContext->isValid())
            throw std::runtime_error{"Failed to create static optimization compute context"};

        inputs  = computeContext->newTensor2d(JobGgmlType::F32, NeDatapoint, NdataBatch);
        weights = computeContext->newTensor2d(JobGgmlType::F32, NeDatapoint, NeOutput);

        if (!inputs || !weights)
            throw std::runtime_error{"Failed to create static optimization input tensors"};

        inputs->setName("test.inputs");
        weights->setName("test.weights");

        inputs->data()->addFlag(JobGgmlTensorFlag::Input);
        weights->data()->addFlag(JobGgmlTensorFlag::Param);

        tensorOp = JobGgmlTensorOp::createUniq(weights->tensor(), computeContext.get());

        if (!tensorOp || !tensorOp->isValid())
            throw std::runtime_error{"Failed to create static optimization tensor operation"};

        outputs = tensorOp->mulMat(*inputs);

        if (!outputs || !outputs->isValid())
            throw std::runtime_error{"Failed to create static optimization output tensor"};

        outputs->setName("test.outputs");
        outputs->data()->addFlag(JobGgmlTensorFlag::Output);

        params = createDefaultOptParams(schedulerFixture, lossType);

        if (!params)
            throw std::runtime_error{"Failed to create static optimization parameters"};

        params->setComputeContext(computeContext.get());
        params->setInputs(inputs.get());
        params->setOutputs(outputs.get());
        params->setOptimizer(optimizer);
        params->setOptPeriod(optimizerPeriod);

        if (useSchedule) {
            optimizerSchedule = JobGgmlOptOptimizerSchedule::createShared(
                [](JobGgmlOptOptimizerParams &, std::int64_t) {});

            params->setOptimizerSchedule(optimizerSchedule);
        }

        if (!params->isValid() || !params->usesStaticGraphs())
            throw std::runtime_error{"Failed to configure static optimization parameters"};

        optContext = JobGgmlOptContext::createUniq(*params);

        if (!optContext || !optContext->isValid())
            throw std::runtime_error{"Failed to create static optimization context"};
    }
};

} // namespace

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Dynamic optimization context exposes its initial state",
          "[ggml][opt][context][usage][dynamic]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture, JobGgmlOptLossType::Mean);

    REQUIRE(params != nullptr);
    REQUIRE(params->isValid());
    REQUIRE_FALSE(params->usesStaticGraphs());

    JobGgmlOptContext context{*params};

    REQUIRE(context.isValid());
    REQUIRE(context.context() != nullptr);

    REQUIRE_FALSE(context.usesStaticGraphs());
    REQUIRE(context.optimizerPeriod() == 1);
    REQUIRE(context.optimizerType() == JobGgmlOptOptimizerType::AdamW);
    REQUIRE(context.ggmlOptimizerType() == GGML_OPT_OPTIMIZER_TYPE_ADAMW);
    REQUIRE(context.optimizerName() != std::string_view{"unknown"});
    REQUIRE_FALSE(context.optimizerName().empty());

    REQUIRE(context.inputs() == nullptr);
    REQUIRE(context.outputs() == nullptr);
    REQUIRE(context.labels() == nullptr);
    REQUIRE(context.loss() == nullptr);
    REQUIRE(context.predictions() == nullptr);
    REQUIRE(context.correctCount() == nullptr);
}

TEST_CASE("Dynamic optimization context preserves selected optimizer metadata",
          "[ggml][opt][context][usage][dynamic][optimizer]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture, JobGgmlOptLossType::Sum);

    REQUIRE(params != nullptr);

    params->setOptimizer(JobGgmlOptOptimizerType::Sgd);
    params->setOptPeriod(4);

    JobGgmlOptContext context{*params};

    REQUIRE(context.isValid());
    REQUIRE_FALSE(context.usesStaticGraphs());

    REQUIRE(context.optimizerType() == JobGgmlOptOptimizerType::Sgd);
    REQUIRE(context.ggmlOptimizerType() == GGML_OPT_OPTIMIZER_TYPE_SGD);
    REQUIRE(context.optimizerPeriod() == 4);
    REQUIRE(context.optimizerName() != std::string_view{"unknown"});
    REQUIRE_FALSE(context.optimizerName().empty());
}

TEST_CASE("Optimization context retains optimizer schedule ownership",
          "[ggml][opt][context][usage][schedule][ownership]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);

    REQUIRE(params != nullptr);

    auto schedule = JobGgmlOptOptimizerSchedule::createShared(
        [](JobGgmlOptOptimizerParams &optimizerParams, std::int64_t callCount) {
            optimizerParams.adamw()->setAlpha(callCount <= 1 ? 1.0e-3f : 5.0e-4f);
        });

    REQUIRE(schedule != nullptr);
    REQUIRE(schedule->isValid());

    JobGgmlOptOptimizerSchedule::WPtr weakSchedule = schedule;

    params->setOptimizerSchedule(schedule);
    REQUIRE(params->optimizerSchedule() == schedule);

    auto context = JobGgmlOptContext::createUniq(*params);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());
    REQUIRE(context->optimizerSchedule() == schedule);

    schedule.reset();
    params.reset();

    REQUIRE_FALSE(weakSchedule.expired());
    REQUIRE(context->optimizerSchedule() != nullptr);

    context.reset();

    REQUIRE(weakSchedule.expired());
}

TEST_CASE("Optimization context without a schedule exposes no schedule",
          "[ggml][opt][context][usage][schedule][default]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);

    REQUIRE(params != nullptr);
    REQUIRE(params->optimizerSchedule() == nullptr);

    JobGgmlOptContext context{*params};

    REQUIRE(context.isValid());
    REQUIRE(context.optimizerSchedule() == nullptr);
}

TEST_CASE("Static optimization context exposes model and loss tensors",
          "[ggml][opt][context][usage][static]")
{
    CpuStaticOptContextFixture fixture;

    JobGgmlOptContext &context = *fixture.optContext;

    REQUIRE(context.isValid());
    REQUIRE(context.usesStaticGraphs());

    auto inputs  = context.inputs();
    auto outputs = context.outputs();
    auto labels  = context.labels();
    auto loss    = context.loss();

    REQUIRE(inputs != nullptr);
    REQUIRE(outputs != nullptr);
    REQUIRE(labels != nullptr);
    REQUIRE(loss != nullptr);

    REQUIRE(inputs->isValid());
    REQUIRE(outputs->isValid());
    REQUIRE(labels->isValid());
    REQUIRE(loss->isValid());

    REQUIRE(inputs->tensor() == fixture.inputs->tensor());
    REQUIRE(outputs->tensor() == fixture.outputs->tensor());

    REQUIRE(inputs->extent(0) == CpuStaticOptContextFixture::NeDatapoint);
    REQUIRE(inputs->extent(1) == CpuStaticOptContextFixture::NdataBatch);

    REQUIRE(outputs->extent(0) == CpuStaticOptContextFixture::NeOutput);
    REQUIRE(outputs->extent(1) == CpuStaticOptContextFixture::NdataBatch);

    REQUIRE(labels->extent(0) == CpuStaticOptContextFixture::NeOutput);
    REQUIRE(labels->extent(1) == CpuStaticOptContextFixture::NdataBatch);

    REQUIRE(loss->isScalar());
    REQUIRE(loss->type() == JobGgmlType::F32);
}

TEST_CASE("Static MSE optimization context does not expose classification tensors",
          "[ggml][opt][context][usage][static][mse]")
{
    CpuStaticOptContextFixture fixture{JobGgmlOptLossType::MeanSquaredError};

    auto predictions  = fixture.optContext->predictions();
    auto correctCount = fixture.optContext->correctCount();

    REQUIRE(predictions == nullptr);
    REQUIRE(correctCount == nullptr);
}

TEST_CASE("Static optimization context exposes native tensor identities",
          "[ggml][opt][context][usage][static][native]")
{
    CpuStaticOptContextFixture fixture;

    auto inputs  = fixture.optContext->inputs();
    auto outputs = fixture.optContext->outputs();
    auto labels  = fixture.optContext->labels();
    auto loss    = fixture.optContext->loss();

    REQUIRE(inputs != nullptr);
    REQUIRE(outputs != nullptr);
    REQUIRE(labels != nullptr);
    REQUIRE(loss != nullptr);

    REQUIRE(inputs->tensor() == ggml_opt_inputs(fixture.optContext->context()));
    REQUIRE(outputs->tensor() == ggml_opt_outputs(fixture.optContext->context()));
    REQUIRE(labels->tensor() == ggml_opt_labels(fixture.optContext->context()));
    REQUIRE(loss->tensor() == ggml_opt_loss(fixture.optContext->context()));
}

TEST_CASE("Static optimization context exposes a parameter gradient accumulator",
          "[ggml][opt][context][usage][gradient_accumulator]")
{
    CpuStaticOptContextFixture fixture{
        JobGgmlOptLossType::MeanSquaredError, JobGgmlOptOptimizerType::AdamW, 2
    };

    auto gradientAccumulator = fixture.optContext->gradientAccumulator(*fixture.weights);

    REQUIRE(gradientAccumulator != nullptr);
    REQUIRE(gradientAccumulator->isValid());
    REQUIRE(gradientAccumulator->hasSameShape(*fixture.weights));
    REQUIRE(gradientAccumulator->type() == JobGgmlType::F32);
}

TEST_CASE("Static optimization context preserves optimizer period and schedule",
          "[ggml][opt][context][usage][static][configuration]")
{
    constexpr std::int32_t optimizerPeriod = 3;

    CpuStaticOptContextFixture fixture{
        JobGgmlOptLossType::MeanSquaredError, JobGgmlOptOptimizerType::Sgd, optimizerPeriod, true
    };

    REQUIRE(fixture.optContext->optimizerPeriod() == optimizerPeriod);
    REQUIRE(fixture.optContext->optimizerType() == JobGgmlOptOptimizerType::Sgd);
    REQUIRE(fixture.optContext->ggmlOptimizerType() == GGML_OPT_OPTIMIZER_TYPE_SGD);
    REQUIRE(fixture.optContext->optimizerSchedule() == fixture.optimizerSchedule);
}

TEST_CASE("Optimization context reset preserves native ownership",
          "[ggml][opt][context][usage][reset]")
{
    CpuStaticOptContextFixture fixture;

    ggml_opt_context_t nativeContext = fixture.optContext->context();

    REQUIRE(nativeContext != nullptr);

    fixture.optContext->reset(false);

    REQUIRE(fixture.optContext->isValid());
    REQUIRE(fixture.optContext->context() == nativeContext);

    fixture.optContext->reset(true);

    REQUIRE(fixture.optContext->isValid());
    REQUIRE(fixture.optContext->context() == nativeContext);
    REQUIRE(fixture.optContext->optimizerPeriod() == 2);
}

TEST_CASE("Dynamic optimization context accepts prepared graph state",
          "[ggml][opt][context][usage][dynamic][prepare]")
{
    CpuSchedulerFixture schedulerFixture;

    auto params = createDefaultOptParams(schedulerFixture, JobGgmlOptLossType::MeanSquaredError);

    REQUIRE(params != nullptr);

    JobGgmlOptContext optContext{*params};

    REQUIRE(optContext.isValid());
    REQUIRE_FALSE(optContext.usesStaticGraphs());

    auto computeContext = JobGgmlContext::createUniqHostContext(32, 8192, GGML_DEFAULT_GRAPH_SIZE, 0, true);

    REQUIRE(computeContext != nullptr);
    REQUIRE(computeContext->isValid());

    auto inputs  = computeContext->newTensor2d(JobGgmlType::F32, 2, 4);
    auto weights = computeContext->newTensor2d(JobGgmlType::F32, 2, 1);

    REQUIRE(inputs != nullptr);
    REQUIRE(weights != nullptr);

    ggml_set_param(weights->tensor());

    struct ggml_tensor *nativeOutputs = ggml_mul_mat(
        computeContext->context(), weights->tensor(), inputs->tensor());

    REQUIRE(nativeOutputs != nullptr);

    auto outputs = JobGgmlTensor::createUniq(nativeOutputs);

    REQUIRE(outputs != nullptr);

    auto graph = computeContext->newGraphCustom(GGML_DEFAULT_GRAPH_SIZE, true);

    REQUIRE(graph != nullptr);

    graph->buildForwardExpand(*outputs);

    REQUIRE_NOTHROW(optContext.prepareAlloc(*computeContext, *graph, *inputs, *outputs));

    REQUIRE_FALSE(optContext.usesStaticGraphs());

    auto preparedInputs  = optContext.inputs();
    auto preparedOutputs = optContext.outputs();

    REQUIRE(preparedInputs != nullptr);
    REQUIRE(preparedOutputs != nullptr);

    REQUIRE(preparedInputs->tensor() == inputs->tensor());
    REQUIRE(preparedOutputs->tensor() == outputs->tensor());
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Optimization context rejects invalid parameter state",
          "[ggml][opt][context][edge][params]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);

    REQUIRE(params != nullptr);

    params->setGetOptimizerParams(nullptr);

    REQUIRE_FALSE(params->isValid());

    REQUIRE_THROWS_AS((JobGgmlOptContext{*params}), std::invalid_argument);
}

TEST_CASE("Optimization context rejects incomplete static graph parameters",
          "[ggml][opt][context][edge][static]")
{
    CpuSchedulerFixture fixture;

    auto computeContext = createAllocatedStaticOptContext(32, 4096);

    REQUIRE(computeContext != nullptr);
    REQUIRE(computeContext->isValid());

    auto inputs = computeContext->newTensor2d(JobGgmlType::F32, 2, 4);

    REQUIRE(inputs != nullptr);

    auto params = createDefaultOptParams(fixture);

    REQUIRE(params != nullptr);

    params->setComputeContext(computeContext.get());
    params->setInputs(inputs.get());

    REQUIRE_FALSE(params->usesStaticGraphs());
    REQUIRE_FALSE(params->isValid());

    REQUIRE_THROWS_AS((JobGgmlOptContext{*params}), std::invalid_argument);
}

TEST_CASE("Dynamic optimization context returns no gradient before graph preparation",
          "[ggml][opt][context][edge][gradient_accumulator][dynamic]")
{
    CpuSchedulerFixture fixture;

    auto params = createDefaultOptParams(fixture);

    REQUIRE(params != nullptr);

    JobGgmlOptContext context{*params};

    auto tensorContext = JobGgmlContext::createUniqMetadata(1);

    REQUIRE(tensorContext != nullptr);
    REQUIRE(tensorContext->isValid());

    auto tensor = tensorContext->newTensor1d(JobGgmlType::F32, 4);

    REQUIRE(tensor != nullptr);

    auto gradient = context.gradientAccumulator(*tensor);

    REQUIRE(gradient == nullptr);
}

TEST_CASE("Static optimization context rejects dynamic graph preparation", "[ggml][opt][context][edge][prepare][static]")
{
    CpuStaticOptContextFixture fixture;

    auto graph = fixture.computeContext->newGraphCustom(GGML_DEFAULT_GRAPH_SIZE, true);

    REQUIRE(graph != nullptr);

    graph->buildForwardExpand(*fixture.outputs);

    REQUIRE_THROWS_AS(
        fixture.optContext->prepareAlloc(
            *fixture.computeContext, *graph, *fixture.inputs, *fixture.outputs),
        std::logic_error);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Dynamic optimization context construction performance", "[ggml][opt][context][benchmark][dynamic][construction]")
{
    CpuSchedulerFixture fixture;
    auto params = createDefaultOptParams(fixture);
    REQUIRE(params != nullptr);
    BENCHMARK("construct dynamic optimization context"){
        return JobGgmlOptContext::createUniq(*params);
    };
}

TEST_CASE("Static optimization context construction performance",
          "[ggml][opt][context][benchmark][static][construction]")
{
    struct StaticOptBenchmarkState
    {
        JobGgmlContext::UPtr    computeContext;
        JobGgmlTensor::UPtr     inputs;
        JobGgmlTensor::UPtr     weights;
        JobGgmlTensor::UPtr     outputs;
        JobGgmlOptParams::UPtr  params;
        JobGgmlOptContext::UPtr optContext;
    };

    CpuSchedulerFixture schedulerFixture;
    BENCHMARK("construct complete static optimization context"){
        auto state = std::make_unique<StaticOptBenchmarkState>();

        state->computeContext = createAllocatedStaticOptContext(128, 16 * 1024);

        if (!state->computeContext || !state->computeContext->isValid()) {
            throw std::runtime_error{"Failed to create benchmark static optimization context"};
        }

        state->inputs  = state->computeContext->newTensor2d(JobGgmlType::F32, 2, 4);
        state->weights = state->computeContext->newTensor2d(JobGgmlType::F32, 2, 1);

        if (!state->inputs || !state->weights) {
            throw std::runtime_error{"Failed to create benchmark optimization tensors"};
        }

        ggml_set_input(state->inputs->tensor());
        ggml_set_param(state->weights->tensor());

        struct ggml_tensor *nativeOutputs = ggml_mul_mat(
            state->computeContext->context(), state->weights->tensor(), state->inputs->tensor());

        if (!nativeOutputs) {
            throw std::runtime_error{"Failed to create benchmark optimization output"};
        }

        state->outputs = JobGgmlTensor::createUniq(nativeOutputs);

        if (!state->outputs || !state->outputs->isValid()) {
            throw std::runtime_error{"Failed to wrap benchmark optimization output"};
        }

        ggml_set_output(state->outputs->tensor());

        state->params = createDefaultOptParams(schedulerFixture, JobGgmlOptLossType::MeanSquaredError);

        if (!state->params) {
            throw std::runtime_error{"Failed to create benchmark optimization parameters"};
        }

        state->params->setComputeContext(state->computeContext.get());
        state->params->setInputs(state->inputs.get());
        state->params->setOutputs(state->outputs.get());
        state->params->setOptPeriod(2);

        if (!state->params->isValid() || !state->params->usesStaticGraphs()) {
            throw std::runtime_error{"Failed to configure benchmark static optimization parameters"};
        }

        state->optContext = JobGgmlOptContext::createUniq(*state->params);

        if (!state->optContext || !state->optContext->isValid()) {
            throw std::runtime_error{"Failed to construct benchmark static optimization context"};
        }

        return state;
    };
}

TEST_CASE("Optimization context tensor inspection performance", "[ggml][opt][context][benchmark][inspection]")
{
    CpuStaticOptContextFixture fixture;

    BENCHMARK("inspect optimization context tensors") {
        auto inputs  = fixture.optContext->inputs();
        auto outputs = fixture.optContext->outputs();
        auto labels  = fixture.optContext->labels();
        auto loss    = fixture.optContext->loss();

        return inputs && outputs && labels && loss;
    };
}

TEST_CASE("Optimization context optimizer reset performance", "[ggml][opt][context][benchmark][reset]")
{
    CpuStaticOptContextFixture fixture;

    BENCHMARK("reset optimization context optimizer state")
    {
        fixture.optContext->reset(true);

        return fixture.optContext->context();
    };
}

TEST_CASE("Optimization context gradient reset performance", "[ggml][opt][context][benchmark][reset]")
{
    CpuStaticOptContextFixture fixture;
    BENCHMARK("reset optimization context gradient state") {
        fixture.optContext->reset(false);
        return fixture.optContext->context();
    };
}
TEST_CASE("Optimization context repeated tensor inspection stress", "[ggml][opt][context][benchmark][stress][inspection]")
{
    constexpr std::size_t iterationCount = 10000;

    CpuStaticOptContextFixture fixture;

    auto inputs  = fixture.optContext->inputs();
    auto outputs = fixture.optContext->outputs();
    auto labels  = fixture.optContext->labels();
    auto loss    = fixture.optContext->loss();

    REQUIRE(inputs != nullptr);
    REQUIRE(outputs != nullptr);
    REQUIRE(labels != nullptr);
    REQUIRE(loss != nullptr);

    REQUIRE(inputs->isValid());
    REQUIRE(outputs->isValid());
    REQUIRE(labels->isValid());
    REQUIRE(loss->isValid());

    BENCHMARK("inspect optimization context tensors 10000 times") {
        for (std::size_t iteration = 0; iteration < iterationCount; ++iteration) {
            inputs  = fixture.optContext->inputs();
            outputs = fixture.optContext->outputs();
            labels  = fixture.optContext->labels();
            loss    = fixture.optContext->loss();
        }
        return inputs && outputs && labels && loss;
    };
}
#endif