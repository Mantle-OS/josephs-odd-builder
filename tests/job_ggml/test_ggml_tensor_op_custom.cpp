#include <catch2/catch_test_macros.hpp>

#include <job_ggml_tensor_op.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

TEST_CASE("Custom map1 callbacks construct expected GGML custom operations",
          "[ggml][tensor][op][custom][map1]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom1(
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == fixture.context.get());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom1);
    REQUIRE(operation->sourceCount() == 1);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
}

TEST_CASE("Custom map2 callbacks construct expected GGML custom operations", "[ggml][tensor][op][custom][map2]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom2(
        *fixture.tensorB,
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b,
           int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)b;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom2);
    REQUIRE(operation->sourceCount() == 2);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
}

TEST_CASE("Custom map3 callbacks construct expected GGML custom operations",
          "[ggml][tensor][op][custom][map3]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom3(
        *fixture.tensorB,
        *fixture.tensorC,
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b,
           const JobGgmlTensor &c, int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)b;
            (void)c;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom3);
    REQUIRE(operation->sourceCount() == 3);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
    REQUIRE(operation->source(2) == fixture.tensorC->tensor());
}

TEST_CASE("Factory custom4d creates raw custom GGML operations",
          "[ggml][tensor][op][custom][custom4d]")
{
    CustomOpFixture fixture;

    std::array<JobGgmlTensor *, 2> args{
        fixture.tensorA.get(),
        fixture.tensorB.get()
    };

    auto result = fixture.opA->custom4d(
        JobGgmlType::F32,
        CustomOpFixture::Ne0,
        CustomOpFixture::Ne1,
        1,
        1,
        args,
        [](JobGgmlTensor &dst, int ith, int nth, void *userdata) {
            (void)dst;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::Custom);
    REQUIRE(operation->sourceCount() == 2);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
}


TEST_CASE("Custom map1 inplace callback constructs expected GGML custom operation",
          "[ggml][tensor][op][custom][map1][inplace]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom1Inplace(
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom1);
    REQUIRE(operation->sourceCount() == 1);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
}

TEST_CASE("Custom map2 inplace callback constructs expected GGML custom operation",
          "[ggml][tensor][op][custom][map2][inplace]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom2Inplace(
        *fixture.tensorB,
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b,
           int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)b;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom2);
    REQUIRE(operation->sourceCount() == 2);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
}

TEST_CASE("Custom map3 inplace callback constructs expected GGML custom operation",
          "[ggml][tensor][op][custom][map3][inplace]")
{
    CustomOpFixture fixture;

    auto result = fixture.opA->mapCustom3Inplace(
        *fixture.tensorB,
        *fixture.tensorC,
        [](JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b,
           const JobGgmlTensor &c, int ith, int nth, void *userdata) {
            (void)dst;
            (void)a;
            (void)b;
            (void)c;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::MapCustom3);
    REQUIRE(operation->sourceCount() == 3);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
    REQUIRE(operation->source(2) == fixture.tensorC->tensor());
}

TEST_CASE("Custom inplace operation preserves expected GGML source ordering",
          "[ggml][tensor][op][custom][custom][inplace]")
{
    CustomOpFixture fixture;

    std::array<JobGgmlTensor *, 2> args{
        fixture.tensorB.get(),
        fixture.tensorC.get()
    };

    auto result = fixture.opA->customInplace(
        args,
        [](JobGgmlTensor &dst, int ith, int nth, void *userdata) {
            (void)dst;
            (void)ith;
            (void)nth;
            (void)userdata;
        },
        1);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->operation() == JobGgmlOp::Custom);
    REQUIRE(operation->sourceCount() == 3);
    REQUIRE(operation->source(0) == fixture.tensorA->tensor());
    REQUIRE(operation->source(1) == fixture.tensorB->tensor());
    REQUIRE(operation->source(2) == fixture.tensorC->tensor());
}
