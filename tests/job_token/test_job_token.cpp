#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_token.h>
#include <job_token_enums.h>
#include <job_token_types.h>
#include <bpe.h>

#include <token/hf_token.h>
#include <token/itoken.h>

#include <vocab/special_tokens.h>
#include <vocab/vocab.h>

#include <encoder/gpt2_byte_encoder.h>

using job::token::ByteEncoding;
using job::token::ByteSymbols;
using job::token::Gpt2ByteEncoder;
using job::token::HfToken;
using job::token::IToken;
using job::token::JobToken;
using job::token::SpecialTokenType;
using job::token::SplitPattern;
using job::token::StructuralType;
using job::token::TokenId;
using job::token::TokenType;
using job::token::Vocab;
using job::token::kInvalidToken;
using job::token::TokenMerges;
using job::token::Bpe;

static constexpr std::string_view Text = "Hello from Joseph's Odd Builder.";

namespace job::token {
class JobTokenTestToken final : public IToken
{
public:
    using Ptr  = std::shared_ptr<JobTokenTestToken>;
    using WPtr = std::weak_ptr<JobTokenTestToken>;
    using UPtr = std::unique_ptr<JobTokenTestToken>;

    JobTokenTestToken() = default;
    ~JobTokenTestToken() override = default;

    JobTokenTestToken(const JobTokenTestToken &) = delete;
    JobTokenTestToken &operator=(const JobTokenTestToken &) = delete;
    JobTokenTestToken(JobTokenTestToken &&) = delete;
    JobTokenTestToken &operator=(JobTokenTestToken &&) = delete;

    [[nodiscard]] static UPtr createUniq() { return std::make_unique<JobTokenTestToken>(); }
    [[nodiscard]] static Ptr createShared() { return std::make_shared<JobTokenTestToken>(); }

protected:
    void extraClear() noexcept override {}
};

class JobTokenTestData
{
public:
    JobTokenTestData() = delete;

    [[nodiscard]] static JobTokenTestToken::UPtr makeByteBpeToken()
    {
        auto token = JobTokenTestToken::createUniq();
        token->setProvider(IToken::Provider::Binary);
        token->setTokenType(TokenType::BPE);
        token->setSplitPattern(SplitPattern::None);
        populateByteVocab(*token->vocab());
        return token;
    }

    [[nodiscard]] static JobTokenTestToken::UPtr makeAsciiBpeToken()
    {
        return makeByteBpeToken();
    }

    [[nodiscard]] static JobTokenTestToken::UPtr makeSequenceBpeToken(TokenId &bosId, TokenId &eosId)
    {
        auto token = makeByteBpeToken();

        bosId = token->vocab()->addToken("<BOS>", 0.0f, StructuralType::Control);
        eosId = token->vocab()->addToken("<EOS>", 0.0f, StructuralType::Control);

        token->specialTokens()->registerSpecial("bos", bosId, SpecialTokenType::Bos);
        token->specialTokens()->registerSpecial("eos", eosId, SpecialTokenType::Eos);

        token->setAddBosToken(true);
        token->setAddEosToken(true);

        return token;
    }

    static void populateByteVocab(Vocab &vocab)
    {
        vocab.reserve(256);

        for (std::uint32_t value = 0; value < 256; ++value) {
            const char character = static_cast<char>(static_cast<std::uint8_t>(value));
            vocab.setToken(static_cast<TokenId>(value), std::string{&character, 1});
        }
    }
};

} // namespace job::token

using job::token::JobTokenTestData;
using job::token::JobTokenTestToken;

//
// Block 1: usage / examples
//

TEST_CASE("JobToken starts unconfigured", "[token][job-token][usage]")
{
    JobToken tokenizer;

    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
}

TEST_CASE("JobToken factories create independent instances", "[token][job-token][usage][factory]")
{
    JobToken::Ptr shared = JobToken::createShared();
    JobToken::UPtr unique = JobToken::createUniq();

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);
    REQUIRE(shared.get() != unique.get());
    REQUIRE_FALSE(shared->isReady());
    REQUIRE_FALSE(unique->isReady());
}

