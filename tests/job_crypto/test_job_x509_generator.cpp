#include "../transient_test_file.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_secure_mem.h>
#include <job_ssl_options.h>
#include <job_x509_generator.h>

using namespace job::crypto;

namespace {

[[nodiscard]] std::string transientPath(const std::string &name)
{
    static std::atomic<uint64_t> counter{0};

    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();

    const std::string fileName =
        "job_x509_"
        + std::to_string(stamp)
        + "_"
        + std::to_string(counter.fetch_add(1, std::memory_order_relaxed))
        + "_"
        + name;

    return (std::filesystem::temp_directory_path() / fileName).string();
}

[[nodiscard]] std::vector<unsigned char> readFile(const std::string &path)
{
    std::ifstream stream(path, std::ios::binary);

    if (!stream)
        return {};

    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
        );
}

[[nodiscard]] std::string readTextFile(const std::string &path)
{
    std::ifstream stream(path, std::ios::binary);

    if (!stream)
        return {};

    return std::string(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
        );
}

[[nodiscard]] JobSecureMem securePassphrase(std::string value)
{
    JobSecureMem passphrase(value.size());

    if (!value.empty()) {
        passphrase.copyFrom(value.data(), value.size());
        sodium_memzero(value.data(), value.size());
    }

    return passphrase;
}

} // namespace

TEST_CASE("JobX509Generator generates usable certificate artifacts", "[job_crypto][x509]")
{
    SECTION("generates a default EC PEM certificate and private key")
    {
        TransientTestFile certificate(transientPath("certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);

        REQUIRE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));

        const std::string certificateData = readTextFile(certificate.path());
        const std::string privateKeyData = readTextFile(privateKey.path());

        REQUIRE_FALSE(certificateData.empty());
        REQUIRE_FALSE(privateKeyData.empty());

        REQUIRE(
            certificateData.find("-----BEGIN CERTIFICATE-----")
            != std::string::npos
            );

        REQUIRE(
            certificateData.find("-----END CERTIFICATE-----")
            != std::string::npos
            );

        REQUIRE(
            privateKeyData.find("-----BEGIN PRIVATE KEY-----")
            != std::string::npos
            );

        REQUIRE(
            privateKeyData.find("-----END PRIVATE KEY-----")
            != std::string::npos
            );
    }

    SECTION("generates an EC DER certificate and private key")
    {
        TransientTestFile certificate(transientPath("certificate.der"), 0, 0);
        TransientTestFile privateKey(transientPath("private_key.der"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::DER);
        options.setKeyType(JobSslOptions::KeyType::EC);
        options.setEcCurve(JobSslOptions::EcCurve::P384);
        options.setDigest(JobSslOptions::Digest::SHA384);

        REQUIRE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));

        const std::vector<unsigned char> certificateData =
            readFile(certificate.path());

        const std::vector<unsigned char> privateKeyData =
            readFile(privateKey.path());

        REQUIRE_FALSE(certificateData.empty());
        REQUIRE_FALSE(privateKeyData.empty());

        REQUIRE(certificateData.front() == 0x30);
        REQUIRE(privateKeyData.front() == 0x30);
    }

    SECTION("generates an RSA PEM certificate and private key")
    {
        TransientTestFile certificate(transientPath("rsa_certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("rsa_private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);
        options.setKeyType(JobSslOptions::KeyType::RSA);
        options.setRsaBits(2048);
        options.setDigest(JobSslOptions::Digest::SHA256);

        REQUIRE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));

        const std::string certificateData = readTextFile(certificate.path());
        const std::string privateKeyData = readTextFile(privateKey.path());

        REQUIRE(
            certificateData.find("-----BEGIN CERTIFICATE-----")
            != std::string::npos
            );

        REQUIRE(
            privateKeyData.find("-----BEGIN PRIVATE KEY-----")
            != std::string::npos
            );
    }

    SECTION("generates a password-protected PKCS12 identity")
    {
        TransientTestFile identity(transientPath("identity.p12"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PKCS12);
        options.setCommonName("localhost");
        options.setOrganization("JOB Tests");
        options.setCountry("US");

        JobSecureMem passphrase = securePassphrase("job-test-passphrase");

        REQUIRE(JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            ));

        const std::vector<unsigned char> identityData =
            readFile(identity.path());

        REQUIRE_FALSE(identityData.empty());
        REQUIRE(identityData.front() == 0x30);
    }

    SECTION("generates a PKCS12 identity with an empty passphrase")
    {
        TransientTestFile identity(transientPath("empty_passphrase.p12"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PKCS12);

        JobSecureMem passphrase;

        REQUIRE(JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            ));

        REQUIRE(std::filesystem::file_size(identity.path()) > 0);
    }

    SECTION("supports custom subject alternative names")
    {
        TransientTestFile certificate(transientPath("san_certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("san_private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);
        options.setCommonName("job.local");
        options.setDnsNames({
            "job.local",
            "api.job.local"
        });
        options.setIpAddresses({
            "127.0.0.1",
            "::1"
        });

        REQUIRE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));

        REQUIRE(std::filesystem::file_size(certificate.path()) > 0);
        REQUIRE(std::filesystem::file_size(privateKey.path()) > 0);
    }
}

