#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <algo/unigram.h>
#include <byte_fallback.h>
#include <vocab/vocab.h>

using job::token::ByteFallback;
using job::token::StructuralType;
using job::token::TokenId;
using job::token::TokenType;
using job::token::Unigram;
using job::token::Vocab;

//
// Block 1: usage / examples
//

TEST_CASE("Unigram reports Unigram algorithm type and borrowed vocabulary", "[token][unigram][usage]")
{
    Vocab vocab;
    Unigram unigram{&vocab};

    REQUIRE(unigram.type() == TokenType::Unigram);
    REQUIRE(unigram.vocab() == &vocab);
}

TEST_CASE("Unigram encodes a single exact vocabulary token", "[token][unigram][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("hello", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == hello);
}

TEST_CASE("Unigram encodes text from individual character tokens", "[token][unigram][usage]")
{
    Vocab vocab;
    const TokenId h = vocab.addToken("h", -1.0f);
    const TokenId e = vocab.addToken("e", -1.0f);
    const TokenId l = vocab.addToken("l", -1.0f);
    const TokenId o = vocab.addToken("o", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 16> output{};
    const std::size_t written = unigram.encode("hello", output);

    REQUIRE(written == 5);
    REQUIRE(output[0] == h);
    REQUIRE(output[1] == e);
    REQUIRE(output[2] == l);
    REQUIRE(output[3] == l);
    REQUIRE(output[4] == o);
}

TEST_CASE("Unigram chooses higher-scoring whole token over character pieces", "[token][unigram][score][usage]")
{
    Vocab vocab;
    vocab.addToken("a", -2.0f);
    vocab.addToken("b", -2.0f);
    const TokenId ab = vocab.addToken("ab", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == ab);
}

TEST_CASE("Unigram chooses character pieces when their combined score is higher", "[token][unigram][score][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a", -0.25f);
    const TokenId b = vocab.addToken("b", -0.25f);
    vocab.addToken("ab", -2.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("ab", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == a);
    REQUIRE(output[1] == b);
}

TEST_CASE("Unigram uses dynamic programming across competing segmentations", "[token][unigram][score][dp][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a", -2.0f);
    vocab.addToken("b", -2.0f);
    const TokenId c = vocab.addToken("c", -2.0f);
    const TokenId ab = vocab.addToken("ab", -0.25f);
    vocab.addToken("bc", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("abc", output);

    REQUIRE(written == 2);

    //
    // ab + c = -2.25
    // a + bc = -3.00
    // a + b + c = -6.00
    //
    REQUIRE(output[0] == ab);
    REQUIRE(output[1] == c);

    (void)a;
}

TEST_CASE("Unigram can choose one whole token across many possible prefixes", "[token][unigram][score][dp][usage]")
{
    Vocab vocab;
    vocab.addToken("t", -3.0f);
    vocab.addToken("th", -2.0f);
    vocab.addToken("the", -1.0f);
    vocab.addToken("ther", -0.75f);
    const TokenId there = vocab.addToken("there", -0.25f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("there", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == there);
}

TEST_CASE("Unigram chooses independently across repeated text", "[token][unigram][score][usage]")
{
    Vocab vocab;
    vocab.addToken("a", -2.0f);
    vocab.addToken("b", -2.0f);
    const TokenId ab = vocab.addToken("ab", -0.25f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("abab", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == ab);
    REQUIRE(output[1] == ab);
}

TEST_CASE("Unigram handles tokens sharing long prefixes", "[token][unigram][trie][usage]")
{
    Vocab vocab;
    vocab.addToken("a", -4.0f);
    vocab.addToken("ab", -3.0f);
    vocab.addToken("abc", -2.0f);
    const TokenId abcd = vocab.addToken("abcd", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("abcd", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == abcd);
}

TEST_CASE("Unigram score can prefer shorter token followed by longer token", "[token][unigram][score][dp][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a", -0.25f);
    const TokenId bc = vocab.addToken("bc", -0.25f);
    vocab.addToken("ab", -2.0f);
    vocab.addToken("c", -2.0f);
    vocab.addToken("abc", -4.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("abc", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == a);
    REQUIRE(output[1] == bc);
}

TEST_CASE("Unigram decodes ordinary vocabulary tokens", "[token][unigram][decode][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello", -1.0f);
    const TokenId space = vocab.addToken(" ", -1.0f);
    const TokenId world = vocab.addToken("world", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 3> tokens = {hello, space, world};
    std::array<char, 64> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 11);
    REQUIRE(std::string_view{output.data(), written} == "hello world");
}

TEST_CASE("Unigram decode translates byte tokens into raw bytes", "[token][unigram][decode][byte-fallback][usage]")
{
    Vocab vocab;
    const TokenId c3 = vocab.addToken("<0xC3>", -1.0f, StructuralType::Byte);
    const TokenId a9 = vocab.addToken("<0xA9>", -1.0f, StructuralType::Byte);

    Unigram unigram{&vocab};

    const std::array<TokenId, 2> tokens = {c3, a9};
    std::array<char, 8> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 2);
    REQUIRE(std::string_view{output.data(), written} == "\xC3\xA9");
}

TEST_CASE("Unigram span encode returns number of token IDs written", "[token][unigram][span][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a", -1.0f);
    const TokenId b = vocab.addToken("b", -1.0f);
    const TokenId c = vocab.addToken("c", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 3> output{};
    const std::size_t written = unigram.encode("abc", output);

    REQUIRE(written == 3);
    REQUIRE(output[0] == a);
    REQUIRE(output[1] == b);
    REQUIRE(output[2] == c);
}

TEST_CASE("Unigram span decode returns number of bytes written", "[token][unigram][span][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 1> tokens = {hello};
    std::array<char, 16> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

TEST_CASE("Unigram byte vocabulary round trips arbitrary UTF-8 bytes", "[token][unigram][roundtrip][byte-fallback][usage]")
{
    Vocab vocab;

    for (std::uint32_t byte = 0; byte <= 0xFF; ++byte) {
        const auto value = static_cast<std::uint8_t>(byte);
        vocab.addToken(ByteFallback::formatByte(value), -1.0f, StructuralType::Byte);
    }

    Unigram unigram{&vocab};

    const std::string original = "caf\xC3\xA9";
    std::array<TokenId, 64> encoded{};
    const std::size_t encodedCount = unigram.encode(original, encoded);

    REQUIRE(encodedCount == original.size());

    std::array<char, 64> decoded{};
    const std::size_t decodedCount = unigram.decode(std::span<const TokenId>{encoded.data(), encodedCount}, decoded);

    REQUIRE(std::string_view{decoded.data(), decodedCount} == original);
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("Unigram empty input encodes to no tokens", "[token][unigram][edge]")
{
    Vocab vocab;
    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};

    REQUIRE(unigram.encode("", output) == 0);
}

TEST_CASE("Unigram empty output span writes nothing", "[token][unigram][edge]")
{
    Vocab vocab;
    vocab.addToken("a", -1.0f);

    Unigram unigram{&vocab};
    std::span<TokenId> output;

    REQUIRE(unigram.encode("a", output) == 0);
}

TEST_CASE("Unigram bounded output span truncates safely", "[token][unigram][edge][span]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a", -1.0f);
    vocab.addToken("b", -1.0f);
    vocab.addToken("c", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 1> output{};
    const std::size_t written = unigram.encode("abc", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == a);
}

TEST_CASE("Unigram empty token span decodes to empty output", "[token][unigram][decode][edge]")
{
    Vocab vocab;
    Unigram unigram{&vocab};

    const std::span<const TokenId> tokens;
    std::array<char, 8> output{};

    REQUIRE(unigram.decode(tokens, output) == 0);
}

TEST_CASE("Unigram decode with empty output span writes nothing", "[token][unigram][decode][edge]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("hello", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 1> tokens = {token};
    std::span<char> output;

    REQUIRE(unigram.decode(tokens, output) == 0);
}

TEST_CASE("Unigram decode respects caller output capacity", "[token][unigram][decode][edge][span]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("abcdef", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 1> tokens = {token};
    std::array<char, 3> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 3);
    REQUIRE(std::string_view{output.data(), written} == "abc");
}

TEST_CASE("Unigram decode ignores invalid and unknown token IDs", "[token][unigram][decode][edge]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 3> tokens = {static_cast<TokenId>(-1), hello, static_cast<TokenId>(9999)};
    std::array<char, 32> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

TEST_CASE("Unigram encode with null vocabulary writes nothing", "[token][unigram][edge][vocab]")
{
    Unigram unigram{nullptr};

    std::array<TokenId, 8> output{};

    REQUIRE(unigram.encode("hello", output) == 0);
}

TEST_CASE("Unigram decode with null vocabulary writes nothing", "[token][unigram][decode][edge][vocab]")
{
    Unigram unigram{nullptr};

    const std::array<TokenId, 1> tokens = {0};
    std::array<char, 16> output{};

    REQUIRE(unigram.decode(tokens, output) == 0);
}

TEST_CASE("Unigram can change borrowed vocabulary", "[token][unigram][edge][vocab]")
{
    Vocab first;
    Vocab second;

    const TokenId firstA = first.addToken("a", -1.0f);
    second.addToken("x", -1.0f);
    const TokenId secondA = second.addToken("a", -1.0f);

    Unigram unigram{&first};

    {
        std::array<TokenId, 4> output{};
        const std::size_t written = unigram.encode("a", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == firstA);
    }

    unigram.setVocab(&second);

    REQUIRE(unigram.vocab() == &second);

    {
        std::array<TokenId, 4> output{};
        const std::size_t written = unigram.encode("a", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == secondA);
    }
}

TEST_CASE("Unigram segmentation does not depend on token insertion order", "[token][unigram][score][edge]")
{
    Vocab first;
    Vocab second;

    first.addToken("a", -2.0f);
    first.addToken("b", -2.0f);
    const TokenId firstAb = first.addToken("ab", -0.25f);

    const TokenId secondAb = second.addToken("ab", -0.25f);
    second.addToken("b", -2.0f);
    second.addToken("a", -2.0f);

    Unigram firstUnigram{&first};
    Unigram secondUnigram{&second};

    std::array<TokenId, 4> firstOutput{};
    std::array<TokenId, 4> secondOutput{};

    const std::size_t firstWritten = firstUnigram.encode("ab", firstOutput);
    const std::size_t secondWritten = secondUnigram.encode("ab", secondOutput);

    REQUIRE(firstWritten == 1);
    REQUIRE(secondWritten == 1);
    REQUIRE(firstOutput[0] == firstAb);
    REQUIRE(secondOutput[0] == secondAb);
}

TEST_CASE("Unigram considers complete token after shorter valid prefix", "[token][unigram][trie][edge]")
{
    Vocab vocab;
    vocab.addToken("a", -5.0f);
    vocab.addToken("ab", -4.0f);
    vocab.addToken("abc", -3.0f);
    vocab.addToken("abcd", -2.0f);
    const TokenId abcde = vocab.addToken("abcde", -1.0f);

    Unigram unigram{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = unigram.encode("abcde", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == abcde);
}

TEST_CASE("Unigram handles binary zero byte token during decode", "[token][unigram][decode][byte-fallback][edge]")
{
    Vocab vocab;
    const TokenId zero = vocab.addToken("<0x00>", -1.0f, StructuralType::Byte);
    const TokenId a = vocab.addToken("A", -1.0f);

    Unigram unigram{&vocab};

    const std::array<TokenId, 2> tokens = {zero, a};
    std::array<char, 8> output{};
    const std::size_t written = unigram.decode(tokens, output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == '\0');
    REQUIRE(output[1] == 'A');
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark Unigram repeated-text encoding", "[token][unigram][benchmark]")
{
    Vocab vocab;
    vocab.addToken("a", -4.0f);
    vocab.addToken("b", -4.0f);
    vocab.addToken("ab", -2.0f);
    vocab.addToken("aba", -1.5f);
    vocab.addToken("abab", -0.5f);

    Unigram unigram{&vocab};

    std::string text;
    for (std::size_t i = 0; i < 1024; ++i)
        text += "abab";

    std::vector<TokenId> output(text.size());
    BENCHMARK("Encode repeated Unigram text") {
        return unigram.encode(text, output);
    };
}

TEST_CASE("Benchmark Unigram token decoding", "[token][unigram][benchmark]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("token", -1.0f);

    Unigram unigram{&vocab};

    std::vector<TokenId> tokens(1024, token);
    std::vector<char> output(tokens.size() * 5);

    BENCHMARK("Decode repeated Unigram tokens") {
        return unigram.decode(tokens, output);
    };
}

#endif