TEST_CASE("JobToken accepts valid token description", "[token][job-token][usage][configure]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();
    IToken *const original = token.get();

    REQUIRE(tokenizer.setToken(std::move(token)));
    REQUIRE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == original);
    REQUIRE(tokenizer.token()->provider() == IToken::Provider::Binary);
    REQUIRE(tokenizer.token()->tokenType() == TokenType::BPE);
}

TEST_CASE("JobToken owns token description after setToken", "[token][job-token][usage][ownership]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();
    IToken *const borrowed = token.get();

    REQUIRE(tokenizer.setToken(std::move(token)));
    REQUIRE(token == nullptr);
    REQUIRE(tokenizer.token() == borrowed);
    REQUIRE(tokenizer.isReady());
}

TEST_CASE("JobToken exposes const token description", "[token][job-token][usage][const]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const JobToken &constTokenizer = tokenizer;
    const IToken *const token = constTokenizer.token();

    REQUIRE(token != nullptr);
    REQUIRE(token->tokenType() == TokenType::BPE);
}

TEST_CASE("JobToken encodes byte backed BPE text", "[token][job-token][usage][encode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const std::vector<TokenId> encoded = tokenizer.encode("hello");

    REQUIRE(encoded.size() == 5);
    REQUIRE(encoded[0] == static_cast<TokenId>('h'));
    REQUIRE(encoded[1] == static_cast<TokenId>('e'));
    REQUIRE(encoded[2] == static_cast<TokenId>('l'));
    REQUIRE(encoded[3] == static_cast<TokenId>('l'));
    REQUIRE(encoded[4] == static_cast<TokenId>('o'));
}

TEST_CASE("JobToken decodes byte backed BPE tokens", "[token][job-token][usage][decode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const std::array<TokenId, 5> tokens = {
        static_cast<TokenId>('h'), static_cast<TokenId>('e'), static_cast<TokenId>('l'),
        static_cast<TokenId>('l'), static_cast<TokenId>('o')
    };

    REQUIRE(tokenizer.decode(tokens) == "hello");
}

TEST_CASE("JobToken round trips ordinary ASCII text", "[token][job-token][usage][round-trip]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    static constexpr std::string_view Text = "Euler is the drunk cousin of RK4. :P";

    const std::vector<TokenId> encoded = tokenizer.encode(Text);

    REQUIRE_FALSE(encoded.empty());
    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("JobToken round trips whitespace and punctuation", "[token][job-token][usage][round-trip]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    static constexpr std::string_view Text = "one two\tthree\nfour!? []{}";

    const std::vector<TokenId> encoded = tokenizer.encode(Text);

    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("JobToken round trips arbitrary byte values through byte BPE vocabulary", "[token][job-token][usage][round-trip][byte]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    std::string input;

    for (std::uint32_t value = 1; value < 256; ++value)
        input.push_back(static_cast<char>(static_cast<std::uint8_t>(value)));

    const std::vector<TokenId> encoded = tokenizer.encode(input);

    REQUIRE(encoded.size() == input.size());
    REQUIRE(tokenizer.decode(encoded) == input);
}

TEST_CASE("JobToken applies BOS and EOS sequence tokens", "[token][job-token][usage][sequence]")
{
    JobToken tokenizer;
    TokenId bosId = kInvalidToken;
    TokenId eosId = kInvalidToken;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeSequenceBpeToken(bosId, eosId)));

    const std::vector<TokenId> encoded = tokenizer.encode("JOB");

    REQUIRE(encoded.size() == 5);
    REQUIRE(encoded.front() == bosId);
    REQUIRE(encoded.back() == eosId);
    REQUIRE(encoded[1] == static_cast<TokenId>('J'));
    REQUIRE(encoded[2] == static_cast<TokenId>('O'));
    REQUIRE(encoded[3] == static_cast<TokenId>('B'));
}

TEST_CASE("JobToken applies BOS and EOS to empty input", "[token][job-token][usage][sequence][empty]")
{
    JobToken tokenizer;
    TokenId bosId = kInvalidToken;
    TokenId eosId = kInvalidToken;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeSequenceBpeToken(bosId, eosId)));

    const std::vector<TokenId> encoded = tokenizer.encode("");

    REQUIRE(encoded.size() == 2);
    REQUIRE(encoded[0] == bosId);
    REQUIRE(encoded[1] == eosId);
}

