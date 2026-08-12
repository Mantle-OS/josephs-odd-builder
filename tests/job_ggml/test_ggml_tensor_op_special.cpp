#include <initializer_list>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <job_ggml_tensor_op.h>

#include "test_ggml_utils.h"

// Notes / GGML quirks:
// - RoPE position count must match a->ne[2] for ordinary RoPE.
// - RoPE records the position tensor as src[1]; it is not graph-unary.
// - ggml_conv_2d is composite; ggml_conv_2d_direct creates GGML_OP_CONV_2D.
// - ggml_ssm_conv requires sx to be 3D and c to be a matrix.
// - ggml_win_part requires a->ne[3] == 1.

using namespace job::ggml;



// ============================================================================
// Rotary Position Embeddings (RoPE)
// ============================================================================
TEST_CASE("Tensor RoPE operations create the expected GGML operations",
          "[ggml][tensor][op][special][rope]")
{
    SpecialOpFixture fixture;

    auto positions = fixture.context->newTensor1d(JobGgmlType::I32, SpecialOpFixture::Ne2);
    REQUIRE(positions != nullptr);

    verifySpecialOperation(
        *fixture.op,
        JobGgmlOp::Rope,
        { fixture.tensor->tensor(), positions->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.rope(*positions, 8, 0);
        });

    verifySpecialOperation(
        *fixture.op,
        JobGgmlOp::Rope,
        { fixture.tensor->tensor(), positions->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.ropeInplace(*positions, 8, 0);
        });

    verifySpecialOperation(
        *fixture.op,
        JobGgmlOp::Rope,
        { fixture.tensor->tensor(), positions->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.ropeExt(*positions, nullptr, 8, 0, 512,
                                  10000.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        });

    verifySpecialOperation(
        *fixture.op,
        JobGgmlOp::RopeBack,
        { fixture.tensor->tensor(), positions->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.ropeExtBack(*positions, nullptr, 8, 0, 512,
                                      10000.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        });
}

// ============================================================================
// Flash Attention
// ============================================================================
TEST_CASE("Tensor Flash Attention operations create the expected GGML operations",
          "[ggml][tensor][op][special][attention]")
{
    auto context = JobGgmlContext::createUniqMetadata(128);
    REQUIRE(context != nullptr);

    auto q = context->newTensor4d(JobGgmlType::F32, 64, 8, 4, 1);
    auto k = context->newTensor4d(JobGgmlType::F32, 64, 8, 4, 1);
    auto v = context->newTensor4d(JobGgmlType::F32, 64, 8, 4, 1);

    REQUIRE(q != nullptr);
    REQUIRE(k != nullptr);
    REQUIRE(v != nullptr);

    auto qOp = JobGgmlTensorOp::createUniq(q->tensor(), context.get());
    REQUIRE(qOp != nullptr);

    verifySpecialOperation(
        *qOp,
        JobGgmlOp::FlashAttentionExt,
        { q->tensor(), k->tensor(), v->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.flashAttnExt(*k, *v, nullptr, 0.125f, 0.0f, 0.0f);
        });
}

// ============================================================================
// Convolutions & Im2Col
// ============================================================================
TEST_CASE("Tensor direct Convolution 2D operation creates the expected GGML operation",
          "[ggml][tensor][op][special][conv][2d]")
{
    auto context = JobGgmlContext::createUniqMetadata(128);
    REQUIRE(context != nullptr);

    auto kernel = context->newTensor4d(JobGgmlType::F32, 3, 3, 1, 1);
    auto data   = context->newTensor4d(JobGgmlType::F32, 16, 16, 1, 1);

    REQUIRE(kernel != nullptr);
    REQUIRE(data != nullptr);

    auto kernelOp = JobGgmlTensorOp::createUniq(kernel->tensor(), context.get());
    REQUIRE(kernelOp != nullptr);

    verifySpecialOperation(
        *kernelOp,
        JobGgmlOp::Conv2d,
        { kernel->tensor(), data->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.conv2dDirect(*data, 1, 1, 1, 1, 1, 1);
        });
}

TEST_CASE("Tensor Im2Col operation creates the expected GGML operation",
          "[ggml][tensor][op][special][im2col]")
{
    auto context = JobGgmlContext::createUniqMetadata(128);
    REQUIRE(context != nullptr);

    auto kernel = context->newTensor4d(JobGgmlType::F32, 3, 3, 1, 1);
    auto data   = context->newTensor4d(JobGgmlType::F32, 16, 16, 1, 1);

    REQUIRE(kernel != nullptr);
    REQUIRE(data != nullptr);

    auto kernelOp = JobGgmlTensorOp::createUniq(kernel->tensor(), context.get());
    REQUIRE(kernelOp != nullptr);

    verifySpecialOperation(
        *kernelOp,
        JobGgmlOp::Im2Col,
        { kernel->tensor(), data->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.im2col(*data, 1, 1, 1, 1, 1, 1, true, JobGgmlType::F32);
        });
}

// ============================================================================
// Pooling Operations
// ============================================================================
TEST_CASE("Tensor Pool 2D operation creates the expected GGML operation",
          "[ggml][tensor][op][special][pool]")
{
    SpecialOpFixture fixture;

    verifySpecialOperation(
        *fixture.op,
        JobGgmlOp::Pool2d,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &source) {
            return source.pool2d(JobGgmlPoolOp::Max, 2, 2, 2, 2, 0.0f, 0.0f);
        });
}

// ============================================================================
// State Space Models
// ============================================================================
TEST_CASE("Tensor SSM Conv operation creates the expected GGML operation",
          "[ggml][tensor][op][special][ssm][conv]")
{
    auto context = JobGgmlContext::createUniqMetadata(128);
    REQUIRE(context != nullptr);

    // c: [d_conv, d_inner]
    // sx: [d_conv - 1 + n_tokens, d_inner, n_sequences]
    auto c  = context->newTensor2d(JobGgmlType::F32, 4, 8);
    auto sx = context->newTensor3d(JobGgmlType::F32, 19, 8, 2);

    REQUIRE(c != nullptr);
    REQUIRE(sx != nullptr);

    auto sxOp = JobGgmlTensorOp::createUniq(sx->tensor(), context.get());
    REQUIRE(sxOp != nullptr);

    verifySpecialOperation(
        *sxOp,
        JobGgmlOp::SsmConv,
        { sx->tensor(), c->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.ssmConv(*c);
        });
}

// ============================================================================
// Clamp & Window Partitioning
// ============================================================================
TEST_CASE("Tensor clamp operation creates the expected GGML operation",
          "[ggml][tensor][op][special][clamp]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Clamp,
        [](JobGgmlTensorOp &source) {
            return source.clamp(-1.0f, 1.0f);
        });
}

