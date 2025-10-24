#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <job_secure_mem.h>
#include <sodium.h>

using namespace job::crypto;

TEST_CASE("JobSecureMem basic allocation", "[job_crypto][secure_mem][alloc]")
{
    REQUIRE_NOTHROW(JobSecureMem(32));
    JobSecureMem mem(32);

    REQUIRE(mem.data() != nullptr);
    REQUIRE(mem.size() == 32);
}

TEST_CASE("JobSecureMem copyFrom and content integrity", "[job_crypto][secure_mem][copy]")
{
    JobSecureMem mem(16);
    const char *text = "SecureMemory!";
    mem.copyFrom(text, strlen(text));

    std::string b64 = mem.toBase64();
    REQUIRE(!b64.empty());

    // Decoding into another secure mem
    JobSecureMem decoded(16);
    REQUIRE(decoded.fromBase64(b64));
    REQUIRE(decoded.size() == strlen(text));
}

TEST_CASE("JobSecureMem Base64 round-trip", "[job_crypto][secure_mem][base64]")
{
    const char *input = "This is a test of JobSecureMem Base64!";
    JobSecureMem original(strlen(input));  // IMPORTANT allocate exactly enough
    original.copyFrom(input, strlen(input));

    std::string encoded = original.toBase64();
    REQUIRE(!encoded.empty());

    JobSecureMem decoded(32);
    REQUIRE(decoded.fromBase64(encoded));

    std::string decodedString = decoded.fromBase64toString(encoded);
    REQUIRE(decodedString.substr(0, 10) == "This is a ");
}

TEST_CASE("JobSecureMem fromBase64toString returns expected data", "[job_crypto][secure_mem][decode]")
{
    JobSecureMem mem(64);
    const char *msg = "hello world!";
    mem.copyFrom(msg, strlen(msg));

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

    // Should safely no-op
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
    REQUIRE(first.data() != second.data()); // distinct memory regions
}
