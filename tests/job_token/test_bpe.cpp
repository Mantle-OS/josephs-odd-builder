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

#include <bpe.h>
#include <byte_fallback.h>
#include <vocab.h>

using job::token::Bpe;
using job::token::ByteFallback;
using job::token::kInvalidToken;
using job::token::StructuralType;
using job::token::TokenId;
using job::token::TokenType;
using job::token::Vocab;

//
// Block 1: usage / examples
//

TEST_CASE("Bpe reports BPE algorithm type and borrowed vocabulary", "[token][bpe][usage]")
{
    Vocab vocab;
    Bpe bpe{&vocab};

    REQUIRE(bpe.type() == TokenType::BPE);
    REQUIRE(bpe.vocab() == &vocab);
}

TEST_CASE("Bpe encodes literal byte tokens without merge rules", "[token][bpe][usage]")
{
    Vocab vocab;
    const TokenId h = vocab.addToken("h");
    const TokenId e = vocab.addToken("e");
    const TokenId l = vocab.addToken("l");
    const TokenId o = vocab.addToken("o");

    Bpe bpe{&vocab};

    std::array<TokenId, 16> output{};
    const std::size_t written = bpe.encode("hello", output);

    REQUIRE(written == 5);
    REQUIRE(output[0] == h);
    REQUIRE(output[1] == e);
    REQUIRE(output[2] == l);
    REQUIRE(output[3] == l);
    REQUIRE(output[4] == o);
}

TEST_CASE("Bpe falls back to byte token strings when literal byte token is missing", "[token][bpe][byte-fallback][usage]")
{
    Vocab vocab;
    const TokenId h = vocab.addToken("<0x68>", 0.0f, StructuralType::Byte);
    const TokenId i = vocab.addToken("<0x69>", 0.0f, StructuralType::Byte);

    Bpe bpe{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("hi", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == h);
    REQUIRE(output[1] == i);
}

TEST_CASE("Bpe prefers literal byte token over byte fallback token", "[token][bpe][byte-fallback][usage]")
{
    Vocab vocab;
    const TokenId literal = vocab.addToken("A");
    vocab.addToken("<0x41>", 0.0f, StructuralType::Byte);

    Bpe bpe{&vocab};

    std::array<TokenId, 4> output{};
    const std::size_t written = bpe.encode("A", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == literal);
}

TEST_CASE("Bpe applies a simple merge rule", "[token][bpe][merge][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId ab = vocab.addToken("ab");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == ab);
}

TEST_CASE("Bpe applies chained merge rules", "[token][bpe][merge][usage]")
{
    Vocab vocab;
    const TokenId t = vocab.addToken("t");
    const TokenId h = vocab.addToken("h");
    const TokenId e = vocab.addToken("e");
    const TokenId th = vocab.addToken("th");
    const TokenId the = vocab.addToken("the");

    Bpe bpe{&vocab};
    bpe.addMergeRule(t, h, th);
    bpe.addMergeRule(th, e, the);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("the", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == the);
}

TEST_CASE("Bpe merge rank determines which available pair merges first", "[token][bpe][merge][rank][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId c = vocab.addToken("c");
    const TokenId ab = vocab.addToken("ab");
    const TokenId bc = vocab.addToken("bc");
    const TokenId abc = vocab.addToken("abc");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);
    bpe.addMergeRule(b, c, bc);
    bpe.addMergeRule(ab, c, abc);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("abc", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == abc);
}

TEST_CASE("Bpe can merge repeated adjacent pairs", "[token][bpe][merge][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId ab = vocab.addToken("ab");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("abab", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == ab);
    REQUIRE(output[1] == ab);
}

TEST_CASE("Bpe setMergeRules replaces merge configuration", "[token][bpe][merge][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId c = vocab.addToken("c");
    const TokenId ab = vocab.addToken("ab");
    const TokenId bc = vocab.addToken("bc");

    Bpe bpe{&vocab};
    bpe.setMergeRules({Bpe::MergeRule{a, b, ab}});

    REQUIRE(bpe.rules().size() == 1);

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = bpe.encode("ab", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == ab);
    }

    bpe.setMergeRules({Bpe::MergeRule{b, c, bc}});

    REQUIRE(bpe.rules().size() == 1);

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = bpe.encode("abc", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == a);
        REQUIRE(output[1] == bc);
    }
}

