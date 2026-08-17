#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <algo/wordpiece.h>
#include <vocab/vocab.h>

using job::token::TokenId;
using job::token::TokenType;
using job::token::Vocab;
using job::token::Wordpiece;

//
// Block 1: usage / examples
//

TEST_CASE("Wordpiece reports WordPiece algorithm type and borrowed vocabulary", "[token][wordpiece][usage]")
{
    Vocab vocab;
    Wordpiece wordpiece{&vocab};

    REQUIRE(wordpiece.type() == TokenType::WordPiece);
    REQUIRE(wordpiece.vocab() == &vocab);
}

TEST_CASE("Wordpiece encodes a whole root token", "[token][wordpiece][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("hello", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == hello);
}

TEST_CASE("Wordpiece uses continuation tokens after the first piece", "[token][wordpiece][usage][continuation]")
{
    Vocab vocab;
    const TokenId play = vocab.addToken("play");
    const TokenId ing = vocab.addToken("##ing");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("playing", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == play);
    REQUIRE(output[1] == ing);
}

TEST_CASE("Wordpiece chooses the longest root token", "[token][wordpiece][greedy][usage]")
{
    Vocab vocab;
    vocab.addToken("a");
    vocab.addToken("ap");
    vocab.addToken("app");
    const TokenId apple = vocab.addToken("apple");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("apple", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == apple);
}

TEST_CASE("Wordpiece chooses the longest continuation token", "[token][wordpiece][greedy][continuation][usage]")
{
    Vocab vocab;
    const TokenId un = vocab.addToken("un");
    vocab.addToken("##b");
    vocab.addToken("##bel");
    vocab.addToken("##believ");
    const TokenId believable = vocab.addToken("##believable");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("unbelievable", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == un);
    REQUIRE(output[1] == believable);
}

TEST_CASE("Wordpiece greedily segments multiple continuation pieces", "[token][wordpiece][greedy][continuation][usage]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("token");
    const TokenId iz = vocab.addToken("##iz");
    const TokenId ation = vocab.addToken("##ation");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("tokenization", output);

    REQUIRE(written == 3);
    REQUIRE(output[0] == token);
    REQUIRE(output[1] == iz);
    REQUIRE(output[2] == ation);
}

TEST_CASE("Wordpiece does not use continuation token as the first piece", "[token][wordpiece][continuation][usage]")
{
    Vocab vocab;
    vocab.addToken("##hello");
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("hello", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);
}

TEST_CASE("Wordpiece does not use root-only token as continuation piece", "[token][wordpiece][continuation][usage]")
{
    Vocab vocab;
    const TokenId walk = vocab.addToken("walk");
    vocab.addToken("ing");
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("walking", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);

    (void)walk;
}

TEST_CASE("Wordpiece prefers whole root token over decomposed pieces", "[token][wordpiece][greedy][usage]")
{
    Vocab vocab;
    vocab.addToken("play");
    vocab.addToken("##ing");
    const TokenId playing = vocab.addToken("playing");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("playing", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == playing);
}

TEST_CASE("Wordpiece handles repeated continuation pieces", "[token][wordpiece][continuation][usage]")
{
    Vocab vocab;
    const TokenId ha = vocab.addToken("ha");
    const TokenId continuationHa = vocab.addToken("##ha");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("hahaha", output);

    REQUIRE(written == 3);
    REQUIRE(output[0] == ha);
    REQUIRE(output[1] == continuationHa);
    REQUIRE(output[2] == continuationHa);
}

TEST_CASE("Wordpiece supports custom continuation prefix", "[token][wordpiece][continuation][custom][usage]")
{
    Vocab vocab;
    const TokenId walk = vocab.addToken("walk");
    const TokenId ing = vocab.addToken("@@ing");

    Wordpiece wordpiece{&vocab, "@@"};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("walking", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == walk);
    REQUIRE(output[1] == ing);
}

TEST_CASE("Wordpiece setContinuationPrefix rebuilds continuation lookup", "[token][wordpiece][continuation][custom][usage]")
{
    Vocab vocab;
    const TokenId walk = vocab.addToken("walk");
    vocab.addToken("##ing");
    const TokenId customIng = vocab.addToken("@@ing");

    Wordpiece wordpiece{&vocab};

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("walking", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == walk);
    }

    wordpiece.setPrefix("@@");

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("walking", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == walk);
        REQUIRE(output[1] == customIng);
    }
}

