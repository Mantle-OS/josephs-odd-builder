#include <array>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_parallel_for.h>
#include <ctx/job_stealing_ctx.h>

#include <job_ggml_tensor_op_graph.h>

#include "test_ggml_utils.h"

// ============================================================================
// Block 1: Usage / AST construction
// ============================================================================
TEST_CASE("Tensor operation graph wraps an existing operation without changing its native tensor",
          "[ggml][tensor][op][graph][wrap]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto tensor = context->newTensor1d(JobGgmlType::F32, 8);
    REQUIRE(tensor != nullptr);

    auto op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
    REQUIRE(op != nullptr);

    auto duplicated = op->dup();
    REQUIRE(duplicated != nullptr);

    auto *nativeTensor = duplicated->tensor();
    auto *nativeContext = duplicated->context();
    auto graphOp = JobGgmlTensorOpGraph::wrap(std::move(duplicated));
    REQUIRE(graphOp != nullptr);
    REQUIRE(graphOp->isValid());
    REQUIRE(graphOp->tensor() == nativeTensor);
    REQUIRE(graphOp->context() == nativeContext);
    REQUIRE(graphOp->operation()->operation() == JobGgmlOp::Dup);
}

TEST_CASE("Tensor operation graph builds a forward graph from its expression root",
          "[ggml][tensor][op][graph][build]")
{
    TensorOpGraphFixture fixture;

    auto graph = fixture.expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());
    REQUIRE(graph->graph() != nullptr);

    REQUIRE(fixture.expression->operation() != nullptr);
    REQUIRE(fixture.expression->operation()->operation() == JobGgmlOp::Mul);
}

TEST_CASE("Tensor operation graph preserves chained operation dependencies",
          "[ggml][tensor][op][graph][chain]")
{
    auto context = JobGgmlContext::createUniqMetadata(256);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto input = context->newTensor1d(JobGgmlType::F32, 8);
    auto bias  = context->newTensor1d(JobGgmlType::F32, 8);

    REQUIRE(input != nullptr);
    REQUIRE(bias != nullptr);

    auto root = JobGgmlTensorOp::createUniq(input->tensor(), context.get());
    REQUIRE(root != nullptr);

    auto multiplied = root->scale(2.0f);
    REQUIRE(multiplied != nullptr);

    auto activated = multiplied->silu();
    REQUIRE(activated != nullptr);

    auto output = activated->add(*bias);
    REQUIRE(output != nullptr);

    REQUIRE(output->operation()->operation() == JobGgmlOp::Add);
    REQUIRE(output->operation()->source(0) == activated->tensor());
    REQUIRE(output->operation()->source(1) == bias->tensor());

    REQUIRE(activated->operation()->operation() == JobGgmlOp::Unary);
    REQUIRE(activated->operation()->unaryOperation() == JobGgmlUnaryOp::Silu);
    REQUIRE(activated->operation()->source(0) == multiplied->tensor());

    REQUIRE(multiplied->operation()->operation() == JobGgmlOp::Scale);
    REQUIRE(multiplied->operation()->source(0) == input->tensor());

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(output));
    REQUIRE(expression != nullptr);

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());
}



TEST_CASE("Tensor operation graph allocates and executes on the CPU scheduler",
          "[ggml][tensor][op][graph][usage][compute]")
{
    CpuSchedulerFixture schedulerFixture;

    auto context = JobGgmlContext::createUniqMetadata(256);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto input = context->newTensor1d(JobGgmlType::F32, 8);
    auto bias  = context->newTensor1d(JobGgmlType::F32, 8);
    REQUIRE(input != nullptr);
    REQUIRE(bias != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), context.get());
    REQUIRE(inputOp != nullptr);

    auto scaled = inputOp->scale(2.0f);
    auto result = scaled->add(*bias);
    REQUIRE(scaled != nullptr);
    REQUIRE(result != nullptr);

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(result));
    REQUIRE(expression != nullptr);

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    schedulerFixture.scheduler()->setTensorBackend(*input, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*bias, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*scaled, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*expression, *schedulerFixture.backend());
    schedulerFixture.scheduler()->splitGraph(*graph);
    REQUIRE(schedulerFixture.scheduler()->allocateGraph(*graph));
    REQUIRE(input->buffer() != nullptr);
    REQUIRE(bias->buffer() != nullptr);
    REQUIRE(expression->buffer() != nullptr);

    std::array<float, 8> inputValues{
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f
    };

    std::array<float, 8> biasValues{
        10.0f, 10.0f, 10.0f, 10.0f,
        10.0f, 10.0f, 10.0f, 10.0f
    };

    schedulerFixture.backend()->setTensorAsync(
        *input,
        inputValues.data(),
        0,
        TensorOpGraphFixture::ByteCount
        );

    schedulerFixture.backend()->setTensorAsync(
        *bias,
        biasValues.data(),
        0,
        TensorOpGraphFixture::ByteCount
        );

    schedulerFixture.backend()->synchronize();
    REQUIRE(schedulerFixture.scheduler()->computeGraph(*graph) == JobGgmlStatus::Success);

    std::array<float, 8> outputValues{};
    schedulerFixture.backend()->getTensorAsync(
        *expression,
        outputValues.data(),
        0,
        TensorOpGraphFixture::ByteCount
        );

    schedulerFixture.backend()->synchronize();
    for (std::size_t index = 0; index < outputValues.size(); ++index)
        REQUIRE(outputValues[index] == Catch::Approx(inputValues[index] * 2.0f + biasValues[index]));
}

