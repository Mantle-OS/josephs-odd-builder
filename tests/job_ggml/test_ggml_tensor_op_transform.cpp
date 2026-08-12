#include <initializer_list>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <job_ggml_tensor_op.h>

#include "test_ggml_utils.h"

// Notes / GGML quirks:
// - ggml_cast stores src[1] as a self-reference to the returned tensor.
// - ggml_acc requires contiguous F32 source/destination tensors.
// - view operations borrow source storage and preserve src[0].
// - mul_mat precision/hint are stored in op_params[0] / op_params[1].

TEST_CASE("Tensor accumulation operations create the expected GGML operations",
          "[ggml][tensor][op][transform][acc]")
{
    TransformOpFixture fixture;

    auto add = fixture.context->newTensor2d(JobGgmlType::F32, 2, 2);
    REQUIRE(add != nullptr);

    const std::size_t nb1 = fixture.tensor->tensor()->nb[1];
    const std::size_t nb2 = fixture.tensor->tensor()->nb[2];
    const std::size_t nb3 = fixture.tensor->tensor()->nb[3];

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Acc,
        { fixture.tensor->tensor(), add->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.acc(*add, nb1, nb2, nb3, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Acc,
        { fixture.tensor->tensor(), add->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.accInplace(*add, nb1, nb2, nb3, 0);
        });
}

TEST_CASE("Tensor parameterized activation operations create the expected GGML operations",
          "[ggml][tensor][op][transform][activation]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::LeakyRelu,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.leakyRelu(0.01f, false);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::LeakyRelu,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.leakyRelu(0.01f, true);
        });

    auto xielu = fixture.op->xielu(0.8f, 0.8f, 0.5f, 1.0e-6f);
    REQUIRE(xielu != nullptr);
    REQUIRE(xielu->isValid());
    REQUIRE(xielu->operation()->operation() == JobGgmlOp::Unary);
    REQUIRE(xielu->operation()->unaryOperation() == JobGgmlUnaryOp::Xielu);
    REQUIRE(xielu->operation()->source(0) == fixture.tensor->tensor());

    auto glu = fixture.op->glu(JobGgmlGluOp::SwiGlu, false);
    REQUIRE(glu != nullptr);
    REQUIRE(glu->isValid());
    REQUIRE(glu->operation()->operation() == JobGgmlOp::Glu);
    REQUIRE(glu->operation()->gluOperation() == JobGgmlGluOp::SwiGlu);
    REQUIRE(glu->operation()->source(0) == fixture.tensor->tensor());
}

TEST_CASE("Tensor normalization operations create the expected GGML operations",
          "[ggml][tensor][op][transform][norm]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Norm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.norm(1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Norm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.normInplace(1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::RmsNorm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.rmsNorm(1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::RmsNorm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.rmsNormInplace(1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::GroupNorm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.groupNorm(2, 1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::GroupNorm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.groupNormInplace(2, 1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::L2Norm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.l2Norm(1.0e-5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::L2Norm,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.l2NormInplace(1.0e-5f);
        });
}

TEST_CASE("Tensor RMS normalization backward operation preserves both sources",
          "[ggml][tensor][op][transform][norm][back]")
{
    TransformOpFixture fixture;

    auto gradient = fixture.context->newTensor2d(JobGgmlType::F32, 8, 4);
    REQUIRE(gradient != nullptr);

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::RmsNormBack,
        { fixture.tensor->tensor(), gradient->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.rmsNormBack(*gradient, 1.0e-5f);
        });
}

TEST_CASE("Tensor scale operations create the expected GGML operations",
          "[ggml][tensor][op][transform][scale]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Scale,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.scale(2.0f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Scale,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.scaleInplace(2.0f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Scale,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.scaleBias(2.0f, 0.5f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Scale,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.scaleBiasInplace(2.0f, 0.5f);
        });
}

TEST_CASE("Tensor set operations create the expected GGML operations",
          "[ggml][tensor][op][transform][set]")
{
    TransformOpFixture fixture;

    auto value = fixture.context->newTensor2d(JobGgmlType::F32, 2, 2);
    REQUIRE(value != nullptr);

    const std::size_t nb1 = fixture.tensor->tensor()->nb[1];
    const std::size_t nb2 = fixture.tensor->tensor()->nb[2];
    const std::size_t nb3 = fixture.tensor->tensor()->nb[3];

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.set(*value, nb1, nb2, nb3, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.setInplace(*value, nb1, nb2, nb3, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.set1d(*value, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.set1dInplace(*value, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.set2d(*value, nb1, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Set,
        { fixture.tensor->tensor(), value->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.set2dInplace(*value, nb1, 0);
        });
}

TEST_CASE("Tensor cast operation preserves GGML copy self-reference semantics",
          "[ggml][tensor][op][transform][cast]")
{
    TransformOpFixture fixture;

    auto result = fixture.op->cast(JobGgmlType::F16);
    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->type() == JobGgmlType::F16);

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::Copy);
    REQUIRE(operation->sourceCount() == 2);
    REQUIRE(operation->source(0) == fixture.tensor->tensor());
    REQUIRE(operation->source(1) == result->tensor());
}

///////////////
TEST_CASE("Tensor contiguous reshape operations preserve element count",
          "[ggml][tensor][op][transform][contiguous]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Contiguous,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.cont1d(32);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Contiguous,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.cont2d(4, 8);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Contiguous,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.cont3d(4, 4, 2);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Contiguous,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.cont4d(4, 4, 2, 1);
        });
}