TEST_CASE("Bpe clearRules restores direct byte-token encoding", "[token][bpe][merge][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId ab = vocab.addToken("ab");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = bpe.encode("ab", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == ab);
    }

    bpe.clearRules();

    REQUIRE(bpe.rules().empty());

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = bpe.encode("ab", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == a);
        REQUIRE(output[1] == b);
    }
}

TEST_CASE("Bpe decodes ordinary vocabulary tokens", "[token][bpe][decode][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");
    const TokenId space = vocab.addToken(" ");
    const TokenId world = vocab.addToken("world");

    Bpe bpe{&vocab};

    const std::array<TokenId, 3> tokens = {hello, space, world};
    std::array<char, 64> output{};
    const std::size_t written = bpe.decode(tokens, output);

    REQUIRE(written == 11);
    REQUIRE(std::string_view{output.data(), written} == "hello world");
}

TEST_CASE("Bpe decode translates byte fallback tokens back into raw bytes", "[token][bpe][decode][byte-fallback][usage]")
{
    Vocab vocab;
    const TokenId c3 = vocab.addToken("<0xC3>", 0.0f, StructuralType::Byte);
    const TokenId a9 = vocab.addToken("<0xA9>", 0.0f, StructuralType::Byte);

    Bpe bpe{&vocab};

    const std::array<TokenId, 2> tokens = {c3, a9};
    std::array<char, 8> output{};
    const std::size_t written = bpe.decode(tokens, output);

    REQUIRE(written == 2);
    REQUIRE(std::string_view{output.data(), written} == "\xC3\xA9");
}

TEST_CASE("Bpe byte fallback encode decode round trips UTF-8 bytes", "[token][bpe][roundtrip][usage]")
{
    Vocab vocab;

    for (std::uint32_t byte = 0; byte <= 0xFF; ++byte) {
        const auto value = static_cast<std::uint8_t>(byte);
        vocab.addToken(ByteFallback::formatByte(value), 0.0f, StructuralType::Byte);
    }

    Bpe bpe{&vocab};

    const std::string original = "caf\xC3\xA9";
    std::array<TokenId, 64> encoded{};
    const std::size_t encodedCount = bpe.encode(original, encoded);

    REQUIRE(encodedCount == original.size());

    std::array<char, 64> decoded{};
    const std::size_t decodedCount = bpe.decode(std::span<const TokenId>{encoded.data(), encodedCount}, decoded);

    REQUIRE(std::string_view{decoded.data(), decodedCount} == original);
}

TEST_CASE("Bpe span encode returns number of token IDs written", "[token][bpe][span][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId c = vocab.addToken("c");

    Bpe bpe{&vocab};

    std::array<TokenId, 3> output{kInvalidToken, kInvalidToken, kInvalidToken};
    const std::size_t written = bpe.encode("abc", output);

    REQUIRE(written == 3);
    REQUIRE(output[0] == a);
    REQUIRE(output[1] == b);
    REQUIRE(output[2] == c);
}

TEST_CASE("Bpe span decode returns number of bytes written", "[token][bpe][span][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Bpe bpe{&vocab};

    const std::array<TokenId, 1> tokens = {hello};
    std::array<char, 16> output{};
    const std::size_t written = bpe.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("Bpe empty input encodes to no tokens", "[token][bpe][edge]")
{
    Vocab vocab;
    Bpe bpe{&vocab};

    std::array<TokenId, 8> output{};

    REQUIRE(bpe.encode("", output) == 0);
}

TEST_CASE("Bpe empty output span writes nothing", "[token][bpe][edge]")
{
    Vocab vocab;
    vocab.addToken("a");

    Bpe bpe{&vocab};
    std::span<TokenId> output;

    REQUIRE(bpe.encode("a", output) == 0);
}

TEST_CASE("Bpe bounded output span truncates encoded token stream safely", "[token][bpe][edge][span]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    vocab.addToken("b");

    Bpe bpe{&vocab};

    std::array<TokenId, 1> output{kInvalidToken};
    const std::size_t written = bpe.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == a);
}

