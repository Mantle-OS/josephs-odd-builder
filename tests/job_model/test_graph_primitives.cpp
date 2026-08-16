#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <limits>

#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <real_type.h>

#include <graph/embedding_graph.h>
#include <graph/gqa_graph.h>
#include <graph/linear_graph.h>
#include <graph/norm_graph.h>
#include <graph/residual_graph.h>
#include <graph/rope_graph.h>

using namespace job::model;
using namespace job::ggml;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("LinearGraph builds a linear projection", "[model][graph][primitive][linear][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input  = ctx->newTensor2d(JobGgmlType::F32, 8, 4);
    auto weight = ctx->newTensor2d(JobGgmlType::F32, 8, 16);

    REQUIRE(input != nullptr);
    REQUIRE(weight != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

    REQUIRE(inputOp != nullptr);
    REQUIRE(inputOp->isValid());

    auto output = LinearGraph::build(std::move(inputOp),
                                     *weight,
                                     nullptr,
                                     JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 16);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("LinearGraph applies an optional bias", "[model][graph][primitive][linear][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input  = ctx->newTensor2d(JobGgmlType::F32, 8, 4);
    auto weight = ctx->newTensor2d(JobGgmlType::F32, 8, 16);
    auto bias   = ctx->newTensor1d(JobGgmlType::F32, 16);

    REQUIRE(input != nullptr);
    REQUIRE(weight != nullptr);
    REQUIRE(bias != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

    auto output = LinearGraph::build(std::move(inputOp),
                                     *weight,
                                     bias.get(),
                                     JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 16);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("NormGraph builds RMS normalization", "[model][graph][primitive][norm][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input  = ctx->newTensor2d(JobGgmlType::F32, 32, 4);
    auto weight = ctx->newTensor1d(JobGgmlType::F32, 32);

    REQUIRE(input != nullptr);
    REQUIRE(weight != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

    auto output = NormGraph::rms(std::move(inputOp),
                                 *weight,
                                 1e-6f);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 32);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("NormGraph supports an optional bias", "[model][graph][primitive][norm][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input  = ctx->newTensor2d(JobGgmlType::F32, 32, 4);
    auto weight = ctx->newTensor1d(JobGgmlType::F32, 32);
    auto bias   = ctx->newTensor1d(JobGgmlType::F32, 32);

    REQUIRE(input != nullptr);
    REQUIRE(weight != nullptr);
    REQUIRE(bias != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

    auto output = NormGraph::rms(std::move(inputOp),
                                 *weight,
                                 1e-6f,
                                 bias.get());

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 32);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("EmbeddingGraph builds token embedding lookup", "[model][graph][primitive][embedding][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tokens = ctx->newTensor1d(JobGgmlType::I32, 4);
    auto weight = ctx->newTensor2d(JobGgmlType::F32, 32, 128);

    REQUIRE(tokens != nullptr);
    REQUIRE(weight != nullptr);

    auto output = EmbeddingGraph::build(*ctx,
                                        *tokens,
                                        *weight);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 32);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("ResidualGraph adds tensors with matching geometry", "[model][graph][primitive][residual][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto lhs = ctx->newTensor2d(JobGgmlType::F32, 32, 4);
    auto rhs = ctx->newTensor2d(JobGgmlType::F32, 32, 4);

    REQUIRE(lhs != nullptr);
    REQUIRE(rhs != nullptr);

    auto lhsOp = JobGgmlTensorOp::createUniq(lhs->tensor(), ctx.get());
    auto rhsOp = JobGgmlTensorOp::createUniq(rhs->tensor(), ctx.get());

    auto output = ResidualGraph::build(std::move(lhsOp),
                                       std::move(rhsOp));

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 32);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("RopeGraph builds rotary positional embedding", "[model][graph][primitive][rope][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input     = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);
    auto positions = ctx->newTensor1d(JobGgmlType::I32, 4);

    REQUIRE(input != nullptr);
    REQUIRE(positions != nullptr);

    auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

    auto output = RopeGraph::build(std::move(inputOp),
                                   *positions,
                                   128,
                                   2);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 128);
    CHECK(output->extent(1) == 8);
    CHECK(output->extent(2) == 4);
}

TEST_CASE("GqaGraph expands KV heads to query head geometry", "[model][graph][primitive][gqa][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    auto output = GqaGraph::expand(std::move(input), 32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 128);
    CHECK(output->extent(1) == 32);
    CHECK(output->extent(2) == 4);
}

TEST_CASE("GqaGraph leaves MHA geometry unchanged", "[model][graph][primitive][gqa][usage]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    auto output = GqaGraph::expand(std::move(input), 8);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == 128);
    CHECK(output->extent(1) == 8);
    CHECK(output->extent(2) == 4);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("LinearGraph rejects a missing input", "[model][graph][primitive][linear][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);

    auto weight = ctx->newTensor2d(JobGgmlType::F32, 8, 16);

    REQUIRE(weight != nullptr);

    CHECK_THROWS_AS(
        LinearGraph::build(nullptr,
                           *weight,
                           nullptr,
                           JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("NormGraph rejects a missing input", "[model][graph][primitive][norm][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);

    auto weight = ctx->newTensor1d(JobGgmlType::F32, 32);

    REQUIRE(weight != nullptr);

    CHECK_THROWS_AS(
        NormGraph::rms(nullptr,
                       *weight,
                       1e-6f),
        std::invalid_argument);
}

TEST_CASE("NormGraph rejects invalid epsilon", "[model][graph][primitive][norm][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(32);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto input  = ctx->newTensor2d(JobGgmlType::F32, 32, 4);
    auto weight = ctx->newTensor1d(JobGgmlType::F32, 32);

    REQUIRE(input != nullptr);
    REQUIRE(weight != nullptr);

    {
        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

        CHECK_THROWS_AS(
            NormGraph::rms(std::move(inputOp), *weight, 0.0f),
            std::invalid_argument);
    }

    {
        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

        CHECK_THROWS_AS(
            NormGraph::rms(std::move(inputOp), *weight, -1e-6f),
            std::invalid_argument);
    }

    {
        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

        CHECK_THROWS_AS(
            NormGraph::rms(std::move(inputOp), *weight, job::core::safeInfinity()),
            std::invalid_argument);
    }

    {
        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());

        CHECK_THROWS_AS(
            NormGraph::rms(std::move(inputOp), *weight, job::core::safeNaN()),
            std::invalid_argument);
    }
}

TEST_CASE("EmbeddingGraph requires I32 token indices", "[model][graph][primitive][embedding][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tokens = ctx->newTensor1d(JobGgmlType::F32, 4);
    auto weight = ctx->newTensor2d(JobGgmlType::F32, 32, 128);

    REQUIRE(tokens != nullptr);
    REQUIRE(weight != nullptr);

    CHECK_THROWS_AS(
        EmbeddingGraph::build(*ctx, *tokens, *weight),
        std::invalid_argument);
}

TEST_CASE("ResidualGraph rejects missing operands", "[model][graph][primitive][residual][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tensor = ctx->newTensor2d(JobGgmlType::F32, 32, 4);

    REQUIRE(tensor != nullptr);

    {
        auto residual = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

        CHECK_THROWS_AS(
            ResidualGraph::build(nullptr, std::move(residual)),
            std::invalid_argument);
    }

    {
        auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

        CHECK_THROWS_AS(
            ResidualGraph::build(std::move(input), nullptr),
            std::invalid_argument);
    }
}

TEST_CASE("RopeGraph rejects a missing input", "[model][graph][primitive][rope][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto positions = ctx->newTensor1d(JobGgmlType::I32, 4);

    REQUIRE(positions != nullptr);

    CHECK_THROWS_AS(
        RopeGraph::build(nullptr,
                         *positions,
                         128),
        std::invalid_argument);
}

TEST_CASE("RopeGraph rejects an invalid rotary dimension", "[model][graph][primitive][rope][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tensor    = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);
    auto positions = ctx->newTensor1d(JobGgmlType::I32, 4);

    REQUIRE(tensor != nullptr);
    REQUIRE(positions != nullptr);

    {
        auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

        CHECK_THROWS_AS(
            RopeGraph::build(std::move(input),
                             *positions,
                             0),
            std::invalid_argument);
    }

    {
        auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

        const uint64_t tooLarge =
            static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1ULL;

        CHECK_THROWS_AS(
            RopeGraph::build(std::move(input),
                             *positions,
                             static_cast<uint32_t>(tooLarge)),
            std::invalid_argument);
    }
}

TEST_CASE("GqaGraph rejects a missing input", "[model][graph][primitive][gqa][edge]")
{
    CHECK_THROWS_AS(
        GqaGraph::expand(nullptr, 32),
        std::invalid_argument);
}

TEST_CASE("GqaGraph requires a three-dimensional input", "[model][graph][primitive][gqa][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tensor = ctx->newTensor2d(JobGgmlType::F32, 128, 8);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    CHECK_THROWS_AS(
        GqaGraph::expand(std::move(input), 32),
        std::invalid_argument);
}

TEST_CASE("GqaGraph requires at least one query head", "[model][graph][primitive][gqa][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);

    auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    CHECK_THROWS_AS(
        GqaGraph::expand(std::move(input), 0),
        std::invalid_argument);
}

TEST_CASE("GqaGraph rejects fewer query heads than KV heads", "[model][graph][primitive][gqa][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);

    auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    CHECK_THROWS_AS(
        GqaGraph::expand(std::move(input), 4),
        std::invalid_argument);
}

TEST_CASE("GqaGraph requires query heads divisible by KV heads", "[model][graph][primitive][gqa][edge]")
{
    auto ctx = JobGgmlContext::createUniqMetadata(8);

    REQUIRE(ctx != nullptr);

    auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 4);

    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());

    CHECK_THROWS_AS(
        GqaGraph::expand(std::move(input), 10),
        std::invalid_argument);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark graph primitive composition", "[model][graph][primitive][benchmark]")
{
    BENCHMARK("Build linear + RMS norm primitive chain")
    {
        auto ctx = JobGgmlContext::createUniqMetadata(32);
        auto input  = ctx->newTensor2d(JobGgmlType::F32, 2560, 4);
        auto weight = ctx->newTensor2d(JobGgmlType::F32, 2560, 2560);
        auto norm   = ctx->newTensor1d(JobGgmlType::F32, 2560);

        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), ctx.get());
        auto linear = LinearGraph::build(std::move(inputOp),
                                         *weight,
                                         nullptr,
                                         JobGgmlType::F32);

        auto normalized = NormGraph::rms(std::move(linear), *norm, 1e-6f);

        return normalized->isValid();
    };
}

TEST_CASE("Benchmark GQA metadata expansion", "[model][graph][primitive][gqa][benchmark]")
{
    BENCHMARK("Expand 8 KV heads to 32 query heads") {
        auto ctx = JobGgmlContext::createUniqMetadata(32);
        auto tensor = ctx->newTensor3d(JobGgmlType::F32, 128, 8, 1024);

        auto input = JobGgmlTensorOp::createUniq(tensor->tensor(), ctx.get());
        auto output = GqaGraph::expand(std::move(input), 32);

        return output->extent(1);
    };
}
#endif