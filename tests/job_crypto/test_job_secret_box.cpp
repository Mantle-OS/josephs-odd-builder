#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstring>
#include <string>
#include <vector>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <sodium.h>

#include "job_secret_box.h"
#include "job_secure_mem.h"

using namespace job::crypto;

TEST_CASE("JobSecretBox symmetric encryption and decryption lifecycles", "[job_crypto][secretbox][example]")
{
    std::string const internalMessage = "Secret operational pipeline payload vectors.";
    std::vector<unsigned char> const plainText(internalMessage.begin(), internalMessage.end());

    // Generate valid 256-bit key inside kernel-locked memory rows
    JobSecureMem secretKey(crypto_secretbox_KEYBYTES);
    std::memset(secretKey.data(), 0x55, secretKey.size());

    std::vector<unsigned char> cipherText;
    std::vector<unsigned char> nonce;

    SECTION("Successful transformation step pass through basic easy envelope")
    {
        REQUIRE(JobSecretBox::encrypt(plainText, secretKey, cipherText, nonce));
        REQUIRE_FALSE(cipherText.empty());
        REQUIRE(nonce.size() == crypto_secretbox_NONCEBYTES);

        // Enforce exact MAC authentication overhead length boundaries
        REQUIRE(cipherText.size() == plainText.size() + crypto_secretbox_MACBYTES);

        JobSecureMem decryptedText;
        REQUIRE(JobSecretBox::decrypt(cipherText, secretKey, nonce, decryptedText));
        REQUIRE(decryptedText.size() == plainText.size());

        // Extract and verify structural message integrity
        std::string const recoveredMessage(reinterpret_cast<const char*>(decryptedText.data()), decryptedText.size());
        REQUIRE(recoveredMessage == internalMessage);
    }
}


// BLOCK TWO
TEST_CASE("JobSecretBox decryption mitigation and error checking limits", "[job_crypto][secretbox][edge]")
{
    std::string const internalMessage = "Top Secret Core Frame.";
    std::vector<unsigned char> const plainText(internalMessage.begin(), internalMessage.end());

    JobSecureMem secretKey(crypto_secretbox_KEYBYTES);
    std::memset(secretKey.data(), 0xAA, secretKey.size());

    std::vector<unsigned char> cipherText;
    std::vector<unsigned char> nonce;
    REQUIRE(JobSecretBox::encrypt(plainText, secretKey, cipherText, nonce));

    SECTION("Reject decryption processing loops if cipher bytes are corrupted")
    {
        // Tamper with a single byte inside the encrypted buffer array space
        cipherText[crypto_secretbox_MACBYTES + 1] ^= 0xFF;

        JobSecureMem deadOutput;
        // Authenticated encryption MUST reject altered bits immediately
        bool const result = JobSecretBox::decrypt(cipherText, secretKey, nonce, deadOutput);
        REQUIRE_FALSE(result);
        REQUIRE(deadOutput.empty());
    }

    SECTION("Enforce exact size validations across key bounds matrices")
    {
        JobSecureMem brokenKey(16); // Malformed size boundary
        std::vector<unsigned char> freshCipher;
        std::vector<unsigned char> freshNonce;

        REQUIRE_FALSE(JobSecretBox::encrypt(plainText, brokenKey, freshCipher, freshNonce));
    }

    SECTION("Reject validation tracking runs if nonce parameters are truncated")
    {
        std::vector<unsigned char> const brokenNonce(12, 0x00); // Invalid dimensions
        JobSecureMem deadOutput;

        REQUIRE_FALSE(JobSecretBox::decrypt(cipherText, secretKey, brokenNonce, deadOutput));
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobSecretBox operational performance profiles", "[job_crypto][secretbox][benchmark]")
{
    // 1MB standard transit memory matrix chunk size allocation
    std::vector<unsigned char> const stressPayload(1 * 1024 * 1024, 0x33);

    JobSecureMem benchmarkKey(crypto_secretbox_KEYBYTES);
    std::memset(benchmarkKey.data(), 0x11, benchmarkKey.size());

    std::vector<unsigned char> runningCipher;
    std::vector<unsigned char> runningNonce;
    REQUIRE(JobSecretBox::encrypt(stressPayload, benchmarkKey, runningCipher, runningNonce));

    JobSecureMem transientOutput;

    BENCHMARK("Symmetric Block Authenticated Encryption (1MB Continuous Payload)") {
        std::vector<unsigned char> cipher;
        std::vector<unsigned char> nonce;
        return JobSecretBox::encrypt(stressPayload, benchmarkKey, cipher, nonce);
    };

    BENCHMARK("Symmetric Block Authenticated Decryption (1MB Continuous Payload)") {
        return JobSecretBox::decrypt(runningCipher, benchmarkKey, runningNonce, transientOutput);
    };
}
#endif