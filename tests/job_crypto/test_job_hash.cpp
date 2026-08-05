#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstring>
#include <string>
#include <vector>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <sodium.h>

#include "job_hash.h"
#include "../transient_test_file.h"

using namespace job::crypto;

TEST_CASE("JobHash basic message digests and standard unkeyed hashing lifecycles", "[job_crypto][hash][example]")
{
    std::string const localData = "The quick brown fox jumps over the lazy dog";
    std::vector<unsigned char> const inputBuffer(localData.begin(), localData.end());

    SECTION("Compute default sizing BLAKE2b buffer digest")
    {
        std::vector<unsigned char> const hash = JobHash::hashBuffer(inputBuffer);

        REQUIRE_FALSE(hash.empty());
        REQUIRE(hash.size() == crypto_generichash_BYTES); // 32-byte standard output length
    }

    SECTION("Streaming file hashing matched verification check bounds")
    {
        std::string const targetPath = "test_hash_payload.bin";
        // Create 256KB of deterministic testing filler data
        TransientTestFile testFile(targetPath, 256 * 1024, 0x55);

        std::vector<unsigned char> const fileHash = JobHash::hashFile(targetPath, 64); // Request max 64-byte width
        REQUIRE(fileHash.size() == crypto_generichash_BYTES_MAX);

        // Re-hash identical data to ensure consistency
        std::vector<unsigned char> const secondPass = JobHash::hashFile(targetPath, 64);
        REQUIRE(fileHash == secondPass);
    }
}

TEST_CASE("JobHash BLAKE2b message authentication code execution loops", "[job_crypto][hash][example]")
{
    std::string const secretMsg = "Authenticated transaction context stream.";
    std::vector<unsigned char> const buffer(secretMsg.begin(), secretMsg.end());

    std::vector<unsigned char> const macKey1(crypto_generichash_KEYBYTES, 0x44);
    std::vector<unsigned char> const macKey2(crypto_generichash_KEYBYTES, 0x99);

    std::vector<unsigned char> const tokenA = JobHash::hashBuffer(buffer, crypto_generichash_BYTES, macKey1.data(), macKey1.size());
    std::vector<unsigned char> const tokenB = JobHash::hashBuffer(buffer, crypto_generichash_BYTES, macKey1.data(), macKey1.size());
    std::vector<unsigned char> const tokenC = JobHash::hashBuffer(buffer, crypto_generichash_BYTES, macKey2.data(), macKey2.size());

    REQUIRE_FALSE(tokenA.empty());
    // Safe deterministic checks match
    REQUIRE(tokenA == tokenB);
    // Keys differ, so generated tokens must completely diverge
    REQUIRE_FALSE(tokenA == tokenC);
}


// BLOCK TWO
TEST_CASE("JobHash edge limits and input validation constraints", "[job_crypto][hash][edge]")
{
    SECTION("Processing true empty zero-length data profiles cleanly")
    {
        std::vector<unsigned char> const emptyInput;
        std::vector<unsigned char> const hash = JobHash::hashBuffer(emptyInput);

        REQUIRE_FALSE(hash.empty());
        REQUIRE(hash.size() == crypto_generichash_BYTES);
    }

    SECTION("Reject validation tracking runs if specified hash size is out of bounds")
    {
        std::vector<unsigned char> const payload(64, 0x12);

        // Truncated size check under the 16-byte minimum
        std::vector<unsigned char> const lowHash = JobHash::hashBuffer(payload, 8);
        REQUIRE(lowHash.empty());

        // Overflow size check above the 64-byte maximum
        std::vector<unsigned char> const highHash = JobHash::hashBuffer(payload, 128);
        REQUIRE(highHash.empty());
    }

    SECTION("Enforce dimension boundaries on MAC key lengths to bounce bad sizes early")
    {
        std::vector<unsigned char> const payload(32, 0xAB);
        std::vector<unsigned char> const shortKey(8, 0x01); // Too small (min is 16)

        std::vector<unsigned char> const deadHash = JobHash::hashBuffer(payload, crypto_generichash_BYTES, shortKey.data(), shortKey.size());
        REQUIRE(deadHash.empty());
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobHash streaming performance evaluation matrices", "[job_crypto][hash][benchmark]")
{
    std::string const benchPath = "bench_hashing_throughput.bin";
    // 5MB target block to evaluate streaming continuous chunk loop speeds
    TransientTestFile testFile(benchPath, 5 * 1024 * 1024, 0xEF);

    std::vector<unsigned char> const shortPayload(4096, 0xCC);

    BENCHMARK("BLAKE2b Pure In-Memory Buffer Digest (4KB Frame)") {
        return JobHash::hashBuffer(shortPayload);
    };

    BENCHMARK("BLAKE2b Continuous Streaming File Processing (5MB Data Footprint)") {
        return JobHash::hashFile(benchPath);
    };
}
#endif