TEST_CASE("Tensor window partition operations create the expected GGML operations",
          "[ggml][tensor][op][special][window]")
{
    auto context = JobGgmlContext::createUniqMetadata(128);
    REQUIRE(context != nullptr);

    auto tensor = context->newTensor4d(JobGgmlType::F32, 16, 8, 4, 1);
    REQUIRE(tensor != nullptr);

    auto op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
    REQUIRE(op != nullptr);

    auto partition = op->winPart(4);

    REQUIRE(partition != nullptr);
    REQUIRE(partition->isValid());
    REQUIRE(partition->operation()->operation() == JobGgmlOp::WindowPartition);
    REQUIRE(partition->operation()->sourceCount() == 1);
    REQUIRE(partition->operation()->source(0) == tensor->tensor());

    auto unpartition = partition->winUnpart(8, 4, 4);

    REQUIRE(unpartition != nullptr);
    REQUIRE(unpartition->isValid());
    REQUIRE(unpartition->operation()->operation() == JobGgmlOp::WindowUnpartition);
    REQUIRE(unpartition->operation()->sourceCount() == 1);
    REQUIRE(unpartition->operation()->source(0) == partition->tensor());
}

TEST_CASE("Tensor padding operations create the expected GGML operations",
          "[ggml][tensor][op][special][pad]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Pad,
        [](JobGgmlTensorOp &source) {
            return source.pad(1, 1, 0, 0);
        });

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Pad,
        [](JobGgmlTensorOp &source) {
            return source.padCircular(1, 1, 0, 0);
        });

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Pad,
        [](JobGgmlTensorOp &source) {
            return source.padExt(1, 1, 1, 1, 0, 0, 0, 0);
        });

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Pad,
        [](JobGgmlTensorOp &source) {
            return source.padExtCircular(1, 1, 1, 1, 0, 0, 0, 0);
        });
}