TEST_CASE("JobToken ignores invalid configured BOS and EOS ids", "[token][job-token][usage][sequence]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setAddBosToken(true);
    token->setAddEosToken(true);

    REQUIRE(token->specialTokens()->bosId() == kInvalidToken);
    REQUIRE(token->specialTokens()->eosId() == kInvalidToken);
    REQUIRE(tokenizer.setToken(std::move(token)));

    const std::vector<TokenId> encoded = tokenizer.encode("A");

    REQUIRE(encoded.size() == 1);
    REQUIRE(encoded[0] == static_cast<TokenId>('A'));
}

TEST_CASE("JobToken decode includes explicitly supplied special token text", "[token][job-token][usage][sequence][decode]")
{
    JobToken tokenizer;
    TokenId bosId = kInvalidToken;
    TokenId eosId = kInvalidToken;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeSequenceBpeToken(bosId, eosId)));

    const std::array<TokenId, 3> encoded = {bosId, static_cast<TokenId>('A'), eosId};

    REQUIRE(tokenizer.decode(encoded) == "<BOS>A<EOS>");
}

TEST_CASE("JobToken addPrefixSpace changes normalized input", "[token][job-token][usage][normalizer]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setAddPrefixSpace(true);

    REQUIRE(tokenizer.setToken(std::move(token)));

    const std::vector<TokenId> encoded = tokenizer.encode("hello");

    REQUIRE(encoded.size() == 6);
    REQUIRE(encoded.front() == static_cast<TokenId>(' '));
    REQUIRE(tokenizer.decode(encoded) == " hello");
}

TEST_CASE("JobToken addPrefixSpace does not duplicate existing leading space", "[token][job-token][usage][normalizer]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setAddPrefixSpace(true);

    REQUIRE(tokenizer.setToken(std::move(token)));

    const std::vector<TokenId> encoded = tokenizer.encode(" hello");

    REQUIRE(encoded.size() == 6);
    REQUIRE(tokenizer.decode(encoded) == " hello");
}