TEST_CASE("Wordpiece decodes ordinary root tokens", "[token][wordpiece][decode][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 1> tokens = {hello};
    std::array<char, 32> output{};
    const std::size_t written = wordpiece.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

TEST_CASE("Wordpiece decode removes continuation prefix", "[token][wordpiece][decode][continuation][usage]")
{
    Vocab vocab;
    const TokenId play = vocab.addToken("play");
    const TokenId ing = vocab.addToken("##ing");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 2> tokens = {play, ing};
    std::array<char, 32> output{};
    const std::size_t written = wordpiece.decode(tokens, output);

    REQUIRE(written == 7);
    REQUIRE(std::string_view{output.data(), written} == "playing");
}

TEST_CASE("Wordpiece encode decode round trips segmented word", "[token][wordpiece][roundtrip][usage]")
{
    Vocab vocab;
    vocab.addToken("token");
    vocab.addToken("##iz");
    vocab.addToken("##ation");

    Wordpiece wordpiece{&vocab};

    const std::string original = "tokenization";
    std::array<TokenId, 16> encoded{};
    const std::size_t encodedCount = wordpiece.encode(original, encoded);

    REQUIRE(encodedCount == 3);

    std::array<char, 32> decoded{};
    const std::size_t decodedCount = wordpiece.decode(std::span<const TokenId>{encoded.data(), encodedCount}, decoded);

    REQUIRE(std::string_view{decoded.data(), decodedCount} == original);
}

TEST_CASE("Wordpiece custom continuation prefix round trips", "[token][wordpiece][roundtrip][continuation][custom][usage]")
{
    Vocab vocab;
    vocab.addToken("walk");
    vocab.addToken("@@ing");

    Wordpiece wordpiece{&vocab, "@@"};

    std::array<TokenId, 8> encoded{};
    const std::size_t encodedCount = wordpiece.encode("walking", encoded);

    REQUIRE(encodedCount == 2);

    std::array<char, 32> decoded{};
    const std::size_t decodedCount = wordpiece.decode(std::span<const TokenId>{encoded.data(), encodedCount}, decoded);

    REQUIRE(std::string_view{decoded.data(), decodedCount} == "walking");
}

TEST_CASE("Wordpiece span encode returns number of token IDs written", "[token][wordpiece][span][usage]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");
    const TokenId b = vocab.addToken("##b");
    const TokenId c = vocab.addToken("##c");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 3> output{};
    const std::size_t written = wordpiece.encode("abc", output);

    REQUIRE(written == 3);
    REQUIRE(output[0] == a);
    REQUIRE(output[1] == b);
    REQUIRE(output[2] == c);
}

TEST_CASE("Wordpiece span decode returns number of bytes written", "[token][wordpiece][span][usage]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 1> tokens = {hello};
    std::array<char, 16> output{};
    const std::size_t written = wordpiece.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("Wordpiece empty input encodes to no tokens", "[token][wordpiece][edge]")
{
    Vocab vocab;
    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};

    REQUIRE(wordpiece.encode("", output) == 0);
}

TEST_CASE("Wordpiece empty output span writes nothing", "[token][wordpiece][edge]")
{
    Vocab vocab;
    vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};
    std::span<TokenId> output;

    REQUIRE(wordpiece.encode("hello", output) == 0);
}

TEST_CASE("Wordpiece unknown word emits configured unknown token", "[token][wordpiece][unk][edge]")
{
    Vocab vocab;
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("nightmare", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);
}

TEST_CASE("Wordpiece failed segmentation discards partial pieces and emits unknown token", "[token][wordpiece][unk][edge]")
{
    Vocab vocab;
    vocab.addToken("play");
    vocab.addToken("##ful");
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("playfully", output);

    //
    // WordPiece is all-or-nothing for the input word. It must not leave
    // "play" / "##ful" behind when the remaining suffix cannot be matched.
    //
    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);
}

TEST_CASE("Wordpiece enforces maximum input characters per word", "[token][wordpiece][unk][edge]")
{
    Vocab vocab;
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);
    vocab.addToken("abcd");

    Wordpiece wordpiece{&vocab, "##", 3};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("abcd", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);
}

TEST_CASE("Wordpiece accepts input exactly at maximum word size", "[token][wordpiece][edge]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("abcd");

    Wordpiece wordpiece{&vocab, "##", 4};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("abcd", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == token);
}

TEST_CASE("Wordpiece bounded output span rejects a word that does not fit", "[token][wordpiece][edge][span]")
{
    Vocab vocab;
    vocab.addToken("a");
    vocab.addToken("##b");
    vocab.addToken("##c");

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 1> output{};
    const std::size_t written = wordpiece.encode("abc", output);

    REQUIRE(written == 0);
}

TEST_CASE("Wordpiece empty token span decodes to empty output", "[token][wordpiece][decode][edge]")
{
    Vocab vocab;
    Wordpiece wordpiece{&vocab};

    const std::span<const TokenId> tokens;
    std::array<char, 8> output{};

    REQUIRE(wordpiece.decode(tokens, output) == 0);
}