TEST_CASE("Tensor explicit reshape operations create the expected GGML operations",
          "[ggml][tensor][op][transform][reshape]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Reshape,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.reshape1d(32);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Reshape,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.reshape2d(4, 8);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Reshape,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.reshape3d(4, 4, 2);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Reshape,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.reshape4d(4, 4, 2, 1);
        });
}

TEST_CASE("Tensor view operations create the expected GGML views",
          "[ggml][tensor][op][transform][view]")
{
    TransformOpFixture fixture;

    const std::size_t nb1 = fixture.tensor->tensor()->nb[1];
    const std::size_t nb2 = fixture.tensor->tensor()->nb[2];
    const std::size_t nb3 = fixture.tensor->tensor()->nb[3];

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::View,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.view1d(8, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::View,
        { fixture.tensor->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.view2d(4, 4, nb1, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::View,
        { fixture.tensor->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.view3d(4, 4, 1, nb1, nb2, 0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::View,
        { fixture.tensor->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.view4d(4, 4, 1, 1, nb1, nb2, nb3, 0);
        });
}

TEST_CASE("Tensor permutation operation creates the expected GGML view",
          "[ggml][tensor][op][transform][permute]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::Permute,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.permute(1, 0, 2, 3);
        });
}

TEST_CASE("Tensor diagonal mask operations create the expected GGML operations",
          "[ggml][tensor][op][transform][diag_mask]")
{
    TransformOpFixture fixture;

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::DiagMaskInf,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.diagMaskInf(0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::DiagMaskInf,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.diagMaskInfInplace(0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::DiagMaskZero,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.diagMaskZero(0);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::DiagMaskZero,
        { fixture.tensor->tensor() },
        [](JobGgmlTensorOp &op) {
            return op.diagMaskZeroInplace(0);
        });
}

TEST_CASE("Tensor extended softmax operations preserve mask sources",
          "[ggml][tensor][op][transform][softmax]")
{
    TransformOpFixture fixture;

    auto mask = fixture.context->newTensor2d(JobGgmlType::F32, 8, 4);
    REQUIRE(mask != nullptr);

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::SoftMax,
        { fixture.tensor->tensor(), mask->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.softMaxExt(*mask, 1.0f, 0.0f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::SoftMax,
        { fixture.tensor->tensor(), mask->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.softMaxExtInplace(*mask, 1.0f, 0.0f);
        });
}

TEST_CASE("Tensor extended softmax backward operations preserve both sources",
          "[ggml][tensor][op][transform][softmax][back]")
{
    TransformOpFixture fixture;

    auto gradient = fixture.context->newTensor2d(JobGgmlType::F32, 8, 4);
    REQUIRE(gradient != nullptr);

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::SoftMaxBack,
        { fixture.tensor->tensor(), gradient->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.softMaxExtBack(*gradient, 1.0f, 0.0f);
        });

    verifyTransformOperation(
        *fixture.op,
        JobGgmlOp::SoftMaxBack,
        { fixture.tensor->tensor(), gradient->tensor() },
        [&](JobGgmlTensorOp &op) {
            return op.softMaxExtBackInplace(*gradient, 1.0f, 0.0f);
        });
}

TEST_CASE("Tensor extended softmax accepts sink tensors",
          "[ggml][tensor][op][transform][softmax][sinks]")
{
    TransformOpFixture fixture;

    auto mask  = fixture.context->newTensor2d(JobGgmlType::F32, 8, 4);
    auto sinks = fixture.context->newTensor1d(JobGgmlType::F32, 1);
    REQUIRE(mask != nullptr);
    REQUIRE(sinks != nullptr);

    auto result = fixture.op->softMaxExt(*mask, 1.0f, 0.0f);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    result->softMaxAddSinks(*sinks);

    REQUIRE(result->operation()->sourceCount() == 3);
    REQUIRE(result->operation()->source(0) == fixture.tensor->tensor());
    REQUIRE(result->operation()->source(1) == mask->tensor());
    REQUIRE(result->operation()->source(2) == sinks->tensor());
}

TEST_CASE("Tensor matrix multiplication precision and hint setters update operation parameters",
          "[ggml][tensor][op][transform][mul_mat][params]")
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

    auto result = op->mulMat(*right);
    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->operation()->operation() == JobGgmlOp::MulMat);

    result->mulMatSetPrec(JobGgmlPrecision::F32);
    result->mulMatSetHint(JobGgmlOpHint::Src0IsHadamard);
    REQUIRE(result->operation()->parameter(0) == static_cast<std::int32_t>(JobGgmlPrecision::F32));
    REQUIRE(result->operation()->parameter(1) == static_cast<std::int32_t>(JobGgmlOpHint::Src0IsHadamard));
}
