#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_crypto_utils.h>
#include <job_sha.h>
#include <job_sha224.h>
#include <job_sha256.h>
#include <job_sha384.h>
#include <job_sha512.h>
#include <job_sha3.h>
#include <job_sha_blake2b.h>
#include <job_sha_generic.h>
#include <job_sha_types.h>

using namespace job::crypto;

TEST_CASE("Job SHA usage and examples", "[job_crypto][sha]")
{
    /////////////////////////////////////////
    // Usage / examples
    /////////////////////////////////////////

    SECTION("SHA type string conversion")
    {
        REQUIRE(toString(JobShaType::Sha224) == "SHA-224");
        REQUIRE(toString(JobShaType::Sha256) == "SHA-256");
        REQUIRE(toString(JobShaType::Sha384) == "SHA-384");
        REQUIRE(toString(JobShaType::Sha512) == "SHA-512");

        REQUIRE(toString(JobShaType::Sha3_224) == "SHA3-224");
        REQUIRE(toString(JobShaType::Sha3_256) == "SHA3-256");
        REQUIRE(toString(JobShaType::Sha3_384) == "SHA3-384");
        REQUIRE(toString(JobShaType::Sha3_512) == "SHA3-512");

        REQUIRE(toString(JobShaType::ShaGeneric) == "GENERIC");
        REQUIRE(toString(JobShaType::ShaBlake2b) == "BLAKE2B");
        REQUIRE(toString(JobShaType::ShaHmac256) == "HMAC-SHA256");
        REQUIRE(toString(JobShaType::Unknown).empty());

        REQUIRE(toShaType("SHA-224") == JobShaType::Sha224);
        REQUIRE(toShaType("SHA-256") == JobShaType::Sha256);
        REQUIRE(toShaType("SHA-384") == JobShaType::Sha384);
        REQUIRE(toShaType("SHA-512") == JobShaType::Sha512);

        REQUIRE(toShaType("SHA3-224") == JobShaType::Sha3_224);
        REQUIRE(toShaType("SHA3-256") == JobShaType::Sha3_256);
        REQUIRE(toShaType("SHA3-384") == JobShaType::Sha3_384);
        REQUIRE(toShaType("SHA3-512") == JobShaType::Sha3_512);

        REQUIRE(toShaType("GENERIC") == JobShaType::ShaGeneric);
        REQUIRE(toShaType("BLAKE2B") == JobShaType::ShaBlake2b);
        REQUIRE(toShaType("HMAC-SHA256") == JobShaType::ShaHmac256);
        REQUIRE(toShaType("not-a-hash") == JobShaType::Unknown);
        REQUIRE(toShaType(nullptr) == JobShaType::Unknown);
    }

    SECTION("SHA-224 known vector")
    {
        constexpr std::string_view data = "abc";
        constexpr std::string_view expected =
            "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7";

        JobSha224::Hash const hash = JobSha224::compute(data);

        REQUIRE(hash.size() == JobSha224::kHashSize);
        REQUIRE(JobSha224::computeHex(data) == expected);
        REQUIRE(utils::toHex(hash) == expected);
    }

    SECTION("SHA-256 known vector")
    {
        constexpr std::string_view data = "abc";
        constexpr std::string_view expected =
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad";

        JobSha256::Hash const hash = JobSha256::compute(data);

        REQUIRE(hash.size() == JobSha256::kHashSize);
        REQUIRE(JobSha256::computeHex(data) == expected);
        REQUIRE(utils::toHex(hash) == expected);
    }

    SECTION("SHA-384 known vector")
    {
        constexpr std::string_view data = "abc";
        constexpr std::string_view expected =
            "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded163"
            "1a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7";

        JobSha384::Hash const hash = JobSha384::compute(data);

        REQUIRE(hash.size() == JobSha384::kHashSize);
        REQUIRE(JobSha384::computeHex(data) == expected);
        REQUIRE(utils::toHex(hash) == expected);
    }

    SECTION("SHA-512 known vector")
    {
        constexpr std::string_view data = "abc";
        constexpr std::string_view expected =
            "ddaf35a193617abacc417349ae204131"
            "12e6fa4e89a97ea20a9eeee64b55d39a"
            "2192992a274fc1a836ba3c23a3feebbd"
            "454d4423643ce80e2a9ac94fa54ca49f";

        JobSha512::Hash const hash = JobSha512::compute(data);

        REQUIRE(hash.size() == JobSha512::kHashSize);
        REQUIRE(JobSha512::computeHex(data) == expected);
        REQUIRE(utils::toHex(hash) == expected);
    }

    SECTION("SHA3-224 known vector")
    {
        constexpr std::string_view expected =
            "e642824c3f8cf24ad09234ee7d3c766fc9a3a5168d0c94ad73b46fdf";

        REQUIRE(JobSha3::computeHex(JobShaType::Sha3_224, "abc") == expected);
        REQUIRE(JobSha3::hashSize(JobShaType::Sha3_224) == 28);
        REQUIRE(JobSha3::supports(JobShaType::Sha3_224));
    }

    SECTION("SHA3-256 known vector")
    {
        constexpr std::string_view expected =
            "3a985da74fe225b2045c172d6bd390bd"
            "855f086e3e9d525b46bfe24511431532";

        REQUIRE(JobSha3::computeHex(JobShaType::Sha3_256, "abc") == expected);
        REQUIRE(JobSha3::hashSize(JobShaType::Sha3_256) == 32);
    }

    SECTION("SHA3-384 known vector")
    {
        constexpr std::string_view expected =
            "ec01498288516fc926459f58e2c6ad8df9b473cb0fc08c25"
            "96da7cf0e49be4b298d88cea927ac7f539f1edf228376d25";

        REQUIRE(JobSha3::computeHex(JobShaType::Sha3_384, "abc") == expected);
        REQUIRE(JobSha3::hashSize(JobShaType::Sha3_384) == 48);
    }

    SECTION("SHA3-512 known vector")
    {
        constexpr std::string_view expected =
            "b751850b1a57168a5693cd924b6b096e"
            "08f621827444f70d884f5d0240d2712"
            "e10e116e9192af3c91a7ec57647e3934"
            "057340b4cf408d5a56592f8274eec53f0";

        REQUIRE(JobSha3::computeHex(JobShaType::Sha3_512, "abc") == expected);
        REQUIRE(JobSha3::hashSize(JobShaType::Sha3_512) == 64);
    }

    SECTION("BLAKE2b known vectors")
    {
        constexpr std::string_view data = "abc";

        constexpr std::string_view expected32 =
            "bddd813c634239723171ef3fee98579b"
            "94964e3bb1cb3e427262c8c068d52319";

        constexpr std::string_view expected64 =
            "ba80a53f981c4d0d6a2797b69f12f6e9"
            "4c212f14685ac4b74b12bb6fdbffa2d1"
            "7d87c5392aab792dc252d5de4533cc951"
            "8d38aa8dbf1925ab92386edd4009923";

        REQUIRE(JobShaBlake2b::computeHex(data, 32) == expected32);
        REQUIRE(JobShaBlake2b::computeHex(data, 64) == expected64);
    }

    SECTION("Generic hash matches explicit BLAKE2b for equivalent output size")
    {
        constexpr std::string_view data = "abc";

        auto const generic = JobShaGeneric::compute(data);
        auto const blake2b = JobShaBlake2b::compute(data);

        REQUIRE(generic == blake2b);
        REQUIRE(JobShaGeneric::computeHex(data) == JobShaBlake2b::computeHex(data));
    }

    SECTION("JobSha dispatcher")
    {
        constexpr std::string_view data = "abc";

        REQUIRE(JobSha::computeHex(JobShaType::Sha224, data) == JobSha224::computeHex(data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha256, data) == JobSha256::computeHex(data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha384, data) == JobSha384::computeHex(data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha512, data) == JobSha512::computeHex(data));

        REQUIRE(JobSha::computeHex(JobShaType::Sha3_224, data) == JobSha3::computeHex(JobShaType::Sha3_224, data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha3_256, data) == JobSha3::computeHex(JobShaType::Sha3_256, data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha3_384, data) == JobSha3::computeHex(JobShaType::Sha3_384, data));
        REQUIRE(JobSha::computeHex(JobShaType::Sha3_512, data) == JobSha3::computeHex(JobShaType::Sha3_512, data));

        REQUIRE(JobSha::computeHex(JobShaType::ShaGeneric, data) == JobShaGeneric::computeHex(data));
        REQUIRE(JobSha::computeHex(JobShaType::ShaBlake2b, data) == JobShaBlake2b::computeHex(data));
    }

    SECTION("JobSha type properties")
    {
        REQUIRE(JobSha::supports(JobShaType::Sha224));
        REQUIRE(JobSha::supports(JobShaType::Sha256));
        REQUIRE(JobSha::supports(JobShaType::Sha384));
        REQUIRE(JobSha::supports(JobShaType::Sha512));
        REQUIRE(JobSha::supports(JobShaType::Sha3_224));
        REQUIRE(JobSha::supports(JobShaType::Sha3_256));
        REQUIRE(JobSha::supports(JobShaType::Sha3_384));
        REQUIRE(JobSha::supports(JobShaType::Sha3_512));
        REQUIRE(JobSha::supports(JobShaType::ShaGeneric));
        REQUIRE(JobSha::supports(JobShaType::ShaBlake2b));

        REQUIRE_FALSE(JobSha::supports(JobShaType::ShaHmac256));
        REQUIRE_FALSE(JobSha::supports(JobShaType::Unknown));

        REQUIRE(JobSha::requiresKey(JobShaType::ShaHmac256));
        REQUIRE_FALSE(JobSha::requiresKey(JobShaType::Sha256));

        REQUIRE(JobSha::variableSize(JobShaType::ShaGeneric));
        REQUIRE(JobSha::variableSize(JobShaType::ShaBlake2b));
        REQUIRE_FALSE(JobSha::variableSize(JobShaType::Sha256));

        REQUIRE(JobSha::hashSize(JobShaType::Sha224) == 28);
        REQUIRE(JobSha::hashSize(JobShaType::Sha256) == 32);
        REQUIRE(JobSha::hashSize(JobShaType::Sha384) == 48);
        REQUIRE(JobSha::hashSize(JobShaType::Sha512) == 64);

        REQUIRE(JobSha::hashSize(JobShaType::Sha3_224) == 28);
        REQUIRE(JobSha::hashSize(JobShaType::Sha3_256) == 32);
        REQUIRE(JobSha::hashSize(JobShaType::Sha3_384) == 48);
        REQUIRE(JobSha::hashSize(JobShaType::Sha3_512) == 64);

        REQUIRE(JobSha::hashSize(JobShaType::ShaGeneric) == 32);
        REQUIRE(JobSha::hashSize(JobShaType::ShaBlake2b) == 32);
        REQUIRE(JobSha::hashSize(JobShaType::ShaHmac256) == 32);
        REQUIRE(JobSha::hashSize(JobShaType::Unknown) == 0);
    }

    SECTION("HEX utility round trip")
    {
        std::vector<unsigned char> const source{
            0x00, 0x01, 0x02, 0x7f, 0x80, 0xfe, 0xff
        };

        std::string const hex = utils::toHex(source);

        REQUIRE(hex == "0001027f80feff");

        std::vector<unsigned char> decoded;
        REQUIRE(utils::hexToBin(decoded, hex));
        REQUIRE(decoded == source);
    }

    SECTION("HEX utility accepts literal string and string_view")
    {
        std::string const source{"JOB"};
        std::string_view const view{source};

        REQUIRE(utils::toHex("JOB") == "4a4f42");
        REQUIRE(utils::toHex(source) == "4a4f42");
        REQUIRE(utils::toHex(view) == "4a4f42");

        std::string decoded;

        REQUIRE(utils::hexToBin(decoded, "4a4f42"));
        REQUIRE(decoded == "JOB");

        REQUIRE(utils::hexToBin(decoded, std::string{"4a4f42"}));
        REQUIRE(decoded == "JOB");

        REQUIRE(utils::hexToBin(decoded, std::string_view{"4a4f42"}));
        REQUIRE(decoded == "JOB");
    }
}

