#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_hmac_sha256.h>

using namespace job::crypto;

// ============================================================================
// 1. Usage / Examples
// ============================================================================

TEST_CASE("JobHmacSha256 generates secure HMAC keys", "[job_crypto][hmac_sha256]")
{
    JobSecureMem key = JobHmacSha256::generateKey();

    REQUIRE_FALSE(key.empty());
    REQUIRE(key.size() == JobHmacSha256::kKeySize);
}

TEST_CASE("JobHmacSha256 computes and verifies a message", "[job_crypto][hmac_sha256]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Joseph's Odd Builder";

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, key);

    REQUIRE(JobHmacSha256::verify(mac, message, key));
}

TEST_CASE("JobHmacSha256 produces deterministic MACs for the same key and message", "[job_crypto][hmac_sha256]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Deterministic HMAC-SHA256 message";

    JobHmacSha256::Mac const first = JobHmacSha256::compute(message, key);
    JobHmacSha256::Mac const second = JobHmacSha256::compute(message, key);

    REQUIRE(first == second);
}

TEST_CASE("JobHmacSha256 computes secure MAC output", "[job_crypto][hmac_sha256]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Secure HMAC-SHA256 output";

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, key);
    JobSecureMem secureMac = JobHmacSha256::computeSecure(message, key);

    REQUIRE_FALSE(secureMac.empty());
    REQUIRE(secureMac.size() == JobHmacSha256::kMacSize);

    REQUIRE(std::equal(mac.begin(), mac.end(), secureMac.data()));
}

TEST_CASE("JobHmacSha256 matches RFC 4231 HMAC-SHA256 test vector", "[job_crypto][hmac_sha256][vector]")
{
    // RFC 4231 - Test Case 1:
    //
    // Key  = 20 bytes of 0x0b
    // Data = "Hi There"
    //
    // This verifies our result against an independent HMAC-SHA256 test vector
    // rather than merely checking JobHmacSha256 against itself.

    constexpr std::array<unsigned char, 20> key{
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };

    constexpr std::string_view message = "Hi There";

    constexpr JobHmacSha256::Mac expected{
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
        0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
        0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
    };

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(
        message.data(),
        message.size(),
        key.data(),
        key.size()
        );

    REQUIRE(mac == expected);
}


// ============================================================================
// 2. Edge Cases / Failure Behavior
// ============================================================================

TEST_CASE("JobHmacSha256 supports empty message data", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message;

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, key);

    REQUIRE(JobHmacSha256::verify(mac, message, key));
}

TEST_CASE("JobHmacSha256 rejects a changed message", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view original = "The original authenticated message";
    constexpr std::string_view modified = "The modified authenticated message";

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(original, key);

    REQUIRE_FALSE(JobHmacSha256::verify(mac, modified, key));
}

TEST_CASE("JobHmacSha256 rejects a different key", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem firstKey = JobHmacSha256::generateKey();
    JobSecureMem secondKey = JobHmacSha256::generateKey();

    REQUIRE_FALSE(firstKey.empty());
    REQUIRE_FALSE(secondKey.empty());

    constexpr std::string_view message = "Authenticated using only one key";

    JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, firstKey);

    REQUIRE_FALSE(JobHmacSha256::verify(mac, message, secondKey));
}

TEST_CASE("JobHmacSha256 rejects a corrupted MAC", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Do not touch my bits";

    JobHmacSha256::Mac mac = JobHmacSha256::compute(message, key);

    mac[0] ^= 0x01;

    REQUIRE_FALSE(JobHmacSha256::verify(mac, message, key));
}

TEST_CASE("JobHmacSha256 generated keys are not trivially identical", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem firstKey = JobHmacSha256::generateKey();
    JobSecureMem secondKey = JobHmacSha256::generateKey();

    REQUIRE_FALSE(firstKey.empty());
    REQUIRE_FALSE(secondKey.empty());

    REQUIRE(firstKey.size() == JobHmacSha256::kKeySize);
    REQUIRE(secondKey.size() == JobHmacSha256::kKeySize);

    REQUIRE(firstKey != secondKey);
}

