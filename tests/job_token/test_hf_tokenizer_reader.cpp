#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string_view>

#include "formats/hf_tokenizer_reader.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("HfTokenizerReader loads synthetic BPE tokenizer from memory", "[token][formats][hf][example]")
{
    std::string_view tokenizerJson = R"({
        "version": "1.0",
        "model": {
            "type": "BPE",
            "byte_fallback": true,
            "unk_token": "<unk>",
            "vocab": {
                "<unk>": 0,
                "<s>": 1,
                "</s>": 2,
                "h": 3,
                "e": 4,
                "l": 5,
                "o": 6,
                "he": 7,
                "ll": 8,
                "hello": 9
            },
            "merges": [
                "h e",
                "l l",
                "he ll",
                "hell o"
            ]
        },
        "added_tokens": [
            {"id": 0, "content": "<unk>", "special": true},
            {"id": 1, "content": "<s>", "special": true},
            {"id": 2, "content": "</s>", "special": true}
        ]
    })";

    std::string_view configJson = R"({
        "bos_token": "<s>",
        "eos_token": "</s>",
        "unk_token": "<unk>",
        "chat_template": "{{ bos_token }}{% for m in messages %}{{ m.content }}{% endfor %}"
    })";

    HfTokenizerReader reader;
    bool success = reader.loadFromMemory(tokenizerJson, configJson);

    REQUIRE(success);
    CHECK(reader.modelType() == HfModelType::BPE);
    CHECK(reader.vocabSize() == 10);
    CHECK(reader.data().byteFallback == true);

    // Merges verification
    REQUIRE(reader.data().merges.size() == 4);
    CHECK(reader.data().merges[0].first == "h");
    CHECK(reader.data().merges[0].second == "e");

    // Token query verification
    CHECK(reader.findTokenId("hello") == 9);
    CHECK(reader.findTokenId("<s>") == 1);
    CHECK(reader.findTokenString(1) == "<s>");
    CHECK(reader.findTokenString(9) == "hello");

    // Config metadata
    CHECK(reader.bosToken() == "<s>");
    CHECK(reader.eosToken() == "</s>");
    CHECK(reader.unkToken() == "<unk>");
    CHECK(!reader.chatTemplate().empty());
}

TEST_CASE("HfTokenizerReader loads real disk fixtures (Gemma / Qwen)", "[token][formats][hf][fixtures]")
{
#if defined(JOB_TOKEN_TEST_DATA_DIR)
    const std::filesystem::path baseDir(JOB_TOKEN_TEST_DATA_DIR);

    SECTION("Load Gemma-4-12B-it fixture") {
        const std::filesystem::path gemmaDir = baseDir / "gemma-4-12b-it";

        REQUIRE(std::filesystem::exists(gemmaDir / "tokenizer.json"));
        REQUIRE(std::filesystem::exists(gemmaDir / "tokenizer_config.json"));

        HfTokenizerReader reader;
        const bool ok = reader.loadFromFile(
            gemmaDir / "tokenizer.json",
            gemmaDir / "tokenizer_config.json");

        REQUIRE(ok);
        CHECK(reader.vocabSize() > 0);
        CHECK(reader.modelType() != HfModelType::Unknown);
        CHECK(!reader.chatTemplate().empty());
    }

    SECTION("Load Qwen3.8-27B fixture") {
        const std::filesystem::path qwenDir = baseDir / "Qwen3.8-27B";

        REQUIRE(std::filesystem::exists(qwenDir / "tokenizer.json"));
        REQUIRE(std::filesystem::exists(qwenDir / "tokenizer_config.json"));

        HfTokenizerReader reader;
        const bool ok = reader.loadFromFile(
            qwenDir / "tokenizer.json",
            qwenDir / "tokenizer_config.json");

        REQUIRE(ok);
        CHECK(reader.vocabSize() > 0);
        CHECK(reader.modelType() != HfModelType::Unknown);
    }
#endif
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("HfTokenizerReader handles invalid or malformed JSON payloads", "[token][formats][hf][edge_cases]")
{
    HfTokenizerReader reader;

    SECTION("Empty JSON payload fails gracefully") {
        CHECK_FALSE(reader.loadFromMemory(""));
        CHECK_FALSE(reader.loadFromMemory("   "));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("JSON missing model object returns false") {
        std::string_view badJson = R"({ "version": "1.0", "added_tokens": [] })";
        CHECK_FALSE(reader.loadFromMemory(badJson));
    }

    SECTION("Malformed JSON syntax returns false") {
        std::string_view syntaxError = R"({ "model": { "vocab": { "a": 1, } } })";
        CHECK_FALSE(reader.loadFromMemory(syntaxError));
    }

    SECTION("Missing file path returns false") {
        CHECK_FALSE(reader.loadFromFile("/non/existent/path/tokenizer.json"));
    }

    SECTION("Null values inside model fields do not throw exception") {
        std::string_view nullFieldJson = R"({
            "model": {
                "type": "BPE",
                "unk_token": null,
                "byte_fallback": null,
                "vocab": {
                    "<unk>": 0,
                    "hello": 1
                },
                "merges": []
            }
        })";

        CHECK(reader.loadFromMemory(nullFieldJson));
        CHECK(reader.vocabSize() == 2);
    }

    SECTION("Unigram score-array vocab structure parsing") {
        std::string_view unigramJson = R"({
            "model": {
                "type": "Unigram",
                "vocab": [
                    ["<unk>", 0.0],
                    ["hello", -1.5],
                    ["world", -2.0]
                ]
            }
        })";

        bool ok = reader.loadFromMemory(unigramJson);
        REQUIRE(ok);
        CHECK(reader.modelType() == HfModelType::Unigram);
        CHECK(reader.vocabSize() == 3);
        CHECK(reader.findTokenId("world") == 2);
        CHECK(reader.data().vocab[1].second == -1.5f);
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark HfTokenizerReader parsing throughput", "[token][formats][hf][benchmark]")
{
    std::string syntheticJson = R"({"model":{"type":"BPE","vocab":{)";
    for (int i = 0; i < 5000; ++i) {
        syntheticJson += "\"token_" + std::to_string(i) + "\":" + std::to_string(i);
        if (i < 4999) syntheticJson += ",";
    }
    syntheticJson += R"(},"merges":[)";
    for (int i = 0; i < 2000; ++i) {
        syntheticJson += "\"t " + std::to_string(i) + "\"";
        if (i < 1999) syntheticJson += ",";
    }
    syntheticJson += R"(]}})";

    HfTokenizerReader reader;

    BENCHMARK("Parse 5000-token synthetic HF tokenizer.json") {
        return reader.loadFromMemory(syntheticJson);
    };

    BENCHMARK("Lookup 1000 tokens in loaded HfTokenizerReader") {
        int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            auto id = reader.findTokenId("token_" + std::to_string(i));
            if (id) sum += *id;
        }
        return sum;
    };
}
#endif

} // namespace job::token::test