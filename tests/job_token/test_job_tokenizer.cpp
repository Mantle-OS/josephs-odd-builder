#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "job_tokenizer.h"
#include "template/chat_message.h"
#include "../transient_test_file.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

namespace detail {

inline std::string createSyntheticHfTokenizerJson()
{
    return R"({
        "version": "1.0",
        "model": {
            "type": "BPE",
            "byte_fallback": true,
            "unk_token": "<unk>",
            "vocab": {
                "<unk>": 0,
                "<s>": 1,
                "</s>": 2,
                "<pad>": 3,
                "h": 4,
                "e": 5,
                "l": 6,
                "o": 7,
                " ": 8,
                "w": 9,
                "r": 10,
                "d": 11,
                "he": 12,
                "ll": 13,
                "hello": 14,
                "world": 15,
                "<0x41>": 16
            },
            "merges": [
                "h e",
                "l l",
                "he ll",
                "hell o",
                "w o",
                "r l",
                "rl d",
                "wo rld"
            ]
        },
        "added_tokens": [
            {"id": 0, "content": "<unk>", "special": true},
            {"id": 1, "content": "<s>", "special": true},
            {"id": 2, "content": "</s>", "special": true},
            {"id": 3, "content": "<pad>", "special": true}
        ]
    })";
}

inline std::string createSyntheticHfConfigJson()
{
    return R"({
        "bos_token": "<s>",
        "eos_token": "</s>",
        "unk_token": "<unk>",
        "pad_token": "<pad>",
        "chat_template": "{{ bos_token }}{% for m in messages %}<|im_start|>{{ m.role }}\n{{ m.content }}<|im_end|>\n{% endfor %}{% if add_generation_prompt %}<|im_start|>assistant\n{% endif %}"
    })";
}

} // namespace detail

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("JobTokenizer encodes and decodes text using BPE and byte fallback", "[token][facade][example]")
{
    JobTokenizer tokenizer;
    REQUIRE(tokenizer.loadHfFromMemory(
        detail::createSyntheticHfTokenizerJson(),
        detail::createSyntheticHfConfigJson()));

    REQUIRE(tokenizer.isLoaded());
    CHECK(tokenizer.vocabSize() == 17);
    CHECK(tokenizer.bosId() == 1);
    CHECK(tokenizer.eosId() == 2);

    SECTION("Encode with BOS and EOS tokens") {
        std::vector<int32_t> tokens = tokenizer.encode("hello", true, true);

        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0] == tokenizer.bosId()); // <s>
        CHECK(tokens[1] == 14);                // "hello"
        CHECK(tokens[2] == tokenizer.eosId()); // </s>
    }

    SECTION("Decode token stream into text") {
        std::vector<int32_t> tokens = {1, 14, 2};

        // Decoded with special tokens stripped
        std::string textStripped = tokenizer.decode(tokens, true);
        CHECK(textStripped == "hello");

        // Decoded with special tokens preserved
        std::string textFull = tokenizer.decode(tokens, false);
        CHECK(textFull == "<s>hello</s>");
    }

    SECTION("Byte fallback tokenization for out-of-vocab byte (ASCII 'A' = 0x41)") {
        std::vector<int32_t> tokens = tokenizer.encode("A", false, false);

        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0] == 16); // <0x41>

        std::string decoded = tokenizer.decode(tokens, false);
        CHECK(decoded == "A");
    }
}

TEST_CASE("JobTokenizer formats and tokenizes chat conversations", "[token][facade][chat]")
{
    JobTokenizer tokenizer;
    REQUIRE(tokenizer.loadHfFromMemory(
        detail::createSyntheticHfTokenizerJson(),
        detail::createSyntheticHfConfigJson()));

    std::vector<ChatMessage> dialogue = {
        {ChatRole::System, "Be concise.", "", ""},
        {ChatRole::User, "hello", "", ""}
    };

    SECTION("applyChatTemplate renders expected Jinja prompt") {
        std::string prompt = tokenizer.applyChatTemplate(dialogue, true);

        std::string expected =
            "<s><|im_start|>system\nBe concise.<|im_end|>\n"
            "<|im_start|>user\nhello<|im_end|>\n"
            "<|im_start|>assistant\n";

        CHECK(prompt == expected);
    }

    SECTION("encodeChat converts dialogue directly to token IDs") {
        std::vector<int32_t> chatTokens = tokenizer.encodeChat(dialogue, true);
        REQUIRE(!chatTokens.empty());
        CHECK(chatTokens.front() == tokenizer.bosId());
    }
}