TEST_CASE("JobToken SplitPattern None preserves complete normalized input", "[token][job-token][usage][splitter]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setSplitPattern(SplitPattern::None);

    REQUIRE(tokenizer.setToken(std::move(token)));

    static constexpr std::string_view Text = "one two three";

    const std::vector<TokenId> encoded = tokenizer.encode(Text);

    REQUIRE(encoded.size() == Text.size());
    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("JobToken custom splitter preserves concatenated tokenized content", "[token][job-token][usage][splitter][custom]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setCustomSplitPattern(R"([A-Za-z]+|\s+|[^A-Za-z\s]+)");

    REQUIRE(token->splitPattern() == SplitPattern::Custom);
    REQUIRE(tokenizer.setToken(std::move(token)));

    static constexpr std::string_view Text = "hello, JOB! 42";

    const std::vector<TokenId> encoded = tokenizer.encode(Text);

    REQUIRE_FALSE(encoded.empty());
    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("JobToken can replace configured token runtime", "[token][job-token][usage][reconfigure]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const IToken *const first = tokenizer.token();

    REQUIRE(first != nullptr);

    JobTokenTestToken::UPtr replacement = JobTokenTestData::makeByteBpeToken();
    replacement->setAddPrefixSpace(true);

    IToken *const second = replacement.get();

    REQUIRE(tokenizer.setToken(std::move(replacement)));
    REQUIRE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == second);
    REQUIRE(tokenizer.token() != first);

    const std::vector<TokenId> encoded = tokenizer.encode("A");

    REQUIRE(encoded.size() == 2);
    REQUIRE(encoded[0] == static_cast<TokenId>(' '));
    REQUIRE(encoded[1] == static_cast<TokenId>('A'));
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("JobToken rejects null token description", "[token][job-token][edge][configure]")
{
    JobToken tokenizer;
    IToken::UPtr token;

    REQUIRE_FALSE(tokenizer.setToken(std::move(token)));
    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
}

TEST_CASE("JobToken rejecting null token clears previous runtime", "[token][job-token][edge][configure]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));
    REQUIRE(tokenizer.isReady());

    IToken::UPtr nullToken;

    REQUIRE_FALSE(tokenizer.setToken(std::move(nullToken)));
    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
    REQUIRE(tokenizer.encode("hello").empty());
}

TEST_CASE("JobToken rejects unknown token algorithm", "[token][job-token][edge][configure]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setTokenType(TokenType::Unknown);

    REQUIRE_FALSE(tokenizer.setToken(std::move(token)));
    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
}

TEST_CASE("JobToken rejects unsupported WordLevel algorithm", "[token][job-token][edge][configure]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setTokenType(TokenType::WordLevel);

    REQUIRE_FALSE(tokenizer.setToken(std::move(token)));
    REQUIRE_FALSE(tokenizer.isReady());
}

TEST_CASE("JobToken rejects custom split pattern without source", "[token][job-token][edge][splitter]")
{
    JobToken tokenizer;
    JobTokenTestToken::UPtr token = JobTokenTestData::makeByteBpeToken();

    token->setSplitPattern(SplitPattern::Custom);

    REQUIRE(token->customSplitPattern().empty());
    REQUIRE_FALSE(tokenizer.setToken(std::move(token)));
    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
}

TEST_CASE("JobToken encode before configuration returns empty result", "[token][job-token][edge][encode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.encode("hello").empty());
}

TEST_CASE("JobToken decode before configuration returns empty result", "[token][job-token][edge][decode]")
{
    JobToken tokenizer;
    const std::array<TokenId, 2> tokens = {1, 2};

    REQUIRE(tokenizer.decode(tokens).empty());
}

TEST_CASE("JobToken decode empty token span returns empty string", "[token][job-token][edge][decode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const std::vector<TokenId> tokens;

    REQUIRE(tokenizer.decode(tokens).empty());
}

TEST_CASE("JobToken empty encode without sequence tokens returns empty vector", "[token][job-token][edge][encode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));
    REQUIRE(tokenizer.encode("").empty());
}

TEST_CASE("JobToken BPE decode skips invalid token ids", "[token][job-token][edge][decode]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    const std::array<TokenId, 5> tokens = {
        static_cast<TokenId>('A'), kInvalidToken, static_cast<TokenId>('B'), 999999, static_cast<TokenId>('C')
};

REQUIRE(tokenizer.decode(tokens) == "ABC");
}

TEST_CASE("JobToken BPE encode exposes missing initial byte tokens", "[token][job-token][edge][vocab]")
{
    JobToken tokenizer;
    auto token = JobTokenTestToken::createUniq();

    token->setProvider(IToken::Provider::Binary);
    token->setTokenType(TokenType::BPE);
    token->setSplitPattern(SplitPattern::None);
    token->vocab()->addToken("A");

    REQUIRE(tokenizer.setToken(std::move(token)));

    const std::vector<TokenId> encoded = tokenizer.encode("AB");

    REQUIRE(encoded.size() == 2);
    REQUIRE(encoded[0] != kInvalidToken);
    REQUIRE(encoded[1] == kInvalidToken);
}

TEST_CASE("JobToken load rejects unknown provider", "[token][job-token][edge][load]")
{
    JobToken tokenizer;

    REQUIRE_FALSE(tokenizer.load(IToken::Provider::Unknown, "rk4_drunk_cousin_is_euler.json"));
    REQUIRE_FALSE(tokenizer.isReady());
    REQUIRE(tokenizer.token() == nullptr);
}

TEST_CASE("JobToken load rejects missing HuggingFace tokenizer", "[token][job-token][edge][load]")
{
    JobToken tokenizer;

    REQUIRE_FALSE(tokenizer.load(IToken::Provider::HuggingFace, "lenny_dykstra_I_mean_edsger_dijkstra_is_so_damn_greedy.json"));
    REQUIRE_FALSE(tokenizer.isReady());
}

TEST_CASE("JobToken load rejects missing GGUF tokenizer", "[token][job-token][edge][load]")
{
    JobToken tokenizer;

    REQUIRE_FALSE(tokenizer.load(IToken::Provider::Gguf, "rk4_drunk_cousin_is_euler.gguf"));
    REQUIRE_FALSE(tokenizer.isReady());
}

TEST_CASE("JobToken load rejects missing binary tokenizer", "[token][job-token][edge][load]")
{
    JobToken tokenizer;

    REQUIRE_FALSE(tokenizer.load(IToken::Provider::Binary, "rk4_drunk_cousin_is_euler.jobtok"));
    REQUIRE_FALSE(tokenizer.isReady());
}

TEST_CASE("JobToken BPE runtime applies configured merge rules",
          "[token][job-token][bpe][merges]")
{
    JobToken tokenizer;

    auto token = JobTokenTestToken::createUniq();

    token->setProvider(IToken::Provider::Binary);
    token->setTokenType(TokenType::BPE);
    token->setSplitPattern(SplitPattern::None);

    const TokenId a = token->vocab()->addToken("a");

    const TokenId b = token->vocab()->addToken("b");
    const TokenId ab = token->vocab()->addToken("ab");

    token->merges().emplace_back("a", "b");

    REQUIRE(tokenizer.setToken(std::move(token)));

    const std::vector<TokenId> encoded = tokenizer.encode("ab");
    REQUIRE(encoded.size() == 1);
    REQUIRE(encoded[0] == ab);

    REQUIRE(tokenizer.decode(encoded) == "ab");
}


//
// Block 3: real provider integration
//

#ifdef JOB_TOKEN_TEST_DATA_DIR

TEST_CASE("Bpe fully merges Qwen assistant from byte symbols",
          "[token][bpe][integration][hf][qwen]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath =
        root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath =
        root / "Qwen3.8-27B" / "tokenizer_config.json";

    HfToken token;

    REQUIRE(
        token.load(
            tokenizerPath,
            configPath));

    Bpe bpe{
        token.vocab()
    };

    std::vector<Bpe::MergeRule> rules;
    rules.reserve(
        token.merges().size());

    for (const auto &[leftText, rightText] :
         token.merges()) {

        const TokenId left =
            token.vocab()->findId(
                leftText);

        const TokenId right =
            token.vocab()->findId(
                rightText);

        if (left == kInvalidToken ||
            right == kInvalidToken) {
            continue;
        }

        std::string mergedText;
        mergedText.reserve(
            leftText.size() +
            rightText.size());

        mergedText += leftText;
        mergedText += rightText;

        const TokenId merged =
            token.vocab()->findId(
                mergedText);

        if (merged == kInvalidToken)
            continue;

        rules.push_back({
            left,
            right,
            merged
        });
    }

    bpe.setMergeRules(
        std::move(rules));

    Gpt2ByteEncoder encoder;

    const ByteSymbols symbols =
        encoder.encode(
            "assistant");

    const std::vector<TokenId> encoded =
        bpe.encode(
            symbols);

    const TokenId assistantId =
        token.vocab()->findId(
            "assistant");

    REQUIRE(assistantId != kInvalidToken);

    REQUIRE(encoded.size() == 1);
    REQUIRE(encoded[0] == assistantId);
}


TEST_CASE("Bpe applies Qwen assistant merge",
          "[token][bpe][merge][qwen]")
{
    Vocab vocab;

    vocab.setToken(395, "ass", 0.0f);
    vocab.setToken(11202, "istant", 0.0f);
    vocab.setToken(77091, "assistant", 0.0f);

    Bpe bpe{&vocab};

    const std::vector<Bpe::MergeRule> rules{
        {
            395,
            11202,
            77091
        }
    };

    bpe.setMergeRules(rules);

    const ByteSymbols symbols{
        "ass",
        "istant"
    };

    const std::vector<TokenId> output =
        bpe.encode(symbols);

    REQUIRE(output.size() == 1);
    REQUIRE(output[0] == 77091);
}


TEST_CASE("HuggingFace Qwen contains assistant BPE merge",
          "[token][hf][bpe][merge]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath =
        root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath =
        root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(
        tokenizer.load(
            IToken::Provider::HuggingFace,
            configPath,
            tokenizerPath));

    const auto assId =
        tokenizer.token()->findTokenId("ass");

    const auto istantId =
        tokenizer.token()->findTokenId("istant");

    const auto assistantId =
        tokenizer.token()->findTokenId("assistant");

    REQUIRE(assId.has_value());
    REQUIRE(istantId.has_value());
    REQUIRE(assistantId.has_value());

    bool foundMerge = false;

    for (const auto &[left, right] :
         tokenizer.token()->merges()) {

        if (left == "ass" &&
            right == "istant") {
            foundMerge = true;
            break;
        }
    }

    REQUIRE(foundMerge);
}


TEST_CASE("JobToken HuggingFace Qwen merges assistant",
          "[token][job-token][integration][hf][bpe]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath =
        root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath =
        root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(
        tokenizer.load(
            IToken::Provider::HuggingFace,
            configPath,
            tokenizerPath));

    const auto assistantId =
        tokenizer.token()->findTokenId(
            "assistant");

    REQUIRE(assistantId.has_value());

    const std::vector<TokenId> encoded =
        tokenizer.encode(
            "assistant");

    REQUIRE(encoded.size() == 1);
    REQUIRE(encoded[0] == *assistantId);
}


TEST_CASE("JobToken encodes Qwen chat special tokens atomically",
          "[token][job-token][integration][hf][special]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));

    const auto imStartId = tokenizer.token()->findTokenId("<|im_start|>");
    const auto imEndId = tokenizer.token()->findTokenId("<|im_end|>");

    REQUIRE(imStartId.has_value());
    REQUIRE(imEndId.has_value());

    const auto *record = tokenizer.token()->vocab()->record(*imStartId);

    REQUIRE(record != nullptr);
    REQUIRE(record->isSpecial());
    REQUIRE(tokenizer.token()->specialTokens()->isSpecial(*imStartId));

    const std::vector<TokenId> startEncoded = tokenizer.encode("<|im_start|>");
    const std::vector<TokenId> endEncoded =tokenizer.encode("<|im_end|>");

    REQUIRE(startEncoded.size() == 1);
    REQUIRE(endEncoded.size() == 1);

    REQUIRE(startEncoded[0] == *imStartId);
    REQUIRE(endEncoded[0] == *imEndId);
}

TEST_CASE("JobToken HuggingFace Qwen produces expected token ids", "[token][job-token][integration][hf][bpe]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;
    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));

    const std::vector<TokenId> encoded = tokenizer.encode(Text);
    const std::vector<TokenId> expected = {
        9419,
        494,
        14674,
        579,
        53187,
        20000,
        13
    };

    REQUIRE(encoded == expected);
    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("JobToken HuggingFace Qwen applies BPE merges",
          "[token][job-token][integration][hf][bpe][merges]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));

    const std::vector<TokenId> encoded = tokenizer.encode(Text);
    REQUIRE_FALSE(encoded.empty());

    for (const TokenId id : encoded)
        REQUIRE(id != kInvalidToken);

    REQUIRE(encoded.size() < Text.size());
    REQUIRE(tokenizer.decode(encoded) == Text);
}