TEST_CASE("Tensor operation graph rejects invalid wrapped operations",
          "[ggml][tensor][op][graph][edge]")
{
    JobGgmlTensorOp::UPtr invalid;
    REQUIRE_THROWS_AS(JobGgmlTensorOpGraph::wrap(std::move(invalid)), std::invalid_argument);
}

#ifdef JOB_TEST_BENCHMARKS
// GGML contexts are arenas. Reset between benchmark iterations or Catch
// will eventually benchmark how fast we can run out of metadata.
TEST_CASE("Tensor operation graph wrapping performance",
          "[ggml][tensor][op][graph][benchmark][wrap]")
{
    auto context = JobGgmlContext::createUniqMetadata(256);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    BENCHMARK("construct and wrap one tensor operation") {
        context->reset();
        auto tensor = context->newTensor1d(JobGgmlType::F32, 8);
        auto op     = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
        return JobGgmlTensorOpGraph::wrap(op->dup());
    };
}

TEST_CASE("Tensor operation graph expression construction performance",
          "[ggml][tensor][op][graph][benchmark][expression]")
{
    auto context = JobGgmlContext::createUniqMetadata(512);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    BENCHMARK("construct four-operation tensor expression") {
        context->reset();

        auto input = context->newTensor1d(JobGgmlType::F32, 8);
        auto bias  = context->newTensor1d(JobGgmlType::F32, 8);
        auto root  = JobGgmlTensorOp::createUniq(input->tensor(), context.get());

        auto scaled     = root->scale(2.0f);
        auto activated  = scaled->silu();
        auto normalized = activated->rmsNorm(1.0e-5f);

        return JobGgmlTensorOpGraph::wrap(normalized->add(*bias));
    };
}

// A Fun test
struct GraphShard
{
    JobGgmlContext::UPtr       context;
    JobGgmlTensor::UPtr        input;
    JobGgmlTensor::UPtr        bias;
    JobGgmlTensorOpGraph::UPtr expression;
};

[[nodiscard]] std::unique_ptr<GraphShard> buildGraphShard()
{
    auto shard = std::make_unique<GraphShard>();
    shard->context = JobGgmlContext::createUniqMetadata(4096);
    if (!shard->context || !shard->context->isValid())
        throw std::runtime_error{"Failed to create graph shard context"};

    shard->input = shard->context->newTensor1d(JobGgmlType::F32, 4096);
    shard->bias  = shard->context->newTensor1d(JobGgmlType::F32, 4096);
    if (!shard->input || !shard->bias)
        throw std::runtime_error{"Failed to create graph shard tensors"};

    auto root = JobGgmlTensorOp::createUniq(shard->input->tensor(), shard->context.get());
    auto current = root->scale(2.0f);
    for (std::size_t i = 0; i < 1024; ++i) {
        auto activated  = current->silu();
        auto normalized = activated->rmsNorm(1.0e-5f);
        auto scaled     = normalized->scale(1.0001f);

        current = std::move(scaled);
    }

    shard->expression = JobGgmlTensorOpGraph::wrap(current->add(*shard->bias));
    if (!shard->expression || !shard->expression->isValid())
        throw std::runtime_error{"Failed to construct graph shard expression"};

    return shard;
}

TEST_CASE("Six graph shards serial construction performance",
          "[ggml][tensor][op][graph][benchmark][shards][serial]")
{
    static constexpr std::size_t ShardCount = 6;
    BENCHMARK("construct six graph shards serial"){
        std::array<std::unique_ptr<GraphShard>, ShardCount> shards;
        for (std::size_t i = 0; i < ShardCount; ++i)
            shards[i] = buildGraphShard();

        return shards;
    };
}

TEST_CASE("Six graph shards parallel construction performance",
          "[ggml][tensor][op][graph][benchmark][shards][parallel]")
{
    static constexpr std::size_t ShardCount = 6;
    job::threads::JobStealerCtx stealer{6};
    REQUIRE(stealer.pool != nullptr);
    REQUIRE(stealer.pool->workerCount() > 1);

    BENCHMARK("construct six graph shards parallel"){
        std::array<std::unique_ptr<GraphShard>, ShardCount> shards;
        job::threads::parallel_for(
            *stealer.pool,
            std::size_t{0},
            ShardCount,
            [&](std::size_t i) {
                shards[i] = buildGraphShard();
            },
            0,
            1,
            job::threads::AccessPattern::Linear
            );

        return shards;
    };
}

#endif