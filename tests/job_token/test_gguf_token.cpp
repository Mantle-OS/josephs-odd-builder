#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

#include <job_token_enums.h>
#include <job_token_types.h>
#include <token/gguf_token.h>
#include <vocab/vocab.h>

using job::token::GgufToken;
using job::token::IToken;
using job::token::SplitPattern;
using job::token::TokenType;

//
// Block 1: usage / examples
//

TEST_CASE("GgufToken starts as GGUF provider", "[token][gguf][usage]")
{
    GgufToken token;

    REQUIRE(token.provider() == IToken::Provider::Gguf);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocab() != nullptr);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.modelName().empty());
    REQUIRE(token.preTokenizer().empty());
    REQUIRE(token.merges().empty());
}
#ifdef JOB_TOKEN_TEST_GGUF_FILE
TEST_CASE("GgufToken loads checked-in configured GGUF model", "[token][gguf][usage][integration][io]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.provider() == IToken::Provider::Gguf);
    REQUIRE(token.tokenType() != TokenType::Unknown);
    REQUIRE(token.vocab() != nullptr);
    REQUIRE(token.vocabSize() > 0);
    REQUIRE_FALSE(token.modelName().empty());
    REQUIRE_FALSE(token.preTokenizer().empty());
}

TEST_CASE("GgufToken configured Qwen fixture resolves to BPE", "[token][gguf][usage][integration][qwen]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.tokenType() == TokenType::BPE);
}

TEST_CASE("GgufToken configured Qwen fixture resolves pre-tokenizer pattern", "[token][gguf][usage][integration][qwen][pretokenizer]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE_FALSE(token.preTokenizer().empty());
    REQUIRE(token.splitPattern() != SplitPattern::None);
}

TEST_CASE("GgufToken configured model populates canonical vocabulary records", "[token][gguf][usage][integration][vocab]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.vocab() != nullptr);
    REQUIRE(token.vocabSize() > 0);
    REQUIRE_FALSE(token.records().empty());

    const auto *first = token.vocab()->record(0);

    REQUIRE(first != nullptr);
    REQUIRE(first->isValid());
    REQUIRE_FALSE(first->text().empty());
}

TEST_CASE("GgufToken configured model exposes BPE merge description", "[token][gguf][usage][integration][bpe][merges]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.tokenType() == TokenType::BPE);
    REQUIRE_FALSE(token.merges().empty());

    for (const auto &[left, right] : token.merges()) {
        REQUIRE_FALSE(left.empty());
        REQUIRE_FALSE(right.empty());
    }
}

TEST_CASE("GgufToken configured model exposes canonical special-token container", "[token][gguf][usage][integration][special]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.specialTokens() != nullptr);
}

TEST_CASE("GgufToken can be loaded repeatedly from the same file", "[token][gguf][usage][integration][state]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));

    const std::size_t firstVocabSize = token.vocabSize();
    const std::size_t firstMergeCount = token.merges().size();
    const std::string firstModelName = token.modelName();
    const std::string firstPreTokenizer = token.preTokenizer();

    REQUIRE(token.load(path));
    REQUIRE(token.vocabSize() == firstVocabSize);
    REQUIRE(token.merges().size() == firstMergeCount);
    REQUIRE(token.modelName() == firstModelName);
    REQUIRE(token.preTokenizer() == firstPreTokenizer);
}
#endif

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("GgufToken rejects missing GGUF file", "[token][gguf][edge][io]")
{
    GgufToken token;

    REQUIRE_FALSE(token.load(std::filesystem::path{"rk4_drunk_cousin_is_euler.json"}));
}

TEST_CASE("GgufToken rejects null memory buffer", "[token][gguf][edge][memory]")
{
    GgufToken token;

    REQUIRE_FALSE(token.load(nullptr, 128));
}

TEST_CASE("GgufToken rejects zero-sized memory buffer", "[token][gguf][edge][memory]")
{
    const std::array<std::byte, 8> buffer{};

    GgufToken token;

    REQUIRE_FALSE(token.load(buffer.data(), 0));
}

TEST_CASE("GgufToken rejects null pointer with zero size", "[token][gguf][edge][memory]")
{
    GgufToken token;

    REQUIRE_FALSE(token.load(nullptr, 0));
}

TEST_CASE("GgufToken rejects empty byte span", "[token][gguf][edge][memory][span]")
{
    const std::span<const std::byte> buffer;

    GgufToken token;

    REQUIRE_FALSE(token.load(buffer));
}

TEST_CASE("GgufToken rejects malformed memory buffer", "[token][gguf][edge][memory]")
{
    const std::array<std::byte, 16> buffer = {
        std::byte{0x52}, std::byte{0x4b}, std::byte{0x34}, std::byte{0x20},
        std::byte{0x69}, std::byte{0x73}, std::byte{0x20}, std::byte{0x62},
        std::byte{0x65}, std::byte{0x74}, std::byte{0x74}, std::byte{0x65},
        std::byte{0x72}, std::byte{0x21}, std::byte{0x21}, std::byte{0x21}
    };

    GgufToken token;

    REQUIRE_FALSE(token.load(buffer.data(), buffer.size()));
}

TEST_CASE("GgufToken rejects malformed byte span", "[token][gguf][edge][memory][span]")
{
    const std::array<std::byte, 8> buffer = {
        std::byte{0xde}, std::byte{0xad}, std::byte{0xbe}, std::byte{0xef},
        std::byte{0xca}, std::byte{0xfe}, std::byte{0xba}, std::byte{0xbe}
    };

    GgufToken token;

    REQUIRE_FALSE(token.load(std::span<const std::byte>{buffer}));
}

#ifdef JOB_TOKEN_TEST_GGUF_FILE
TEST_CASE("GgufToken failed load clears previous tokenizer state", "[token][gguf][edge][state][failure]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.vocabSize() > 0);
    REQUIRE_FALSE(token.modelName().empty());

    REQUIRE_FALSE(token.load(std::filesystem::path{"rk4_drunk_cousin_id_euler.json"}));

    REQUIRE(token.provider() == IToken::Provider::Gguf);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.modelName().empty());
    REQUIRE(token.preTokenizer().empty());
    REQUIRE(token.merges().empty());
    REQUIRE(token.splitPattern() == SplitPattern::None);
    REQUIRE(token.chatTemplate().empty());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
}

TEST_CASE("GgufToken clear restores GGUF defaults", "[token][gguf][edge][state]")
{
    const std::filesystem::path path{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(path));

    GgufToken token;

    REQUIRE(token.load(path));
    REQUIRE(token.vocabSize() > 0);

    token.clear();

    REQUIRE(token.provider() == IToken::Provider::Gguf);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.modelName().empty());
    REQUIRE(token.preTokenizer().empty());
    REQUIRE(token.merges().empty());
    REQUIRE(token.splitPattern() == SplitPattern::None);
    REQUIRE(token.customSplitPattern().empty());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
    REQUIRE(token.chatTemplate().empty());
}
#endif