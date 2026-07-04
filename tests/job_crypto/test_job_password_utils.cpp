#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstring>
#include <string>
#include <vector>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <sodium.h>

#include "job_password_utils.h"
#include "job_secure_mem.h"

using namespace job::crypto;

namespace {

JobSecureMem makeSecurePassword(const std::string &passwordText)
{
    JobSecureMem password(passwordText.size());

    if (!passwordText.empty())
        password.copyFrom(passwordText.data(), passwordText.size());

    return password;
}

} // namespace

TEST_CASE("JobPasswordUtils storage hashing and authentication flows", "[job_crypto][pwhash][example]")
{
    JobSecureMem userPassword = makeSecurePassword("SuperSecurePassword123!");

    std::string storageHash;

    REQUIRE(JobPasswordUtils::hashPasswordForStorage(userPassword, storageHash));
    REQUIRE_FALSE(storageHash.empty());

    // Enforce Argon2id string prefix verification check boundaries
    REQUIRE(storageHash.find("$argon2id$") != std::string::npos);

    SECTION("Authenticate matching entry values against stored record")
    {
        bool const isVerified =
            JobPasswordUtils::verifyPasswordAgainstStorage(userPassword, storageHash);

        REQUIRE(isVerified == true);
    }

    SECTION("Reject altered or completely fraudulent password input streams")
    {
        JobSecureMem typoPassword = makeSecurePassword("supersecurepassword123!");

        bool const isVerified =
            JobPasswordUtils::verifyPasswordAgainstStorage(typoPassword, storageHash);

        REQUIRE_FALSE(isVerified);
    }
}

TEST_CASE("JobPasswordUtils deterministic key derivation vectors", "[job_crypto][pwhash][example]")
{
    // Highway to the danger zone Goose ... generating a symmetric key out of a passphrase
    JobSecureMem passBuffer(16);
    passBuffer.copyFrom("SystemPassphrase", 16);

    // Argon2id requires exactly 16 bytes of salt material
    std::vector<unsigned char> const staticSalt(crypto_pwhash_SALTBYTES, 0x42);

    JobSecureMem derivedKeyA;
    JobSecureMem derivedKeyB;

    REQUIRE(JobPasswordUtils::deriveKeyFromPassword(derivedKeyA, passBuffer, staticSalt));
    REQUIRE(JobPasswordUtils::deriveKeyFromPassword(derivedKeyB, passBuffer, staticSalt));

    // Confirm allocations meet standard 256-bit symmetric block requirements
    REQUIRE(derivedKeyA.size() == crypto_secretbox_KEYBYTES);

    // Deterministic pass: Same salt + same password MUST yield the exact same key matrix
    REQUIRE(derivedKeyA == derivedKeyB);
}

TEST_CASE("JobPasswordUtils boundary limits and corruption mitigation", "[job_crypto][pwhash][edge]")
{
    SECTION("Handling true empty blank password memory gracefully")
    {
        JobSecureMem emptyPassword;

        std::string emptyHash;

        REQUIRE_FALSE(JobPasswordUtils::hashPasswordForStorage(emptyPassword, emptyHash));
        REQUIRE(emptyHash.empty());
    }

    SECTION("Enforce dimension constraints on raw derivation salt matrices")
    {
        JobSecureMem password(8);
        std::vector<unsigned char> const truncatedSalt(8, 0xAA); // Invalid dimension row size
        JobSecureMem outputKey;

        // Late to the party? Bad salt sizes must bounce instantly instead of throwing faults
        bool const result =
            JobPasswordUtils::deriveKeyFromPassword(outputKey, password, truncatedSalt);

        REQUIRE_FALSE(result);
        REQUIRE(outputKey.empty());
    }

    SECTION("Gracefully handle verification parsing of random unformatted trash inputs")
    {
        std::string const arbitraryTrash = "NotAnArgon2idHashStringAtAll!!!";
        JobSecureMem userPassword = makeSecurePassword("password");

        // Verification must return failure safely rather than crashing on parsing errors
        REQUIRE_FALSE(
            JobPasswordUtils::verifyPasswordAgainstStorage(userPassword, arbitraryTrash)
            );
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobPasswordUtils Argon2id heavy compute performance metrics", "[job_crypto][pwhash][benchmark]")
{
    JobSecureMem benchmarkPassword = makeSecurePassword("PerformanceTestingPassphrase2026");

    std::string runningHash;
    REQUIRE(JobPasswordUtils::hashPasswordForStorage(benchmarkPassword, runningHash));

    std::vector<unsigned char> const fixedSalt(crypto_pwhash_SALTBYTES, 0x77);
    JobSecureMem derivedTarget;

    BENCHMARK("Argon2id Interactive Storage Hash String Generation Loop") {
        std::string outHash;
        return JobPasswordUtils::hashPasswordForStorage(benchmarkPassword, outHash);
    };

    BENCHMARK("Argon2id Interactive Storage Verification Matching Step") {
        return JobPasswordUtils::verifyPasswordAgainstStorage(benchmarkPassword, runningHash);
    };

    BENCHMARK("Argon2id Symmetric Token Derivation Computation Step") {
        return JobPasswordUtils::deriveKeyFromPassword(
            derivedTarget,
            benchmarkPassword,
            fixedSalt
            );
    };
}
#endif