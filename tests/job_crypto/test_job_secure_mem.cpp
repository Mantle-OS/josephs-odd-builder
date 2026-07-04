#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstring>
#include <string>
#include <vector>

#include <sodium.h>

#include "job_secure_mem.h"

using namespace job::crypto;

TEST_CASE("JobSecureMem basic allocation", "[job_crypto][secure_mem][alloc]")
{
    REQUIRE_NOTHROW(JobSecureMem(32));
    JobSecureMem mem(32);

    REQUIRE(mem.data() != nullptr);
    REQUIRE(mem.size() == 32);
    REQUIRE(mem.empty() == false);
}

TEST_CASE("JobSecureMem copyFrom and content integrity", "[job_crypto][secure_mem][copy]")
{
    JobSecureMem mem(16);
    const char *text = "SecureMemory!";
    mem.copyFrom(text, std::strlen(text));

    std::string b64 = mem.toBase64();
    REQUIRE(!b64.empty());

    // New SOT behavior: fromBase64 resizes the buffer to match exact decoded size (13 bytes)
    JobSecureMem decoded(16);
    REQUIRE(decoded.fromBase64(b64));
    REQUIRE(decoded.size() == std::strlen(text));
}

TEST_CASE("JobSecureMem Base64 round-trip", "[job_crypto][secure_mem][base64]")
{
    const char *input = "This is a test of JobSecureMem Base64!";
    JobSecureMem original(std::strlen(input));
    original.copyFrom(input, std::strlen(input));

    std::string encoded = original.toBase64();
    REQUIRE(!encoded.empty());

    JobSecureMem decoded(32);
    REQUIRE(decoded.fromBase64(encoded));
    REQUIRE(decoded.size() == std::strlen(input));

    std::string decodedString = decoded.fromBase64toString(encoded);
    REQUIRE(decodedString == input);
}

TEST_CASE("JobSecureMem fromBase64toString returns expected data", "[job_crypto][secure_mem][decode]")
{
    JobSecureMem mem(64);
    const char *msg = "hello world!";
    mem.copyFrom(msg, std::strlen(msg));

    std::string encoded = mem.toBase64();
    REQUIRE(!encoded.empty());

    std::string decoded = mem.fromBase64toString(encoded);
    REQUIRE(decoded == "hello world!");
}

TEST_CASE("JobSecureMem handles zero-length and null input safely", "[job_crypto][secure_mem][edge]")
{
    JobSecureMem zero(0);
    REQUIRE(zero.size() == 0);
    REQUIRE(zero.data() == nullptr);
    REQUIRE(zero.empty() == true);

    REQUIRE_NOTHROW(zero.toBase64());
    REQUIRE_NOTHROW(zero.fromBase64(""));
}

TEST_CASE("JobSecureMem prevents data aliasing", "[job_crypto][secure_mem][safety]")
{
    JobSecureMem first(8);
    const char *data = "12345678";
    first.copyFrom(data, 8);

    std::string b64 = first.toBase64();
    JobSecureMem second(8);
    second.fromBase64(b64);

    REQUIRE(first.toBase64() == second.toBase64());
    REQUIRE(first == second);
    REQUIRE(first.data() != second.data()); // distinct memory regions
}

TEST_CASE("JobSecureMem move and copy lifecycle operations", "[job_crypto][secure_mem][lifecycle]")
{
    SECTION("Copy constructor and assignment allocation duplication verification")
    {
        JobSecureMem source(6);
        source.copyFrom("SOURCE", 6);

        JobSecureMem copyBuilt(source);
        REQUIRE(copyBuilt == source);
        REQUIRE(copyBuilt.data() != source.data());

        JobSecureMem copyAssigned;
        copyAssigned = source;
        REQUIRE(copyAssigned == source);
        REQUIRE(copyAssigned.data() != source.data());
    }

    SECTION("Move constructor and assignment page-pointer handoff verification")
    {
        JobSecureMem source(4);
        source.copyFrom("MOVE", 4);
        const unsigned char* originalDataPtr = source.data();

        JobSecureMem movedTo(std::move(source));
        REQUIRE(movedTo.size() == 4);
        REQUIRE(movedTo.data() == originalDataPtr);
        REQUIRE(source.data() == nullptr);
        REQUIRE(source.size() == 0);
    }
}

TEST_CASE("JobSecureMem explicit buffer swapping", "[job_crypto][secure_mem][swap]")
{
    JobSecureMem a(3);
    a.copyFrom("AAA", 3);
    JobSecureMem b(3);
    b.copyFrom("BBB", 3);

    const unsigned char* ptrA = a.data();
    const unsigned char* ptrB = b.data();

    a.swap(b);

    REQUIRE(a.data() == ptrB);
    REQUIRE(b.data() == ptrA);
}



#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobSecureMem Core Performance Profile", "[job_crypto][secure_mem][bench]")
{
    // Pre-bake standard plaintext vectors to isolate allocation/conversion costs
    std::vector<uint8_t> const rawToken(32, 0xAA);
    std::vector<uint8_t> const rawPage(4096, 0xBB);

    // Generate valid base64 strings ahead of time for the decoding loops
    JobSecureMem b64Setup(1024);
    b64Setup.copyFrom(rawPage.data(), 1024);
    std::string const validB64Str = b64Setup.toBase64();

    BENCHMARK("Transient Token Lifecycle (32 Bytes: Alloc + Zero + Free)") {
        JobSecureMem mem(32);
        return mem.data(); // Prevent aggressive compiler elimination
    };

    BENCHMARK("Page Real Estate Lifecycle (4 KB: Alloc + Lock + Zero + Free)") {
        JobSecureMem mem(4096);
        return mem.data();
    };

    BENCHMARK("Memory Footprint Copy Throughput (4 KB)") {
        JobSecureMem mem(4096);
        mem.copyFrom(rawPage.data(), rawPage.size());
        return mem.size();
    };

    BENCHMARK("Transcoding: Binary to Base64 (4 KB Data Stream)") {
        JobSecureMem mem(4096);
        mem.copyFrom(rawPage.data(), rawPage.size());
        return mem.toBase64();
    };

    BENCHMARK("Transcoding: Base64 back to Secure Vector (1 KB Transcode)") {
        JobSecureMem mem;
        return mem.fromBase64(validB64Str);
    };

    BENCHMARK("Bulk Load Stress Pass (1 MB Allocation Lifecycle)") {
        JobSecureMem mem(1024 * 1024);
        return mem.data();
    };
}
#endif






