#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "formats/gguf_tokenizer_reader.h"
#include "job_gguf.h"
#include "job_gguf_kv.h"
#include "../transient_test_file.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

namespace detail {

inline ggml::JobGguf::UPtr createSyntheticGgufTokenizerContext()
{
    auto gguf = ggml::JobGguf::createUniq();

    // Model and architecture
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.model", std::string("llama")));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.pre", std::string("llama-v3")));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.chat_template", std::string("{{ bos_token }}{% for m in messages %}{{ m.content }}{% endfor %}")));

    // Special token IDs
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.bos_token_id", int32_t(1)));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.eos_token_id", int32_t(2)));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.unknown_token_id", int32_t(0)));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.padding_token_id", int32_t(3)));

    // Flags
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.add_bos_token", true));
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.add_eos_token", false));

    // Vocabulary tokens
    std::vector<std::string> tokens = {"<unk>", "<bos>", "<eos>", "<pad>", "h", "e", "l", "o", "he", "ll", "hello", "world"};
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.tokens", tokens));

    // Token scores
    std::vector<float> scores = {0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -1.0f, -1.0f, -1.0f, 2.0f, 2.0f, 5.0f, 4.0f};
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.scores", scores));

    // Token types (1=Normal, 2=Unknown, 3=Control)
    std::vector<int32_t> types = {2, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1};
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.token_type", types));

    // BPE Merges
    std::vector<std::string> merges = {"h e", "l l", "he ll", "hell o"};
    gguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.merges", merges));

    return gguf;
}

} // namespace detail

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("GgufTokenizerReader loads tokenizer from JobGguf instance", "[token][formats][gguf][example]")
{
    auto gguf = detail::createSyntheticGgufTokenizerContext();
    REQUIRE(gguf != nullptr);

    GgufTokenizerReader reader;
    bool success = reader.loadFromGguf(*gguf);

    REQUIRE(success);
    CHECK(reader.modelName() == "llama");
    CHECK(reader.modelType() == HfModelType::BPE);
    CHECK(reader.vocabSize() == 12);
    CHECK(reader.data().preTokenizer == "llama-v3");

    // Special token IDs and resolved strings
    CHECK(reader.bosId() == 1);
    CHECK(reader.eosId() == 2);
    CHECK(reader.unkId() == 0);
    CHECK(reader.padId() == 3);

    CHECK(reader.bosToken() == "<bos>");
    CHECK(reader.eosToken() == "<eos>");
    CHECK(reader.unkToken() == "<unk>");
    CHECK(reader.padToken() == "<pad>");

    // Token Lookups
    CHECK(reader.findTokenId("hello") == 10);
    CHECK(reader.findTokenId("world") == 11);
    CHECK(reader.findTokenString(10) == "hello");
    CHECK(reader.findTokenString(1) == "<bos>");

    // Merges
    REQUIRE(reader.data().merges.size() == 4);
    CHECK(reader.data().merges[0].first == "h");
    CHECK(reader.data().merges[0].second == "e");

    // Chat Template
    CHECK(!reader.chatTemplate().empty());
}

TEST_CASE("GgufTokenizerReader loads GGUF binary metadata from disk and memory", "[token][formats][gguf][metadata_roundtrip]")
{
    auto gguf = detail::createSyntheticGgufTokenizerContext();
    REQUIRE(gguf != nullptr);

    std::vector<std::byte> metadataBytes = gguf->metadata();
    REQUIRE(!metadataBytes.empty());

    SECTION("Load from memory buffer") {
        GgufTokenizerReader reader;
        bool ok = reader.loadFromMemory(metadataBytes);

        REQUIRE(ok);
        CHECK(reader.vocabSize() == 12);
        CHECK(reader.findTokenId("hello") == 10);
        CHECK(reader.bosId() == 1);
    }

    SECTION("Load from transient file on disk") {
        TransientTestFile tempFile("test_model.gguf", metadataBytes);

        GgufTokenizerReader reader;
        bool ok = reader.loadFromFile(tempFile.path());

        REQUIRE(ok);
        CHECK(reader.vocabSize() == 12);
        CHECK(reader.findTokenId("world") == 11);
        CHECK(reader.eosToken() == "<eos>");
    }
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("GgufTokenizerReader handles missing keys, invalid types, and empty vocabularies", "[token][formats][gguf][edge_cases]")
{
    GgufTokenizerReader reader;

    SECTION("GGUF missing tokenizer.ggml.tokens array fails validation") {
        auto badGguf = ggml::JobGguf::createUniq();
        badGguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.model", std::string("llama")));

        CHECK_FALSE(reader.loadFromGguf(*badGguf));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("GGUF with empty tokens array returns false") {
        auto emptyGguf = ggml::JobGguf::createUniq();
        emptyGguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.tokens", std::vector<std::string>{}));

        CHECK_FALSE(reader.loadFromGguf(*emptyGguf));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("GGUF with minimal configuration defaults optional fields gracefully") {
        auto minimalGguf = ggml::JobGguf::createUniq();
        std::vector<std::string> tokens = {"foo", "bar"};
        minimalGguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.tokens", tokens));

        bool ok = reader.loadFromGguf(*minimalGguf);
        REQUIRE(ok);
        CHECK(reader.vocabSize() == 2);
        CHECK(reader.bosId() == -1);
        CHECK(reader.eosId() == -1);
        CHECK(reader.unkId() == -1);
        CHECK(reader.data().merges.empty());
        CHECK(reader.chatTemplate().empty());
    }

    SECTION("Invalid memory pointer or zero size fails gracefully") {
        CHECK_FALSE(reader.loadFromMemory(nullptr, 0));
        CHECK_FALSE(reader.loadFromMemory(nullptr, 1024));
    }

    SECTION("Non-existent file path returns false") {
        CHECK_FALSE(reader.loadFromFile("/non/existent/model.gguf"));
    }

    SECTION("Out-of-range lookups return std::nullopt") {
        auto gguf = detail::createSyntheticGgufTokenizerContext();
        REQUIRE(gguf != nullptr);
        REQUIRE(reader.loadFromGguf(*gguf));

        CHECK_FALSE(reader.findTokenId("nonexistent_word").has_value());
        CHECK_FALSE(reader.findTokenString(-1).has_value());
        CHECK_FALSE(reader.findTokenString(9999).has_value());
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark GgufTokenizerReader loading and token lookup", "[token][formats][gguf][benchmark]")
{
    auto largeGguf = ggml::JobGguf::createUniq();
    std::vector<std::string> tokens;
    tokens.reserve(32000);
    for (int i = 0; i < 32000; ++i) {
        tokens.push_back("tok_" + std::to_string(i));
    }
    largeGguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.tokens", tokens));

    std::vector<std::string> merges;
    merges.reserve(5000);
    for (int i = 0; i < 5000; ++i) {
        merges.push_back("t " + std::to_string(i));
    }
    largeGguf->setKeyValue(ggml::JobGgufKv("tokenizer.ggml.merges", merges));

    GgufTokenizerReader reader;

    BENCHMARK("Load 32,000-token GGUF tokenizer from JobGguf") {
        return reader.loadFromGguf(*largeGguf);
    };

    BENCHMARK("Lookup 1000 tokens in loaded GgufTokenizerReader") {
        int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            auto id = reader.findTokenId("tok_" + std::to_string(i));
            if (id) sum += *id;
        }
        return sum;
    };
}
#endif

} // namespace job::token::test