TEST_CASE("JobX509Generator rejects invalid generation requests", "[job_crypto][x509]")
{
    SECTION("rejects empty certificate path")
    {
        TransientTestFile privateKey(transientPath("private_key.pem"), 0, 0);

        JobSslOptions options;
        const std::filesystem::path emptyPath;

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            emptyPath,
            privateKey.path()
            ));
    }

    SECTION("rejects empty private-key path")
    {
        TransientTestFile certificate(transientPath("certificate.pem"), 0, 0);

        JobSslOptions options;
        const std::filesystem::path emptyPath;

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            certificate.path(),
            emptyPath
            ));
    }

    SECTION("rejects the same path for certificate and private key")
    {
        TransientTestFile output(transientPath("same_output.pem"), 0, 0);

        JobSslOptions options;

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            output.path(),
            output.path()
            ));
    }

    SECTION("rejects PKCS12 encoding for separate outputs")
    {
        TransientTestFile certificate(transientPath("certificate.p12"), 0, 0);
        TransientTestFile privateKey(transientPath("private_key.p12"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PKCS12);

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));
    }

    SECTION("rejects PEM encoding for combined identity")
    {
        TransientTestFile identity(transientPath("identity.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);

        JobSecureMem passphrase;

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            ));
    }

    SECTION("rejects DER encoding for combined identity")
    {
        TransientTestFile identity(transientPath("identity.der"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::DER);

        JobSecureMem passphrase;

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            ));
    }

    SECTION("rejects an RSA key smaller than 2048 bits")
    {
        TransientTestFile certificate(transientPath("weak_certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("weak_private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setKeyType(JobSslOptions::KeyType::RSA);
        options.setRsaBits(1024);

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));
    }

    SECTION("rejects zero-day certificate validity")
    {
        TransientTestFile certificate(transientPath("expired_certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("expired_private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setValidDays(0);

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));
    }

    SECTION("rejects invalid IP subject alternative name")
    {
        TransientTestFile certificate(transientPath("invalid_ip_certificate.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("invalid_ip_private_key.pem"), 0, 0);

        JobSslOptions options;
        options.setIpAddresses({
            "127.0.0.1",
            "not-an-ip-address"
        });

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            ));
    }

    SECTION("rejects embedded null bytes in PKCS12 passphrase")
    {
        TransientTestFile identity(transientPath("invalid_passphrase.p12"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PKCS12);

        const std::array<unsigned char, 5> bytes{
            't',
            'e',
            '\0',
            's',
            't'
        };

        JobSecureMem passphrase(bytes.size());
        passphrase.copyFrom(bytes.data(), bytes.size());

        REQUIRE_FALSE(JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            ));
    }
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobX509Generator benchmarks", "[job_crypto][x509][benchmark]")
{
    BENCHMARK("Generate EC P-256 PEM certificate")
    {
        TransientTestFile certificate(transientPath("benchmark_ec_cert.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("benchmark_ec_key.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);
        options.setKeyType(JobSslOptions::KeyType::EC);
        options.setEcCurve(JobSslOptions::EcCurve::P256);
        options.setDigest(JobSslOptions::Digest::SHA256);

        return JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            );
    };

    BENCHMARK("Generate RSA-2048 PEM certificate")
    {
        TransientTestFile certificate(transientPath("benchmark_rsa_cert.pem"), 0, 0);
        TransientTestFile privateKey(transientPath("benchmark_rsa_key.pem"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PEM);
        options.setKeyType(JobSslOptions::KeyType::RSA);
        options.setRsaBits(2048);
        options.setDigest(JobSslOptions::Digest::SHA256);

        return JobX509Generator::generate(
            options,
            certificate.path(),
            privateKey.path()
            );
    };

    BENCHMARK("Generate EC P-256 PKCS12 identity")
    {
        TransientTestFile identity(transientPath("benchmark_identity.p12"), 0, 0);

        JobSslOptions options;
        options.setEncoding(JobSslOptions::Encoding::PKCS12);

        JobSecureMem passphrase = securePassphrase("benchmark-passphrase");

        return JobX509Generator::generate(
            options,
            identity.path(),
            passphrase
            );
    };
}

#endif