TEST_CASE("Bpe missing initial byte token produces invalid token ID", "[token][bpe][edge]")
{
    Vocab vocab;
    Bpe bpe{&vocab};

    std::array<TokenId, 4> output{};
    const std::size_t written = bpe.encode("x", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == kInvalidToken);
}

TEST_CASE("Bpe merge rule can operate on non-byte-range token IDs", "[token][bpe][merge][edge]")
{
    Vocab vocab;

    for (std::size_t i = 0; i < 300; ++i)
        vocab.addToken("padding-" + std::to_string(i));

    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId ab = vocab.addToken("ab");

    REQUIRE(a >= 256);
    REQUIRE(b >= 256);

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == ab);
}

TEST_CASE("Bpe duplicate merge pair uses newest rule entry", "[token][bpe][merge][edge]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId first = vocab.addToken("first");
    const TokenId second = vocab.addToken("second");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, first);
    bpe.addMergeRule(a, b, second);

    std::array<TokenId, 8> output{};
    const std::size_t written = bpe.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == second);
}

TEST_CASE("Bpe decode ignores invalid and unknown token IDs", "[token][bpe][decode][edge]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Bpe bpe{&vocab};

    const std::array<TokenId, 4> tokens = {kInvalidToken, hello, 9999, kInvalidToken};
    std::array<char, 32> output{};
    const std::size_t written = bpe.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

TEST_CASE("Bpe empty token span decodes to empty output", "[token][bpe][decode][edge]")
{
    Vocab vocab;
    Bpe bpe{&vocab};

    const std::span<const TokenId> tokens;
    std::array<char, 8> output{};

    REQUIRE(bpe.decode(tokens, output) == 0);
}

TEST_CASE("Bpe decode with null vocabulary writes nothing", "[token][bpe][decode][edge]")
{
    Bpe bpe{nullptr};

    const std::array<TokenId, 1> tokens = {0};
    std::array<char, 16> output{};

    REQUIRE(bpe.decode(tokens, output) == 0);
}

TEST_CASE("Bpe decode respects caller output capacity", "[token][bpe][decode][edge][span]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("abcdef");

    Bpe bpe{&vocab};

    const std::array<TokenId, 1> tokens = {token};
    std::array<char, 3> output{};
    const std::size_t written = bpe.decode(tokens, output);

    REQUIRE(written == 3);
    REQUIRE(std::string_view{output.data(), written} == "abc");
}

TEST_CASE("Bpe can change borrowed vocabulary", "[token][bpe][edge][vocab]")
{
    Vocab first;
    Vocab second;

    const TokenId firstA = first.addToken("a");
    second.addToken("x");
    const TokenId secondA = second.addToken("a");

    Bpe bpe{&first};

    {
        std::array<TokenId, 4> output{};
        const std::size_t written = bpe.encode("a", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == firstA);
    }

    bpe.setVocab(&second);

    REQUIRE(bpe.vocab() == &second);

    {
        std::array<TokenId, 4> output{};
        const std::size_t written = bpe.encode("a", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == secondA);
    }
}

TEST_CASE("Bpe clearRules is safe when already empty", "[token][bpe][edge]")
{
    Vocab vocab;
    Bpe bpe{&vocab};

    REQUIRE(bpe.rules().empty());

    bpe.clearRules();

    REQUIRE(bpe.rules().empty());
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark Bpe repeated-text encoding", "[token][bpe][benchmark]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("b");
    const TokenId ab = vocab.addToken("ab");
    const TokenId abab = vocab.addToken("abab");

    Bpe bpe{&vocab};
    bpe.addMergeRule(a, b, ab);
    bpe.addMergeRule(ab, ab, abab);

    std::string text;
    for (std::size_t i = 0; i < 1024; ++i)
        text += "abab";

    std::vector<TokenId> output(text.size());

    BENCHMARK("Encode repeated BPE text") {
        return bpe.encode(text, output);
    };
}

TEST_CASE("Benchmark Bpe token decoding", "[token][bpe][benchmark]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("token");

    Bpe bpe{&vocab};

    std::vector<TokenId> tokens(1024, token);
    std::vector<char> output(tokens.size() * 5);

    BENCHMARK("Decode repeated BPE tokens") {
        return bpe.decode(tokens, output);
    };
}

#endif