#include <catch2/catch_test_macros.hpp>
#include "job_crypto_init.h"

using namespace job::crypto;

TEST_CASE("JobCryptoInit initializes libsodium exactly once", "[crypto][init]")
{
    SECTION("Initialization succeeds and sets flag")
    {
        REQUIRE(JobCryptoInit::initialize());
        REQUIRE(JobCryptoInit::isInitialized());
    }

    SECTION("Calling initialize() again is idempotent")
    {
        bool first = JobCryptoInit::initialize();
        bool second = JobCryptoInit::initialize();
        REQUIRE(first);
        REQUIRE(second);
        REQUIRE(JobCryptoInit::isInitialized());
    }
}

