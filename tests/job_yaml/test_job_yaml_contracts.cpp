#include <catch2/catch_test_macros.hpp>

#include <contracts>

#include <job_yaml_contracts.h>

#include "test_job_yaml_fixtures.h"

namespace job::yaml::tests {

// ========================================
// Usage / examples
// ========================================

TEST_CASE("job_yaml preconditions allow valid calls",
          "[job_yaml][contracts][pre]")
{
    ContractFixture::reset();

    ContractFixture::requirePositive(1);

    REQUIRE(ContractFixture::preBodyCalls == 1);
}

TEST_CASE("job_yaml postconditions allow valid results",
          "[job_yaml][contracts][post]")
{
    ContractFixture::reset();

    REQUIRE(ContractFixture::producePositive(true) == 1);
    REQUIRE(ContractFixture::postBodyCalls == 1);
}

TEST_CASE("job_yaml contract assertions allow valid state",
          "[job_yaml][contracts][assert]")
{
    ContractFixture::reset();

    ContractFixture::requireNonZero(1);

    REQUIRE(ContractFixture::assertBodyCalls == 1);
}

// ========================================
// Observe-mode contract behavior
// ========================================

TEST_CASE("job_yaml precondition violations reach the process contract handler",
          "[job_yaml][contracts][pre][observe]")
{
    ContractFixture::reset();

    ContractFixture::requirePositive(0);

    REQUIRE(ContractFixture::preBodyCalls == 1);
}

TEST_CASE("job_yaml postcondition violations reach the process contract handler",
          "[job_yaml][contracts][post][observe]")
{
    ContractFixture::reset();

    REQUIRE(ContractFixture::producePositive(false) == 0);
    REQUIRE(ContractFixture::postBodyCalls == 1);
}

TEST_CASE("job_yaml contract assertion violations reach the process contract handler",
          "[job_yaml][contracts][assert][observe]")
{
    ContractFixture::reset();

    ContractFixture::requireNonZero(0);

    REQUIRE(ContractFixture::assertBodyCalls == 1);
}

// ========================================
// Repeated use
// ========================================

TEST_CASE("job_yaml contracts remain usable after observed violations",
          "[job_yaml][contracts][observe][reuse]")
{
    ContractFixture::reset();

    ContractFixture::requirePositive(0);
    ContractFixture::requirePositive(1);

    REQUIRE(ContractFixture::preBodyCalls == 2);

    REQUIRE(ContractFixture::producePositive(false) == 0);
    REQUIRE(ContractFixture::producePositive(true) == 1);

    REQUIRE(ContractFixture::postBodyCalls == 2);

    ContractFixture::requireNonZero(0);
    ContractFixture::requireNonZero(1);

    REQUIRE(ContractFixture::assertBodyCalls == 2);
}

} // namespace job::yaml::tests