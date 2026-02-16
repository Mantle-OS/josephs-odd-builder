#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// job::threads
// #include <job_stealing_ctx.h>

// job::ai
// #include  "learner/learn_config.h"

#include "token/byte_token.h"
#include "token/ascii_token.h"
#include "char_token.h"

using namespace job::ai::token;

TEST_CASE("ByteToken: Simple Tests", "[token][byte][impl]")
{
    ByteToken token;

    SECTION("Standard Encode/Decode Cycle")
    {
        std::vector<uint8_t> raw = { 0xDE, 0xAD, 0xBE, 0xEF };
        std::vector<ByteLattice> lattices(4);
        std::vector<uint8_t> result(4);

        size_t written = token.encode(raw, lattices);
        REQUIRE(written == 4);

        size_t read = token.decode(lattices, result);
        REQUIRE(read == 4);


        REQUIRE(raw == result);
    }

    SECTION("Buffer Mismatches (Truncation Logic)")
    {
        std::vector<uint8_t> heavyLoad(100, 0xFF);
        std::vector<ByteLattice> tinyBuffer(10);

        size_t n = token.encode(heavyLoad, tinyBuffer);
        REQUIRE(n == 10);
        std::vector<ByteLattice> hugeBuffer(100);
        std::vector<uint8_t> tinyInput(10, 0xAA);

        n = token.encode(tinyInput, hugeBuffer);
        REQUIRE(n == 10);
    }

    SECTION("Evolution is a Lie")
    {
        uint64_t seed = 12345;
        REQUIRE_NOTHROW(token.mutate(seed));
    }

    SECTION("Works as IToken interface")
    {
        IToken::UPtr abstractToken = std::make_unique<ByteToken>();

        std::vector<uint8_t> in = { 1, 2, 3 };
        std::vector<ByteLattice> out(3);

        size_t count = abstractToken->encode(in, out);
        REQUIRE(count == 3);
    }
}


/////////////////////////////////////////////////////
// ASCII
/////////////////////////////////////////////////////


TEST_CASE("AsciiToken: The Bouncer", "[token][ascii][clamp]")
{
    AsciiToken token;

    // Usage / Happy Path
    SECTION("Standard Printable Characters Pass Through")
    {
        std::string text = "Hello ~";
        std::vector<uint8_t> input(text.begin(), text.end());
        std::vector<ByteLattice> encoded(input.size());
        std::vector<uint8_t> decoded(input.size());
        token.encode(input, encoded);
        token.decode(encoded, decoded);
        REQUIRE(input == decoded);
    }

    // Edge Cases / Clamping
    SECTION("The Clamping Logic (Encode Side)")
    {
        std::vector<uint8_t> anarchy = { 0, 10, 31, 127, 255 };
        std::vector<ByteLattice> order(anarchy.size());

        token.encode(anarchy, order);

        // 0 -> 32
        REQUIRE(ByteLattice::decode(order[0]) == 32);
        // 10 -> 32
        REQUIRE(ByteLattice::decode(order[1]) == 32);
        // 127 -> 126
        REQUIRE(ByteLattice::decode(order[3]) == 126);
        // 255 -> 126
        REQUIRE(ByteLattice::decode(order[4]) == 126);
    }

    SECTION("The Clamping Logic (Decode Side)")
    {
        std::vector<ByteLattice> illegalPayload = {
            ByteLattice::encode(0),
            ByteLattice::encode(200)
        };
        std::vector<uint8_t> sanitized(2);

        token.decode(illegalPayload, sanitized);

        REQUIRE(sanitized[0] == 32);  // Clamped up
        REQUIRE(sanitized[1] == 126); // Clamped down
    }

    SECTION("Ascii Constants")
    {
        REQUIRE(AsciiToken::kAsciiMin == 32);
        REQUIRE(AsciiToken::kAsciiMax == 126);
        REQUIRE(AsciiToken::kAsciiVocab == 95);
    }
}