TEST_CASE("Tensor roll operation creates the expected GGML operation",
          "[ggml][tensor][op][special][roll]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Roll,
        [](JobGgmlTensorOp &source) {
            return source.roll(1, -1, 1, 0);
        });
}

TEST_CASE("Tensor timestep embedding operation creates the expected GGML operation",
          "[ggml][tensor][op][special][embedding][timestep]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);
    REQUIRE(context != nullptr);

    auto timesteps = context->newTensor1d(JobGgmlType::F32, 8);
    REQUIRE(timesteps != nullptr);

    auto op = JobGgmlTensorOp::createUniq(timesteps->tensor(), context.get());
    REQUIRE(op != nullptr);

    verifyUnaryOperation(
        *op,
        JobGgmlOp::TimestepEmbedding,
        [](JobGgmlTensorOp &source) {
            return source.timestepEmbedding(32, 10000);
        });
}

TEST_CASE("Tensor sorting operations create the expected GGML operations",
          "[ggml][tensor][op][special][sort]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::ArgSort,
        [](JobGgmlTensorOp &source) {
            return source.argsort(JobGgmlSortOrder::Ascending);
        });

    auto argsortTopK = fixture.op->argsortTopK(4);
    REQUIRE(argsortTopK != nullptr);
    REQUIRE(argsortTopK->isValid());
    REQUIRE(argsortTopK->operation()->operation() == JobGgmlOp::View);
    REQUIRE(argsortTopK->operation()->sourceCount() == 1);
    REQUIRE(argsortTopK->operation()->source(0) != fixture.tensor->tensor());
    REQUIRE(argsortTopK->operation()->source(0)->op == GGML_OP_ARGSORT);

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::TopK,
        [](JobGgmlTensorOp &source) {
            return source.topK(4);
        });
}


TEST_CASE("Tensor fill operations create the expected GGML operations",
          "[ggml][tensor][op][special][generation][fill]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Fill,
        [](JobGgmlTensorOp &source) {
            return source.fill(3.5f);
        });

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Fill,
        [](JobGgmlTensorOp &source) {
            return source.fillInplace(3.5f);
        });
}

TEST_CASE("Tensor triangular operation creates the expected GGML operation",
          "[ggml][tensor][op][special][generation][tri]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto tensor = context->newTensor2d(JobGgmlType::F32, 8, 8);
    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());

    auto op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    verifyUnaryOperation(
        *op,
        JobGgmlOp::Tri,
        [](JobGgmlTensorOp &source) {
            return source.tri(JobGgmlTriType::Upper);
        });
}

TEST_CASE("Tensor Pool 1D operation creates the expected GGML operation",
          "[ggml][tensor][op][special][pool][1d]")
{
    SpecialOpFixture fixture;

    verifyUnaryOperation(
        *fixture.op,
        JobGgmlOp::Pool1d,
        [](JobGgmlTensorOp &source) {
            return source.pool1d(JobGgmlPoolOp::Max, 2, 2, 0);
        });
}