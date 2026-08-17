#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include <algo/bpe.h>
#include <algo/token_algo_factory.h>
#include <algo/unigram.h>
#include <algo/wordpiece.h>
#include <vocab/vocab.h>
#include <job_token_types.h>

using job::token::Bpe;
using job::token::ITokenAlgo;
using job::token::TokenAlgoFactory;
using job::token::TokenId;
using job::token::TokenType;
using job::token::Unigram;
using job::token::Vocab;
using job::token::Wordpiece;

//
// Block 1: usage / examples
//

TEST_CASE("TokenAlgoFactory creates BPE algorithm", "[token][algo][factory][usage][bpe]")
{
    Vocab vocab;
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::BPE, &vocab);

    REQUIRE(algorithm != nullptr);
    REQUIRE(algorithm->type() == TokenType::BPE);
    REQUIRE(algorithm->vocab() == &vocab);
    REQUIRE(dynamic_cast<Bpe *>(algorithm.get()) != nullptr);
}

TEST_CASE("TokenAlgoFactory creates Unigram algorithm", "[token][algo][factory][usage][unigram]")
{
    Vocab vocab;
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::Unigram, &vocab);

    REQUIRE(algorithm != nullptr);
    REQUIRE(algorithm->type() == TokenType::Unigram);
    REQUIRE(algorithm->vocab() == &vocab);
    REQUIRE(dynamic_cast<Unigram *>(algorithm.get()) != nullptr);
}

TEST_CASE("TokenAlgoFactory creates WordPiece algorithm", "[token][algo][factory][usage][wordpiece]")
{
    Vocab vocab;
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::WordPiece, &vocab);

    REQUIRE(algorithm != nullptr);
    REQUIRE(algorithm->type() == TokenType::WordPiece);
    REQUIRE(algorithm->vocab() == &vocab);
    REQUIRE(dynamic_cast<Wordpiece *>(algorithm.get()) != nullptr);
}

TEST_CASE("TokenAlgoFactory returns independently owned algorithms", "[token][algo][factory][usage][ownership]")
{
    Vocab vocab;
    ITokenAlgo::UPtr first = TokenAlgoFactory::create(TokenType::BPE, &vocab);
    ITokenAlgo::UPtr second = TokenAlgoFactory::create(TokenType::BPE, &vocab);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first.get() != second.get());
    REQUIRE(first->vocab() == &vocab);
    REQUIRE(second->vocab() == &vocab);
}

TEST_CASE("TokenAlgoFactory BPE result can encode through ITokenAlgo", "[token][algo][factory][usage][bpe][encode]")
{
    Vocab vocab;
    const TokenId a = vocab.addToken("a");

    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::BPE, &vocab);

    REQUIRE(algorithm != nullptr);

    std::array<TokenId, 4> output{};
    const std::size_t written = algorithm->encode("a", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == a);
}

TEST_CASE("TokenAlgoFactory Unigram result can encode through ITokenAlgo", "[token][algo][factory][usage][unigram][encode]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello", -1.0f);

    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::Unigram, &vocab);

    REQUIRE(algorithm != nullptr);

    std::array<TokenId, 4> output{};
    const std::size_t written = algorithm->encode("hello", output);

    REQUIRE(written == 1);
    REQUIRE(output[0] == hello);
}

TEST_CASE("TokenAlgoFactory WordPiece result can encode through ITokenAlgo", "[token][algo][factory][usage][wordpiece][encode]")
{
    Vocab vocab;
    const TokenId play = vocab.addToken("play");
    const TokenId ing = vocab.addToken("##ing");

    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::WordPiece, &vocab);

    REQUIRE(algorithm != nullptr);

    std::array<TokenId, 4> output{};
    const std::size_t written = algorithm->encode("playing", output);

    REQUIRE(written == 2);
    REQUIRE(output[0] == play);
    REQUIRE(output[1] == ing);
}

TEST_CASE("TokenAlgoFactory algorithms decode through ITokenAlgo", "[token][algo][factory][usage][decode]")
{
    Vocab vocab;
    const TokenId hello = vocab.addToken("hello");

    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::Unigram, &vocab);

    REQUIRE(algorithm != nullptr);

    const std::array<TokenId, 1> tokens = {hello};
    std::array<char, 16> output{};
    const std::size_t written = algorithm->decode(tokens, output);

    REQUIRE(written == 5);
    REQUIRE(std::string_view{output.data(), written} == "hello");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("TokenAlgoFactory rejects null vocabulary for supported algorithms", "[token][algo][factory][edge][vocab]")
{
    REQUIRE(TokenAlgoFactory::create(TokenType::BPE, nullptr) == nullptr);
    REQUIRE(TokenAlgoFactory::create(TokenType::Unigram, nullptr) == nullptr);
    REQUIRE(TokenAlgoFactory::create(TokenType::WordPiece, nullptr) == nullptr);
}

TEST_CASE("TokenAlgoFactory rejects unknown token type", "[token][algo][factory][edge][type]")
{
    Vocab vocab;
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::Unknown, &vocab);

    REQUIRE(algorithm == nullptr);
}

TEST_CASE("TokenAlgoFactory rejects unsupported WordLevel token type", "[token][algo][factory][edge][type]")
{
    Vocab vocab;
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(TokenType::WordLevel, &vocab);

    REQUIRE(algorithm == nullptr);
}

TEST_CASE("TokenAlgoFactory rejects invalid token type value", "[token][algo][factory][edge][type]")
{
    Vocab vocab;
    const auto invalidType = static_cast<TokenType>(255);
    ITokenAlgo::UPtr algorithm = TokenAlgoFactory::create(invalidType, &vocab);

    REQUIRE(algorithm == nullptr);
}

TEST_CASE("TokenAlgoFactory preserves exact borrowed vocabulary identity", "[token][algo][factory][edge][vocab]")
{
    Vocab first;
    Vocab second;

    ITokenAlgo::UPtr firstAlgorithm = TokenAlgoFactory::create(TokenType::Unigram, &first);
    ITokenAlgo::UPtr secondAlgorithm = TokenAlgoFactory::create(TokenType::Unigram, &second);

    REQUIRE(firstAlgorithm != nullptr);
    REQUIRE(secondAlgorithm != nullptr);
    REQUIRE(firstAlgorithm->vocab() == &first);
    REQUIRE(secondAlgorithm->vocab() == &second);
    REQUIRE(firstAlgorithm->vocab() != secondAlgorithm->vocab());
}

TEST_CASE("TokenAlgoFactory accepts an empty vocabulary", "[token][algo][factory][edge][vocab]")
{
    Vocab vocab;

    REQUIRE(vocab.empty());

    ITokenAlgo::UPtr bpe = TokenAlgoFactory::create(TokenType::BPE, &vocab);
    ITokenAlgo::UPtr unigram = TokenAlgoFactory::create(TokenType::Unigram, &vocab);
    ITokenAlgo::UPtr wordpiece = TokenAlgoFactory::create(TokenType::WordPiece, &vocab);

    REQUIRE(bpe != nullptr);
    REQUIRE(unigram != nullptr);
    REQUIRE(wordpiece != nullptr);
    REQUIRE(bpe->vocab() == &vocab);
    REQUIRE(unigram->vocab() == &vocab);
    REQUIRE(wordpiece->vocab() == &vocab);
}