TEST_CASE("JobTokenizer loads real disk fixtures when available", "[token][facade][fixtures]")
{
#if defined(JOB_TOKEN_TEST_DATA_DIR)
    const std::filesystem::path baseDir(JOB_TOKEN_TEST_DATA_DIR);

    SECTION("Load Gemma-4-12B-it via facade") {
        const std::filesystem::path gemmaDir = baseDir / "gemma-4-12b-it";
        REQUIRE(std::filesystem::exists(gemmaDir / "tokenizer.json"));
        REQUIRE(std::filesystem::exists(gemmaDir / "tokenizer_config.json"));

        JobTokenizer tokenizer;
        bool ok = tokenizer.loadHf(
            gemmaDir / "tokenizer.json",
            gemmaDir / "tokenizer_config.json");

        REQUIRE(ok);
        CHECK(tokenizer.isLoaded());
        CHECK(tokenizer.vocabSize() > 0);
        CHECK(!tokenizer.chatTemplate().empty());
    }

    SECTION("Load Qwen3.8-27B via facade") {
        const std::filesystem::path qwenDir = baseDir / "Qwen3.8-27B";
        REQUIRE(std::filesystem::exists(qwenDir / "tokenizer.json"));
        REQUIRE(std::filesystem::exists(qwenDir / "tokenizer_config.json"));

        JobTokenizer tokenizer;
        bool ok = tokenizer.loadHf(
            qwenDir / "tokenizer.json",
            qwenDir / "tokenizer_config.json");

        REQUIRE(ok);
        CHECK(tokenizer.isLoaded());
        CHECK(tokenizer.vocabSize() > 0);
    }
#endif
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("JobTokenizer guards invariants against uninitialized state and empty inputs", "[token][facade][edge_cases]")
{
    JobTokenizer uninitTokenizer;

    SECTION("Operations on uninitialized tokenizer fail or return empty gracefully") {
        CHECK_FALSE(uninitTokenizer.isLoaded());
        CHECK(uninitTokenizer.vocabSize() == 0);
        CHECK(uninitTokenizer.encode("test").empty());
        CHECK(uninitTokenizer.decode(std::vector<int32_t>{1, 2, 3}).empty());
        CHECK(uninitTokenizer.applyChatTemplate(std::vector<ChatMessage>{}).empty());
        CHECK_FALSE(uninitTokenizer.tokenToId("hello").has_value());
        CHECK_FALSE(uninitTokenizer.idToToken(0).has_value());
    }

    SECTION("Encoding empty string with BOS/EOS produces only boundary tokens") {
        JobTokenizer tokenizer;
        REQUIRE(tokenizer.loadHfFromMemory(
            detail::createSyntheticHfTokenizerJson(),
            detail::createSyntheticHfConfigJson()));

        auto tokensBosEos = tokenizer.encode("", true, true);
        REQUIRE(tokensBosEos.size() == 2);
        CHECK(tokensBosEos[0] == tokenizer.bosId());
        CHECK(tokensBosEos[1] == tokenizer.eosId());

        auto tokensNone = tokenizer.encode("", false, false);
        CHECK(tokensNone.empty());
    }

    SECTION("Out-of-range token IDs in decode are skipped without crashing") {
        JobTokenizer tokenizer;
        REQUIRE(tokenizer.loadHfFromMemory(
            detail::createSyntheticHfTokenizerJson(),
            detail::createSyntheticHfConfigJson()));

        std::vector<int32_t> invalidTokens = {-1, 99999, 14, -500};
        std::string decoded = tokenizer.decode(invalidTokens, true);
        CHECK(decoded == "hello");
    }

    SECTION("clear() resets all internal state and lookups") {
        JobTokenizer tokenizer;
        REQUIRE(tokenizer.loadHfFromMemory(
            detail::createSyntheticHfTokenizerJson(),
            detail::createSyntheticHfConfigJson()));
        REQUIRE(tokenizer.isLoaded());

        tokenizer.clear();
        CHECK_FALSE(tokenizer.isLoaded());
        CHECK(tokenizer.vocabSize() == 0);
        CHECK(tokenizer.bosId() == -1);
        CHECK(tokenizer.eosId() == -1);
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark JobTokenizer encoding and decoding pipelines", "[token][facade][benchmark]")
{
    JobTokenizer tokenizer;
    REQUIRE(tokenizer.loadHfFromMemory(
        detail::createSyntheticHfTokenizerJson(),
        detail::createSyntheticHfConfigJson()));

    std::string textToEncode;
    for (int i = 0; i < 100; ++i) {
        textToEncode += "hello world ";
    }

    BENCHMARK("Encode 100-word repeated text") {
        return tokenizer.encode(textToEncode, true, true);
    };

    std::vector<int32_t> tokenStream = tokenizer.encode(textToEncode, false, false);

    BENCHMARK("Decode token stream into string") {
        return tokenizer.decode(tokenStream, false);
    };

    std::vector<ChatMessage> conversation = {
        {ChatRole::System, "You are a fast tokenizing engine.", "", ""},
        {ChatRole::User, "hello world hello world hello world", "", ""},
        {ChatRole::Assistant, "hello world", "", ""}
    };

    BENCHMARK("End-to-end chat template render + encode") {
        return tokenizer.encodeChat(conversation, true);
    };
}
#endif

} // namespace job::token::test