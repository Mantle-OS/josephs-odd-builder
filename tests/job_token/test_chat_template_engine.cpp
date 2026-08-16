#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "template/chat_message.h"
#include "template/chat_template_engine.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("ChatTemplateEngine formats ChatML and LLaMA3 prompts", "[token][template][engine][example]")
{
    std::vector<ChatMessage> messages = {
        {ChatRole::System, "You are a helpful assistant.", "", ""},
        {ChatRole::User, "Hello!", "", ""}
    };

    SECTION("ChatML prompt rendering") {
        auto engine = ChatTemplateEngine::create(ChatTemplateType::ChatML);
        std::string prompt = engine->apply(messages, true);

        CHECK(prompt ==
              "<|im_start|>system\nYou are a helpful assistant.<|im_end|>\n"
              "<|im_start|>user\nHello!<|im_end|>\n"
              "<|im_start|>assistant\n");
    }

    SECTION("LLaMA-3 prompt rendering") {
        auto engine = ChatTemplateEngine::create(ChatTemplateType::LLaMA3);
        std::string prompt = engine->apply(messages, true, "<|begin_of_text|>");

        CHECK(prompt ==
              "<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n"
              "You are a helpful assistant.<|eot_id|>"
              "<|start_header_id|>user<|end_header_id|>\n\n"
              "Hello!<|eot_id|>"
              "<|start_header_id|>assistant<|end_header_id|>\n\n");
    }
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("ChatTemplateEngine handles empty message lists and custom roles", "[token][template][engine][edge_cases]")
{
    auto engine = ChatTemplateEngine::create(ChatTemplateType::ChatML);

    SECTION("Empty messages only outputs generation prompt if enabled") {
        std::vector<ChatMessage> emptyMessages;
        std::string promptWithGen = engine->apply(emptyMessages, true);
        std::string promptNoGen   = engine->apply(emptyMessages, false);

        CHECK(promptWithGen == "<|im_start|>assistant\n");
        CHECK(promptNoGen.empty());
    }

    SECTION("Custom role name takes precedence") {
        std::vector<ChatMessage> customMessages = {
            {ChatRole::Custom, "Executing tool...", "tool_call", "search_engine"}
        };
        std::string prompt = engine->apply(customMessages, false);

        CHECK(prompt.find("<|im_start|>tool_call\nExecuting tool...<|im_end|>\n") != std::string::npos);
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark ChatTemplateEngine canonical formatter", "[token][template][engine][benchmark]")
{
    std::vector<ChatMessage> messages;
    for (int i = 0; i < 50; ++i) {
        messages.push_back({
            (i % 2 == 0) ? ChatRole::User : ChatRole::Assistant,
            "Standard conversation turns across chat engine evaluation."
        });
    }

    auto engine = ChatTemplateEngine::create(ChatTemplateType::ChatML);

    BENCHMARK("Format 50 messages with ChatML engine") {
        return engine->apply(messages, true);
    };
}
#endif

} // namespace job::token::test