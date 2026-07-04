#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <fstream>
#include <memory>
#include <string>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_crypto_keys.h>
#include <job_crypto_sign.h>
#include <job_secure_mem.h>

#include "transient_test_file.h"

using namespace job::crypto;

TEST_CASE("JobCryptoSign streaming detached file signatures", "[job_crypto][sign][example]")
{
    std::string const targetFile = "test_payload.bin";
    TransientTestFile testFile(targetFile, 128 * 1024, 0x41);

    JobCryptoSign::Ptr signer = std::make_shared<JobCryptoSign>();
    REQUIRE(signer->createKeys(JobCryptoKeys::KeyType::Sign));

    std::string signatureB64;

    SECTION("Compute detached streaming file signature verification matrix")
    {
        REQUIRE(signer->signFile(targetFile, signatureB64));
        REQUIRE_FALSE(signatureB64.empty());

        bool const isValidSig = signer->verifyFile(targetFile, signatureB64);
        REQUIRE(isValidSig == true);
    }
}

TEST_CASE("JobCryptoSign tracking signatures via internal stream associations", "[job_crypto][sign][example]")
{
    std::string const targetFile = "test_associated_payload.bin";
    TransientTestFile testFile(targetFile, 5, 'J');

    JobCryptoSign signer;
    REQUIRE(signer.createKeys(JobCryptoKeys::KeyType::Sign));

    // Highway to the danger zone Goose ... alignment fix to match class alias
    JobCryptoSign::File_Ptr fileStream = std::make_shared<std::ifstream>();
    signer.setFile(fileStream);

    std::string signatureB64;
    REQUIRE_FALSE(signer.signAssociatedFile(signatureB64));
}

// BLOCK TWO
TEST_CASE("JobCryptoSign error boundary validation rules", "[job_crypto][sign][edge]")
{
    JobCryptoSign signer;

    SECTION("Reject signature execution runs if identity keys are missing")
    {
        std::string signatureB64;
        std::string const missingFile = "imaginary_file.dat";

        REQUIRE_FALSE(signer.signFile(missingFile, signatureB64));
    }

    SECTION("Processing true empty zero-length data streams")
    {
        REQUIRE(signer.createKeys(JobCryptoKeys::KeyType::Sign));

        std::string const emptyFile = "empty.bin";
        TransientTestFile testFile(emptyFile, 0, 0x00);

        std::string emptySignature;
        REQUIRE(signer.signFile(emptyFile, emptySignature));
        REQUIRE_FALSE(emptySignature.empty());

        REQUIRE(signer.verifyFile(emptyFile, emptySignature) == true);
    }

    SECTION("Rejecting evaluation pipelines if signature data is corrupted")
    {
        REQUIRE(signer.createKeys(JobCryptoKeys::KeyType::Sign));

        std::string const payloadFile = "corrupt_test.bin";
        TransientTestFile testFile(payloadFile, 1024, 0x77);

        std::string signatureB64;
        REQUIRE(signer.signFile(payloadFile, signatureB64));

        // Overwrite the file contents behind the signature matrix row
        TransientTestFile testFile2(payloadFile, 1024, 0x99);
        REQUIRE_FALSE(signer.verifyFile(payloadFile, signatureB64));
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobCryptoSign streaming file operational performance profiles", "[job_crypto][sign][benchmark]")
{
    std::string const benchFile = "bench_streaming.bin";
    TransientTestFile testFile(benchFile, 2 * 1024 * 1024, 0xAA);

    JobCryptoSign signer;
    REQUIRE(signer.createKeys(JobCryptoKeys::KeyType::Sign));

    std::string runningSignature;
    REQUIRE(signer.signFile(benchFile, runningSignature));

    BENCHMARK("Streaming File Core Signature Computation Pass (2MB Payload File)") {
        std::string outSig;
        return signer.signFile(benchFile, outSig);
    };

    BENCHMARK("Streaming File Core Signature Verification Pass (2MB Payload File)") {
        return signer.verifyFile(benchFile, runningSignature);
    };
}
#endif