TEST_CASE("JobToken HuggingFace Qwen exposes BPE merge metadata", "[token][job-token][integration][hf][bpe][merges]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));
    REQUIRE(tokenizer.isReady());
    REQUIRE(tokenizer.token() != nullptr);
    REQUIRE(tokenizer.token()->tokenType() == TokenType::BPE);

    const TokenMerges &merges = tokenizer.token()->merges();

    REQUIRE_FALSE(merges.empty());
    // WARN("Qwen BPE merge count: " << merges.size());

    REQUIRE(merges.size() > 100000);

    const auto &[left, right] = merges.front();

    // WARN("First Qwen BPE merge: ["
    //     << left
    //     << "] + ["
    //     << right
    //     << "]");

    REQUIRE_FALSE(left.empty());
    REQUIRE_FALSE(right.empty());

    const TokenId leftId = tokenizer.token()->vocab()->findId(left);
    const TokenId rightId = tokenizer.token()->vocab()->findId(right);

    // WARN("First merge left id: " << leftId);
    // WARN("First merge right id: " << rightId);

    REQUIRE(leftId != kInvalidToken);

    REQUIRE(rightId != kInvalidToken);

    std::string mergedText;
    mergedText.reserve(left.size() + right.size());
    mergedText += left;
    mergedText += right;

    const TokenId mergedId = tokenizer.token()->vocab()->findId(mergedText);

    // WARN("First merged token text: [" << mergedText << "]");
    // WARN("First merged token id: " << mergedId);

    REQUIRE(mergedId != kInvalidToken);
}

