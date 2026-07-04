#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <string>
#include <vector>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include "job_crypto.h"
#include "job_secure_mem.h"
#include "job_crypto_utils.h"
#include "transient_test_file.h"

using namespace job::crypto;

TEST_CASE("JobCrypto facade orchestrates top-level configuration workflows cleanly", "[job_crypto][facade][example]")
{
    JobCrypto cryptoEngine;
    REQUIRE(cryptoEngine.isInitialized());

    SECTION("Symmetric configuration encryption and decryption lifecycle round-trip")
    {
        std::string const secrets = "{ \"db_pass\": \"super_secure_vault_2026\" }";
        std::vector<unsigned char> const plainText(secrets.begin(), secrets.end());

        JobSecureMem secretKey(32);
        std::vector<unsigned char> const rawKeyMaterial(32, 0xA5);
        secretKey.copyFrom(rawKeyMaterial.data(), rawKeyMaterial.size());

        std::vector<unsigned char> cipherText;
        std::vector<unsigned char> nonce;

        bool const encryptOk = JobCrypto::encryptConfig(plainText, secretKey, cipherText, nonce);
        REQUIRE(encryptOk);
        REQUIRE_FALSE(cipherText.empty());
        REQUIRE_FALSE(nonce.empty());

        JobSecureMem decryptedText;
        bool const decryptOk = JobCrypto::decryptConfig(cipherText, secretKey, nonce, decryptedText);
        REQUIRE(decryptOk);
        REQUIRE(decryptedText.size() == plainText.size());

        std::vector<unsigned char> const unpackedResult(
            decryptedText.data(),
            decryptedText.data() + decryptedText.size());

        REQUIRE(unpackedResult == plainText);
    }

    SECTION("Hex-encoded Blake2b file verification stream check")
    {
        std::string const targetFile = "facade_hash_test.dat";
        TransientTestFile testFile(targetFile, 128 * 1024, 0x77); // 128KB payload

        std::string const hexHash = cryptoEngine.computeFileBlake2bHex(targetFile);
        REQUIRE_FALSE(hexHash.empty());

        REQUIRE(hexHash.find_first_not_of("0123456789abcdef") == std::string::npos);
    }
}


// BLOCK TWO
TEST_CASE("JobCrypto boundary error states and vector signature verification checks", "[job_crypto][facade][edge]")
{
    JobCrypto cryptoEngine;

    // SECTION("Verify failure metrics on empty binary inputs or non-existent file loops")
    // {
    //     std::vector<unsigned char> const emptyKey;
    //     std::vector<unsigned char> const emptySig;

    //     bool const result = cryptoEngine.verifyFileSignature("ghost_file_missing.bin", emptyKey, emptySig);
    //     REQUIRE_FALSE(result);
    // }

    SECTION("Handle invalid raw parameter validation tracking safely inside configuration channels")
    {
        std::vector<unsigned char> const dummyPlain(16, 0xFF);
        std::vector<unsigned char> badCipher;
        std::vector<unsigned char> badNonce;

        JobSecureMem emptyKey;
        bool const encryptFail = JobCrypto::encryptConfig(dummyPlain, emptyKey, badCipher, badNonce);
        REQUIRE_FALSE(encryptFail);
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobCrypto full ecosystem integration timing passes", "[job_crypto][facade][benchmark]")
{
    JobCrypto cryptoEngine;

    std::string const benchFile = "facade_bench_stream.bin";
    TransientTestFile testFile(benchFile, 2 * 1024 * 1024, 0xAA); // 2MB benchmark vector

    BENCHMARK("Ecosystem Facade: Compute Hex Blake2b Stream (2MB Asset)") {
        return cryptoEngine.computeFileBlake2bHex(benchFile);
    };
}
#endif