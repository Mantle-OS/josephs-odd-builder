#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <job_token_enums.h>
#include <job_token_types.h>
#include <token/binary_token.h>
#include <token/gguf_token.h>
#include <token/hf_token.h>
#include <token/token_factory.h>
#include <vocab/vocab.h>

#include "../transient_test_file.h"

using job::token::BinaryHeader;
using job::token::BinaryToken;
using job::token::BinaryTokenType;
using job::token::GgufToken;
using job::token::HfToken;
using job::token::IToken;
using job::token::SplitPattern;
using job::token::TokenFactory;
using job::token::TokenType;

namespace job::token {

class TokenFactoryBinaryTestData
{
public:
    struct Token
    {
        std::string text;
        float score{0.0f};
        BinaryTokenType type{BinaryTokenType::Normal};
    };

    [[nodiscard]] static std::vector<std::uint8_t> make(std::string_view tokenText)
    {
        BinaryHeader header{};
        header.magic = BinaryToken::MAGIC;
        header.version = BinaryToken::CURRENT_VERSION;
        header.modelType = static_cast<std::uint8_t>(TokenType::BPE);
        header.splitPattern = static_cast<std::uint8_t>(SplitPattern::GPT2);
        header.vocabSize = 1;

        std::vector<std::uint8_t> output;

        append(output, header);

        const auto length = static_cast<std::uint16_t>(tokenText.size());
        append(output, length);
        appendBytes(output, tokenText.data(), tokenText.size());

        const float score = 0.0f;
        append(output, score);
        append(output, static_cast<std::uint8_t>(BinaryTokenType::Normal));

        return output;
    }

    static void write(const std::filesystem::path &path, std::string_view tokenText)
    {
        const std::vector<std::uint8_t> data = make(tokenText);

        std::ofstream stream{path, std::ios::binary | std::ios::trunc};

        REQUIRE(stream.is_open());

        stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));

        REQUIRE(stream.good());
    }

private:
    template<typename T>
    static void append(std::vector<std::uint8_t> &output, const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        const std::size_t offset = output.size();
        output.resize(offset + sizeof(T));
        std::memcpy(output.data() + offset, &value, sizeof(T));
    }

    static void appendBytes(std::vector<std::uint8_t> &output, const void *data, std::size_t size)
    {
        if (size == 0)
            return;

        const std::size_t offset = output.size();
        output.resize(offset + size);
        std::memcpy(output.data() + offset, data, size);
    }
};

} // namespace job::token

using job::token::TokenFactoryBinaryTestData;

static std::filesystem::path hfDataPath(std::string_view relativePath)
{
    return std::filesystem::path{JOB_TOKEN_TEST_DATA_DIR} / std::filesystem::path{relativePath};
}

//
// Block 1: usage / examples
//