TEST_CASE("JobToken loads HuggingFace Qwen tokenizer through facade", "[token][job-token][integration][hf]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    REQUIRE(std::filesystem::exists(tokenizerPath));
    REQUIRE(std::filesystem::exists(configPath));

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));
    REQUIRE(tokenizer.isReady());
    REQUIRE(tokenizer.token() != nullptr);
    REQUIRE(tokenizer.token()->provider() == IToken::Provider::HuggingFace);
    REQUIRE(tokenizer.token()->tokenType() == TokenType::BPE);
    REQUIRE(tokenizer.token()->vocabSize() > 0);
}

TEST_CASE("JobToken HuggingFace Qwen facade round trips simple text", "[token][job-token][integration][hf][round-trip]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));
    REQUIRE(tokenizer.isReady());
    REQUIRE(tokenizer.token() != nullptr);
    REQUIRE(tokenizer.token()->byteEncoding() == ByteEncoding::Gpt2);
    REQUIRE_FALSE(tokenizer.token()->addPrefixSpace());

    static constexpr std::string_view Text = "Hello from Joseph's Odd Builder.";

    const std::vector<TokenId> encoded = tokenizer.encode(Text);

    REQUIRE_FALSE(encoded.empty());

    for (const TokenId id : encoded)
        REQUIRE(id != kInvalidToken);

    REQUIRE(tokenizer.decode(encoded) == Text);
}

