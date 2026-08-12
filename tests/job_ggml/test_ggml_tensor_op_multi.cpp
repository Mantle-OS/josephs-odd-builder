#include <initializer_list>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <job_ggml_tensor_op.h>

#include "test_ggml_utils.h"

// Notes / GGML quirks:
// - ggml_add_cast requires source a to be quantized, F16, or BF16.
// - ggml_set_rows stores src[] as { b, c, a }, not native argument order { a, b, c }.

using namespace job::ggml;


TEST_CASE("Tensor binary arithmetic operations create the expected GGML operations",
          "[ggml][tensor][op][multi][binary][arithmetic]")
{
    MultiOpFixture fixture;

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Add,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.add(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Add,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.addInplace(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Sub,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.sub(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Sub,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.subInplace(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Mul,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.mul(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Mul,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.mulInplace(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Div,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.div(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Div,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.divInplace(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::CountEqual,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.countEqual(*fixture.right);
        });

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::SiluBack,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.siluBack(*fixture.right);
        });
}

TEST_CASE("Tensor add cast operation creates the expected GGML operation", "[ggml][tensor][op][multi][binary][add_cast]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto left = context->newTensor2d(JobGgmlType::F16, 8, 4);
    auto right = context->newTensor2d(JobGgmlType::F32, 8, 4);

    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());

    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    verifyMultiOperation(
        *op,
        JobGgmlOp::Add,
        {
            left->tensor(),
            right->tensor()
        },
        [&](JobGgmlTensorOp &source) {
            return source.addCast(*right, JobGgmlType::F32);
        });
}

TEST_CASE("Tensor repeat operations preserve their native source relationships",
          "[ggml][tensor][op][multi][repeat]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto small = context->newTensor2d(JobGgmlType::F32, 2, 1);
    auto large = context->newTensor2d(JobGgmlType::F32, 2, 4);

    REQUIRE(small != nullptr);
    REQUIRE(large != nullptr);

    auto smallOp = JobGgmlTensorOp::createUniq(small->tensor(), context.get());
    auto largeOp = JobGgmlTensorOp::createUniq(large->tensor(), context.get());

    REQUIRE(smallOp != nullptr);
    REQUIRE(largeOp != nullptr);

    verifyMultiOperation(
        *smallOp,
        JobGgmlOp::Repeat,
        { small->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.repeat(*large);
        });

    verifyMultiOperation(
        *largeOp,
        JobGgmlOp::RepeatBack,
        { large->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.repeatBack(*small);
        });
}

TEST_CASE("Tensor concatenation creates the expected multi-source operation",
          "[ggml][tensor][op][multi][concat]")
{
    MultiOpFixture fixture;

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Concat,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.concat(*fixture.right, 1);
        });
}

TEST_CASE("Tensor split GLU convenience operations preserve their GLU subtype",
          "[ggml][tensor][op][multi][glu][split]")
{
    MultiOpFixture fixture;

    // GGML requires split GLU operands to be contiguous,
    // identically shaped, and of the same type.
    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::ReGlu,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.regluSplit(other);
        });

    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::GeGlu,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.gegluSplit(other);
        });

    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::SwiGlu,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.swigluSplit(other);
        });

    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::GeGluErf,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.gegluErfSplit(other);
        });

    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::GeGluQuick,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.gegluQuickSplit(other);
        });

    verifySplitGluOperation(
        *fixture.op,
        *fixture.right,
        JobGgmlGluOp::ReGlu,
        [](JobGgmlTensorOp &op, JobGgmlTensor &other) {
            return op.gluSplit(other, JobGgmlGluOp::ReGlu);
        });
}

/////////////////////////////////
// SHAPE SENSITIVE

TEST_CASE("Tensor matrix multiplication creates the expected GGML operation",
          "[ggml][tensor][op][multi][matrix][mul_mat]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto left  = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto right = context->newTensor2d(JobGgmlType::F32, 4, 2);

    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());

    REQUIRE(op != nullptr);

    verifyMultiOperation(
        *op,
        JobGgmlOp::MulMat,
        { left->tensor(), right->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.mulMat(*right);
        });
}

TEST_CASE("Tensor reshape by shape tensor records only the data tensor as a graph source",
          "[ggml][tensor][op][multi][reshape]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto source = context->newTensor2d(JobGgmlType::F32, 8, 4);
    auto shape  = context->newTensor2d(JobGgmlType::F32, 4, 8);

    REQUIRE(source != nullptr);
    REQUIRE(shape != nullptr);

    auto op = JobGgmlTensorOp::createUniq(source->tensor(), context.get());

    REQUIRE(op != nullptr);

    verifyMultiOperation(
        *op,
        JobGgmlOp::Reshape,
        { source->tensor() },
        [&](JobGgmlTensorOp &sourceOp) {
            return sourceOp.reshape(*shape);
        });
}

TEST_CASE("Tensor copy operation records source and destination tensors",
          "[ggml][tensor][op][multi][copy]")
{
    MultiOpFixture fixture;

    verifyMultiOperation(
        *fixture.op,
        JobGgmlOp::Copy,
        { fixture.left->tensor(), fixture.right->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.cpy(*fixture.right);
        });
}

/////////////////////////////////////////////
// ROW OP

TEST_CASE("Tensor get rows operation creates the expected GGML operation",
          "[ggml][tensor][op][multi][rows][get]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto data    = context->newTensor2d(JobGgmlType::F32, 4, 8);
    auto indices = context->newTensor1d(JobGgmlType::I32, 3);
    REQUIRE(data != nullptr);
    REQUIRE(indices != nullptr);

    auto op = JobGgmlTensorOp::createUniq(data->tensor(), context.get());
    REQUIRE(op != nullptr);

    verifyMultiOperation(
        *op,
        JobGgmlOp::GetRows,
        { data->tensor(), indices->tensor() },
        [&](JobGgmlTensorOp &source) {
            return source.getRows(*indices);
        });
}

TEST_CASE("Tensor set rows operation creates the expected GGML operation",
          "[ggml][tensor][op][multi][rows][set]")
{
    auto context = JobGgmlContext::createUniqMetadata(64);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto destination = context->newTensor2d(JobGgmlType::F32, 4, 8);
    auto source      = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto indices     = context->newTensor1d(JobGgmlType::I32, 3);
    REQUIRE(destination != nullptr);
    REQUIRE(source != nullptr);
    REQUIRE(indices != nullptr);

    auto op = JobGgmlTensorOp::createUniq(destination->tensor(), context.get());
    REQUIRE(op != nullptr);

    verifyMultiOperation(
        *op,
        JobGgmlOp::SetRows,
        { source->tensor(), indices->tensor(), destination->tensor() },
        [&](JobGgmlTensorOp &destinationOp) {
            return destinationOp.setRows(*source, *indices);
        });
}