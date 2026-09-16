#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include "utils/utf8_decoder.h"

using namespace job::ansi::utils;

TEST_CASE("Utf8Decoder Static Encoding", "[utils][utf8][encode]")
{
    SECTION("ASCII (1 byte)") {
        std::string s = Utf8Decoder::encode(U'A');
        CHECK(s.size() == 1);
        CHECK(s[0] == 'A');
    }

    SECTION("2-byte Sequence (Latin-1 Supplement)") {
        // U+00A9 Copyright Symbol (©) -> C2 A9
        std::string s = Utf8Decoder::encode(0x00A9);
        CHECK(s.size() == 2);
        CHECK(static_cast<unsigned char>(s[0]) == 0xC2);
        CHECK(static_cast<unsigned char>(s[1]) == 0xA9);
    }

    SECTION("3-byte Sequence (Euro Sign)") {
        // U+20AC Euro (€) -> E2 82 AC
        std::string s = Utf8Decoder::encode(0x20AC);
        CHECK(s.size() == 3);
        CHECK(static_cast<unsigned char>(s[0]) == 0xE2);
        CHECK(static_cast<unsigned char>(s[1]) == 0x82);
        CHECK(static_cast<unsigned char>(s[2]) == 0xAC);
    }

    SECTION("4-byte Sequence (Emoji)") {
        // U+1F600 Grinning Face (😀) -> F0 9F 98 80
        std::string s = Utf8Decoder::encode(0x1F600);
        CHECK(s.size() == 4);
        CHECK(static_cast<unsigned char>(s[0]) == 0xF0);
        CHECK(static_cast<unsigned char>(s[1]) == 0x9F);
        CHECK(static_cast<unsigned char>(s[2]) == 0x98);
        CHECK(static_cast<unsigned char>(s[3]) == 0x80);
    }
}

TEST_CASE("Utf8Decoder Static Decoding", "[utils][utf8][decode]")
{
    SECTION("Valid single characters") {
        CHECK(Utf8Decoder::decodeUtf8("A") == U'A');
        CHECK(Utf8Decoder::decodeUtf8("\xC2\xA9") == 0x00A9); // ©
        CHECK(Utf8Decoder::decodeUtf8("\xE2\x82\xAC") == 0x20AC); // €
        CHECK(Utf8Decoder::decodeUtf8("\xF0\x9F\x98\x80") == 0x1F600); // 😀
    }

    SECTION("Decode Stream (Vector)") {
        std::string input = "A\xE2\x82\xAC" "B"; // A € B
        auto res = Utf8Decoder::decodeUtf8Stream(input);

        REQUIRE(res.size() == 3);
        CHECK(res[0] == U'A');
        CHECK(res[1] == 0x20AC);
        CHECK(res[2] == U'B');
    }
}

TEST_CASE("Utf8Decoder Incremental Parsing", "[utils][utf8][incremental]")
{
    Utf8Decoder decoder;

    SECTION("Single byte chars are immediate") {
        decoder.appendByte('A');
        REQUIRE(decoder.hasChar());
        CHECK(decoder.takeChar() == U'A');

        // Should be reset
        CHECK_FALSE(decoder.hasChar());
    }

    SECTION("Multi-byte characters fed incrementally") {
        // Feeding Euro (€) E2 82 AC byte by byte

        decoder.appendByte(static_cast<char>(0xE2));
        CHECK_FALSE(decoder.hasChar()); // Incomplete

        decoder.appendByte(static_cast<char>(0x82));
        CHECK_FALSE(decoder.hasChar()); // Still incomplete

        decoder.appendByte(static_cast<char>(0xAC));
        REQUIRE(decoder.hasChar());     // Complete!

        CHECK(decoder.takeChar() == 0x20AC);
    }

    SECTION("Reset clears buffer") {
        decoder.appendByte(static_cast<char>(0xE2)); // Start of 3-byte
        decoder.reset();
        decoder.appendByte('A'); // Now sending ASCII

        REQUIRE(decoder.hasChar());
        CHECK(decoder.takeChar() == U'A');
    }
}