/////////////////////////////////////////////////////
// Char
/////////////////////////////////////////////////////

TEST_CASE("CharToken: The Universal Translator", "[token][char][utf8]")
{
    CharToken token;

    SECTION("ASCII Passthrough || Does it work with standard English ?")
    {
        std::string text = "Hello Job!";
        std::vector<uint8_t> input(text.begin(), text.end());

        std::vector<ByteLattice> encoded(20);
        std::vector<uint8_t> decoded(20);

        size_t n = token.encode(input, encoded);
        REQUIRE(n == text.size());
        REQUIRE(ByteLattice::decode(encoded[0]) == 'H');

        size_t m = token.decode(std::span(encoded.data(), n), decoded);
        REQUIRE(m == text.size());

        std::string result(decoded.begin(), decoded.begin() + m);
        REQUIRE(result == text);
    }

    SECTION("Multi-byte UTF-8 Support || Does it speak Emoji?")
    {
        // "Rocket" 🚀 is 4 bytes: F0 9F 9A 80
        std::vector<uint8_t> rocket = { 0xF0, 0x9F, 0x9A, 0x80 };

        std::vector<ByteLattice> encoded(10);
        std::vector<uint8_t> decoded(10);

        size_t n = token.encode(rocket, encoded);
        REQUIRE(n == 4); // Should produce 4 lattices

        size_t m = token.decode(std::span(encoded.data(), n), decoded);
        REQUIRE(m == 4);

        REQUIRE(decoded[0] == 0xF0);
        REQUIRE(decoded[3] == 0x80);
    }

    SECTION("Large Payload Chunking || Testing the 256-byte stack buffer limit")
    {
        // We need > 256 bytes to force the `while` loop in CharToken to iterate.
        // Let's use 600 bytes.
        std::vector<uint8_t> hugeInput(600, 'A');
        std::vector<ByteLattice> hugeLattice(600);
        std::vector<uint8_t> hugeOutput(600);

        size_t n = token.encode(hugeInput, hugeLattice);
        REQUIRE(n == 600);

        size_t m = token.decode(hugeLattice, hugeOutput);
        REQUIRE(m == 600);

        REQUIRE(hugeOutput[0] == 'A');
        REQUIRE(hugeOutput[599] == 'A');
    }

    SECTION("Invalid UTF-8 Correction || Garbage In, Good Out")
    {
        // We simulate a "hallucinating" neural net or bad input.
        // 0xFF is invalid in UTF-8.
        // Input:  'A', 0xFF, 'B'
        // Expect: 'A', EF, BF, BD, 'B' (Replacement Char U+FFFD is 3 bytes)

        std::vector<uint8_t> garbage = { 'A', 0xFF, 'B' };
        std::vector<ByteLattice> encoded(8); // This might be a bug that using garbage.size


        size_t n = token.encode(garbage, encoded);

        // 'A' (1) + U+FFFD (3) + 'B' (1) = 5 atoms produced
        REQUIRE(n == 5);

        REQUIRE(ByteLattice::decode(encoded[0]) == 'A');

        // Verify the replacement char sequence
        REQUIRE(ByteLattice::decode(encoded[1]) == 0xEF);
        REQUIRE(ByteLattice::decode(encoded[2]) == 0xBF);
        REQUIRE(ByteLattice::decode(encoded[3]) == 0xBD);

        REQUIRE(ByteLattice::decode(encoded[4]) == 'B');
    }

    SECTION("Output Buffer Saturation")
    {
        // What happens if we run out of space mid-stream?
        std::vector<uint8_t> input(10, 'X');
        std::vector<ByteLattice> tinyBuf(5); // Only room for 5

        size_t n = token.encode(input, tinyBuf);
        REQUIRE(n == 5);
        REQUIRE(ByteLattice::decode(tinyBuf[4]) == 'X');
    }
}