TEST_CASE("Wordpiece decode with empty output span writes nothing", "[token][wordpiece][decode][edge]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 1> tokens = {token};
    std::span<char> output;

    REQUIRE(wordpiece.decode(tokens, output) == 0);
}

TEST_CASE("Wordpiece decode respects caller output capacity", "[token][wordpiece][decode][edge][span]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("abcdef");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 1> tokens = {token};
    std::array<char, 3> output{};
    const std::size_t written = wordpiece.decode(tokens, output);

    REQUIRE(written == 3);
    REQUIRE(std::string_view{output.data(), written} == "abc");
}

TEST_CASE("Wordpiece decode ignores invalid and unknown token IDs", "[token][wordpiece][decode][edge]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    Wordpiece wordpiece{&vocab};

    const std::array<TokenId, 3> tokens = {static_cast<TokenId>(-1), hello, static_cast<TokenId>(9999)};
    std::array<char, 32> output{};
    const std::size_t written = wordpiece.decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

TEST_CASE("Wordpiece encode with null vocabulary writes nothing", "[token][wordpiece][edge][vocab]")
{
    Wordpiece wordpiece{nullptr};

    std::array<TokenId, 8> output{};

    REQUIRE(wordpiece.encode("hello", output) == 0);
}

TEST_CASE("Wordpiece decode with null vocabulary writes nothing", "[token][wordpiece][decode][edge][vocab]")
{
    Wordpiece wordpiece{nullptr};

    const std::array<TokenId, 1> tokens = {0};
    std::array<char, 16> output{};

    REQUIRE(wordpiece.decode(tokens, output) == 0);
}

TEST_CASE("Wordpiece can change borrowed vocabulary and rebuild lookup tries", "[token][wordpiece][edge][vocab]")
{
    Vocab first;
    Vocab second;

    const TokenId firstPlay = first.addToken("play");
    const TokenId firstIng = first.addToken("##ing");

    second.addToken("unused");
    const TokenId secondPlay = second.addToken("play");
    const TokenId secondIng = second.addToken("##ing");

    Wordpiece wordpiece{&first};

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("playing", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == firstPlay);
        REQUIRE(output[1] == firstIng);
    }

    wordpiece.setVocab(&second);

    REQUIRE(wordpiece.vocab() == &second);

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("playing", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == secondPlay);
        REQUIRE(output[1] == secondIng);
    }
}

TEST_CASE("Wordpiece root and continuation tries keep identical text logically separate", "[token][wordpiece][continuation][edge]")
{
    Vocab vocab;
    const TokenId rootIng = vocab.addToken("ing");
    const TokenId continuationIng = vocab.addToken("##ing");
    const TokenId walk = vocab.addToken("walk");

    Wordpiece wordpiece{&vocab};

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("ing", output);

        REQUIRE(written == 1);
        REQUIRE(output[0] == rootIng);
    }

    {
        std::array<TokenId, 8> output{};
        const std::size_t written = wordpiece.encode("walking", output);

        REQUIRE(written == 2);
        REQUIRE(output[0] == walk);
        REQUIRE(output[1] == continuationIng);
    }
}

TEST_CASE("Wordpiece continuation prefix alone is not a usable token piece", "[token][wordpiece][continuation][edge]")
{
    Vocab vocab;
    const TokenId root = vocab.addToken("a");
    vocab.addToken("##");
    const TokenId unk = vocab.addToken("[UNK]");
    vocab.specialTokens().setUnkId(unk);

    Wordpiece wordpiece{&vocab};

    std::array<TokenId, 8> output{};
    const std::size_t written = wordpiece.encode("ab", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == unk);

    (void)root;
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark Wordpiece repeated-word encoding", "[token][wordpiece][benchmark]")
{
    Vocab vocab;
    vocab.addToken("token");
    vocab.addToken("##iz");
    vocab.addToken("##ation");

    Wordpiece wordpiece{&vocab};
    std::array<TokenId, 8> output{};

    BENCHMARK("Encode repeated WordPiece words")
    {
        std::size_t totalWritten = 0;
        for (std::size_t i = 0; i < 1024; ++i)
            totalWritten += wordpiece.encode("tokenization", output);
        return totalWritten;
    };
}

TEST_CASE("Benchmark Wordpiece token decoding", "[token][wordpiece][benchmark]")
{
    Vocab vocab;
    const TokenId token = vocab.addToken("token");
    const TokenId continuation = vocab.addToken("##ization");

    Wordpiece wordpiece{&vocab};

    std::vector<TokenId> tokens;
    tokens.reserve(2048);
    for (std::size_t i = 0; i < 1024; ++i) {
        tokens.push_back(token);
        tokens.push_back(continuation);
    }

    std::vector<char> output(1024 * 12);

    BENCHMARK("Decode repeated WordPiece tokens") {
        return wordpiece.decode(tokens, output);
    };
}

#endif