TEST_CASE("Utf8Decoder Error Handling", "[utils][utf8][error]")
{
    SECTION("Invalid Start Byte returns Replacement") {
        // 0xFF is never valid in UTF-8
        auto res = Utf8Decoder::decodeUtf8("\xFF");
        CHECK(res == U'\uFFFD');
    }

    SECTION("Truncated Sequence returns Replacement") {
        // Euro starts with E2, but we only give that
        auto res = Utf8Decoder::decodeUtf8("\xE2");
        CHECK(res == U'\uFFFD');
    }

    SECTION("Overlong Encodings are rejected") {
        // ASCII 'A' (0x41) encoded as 2 bytes: C1 81
        // This is a security risk and must be rejected.
        auto res = Utf8Decoder::decodeUtf8("\xC1\x81");
        CHECK(res == U'\uFFFD');
    }

    SECTION("Surrogates are rejected") {
        // U+D800 (High Surrogate) is invalid in UTF-8
        // Encoded as: ED A0 80
        auto res = Utf8Decoder::decodeUtf8("\xED\xA0\x80");
        CHECK(res == U'\uFFFD');
    }

    SECTION("Incremental: Invalid byte resets gracefully") {
        Utf8Decoder decoder;
        // Start a 3-byte sequence
        decoder.appendByte(static_cast<char>(0xE2));

        decoder.appendByte(' ');
        decoder.appendByte(' ');

        // Now buffer size == 3, expected == 3. hasChar() is true.
        REQUIRE(decoder.hasChar());
        // Validation should fail
        CHECK(decoder.takeChar() == U'\uFFFD');
    }
}

TEST_CASE("Utf8Decoder Normalization", "[utils][utf8][norm]")
{
    SECTION("Replaces bad bytes with Replacement Char") {
        // Input: "A" + 0xFF + "B"
        std::vector<uint8_t> input = { 'A', 0xFF, 'B' };
        std::vector<uint8_t> output(32);

        auto res = Utf8Decoder::normalizeUtf8To(input, output);

        // Expected: 'A' (1) + EF BF BD (3) + 'B' (1) = 5 bytes
        REQUIRE(res.bytesWritten == 5);

        CHECK(output[0] == 'A');
        // Replacement char EF BF BD
        CHECK(output[1] == 0xEF);
        CHECK(output[2] == 0xBF);
        CHECK(output[3] == 0xBD);
        CHECK(output[4] == 'B');
    }

    SECTION("Handles Output Buffer Limit") {
        // Input: "ABC"
        std::vector<uint8_t> input = { 'A', 'B', 'C' };
        std::vector<uint8_t> output(2); // Only room for 2

        auto res = Utf8Decoder::normalizeUtf8To(input, output);

        CHECK(res.bytesRead == 2); // Should stop after B
        CHECK(res.bytesWritten == 2);
        CHECK(output[0] == 'A');
        CHECK(output[1] == 'B');
    }
}

TEST_CASE("Utf8Decoder Validation", "[utils][utf8][valid]")
{
    CHECK(Utf8Decoder::isValid("Hello World")); // ASCII
    CHECK(Utf8Decoder::isValid("H\xC3\xA9llo")); // Héllö (valid)
    CHECK_FALSE(Utf8Decoder::isValid("\xFF\xFF")); // Garbage
    CHECK_FALSE(Utf8Decoder::isValid("\xE2\x82")); // Truncated Euro
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Utf8Decoder Benchmarks", "[utils][utf8][benchmark]")
{
    char32_t emoji = 0x1F600; // 😀
    std::string emojiStr = "\xF0\x9F\x98\x80";

    BENCHMARK("Static Encode (4 bytes)") {
        return Utf8Decoder::encode(emoji);
    };

    BENCHMARK("Static Decode (4 bytes)") {
        return Utf8Decoder::decodeUtf8(emojiStr);
    };

    BENCHMARK("Decode Stream (Mixed)") {
        return Utf8Decoder::decodeUtf8Stream("Hello \xF0\x9F\x98\x80 World");
    };
}

#endif