TEST_CASE("Gpt2ByteEncoder decodes merged tokenizer symbols", "[token][encoder][gpt2][decode]")
{
    Gpt2ByteEncoder encoder;
    const ByteSymbols symbols{"Hello", "Ġfrom", "ĠJoseph"};

    REQUIRE(encoder.decode(symbols) == "Hello from Joseph");
}

TEST_CASE("Gpt2ByteEncoder decodes complete mapped text stored in one symbol", "[token][encoder][gpt2][decode]")
{
    Gpt2ByteEncoder encoder;
    const ByteSymbols symbols{"HelloĠfromĠJoseph"};

    REQUIRE(encoder.decode(symbols) == "Hello from Joseph");
}

TEST_CASE("JobToken HuggingFace Qwen facade produces valid token ids", "[token][job-token][integration][hf]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));

    const std::vector<TokenId> encoded = tokenizer.encode("The quick brown fox jumps over the lazy dog.");

    REQUIRE_FALSE(encoded.empty());

    for (const TokenId id : encoded) {
        REQUIRE(id != kInvalidToken);
        REQUIRE(tokenizer.token()->vocab()->record(id) != nullptr);
    }
}

#endif

//
// Block 4: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobToken byte BPE encode pipeline", "[token][job-token][benchmark]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    std::string text;

    for (std::size_t i = 0; i < 100; ++i)
        text += "Joseph gave the tokenizer one job per class, " "and somehow Euler still found the bar. ";

    BENCHMARK("Encode through complete JobToken pipeline")
    {
        return tokenizer.encode(text);
    };
}

TEST_CASE("Benchmark JobToken byte BPE decode pipeline", "[token][job-token][benchmark]")
{
    JobToken tokenizer;

    REQUIRE(tokenizer.setToken(JobTokenTestData::makeByteBpeToken()));

    std::string text;

    for (std::size_t i = 0; i < 100; ++i)
        text += "the dog requires additional compiler food ";

    const std::vector<TokenId> encoded = tokenizer.encode(text);

    REQUIRE_FALSE(encoded.empty());

    BENCHMARK("Decode through complete JobToken pipeline")
    {
        return tokenizer.decode(encoded);
    };
}

#ifdef JOB_TOKEN_TEST_DATA_DIR

TEST_CASE("Benchmark JobToken HuggingFace facade encoding", "[token][job-token][benchmark][hf]")
{
    const std::filesystem::path root{JOB_TOKEN_TEST_DATA_DIR};
    const std::filesystem::path tokenizerPath = root / "Qwen3.8-27B" / "tokenizer.json";
    const std::filesystem::path configPath = root / "Qwen3.8-27B" / "tokenizer_config.json";

    JobToken tokenizer;

    REQUIRE(tokenizer.load(IToken::Provider::HuggingFace, configPath, tokenizerPath));

    static constexpr std::string_view Text =
        "This is a realistic tokenizer facade benchmark. "
        "It passes through normalization, pre-tokenization, "
        "and the selected token algorithm before returning IDs.";

    BENCHMARK("Encode Qwen text through JobToken facade")
    {
        return tokenizer.encode(Text);
    };
}

#endif

#endif