TEST_CASE("Job SHA edge cases and failure behavior", "[job_crypto][sha]")
{
    /////////////////////////////////////////
    // Edge cases / failure behavior
    /////////////////////////////////////////

    SECTION("Empty input is hashable")
    {
        REQUIRE_FALSE(JobSha224::computeHex(std::string_view{}).empty());
        REQUIRE_FALSE(JobSha256::computeHex(std::string_view{}).empty());
        REQUIRE_FALSE(JobSha384::computeHex(std::string_view{}).empty());
        REQUIRE_FALSE(JobSha512::computeHex(std::string_view{}).empty());

        REQUIRE_FALSE(JobSha3::computeHex(JobShaType::Sha3_256, std::string_view{}).empty());
        REQUIRE_FALSE(JobShaGeneric::computeHex(std::string_view{}).empty());
        REQUIRE_FALSE(JobShaBlake2b::computeHex(std::string_view{}).empty());
    }

    SECTION("Null pointer with non-zero size is rejected")
    {
        REQUIRE(JobSha224::compute(nullptr, 1) == JobSha224::Hash{});
        REQUIRE(JobSha256::compute(nullptr, 1) == JobSha256::Hash{});
        REQUIRE(JobSha384::compute(nullptr, 1) == JobSha384::Hash{});
        REQUIRE(JobSha512::compute(nullptr, 1) == JobSha512::Hash{});

        REQUIRE(JobSha3::compute(JobShaType::Sha3_256, nullptr, 1).empty());
        REQUIRE(JobShaGeneric::compute(nullptr, 1).empty());
        REQUIRE(JobShaBlake2b::compute(nullptr, 1).empty());

        REQUIRE(JobSha::compute(JobShaType::Sha256, nullptr, 1).empty());
    }

    SECTION("SHA3 rejects non-SHA3 types")
    {
        REQUIRE_FALSE(JobSha3::supports(JobShaType::Sha224));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::Sha256));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::Sha384));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::Sha512));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::ShaGeneric));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::ShaBlake2b));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::ShaHmac256));
        REQUIRE_FALSE(JobSha3::supports(JobShaType::Unknown));

        REQUIRE(JobSha3::compute(JobShaType::Sha256, "abc").empty());
        REQUIRE(JobSha3::compute(JobShaType::ShaGeneric, "abc").empty());
    }

    SECTION("JobSha rejects keyed HMAC through the unkeyed interface")
    {
        REQUIRE(JobSha::requiresKey(JobShaType::ShaHmac256));
        REQUIRE(JobSha::compute(JobShaType::ShaHmac256, "abc").empty());
        REQUIRE(JobSha::computeHex(JobShaType::ShaHmac256, "abc").empty());
    }

    SECTION("JobSha rejects unknown type")
    {
        REQUIRE(JobSha::compute(JobShaType::Unknown, "abc").empty());
        REQUIRE(JobSha::computeHex(JobShaType::Unknown, "abc").empty());
    }

    SECTION("BLAKE2b rejects invalid hash sizes")
    {
        constexpr std::string_view data = "abc";

        REQUIRE(JobShaBlake2b::compute(data, JobShaBlake2b::kMinHashSize - 1).empty());
        REQUIRE(JobShaBlake2b::compute(data, JobShaBlake2b::kMaxHashSize + 1).empty());

        REQUIRE(JobShaBlake2b::computeHex(data, JobShaBlake2b::kMinHashSize - 1).empty());
        REQUIRE(JobShaBlake2b::computeHex(data, JobShaBlake2b::kMaxHashSize + 1).empty());
    }

    SECTION("Generic hash rejects invalid hash sizes")
    {
        constexpr std::string_view data = "abc";

        REQUIRE(JobShaGeneric::compute(data, JobShaGeneric::kMinHashSize - 1).empty());
        REQUIRE(JobShaGeneric::compute(data, JobShaGeneric::kMaxHashSize + 1).empty());

        REQUIRE(JobShaGeneric::computeHex(data, JobShaGeneric::kMinHashSize - 1).empty());
        REQUIRE(JobShaGeneric::computeHex(data, JobShaGeneric::kMaxHashSize + 1).empty());
    }

    SECTION("Different inputs produce different hashes")
    {
        REQUIRE(JobSha224::computeHex("abc") != JobSha224::computeHex("abd"));
        REQUIRE(JobSha256::computeHex("abc") != JobSha256::computeHex("abd"));
        REQUIRE(JobSha384::computeHex("abc") != JobSha384::computeHex("abd"));
        REQUIRE(JobSha512::computeHex("abc") != JobSha512::computeHex("abd"));

        REQUIRE(JobSha3::computeHex(JobShaType::Sha3_256, "abc") !=
                JobSha3::computeHex(JobShaType::Sha3_256, "abd"));
    }

    SECTION("Hash computation is deterministic")
    {
        constexpr std::string_view data = "Joseph's Odd Builder";

        REQUIRE(JobSha224::compute(data) == JobSha224::compute(data));
        REQUIRE(JobSha256::compute(data) == JobSha256::compute(data));
        REQUIRE(JobSha384::compute(data) == JobSha384::compute(data));
        REQUIRE(JobSha512::compute(data) == JobSha512::compute(data));

        REQUIRE(JobSha3::compute(JobShaType::Sha3_256, data) ==
                JobSha3::compute(JobShaType::Sha3_256, data));

        REQUIRE(JobShaGeneric::compute(data) == JobShaGeneric::compute(data));
        REQUIRE(JobShaBlake2b::compute(data) == JobShaBlake2b::compute(data));
    }

    SECTION("Invalid HEX input is rejected")
    {
        std::vector<unsigned char> out{0xaa, 0xbb};

        REQUIRE_FALSE(utils::hexToBin(out, "this-is-not-hex"));
        REQUIRE(out.empty());
    }

    SECTION("Null HEX input is rejected")
    {
        std::vector<unsigned char> out{0xaa, 0xbb};

        REQUIRE_FALSE(utils::hexToBin(out, static_cast<const char *>(nullptr)));
        REQUIRE(out.empty());
    }

    SECTION("Empty HEX input produces empty output")
    {
        std::vector<unsigned char> out{0xaa};

        REQUIRE(utils::hexToBin(out, std::string_view{}));
        REQUIRE(out.empty());
    }
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Job SHA benchmarks and stress", "[job_crypto][sha][benchmark]")
{
    /////////////////////////////////////////
    // Benchmarks / stress
    /////////////////////////////////////////

    std::string const smallData = "Joseph's Odd Builder SHA benchmark";
    std::vector<unsigned char> const largeData(1024 * 1024, 0x5a);

    BENCHMARK("SHA-224 small message")
    {
        return JobSha224::compute(smallData);
    };

    BENCHMARK("SHA-256 small message")
    {
        return JobSha256::compute(smallData);
    };

    BENCHMARK("SHA-384 small message")
    {
        return JobSha384::compute(smallData);
    };

    BENCHMARK("SHA-512 small message")
    {
        return JobSha512::compute(smallData);
    };

    BENCHMARK("SHA3-256 small message")
    {
        return JobSha3::compute(JobShaType::Sha3_256, smallData);
    };

    BENCHMARK("BLAKE2b small message")
    {
        return JobShaBlake2b::compute(smallData);
    };

    BENCHMARK("Generic hash small message")
    {
        return JobShaGeneric::compute(smallData);
    };

    BENCHMARK("JobSha dispatcher SHA-256 small message")
    {
        return JobSha::compute(JobShaType::Sha256, smallData);
    };

    BENCHMARK("SHA-224 1 MiB")
    {
        return JobSha224::compute(largeData.data(), largeData.size());
    };

    BENCHMARK("SHA-256 1 MiB")
    {
        return JobSha256::compute(largeData.data(), largeData.size());
    };

    BENCHMARK("SHA-384 1 MiB")
    {
        return JobSha384::compute(largeData.data(), largeData.size());
    };

    BENCHMARK("SHA-512 1 MiB")
    {
        return JobSha512::compute(largeData.data(), largeData.size());
    };

    BENCHMARK("SHA3-256 1 MiB")
    {
        return JobSha3::compute(JobShaType::Sha3_256, largeData.data(), largeData.size());
    };

    BENCHMARK("BLAKE2b 1 MiB")
    {
        return JobShaBlake2b::compute(largeData.data(), largeData.size());
    };

    BENCHMARK("Generic hash 1 MiB")
    {
        return JobShaGeneric::compute(largeData.data(), largeData.size());
    };

    SECTION("Repeated deterministic stress")
    {
        constexpr std::size_t kIterations = 10000;
        std::string const expected = JobSha256::computeHex(smallData);

        for (std::size_t i = 0; i < kIterations; ++i)
            REQUIRE(JobSha256::computeHex(smallData) == expected);
    }
}

#endif