TEST_CASE("TokenFactory creates HuggingFace provider from tokenizer file", "[token][factory][usage][hf]")
{
    const std::filesystem::path tokenizerPath = hfDataPath("gemma-4-12b-it/tokenizer.json");

    REQUIRE(std::filesystem::exists(tokenizerPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::HuggingFace, tokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::HuggingFace);
    REQUIRE(token->tokenType() != TokenType::Unknown);
    REQUIRE(token->vocabSize() > 0);
    REQUIRE(dynamic_cast<HfToken *>(token.get()) != nullptr);
}

TEST_CASE("TokenFactory creates HuggingFace provider from tokenizer and config paths", "[token][factory][usage][hf][config]")
{
    const std::filesystem::path tokenizerPath = hfDataPath("gemma-4-12b-it/tokenizer.json");
    const std::filesystem::path configPath = hfDataPath("gemma-4-12b-it/tokenizer_config.json");

    REQUIRE(std::filesystem::exists(tokenizerPath));
    REQUIRE(std::filesystem::exists(configPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::HuggingFace, configPath, tokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::HuggingFace);
    REQUIRE(token->tokenType() != TokenType::Unknown);
    REQUIRE(token->vocabSize() > 0);
    REQUIRE(dynamic_cast<HfToken *>(token.get()) != nullptr);
}

TEST_CASE("TokenFactory HuggingFace two-path overload routes tokenizer path first", "[token][factory][usage][hf][routing]")
{
    const std::filesystem::path tokenizerPath = hfDataPath("Qwen3.8-27B/tokenizer.json");
    const std::filesystem::path configPath = hfDataPath("Qwen3.8-27B/tokenizer_config.json");

    REQUIRE(std::filesystem::exists(tokenizerPath));
    REQUIRE(std::filesystem::exists(configPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::HuggingFace, configPath, tokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::HuggingFace);
    REQUIRE(token->vocabSize() > 0);
}

TEST_CASE("TokenFactory creates GGUF provider from model path", "[token][factory][usage][gguf]")
{
    const std::filesystem::path modelPath{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(modelPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::Gguf, modelPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Gguf);
    REQUIRE(token->tokenType() != TokenType::Unknown);
    REQUIRE(token->vocabSize() > 0);
    REQUIRE(dynamic_cast<GgufToken *>(token.get()) != nullptr);
}

TEST_CASE("TokenFactory GGUF two-path overload uses model path", "[token][factory][usage][gguf][routing]")
{
    const std::filesystem::path modelPath{JOB_TOKEN_TEST_GGUF_FILE};
    const std::filesystem::path ignoredTokenizerPath{"rk4_drunk_cousin_is_euler.json"};

    REQUIRE(std::filesystem::exists(modelPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::Gguf, modelPath, ignoredTokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Gguf);
    REQUIRE(token->vocabSize() > 0);
}

TEST_CASE("TokenFactory creates Binary provider from binary tokenizer path", "[token][factory][usage][binary]")
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "test_token_factory_binary.jobv";

    TokenFactoryBinaryTestData::write(path, "factory");

    IToken::UPtr token = TokenFactory::create(IToken::Provider::Binary, path);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Binary);
    REQUIRE(token->tokenType() == TokenType::BPE);
    REQUIRE(token->vocabSize() == 1);
    REQUIRE(token->vocab()->tokenText(0) == "factory");
    REQUIRE(dynamic_cast<BinaryToken *>(token.get()) != nullptr);

    std::filesystem::remove(path);
}

TEST_CASE("TokenFactory Binary two-path overload uses explicit tokenizer path", "[token][factory][usage][binary][routing]")
{
    const std::filesystem::path modelPath = std::filesystem::temp_directory_path() / "rk4_drunk_cousin_model_is_not_the_tokenizer.jobv";
    const std::filesystem::path tokenizerPath = std::filesystem::temp_directory_path() / "test_token_factory_explicit_binary.jobv";

    TokenFactoryBinaryTestData::write(tokenizerPath, "correct-tokenizer");

    IToken::UPtr token = TokenFactory::create(IToken::Provider::Binary, modelPath, tokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Binary);
    REQUIRE(token->vocabSize() == 1);
    REQUIRE(token->vocab()->tokenText(0) == "correct-tokenizer");

    std::filesystem::remove(tokenizerPath);
}

TEST_CASE("TokenFactory returns independently owned providers", "[token][factory][usage][ownership]")
{
    const std::filesystem::path tokenizerPath = hfDataPath("gemma-4-12b-it/tokenizer.json");

    REQUIRE(std::filesystem::exists(tokenizerPath));

    IToken::UPtr first = TokenFactory::create(IToken::Provider::HuggingFace, tokenizerPath);
    IToken::UPtr second = TokenFactory::create(IToken::Provider::HuggingFace, tokenizerPath);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first.get() != second.get());
    REQUIRE(first->provider() == IToken::Provider::HuggingFace);
    REQUIRE(second->provider() == IToken::Provider::HuggingFace);
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("TokenFactory rejects unknown provider", "[token][factory][edge][provider]")
{
    IToken::UPtr token = TokenFactory::create(IToken::Provider::Unknown, std::filesystem::path{"rk4_drunk_cousin_id_euler.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory rejects invalid provider value", "[token][factory][edge][provider]")
{
    const auto provider = static_cast<IToken::Provider>(255);

    IToken::UPtr token = TokenFactory::create(provider, std::filesystem::path{"rk4_drunk_cousin_id_euler.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory HuggingFace returns null for missing tokenizer file", "[token][factory][edge][hf][io]")
{
    IToken::UPtr token = TokenFactory::create(IToken::Provider::HuggingFace, std::filesystem::path{"lenny_dykstra_I_mean_edsger_dijkstra_is_so_damn_greedy.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory GGUF returns null for missing model file", "[token][factory][edge][gguf][io]")
{
    IToken::UPtr token = TokenFactory::create(IToken::Provider::Gguf, std::filesystem::path{"rk4_drunk_cousin_is_euler.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory Binary returns null for missing tokenizer file", "[token][factory][edge][binary][io]")
{
    IToken::UPtr token = TokenFactory::create(IToken::Provider::Binary, std::filesystem::path{"rk4_drunk_cousin_id_euler.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory HuggingFace two-path overload fails when tokenizer path is missing", "[token][factory][edge][hf][routing]")
{
    const std::filesystem::path configPath = hfDataPath("gemma-4-12b-it/tokenizer_config.json");

    REQUIRE(std::filesystem::exists(configPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::HuggingFace, configPath, std::filesystem::path{"lenny_dykstra_I_mean_edsger_dijkstra_is_so_damn_greedy.json"});

    REQUIRE(token == nullptr);
}

TEST_CASE("TokenFactory GGUF two-path overload ignores missing tokenizer path", "[token][factory][edge][gguf][routing]")
{
    const std::filesystem::path modelPath{JOB_TOKEN_TEST_GGUF_FILE};

    REQUIRE(std::filesystem::exists(modelPath));

    IToken::UPtr token = TokenFactory::create(IToken::Provider::Gguf, modelPath, std::filesystem::path{"rk4_drunk_cousin_tokenizer_is_irrelevant_here.json"});

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Gguf);
    REQUIRE(token->vocabSize() > 0);
}

TEST_CASE("TokenFactory Binary two-path overload ignores missing model path", "[token][factory][edge][binary][routing]")
{
    const std::filesystem::path tokenizerPath = std::filesystem::temp_directory_path() / "test_token_factory_binary_routing.jobv";

    TokenFactoryBinaryTestData::write(tokenizerPath, "binary-routing");

    IToken::UPtr token = TokenFactory::create(
        IToken::Provider::Binary,
        std::filesystem::path{"SplitMix64_walks_into_a_bar_The_bartender_says_Why_the_long_face.json"},
        tokenizerPath);

    REQUIRE(token != nullptr);
    REQUIRE(token->provider() == IToken::Provider::Binary);
    REQUIRE(token->vocab()->tokenText(0) == "binary-routing");

    std::filesystem::remove(tokenizerPath);
}

TEST_CASE("TokenFactory Binary two-path overload fails when tokenizer path is missing", "[token][factory][edge][binary][routing]")
{
    IToken::UPtr token = TokenFactory::create(
        IToken::Provider::Binary,
        std::filesystem::path{"model_path_is_irrelevant.jobv"},
        std::filesystem::path{"64_bits_but_everyone_still_treats_me_like_I_am_just_a_stepping_stone_to_something_better.json"});

    REQUIRE(token == nullptr);
}