TEST_CASE("JobHmacSha256 secure result can be explicitly cleared", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    JobSecureMem mac = JobHmacSha256::computeSecure("Sensitive derived material", key);

    REQUIRE_FALSE(mac.empty());
    REQUIRE(mac.size() == JobHmacSha256::kMacSize);

    mac.clear();

    REQUIRE(mac.empty());
    REQUIRE(mac.size() == 0);
}

TEST_CASE("JobHmacSha256 rejects null data with a non-zero size", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    JobHmacSha256::Mac const mac =
        JobHmacSha256::compute(nullptr, 1, key);

    REQUIRE(std::all_of(mac.begin(), mac.end(), [](unsigned char value) {
        return value == 0;
    }));
}

TEST_CASE("JobHmacSha256 treats null data with zero size as an empty message", "[job_crypto][hmac_sha256][edge]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    JobHmacSha256::Mac const nullData =
        JobHmacSha256::compute(nullptr, 0, key);

    JobHmacSha256::Mac const emptyData =
        JobHmacSha256::compute(std::string_view{}, key);

    REQUIRE(nullData == emptyData);
}

TEST_CASE("JobHmacSha256 rejects null key with a non-zero size", "[job_crypto][hmac_sha256][edge]")
{
    constexpr std::string_view message = "Bad key pointer";

    JobHmacSha256::Mac const mac =
        JobHmacSha256::compute(
            message.data(),
            message.size(),
            nullptr,
            JobHmacSha256::kKeySize
            );

    REQUIRE(std::all_of(mac.begin(), mac.end(), [](unsigned char value) {
        return value == 0;
    }));
}


#ifdef JOB_TEST_BENCHMARKS

// ============================================================================
// 3. Benchmarks / Stress
// ============================================================================

TEST_CASE("JobHmacSha256 small message throughput", "[job_crypto][hmac_sha256][benchmark]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message =
        "A relatively small authenticated payload used for HMAC benchmarking.";

    BENCHMARK("HMAC-SHA256 small message")
    {
        return JobHmacSha256::compute(message, key);
    };
}

TEST_CASE("JobHmacSha256 one MiB throughput", "[job_crypto][hmac_sha256][benchmark]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    std::array<unsigned char, 1024 * 1024> data{};
    data.fill(0x5a);

    BENCHMARK("HMAC-SHA256 1 MiB")
    {
        return JobHmacSha256::compute(
            data.data(),
            data.size(),
            key
            );
    };
}

TEST_CASE("JobHmacSha256 secure output overhead", "[job_crypto][hmac_sha256][benchmark]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Benchmark secure HMAC output";

    BENCHMARK("HMAC-SHA256 normal output")
    {
        return JobHmacSha256::compute(message, key);
    };

    BENCHMARK("HMAC-SHA256 secure output")
    {
        return JobHmacSha256::computeSecure(message, key);
    };
}

TEST_CASE("JobHmacSha256 verification throughput", "[job_crypto][hmac_sha256][benchmark]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Benchmark HMAC verification";
    JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, key);

    BENCHMARK("HMAC-SHA256 verify") {
        return JobHmacSha256::verify(mac, message, key);
    };
}

TEST_CASE("JobHmacSha256 remains deterministic under repeated computation",
          "[job_crypto][hmac_sha256][benchmark][stress]")
{
    JobSecureMem key = JobHmacSha256::generateKey();
    REQUIRE_FALSE(key.empty());

    constexpr std::string_view message = "Repeated HMAC-SHA256 stress payload";

    JobHmacSha256::Mac const expected = JobHmacSha256::compute(message, key);

    constexpr std::size_t iterations = 100000;

    for (std::size_t i = 0; i < iterations; ++i) {
        JobHmacSha256::Mac const mac = JobHmacSha256::compute(message, key);

        if (mac != expected) {
            FAIL("HMAC-SHA256 output changed during repeated stress computation");
            break;
        }
    }

    SUCCEED();
}

#endif