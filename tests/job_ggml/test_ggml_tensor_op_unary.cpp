#include <catch2/catch_test_macros.hpp>

#include <job_ggml_tensor_op.h>

#include "test_ggml_utils.h"

using namespace job::ggml;



TEST_CASE("Tensor operation wrapper preserves its source tensor and context", "[ggml][tensor][op][unary][construction]")
{
    UnaryOpFixture fixture;

    REQUIRE(fixture.op != nullptr);
    REQUIRE(fixture.op->isValid());

    REQUIRE(fixture.op->tensor() == fixture.tensor->tensor());
    REQUIRE(fixture.op->context() == fixture.context.get());
}

TEST_CASE("Tensor direct unary operations create the expected GGML operations", "[ggml][tensor][op][unary][direct]")
{
    UnaryOpFixture fixture;

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Dup, [](JobGgmlTensorOp &op) {
        return op.dup();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Dup, [](JobGgmlTensorOp &op) {
        return op.dupInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sqr, [](JobGgmlTensorOp &op) {
        return op.sqr();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sqr, [](JobGgmlTensorOp &op) {
        return op.sqrInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sqrt, [](JobGgmlTensorOp &op) {
        return op.sqrt();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sqrt, [](JobGgmlTensorOp &op) {
        return op.sqrtInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Log, [](JobGgmlTensorOp &op) {
        return op.log();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Log, [](JobGgmlTensorOp &op) {
        return op.logInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sin, [](JobGgmlTensorOp &op) {
        return op.sin();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sin, [](JobGgmlTensorOp &op) {
        return op.sinInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Cos, [](JobGgmlTensorOp &op) {
        return op.cos();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Cos, [](JobGgmlTensorOp &op) {
        return op.cosInplace();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Sum, [](JobGgmlTensorOp &op) {
        return op.sum();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::SumRows, [](JobGgmlTensorOp &op) {
        return op.sumRows();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::CumSum, [](JobGgmlTensorOp &op) {
        return op.cumsum();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Mean, [](JobGgmlTensorOp &op) {
        return op.mean();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::ArgMax, [](JobGgmlTensorOp &op) {
        return op.argmax();
    });
}

TEST_CASE("Tensor unary family operations preserve their unary subtype",
          "[ggml][tensor][op][unary][family]")
{
    UnaryOpFixture fixture;

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Abs, [](JobGgmlTensorOp &op) {
        return op.abs();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Abs, [](JobGgmlTensorOp &op) {
        return op.absInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Sign, [](JobGgmlTensorOp &op) {
        return op.sgn();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Sign, [](JobGgmlTensorOp &op) {
        return op.sgnInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Negate, [](JobGgmlTensorOp &op) {
        return op.neg();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Negate, [](JobGgmlTensorOp &op) {
        return op.negInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Step, [](JobGgmlTensorOp &op) {
        return op.step();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Step, [](JobGgmlTensorOp &op) {
        return op.stepInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Tanh, [](JobGgmlTensorOp &op) {
        return op.tanh();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Tanh, [](JobGgmlTensorOp &op) {
        return op.tanhInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Elu, [](JobGgmlTensorOp &op) {
        return op.elu();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Elu, [](JobGgmlTensorOp &op) {
        return op.eluInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Relu, [](JobGgmlTensorOp &op) {
        return op.relu();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Relu, [](JobGgmlTensorOp &op) {
        return op.reluInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Sigmoid, [](JobGgmlTensorOp &op) {
        return op.sigmoid();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Sigmoid, [](JobGgmlTensorOp &op) {
        return op.sigmoidInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Gelu, [](JobGgmlTensorOp &op) {
        return op.gelu();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Gelu, [](JobGgmlTensorOp &op) {
        return op.geluInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::GeluErf, [](JobGgmlTensorOp &op) {
        return op.geluErf();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::GeluErf, [](JobGgmlTensorOp &op) {
        return op.geluErfInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::GeluQuick, [](JobGgmlTensorOp &op) {
        return op.geluQuick();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::GeluQuick, [](JobGgmlTensorOp &op) {
        return op.geluQuickInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Silu, [](JobGgmlTensorOp &op) {
        return op.silu();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Silu, [](JobGgmlTensorOp &op) {
        return op.siluInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::HardSwish, [](JobGgmlTensorOp &op) {
        return op.hardswish();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::HardSigmoid, [](JobGgmlTensorOp &op) {
        return op.hardsigmoid();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Exp, [](JobGgmlTensorOp &op) {
        return op.exp();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Exp, [](JobGgmlTensorOp &op) {
        return op.expInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Expm1, [](JobGgmlTensorOp &op) {
        return op.expm1();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Expm1, [](JobGgmlTensorOp &op) {
        return op.expm1Inplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::SoftPlus, [](JobGgmlTensorOp &op) {
        return op.softplus();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::SoftPlus, [](JobGgmlTensorOp &op) {
        return op.softplusInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Floor, [](JobGgmlTensorOp &op) {
        return op.floor();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Floor, [](JobGgmlTensorOp &op) {
        return op.floorInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Ceil, [](JobGgmlTensorOp &op) {
        return op.ceil();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Ceil, [](JobGgmlTensorOp &op) {
        return op.ceilInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Round, [](JobGgmlTensorOp &op) {
        return op.round();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Round, [](JobGgmlTensorOp &op) {
        return op.roundInplace();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Truncate, [](JobGgmlTensorOp &op) {
        return op.trunc();
    });

    verifyUnarySubtype(*fixture.op, JobGgmlUnaryOp::Truncate, [](JobGgmlTensorOp &op) {
        return op.truncInplace();
    });
}

TEST_CASE("Tensor unary GLU convenience operations preserve their GLU subtype",
          "[ggml][tensor][op][unary][glu]")
{
    UnaryOpFixture fixture;

    verifyGluOperation(*fixture.op, JobGgmlGluOp::ReGlu, [](JobGgmlTensorOp &op) {
        return op.reglu();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::ReGlu, [](JobGgmlTensorOp &op) {
        return op.regluSwapped();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGlu, [](JobGgmlTensorOp &op) {
        return op.geglu();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGlu, [](JobGgmlTensorOp &op) {
        return op.gegluSwapped();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::SwiGlu, [](JobGgmlTensorOp &op) {
        return op.swiglu();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::SwiGlu, [](JobGgmlTensorOp &op) {
        return op.swigluSwapped();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGluErf, [](JobGgmlTensorOp &op) {
        return op.gegluErf();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGluErf, [](JobGgmlTensorOp &op) {
        return op.gegluErfSwapped();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGluQuick, [](JobGgmlTensorOp &op) {
        return op.gegluQuick();
    });

    verifyGluOperation(*fixture.op, JobGgmlGluOp::GeGluQuick, [](JobGgmlTensorOp &op) {
        return op.gegluQuickSwapped();
    });
}

TEST_CASE("Tensor diag operation creates the expected GGML operation",
          "[ggml][tensor][op][unary][structural][diag]")
{

    auto context = JobGgmlContext::createUniqMetadata(16);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto tensor = context->newTensor2d(JobGgmlType::F32, 8, 1);

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());

    auto op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());

    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    verifyUnaryOperation(*op, JobGgmlOp::Diag, [](JobGgmlTensorOp &source) {
        return source.diag();
    });
}

TEST_CASE("Tensor unary structural operations create the expected GGML operations",
          "[ggml][tensor][op][unary][structural]")
{
    UnaryOpFixture fixture;

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Contiguous, [](JobGgmlTensorOp &op) {
        return op.cont();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::Transpose, [](JobGgmlTensorOp &op) {
        return op.transpose();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::SoftMax, [](JobGgmlTensorOp &op) {
        return op.softMax();
    });

    verifyUnaryOperation(*fixture.op, JobGgmlOp::SoftMax, [](JobGgmlTensorOp &op) {
        return op.softMaxInplace();
    });
}

