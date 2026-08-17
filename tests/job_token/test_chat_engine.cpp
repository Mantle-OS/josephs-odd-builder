#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <string>
#include <string_view>

#include <chat/chat_engine.h>
#include <chat/chat_message.h>
#include <job_token_enums.h>

using job::token::ChatEngine;
using job::token::ChatMessage;
using job::token::ChatRole;
using job::token::ChatType;

//
// Block 1: usage / examples
//

TEST_CASE("ChatEngine defaults to ChatML", "[token][chat][engine][usage]")
{
    ChatEngine engine;

    REQUIRE(engine.type() == ChatType::ChatML);
    REQUIRE(engine.templateString().empty());
    REQUIRE_FALSE(engine.hasCompiledAst());
}

TEST_CASE("ChatEngine constructed with explicit type keeps that type", "[token][chat][engine][usage][type]")
{
    ChatEngine engine{ChatType::LLaMA3};

    REQUIRE(engine.type() == ChatType::LLaMA3);
    REQUIRE(engine.templateString().empty());
    REQUIRE_FALSE(engine.hasCompiledAst());
}

TEST_CASE("ChatEngine detects ChatML template", "[token][chat][engine][detect][chatml]")
{
    REQUIRE(ChatEngine::detectType("<|im_start|>user\nhello<|im_end|>") == ChatType::ChatML);
}

TEST_CASE("ChatEngine detects LLaMA3 template", "[token][chat][engine][detect][llama3]")
{
    REQUIRE(ChatEngine::detectType("<|start_header_id|>user<|end_header_id|>") == ChatType::LLaMA3);
}

TEST_CASE("ChatEngine detects Gemma template", "[token][chat][engine][detect][gemma]")
{
    REQUIRE(ChatEngine::detectType("<start_of_turn>user") == ChatType::Gemma);
}

TEST_CASE("ChatEngine detects Mistral template", "[token][chat][engine][detect][mistral]")
{
    REQUIRE(ChatEngine::detectType("[INST] hello [/INST]") == ChatType::Mistral);
}

TEST_CASE("ChatEngine detects unknown template as Custom", "[token][chat][engine][detect][custom]")
{
    REQUIRE(ChatEngine::detectType("{{ messages }}") == ChatType::Custom);
}

TEST_CASE("ChatEngine detects empty template as ChatML", "[token][chat][engine][detect][edge]")
{
    REQUIRE(ChatEngine::detectType("") == ChatType::ChatML);
}

TEST_CASE("ChatEngine setTemplate stores and detects template", "[token][chat][engine][usage][template]")
{
    ChatEngine engine;

    engine.setTemplate("<|start_header_id|>" "{{ message.role }}" "<|end_header_id|>");

    REQUIRE(engine.type() == ChatType::LLaMA3);
    REQUIRE(engine.templateString() == "<|start_header_id|>" "{{ message.role }}" "<|end_header_id|>");
}

TEST_CASE("ChatEngine formats ChatML conversation", "[token][chat][engine][usage][chatml]")
{
    ChatEngine engine{ChatType::ChatML};

    ChatMessage system;
    system.setRole(ChatRole::System);
    system.setContent("You are helpful.");

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    const std::array<ChatMessage, 2> messages = {system, user};
    const std::string result = engine.apply(messages);

    REQUIRE(result ==
            "<|im_start|>system\n"
            "You are helpful.<|im_end|>\n"
            "<|im_start|>user\n"
            "Hello<|im_end|>\n"
            "<|im_start|>assistant\n");
}

TEST_CASE("ChatEngine ChatML can omit generation prompt", "[token][chat][engine][usage][chatml][generation]")
{
    ChatEngine engine{ChatType::ChatML};

    ChatMessage user;
    user.setContent("Hello");

    const std::array<ChatMessage, 1> messages = {user};
    const std::string result = engine.apply(messages, false);

    REQUIRE(result == "<|im_start|>user\n" "Hello<|im_end|>\n");
}

TEST_CASE("ChatEngine ChatML prepends supplied BOS token", "[token][chat][engine][usage][chatml][bos]")
{
    ChatEngine engine{ChatType::ChatML};

    ChatMessage user;
    user.setContent("Hello");

    const std::array<ChatMessage, 1> messages = {user};
    const std::string result = engine.apply(messages, false, "<BOS>");

    REQUIRE(result == "<BOS>" "<|im_start|>user\n" "Hello<|im_end|>\n");
}

TEST_CASE("ChatEngine formats LLaMA3 conversation", "[token][chat][engine][usage][llama3]")
{
    ChatEngine engine{ChatType::LLaMA3};

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("Hi");

    const std::array<ChatMessage, 2> messages = {user, assistant};
    const std::string result = engine.apply(messages);

    REQUIRE(result ==
            "<|begin_of_text|>"
            "<|start_header_id|>user<|end_header_id|>\n\n"
            "Hello"
            "<|eot_id|>"
            "<|start_header_id|>assistant<|end_header_id|>\n\n"
            "Hi"
            "<|eot_id|>"
            "<|start_header_id|>assistant<|end_header_id|>\n\n");
}

TEST_CASE("ChatEngine LLaMA3 uses supplied BOS instead of default begin token", "[token][chat][engine][usage][llama3][bos]")
{
    ChatEngine engine{ChatType::LLaMA3};

    ChatMessage user;
    user.setContent("Hello");

    const std::array<ChatMessage, 1> messages = {user};
    const std::string result = engine.apply(messages, false, "<CUSTOM_BOS>");

    REQUIRE(result ==
            "<CUSTOM_BOS>"
            "<|start_header_id|>user<|end_header_id|>\n\n"
            "Hello"
            "<|eot_id|>");
}

TEST_CASE("ChatEngine Gemma maps assistant role to model", "[token][chat][engine][usage][gemma]")
{
    ChatEngine engine{ChatType::Gemma};

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("Hi");

    const std::array<ChatMessage, 2> messages = {user, assistant};
    const std::string result = engine.apply(messages);

    REQUIRE(result ==
            "<start_of_turn>user\n"
            "Hello<end_of_turn>\n"
            "<start_of_turn>model\n"
            "Hi<end_of_turn>\n"
            "<start_of_turn>model\n");
}

TEST_CASE("ChatEngine Gemma prepends supplied BOS", "[token][chat][engine][usage][gemma][bos]")
{
    ChatEngine engine{ChatType::Gemma};

    ChatMessage user;
    user.setContent("Hello");

    const std::array<ChatMessage, 1> messages = {user};
    const std::string result = engine.apply(messages, false, "<bos>");

    REQUIRE(result == "<bos>" "<start_of_turn>user\n" "Hello<end_of_turn>\n");
}

TEST_CASE("ChatEngine formats Mistral system user assistant conversation", "[token][chat][engine][usage][mistral]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage system;
    system.setRole(ChatRole::System);
    system.setContent("Follow instructions.");

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("Hi");

    const std::array<ChatMessage, 3> messages = {system, user, assistant};
    const std::string result = engine.apply(messages, false);

    REQUIRE(result == "[INST] " "Follow instructions.\n\n" "Hello" " [/INST]" " Hi" "</s>");
}

TEST_CASE("ChatEngine Mistral uses supplied BOS and EOS tokens", "[token][chat][engine][usage][mistral][special]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("Hi");

    const std::array<ChatMessage, 2> messages = {user, assistant};
    const std::string result = engine.apply(messages, false, "<BOS>", "<EOS>");

    REQUIRE(result == "<BOS>" "[INST] Hello [/INST]" " Hi" "<EOS>");
}

TEST_CASE("ChatEngine Mistral appends generation separator", "[token][chat][engine][usage][mistral][generation]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage user;
    user.setContent("Hello");

    const std::array<ChatMessage, 1> messages = {user};
    const std::string result = engine.apply(messages, true);

    REQUIRE(result == "[INST] Hello [/INST] ");
}

TEST_CASE("ChatEngine formats Raw conversation as content only", "[token][chat][engine][usage][raw]")
{
    ChatEngine engine{ChatType::Raw};

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("one");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("two");

    const std::array<ChatMessage, 2> messages = {user, assistant};

    REQUIRE(engine.apply(messages) == "one\n" "two\n");
}

TEST_CASE("ChatEngine Generic Custom formatter includes roles and generation prompt", "[token][chat][engine][usage][custom]")
{
    ChatEngine engine{ChatType::Custom};

    ChatMessage system;
    system.setRole(ChatRole::System);
    system.setContent("Rules");

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    const std::array<ChatMessage, 2> messages = {system, user};
    const std::string result = engine.apply(messages, true, "<BOS>");

    REQUIRE(result ==
            "<BOS>"
            "### system:\n"
            "Rules\n\n"
            "### user:\n"
            "Hello\n\n"
            "### assistant:\n");
}

TEST_CASE("ChatEngine canonical formatters honor custom chat roles", "[token][chat][engine][usage][custom-role]")
{
    ChatEngine engine{ChatType::ChatML};

    ChatMessage message;
    message.setRole(ChatRole::Custom);
    message.setCustomRole("critic");
    message.setContent("Nope.");

    const std::array<ChatMessage, 1> messages = {message};
    const std::string result = engine.apply(messages, false);

    REQUIRE(result == "<|im_start|>critic\n" "Nope.<|im_end|>\n");
}

TEST_CASE("ChatEngine executes compiled Jinja template", "[token][chat][engine][usage][jinja]")
{
    const std::string templateString = R"({% for message in messages %}{{ message.role }}={{ message.content }};{% endfor %})";

    ChatEngine engine{templateString};

    REQUIRE(engine.type() == ChatType::Custom);
    REQUIRE(engine.templateString() == templateString);
    REQUIRE(engine.hasCompiledAst());

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("hello");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("world");

    const std::array<ChatMessage, 2> messages = {user, assistant};

    REQUIRE(engine.apply(messages) == "user=hello;assistant=world;");
}

TEST_CASE("ChatEngine Jinja exposes generation prompt and special tokens", "[token][chat][engine][usage][jinja][context]")
{
    const std::string templateString = R"({{ bos_token }}|{{ eos_token }}|{{ add_generation_prompt }})";

    ChatEngine engine{templateString};

    REQUIRE(engine.hasCompiledAst());

    const std::array<ChatMessage, 0> messages{};

    REQUIRE(engine.apply(messages, true, "<BOS>", "<EOS>") == "<BOS>|<EOS>|True");
}

TEST_CASE("ChatEngine Jinja exposes message name when present", "[token][chat][engine][usage][jinja][name]")
{
    const std::string templateString = R"({% for message in messages %}{{ message.name }}:{{ message.content }}{% endfor %})";

    ChatEngine engine{templateString};

    REQUIRE(engine.hasCompiledAst());

    ChatMessage message;
    message.setRole(ChatRole::Tool);
    message.setName("calculator");
    message.setContent("42");

    const std::array<ChatMessage, 1> messages = {message};

    REQUIRE(engine.apply(messages) == "calculator:42");
}

TEST_CASE("ChatEngine setTemplate recompiles new Jinja template", "[token][chat][engine][usage][jinja][state]")
{
    ChatEngine engine;

    engine.setTemplate("{{ bos_token }}");

    REQUIRE(engine.type() == ChatType::Custom);
    REQUIRE(engine.hasCompiledAst());

    const std::array<ChatMessage, 0> messages{};

    REQUIRE(engine.apply(messages, false, "FIRST") == "FIRST");

    engine.setTemplate("{{ eos_token }}");

    REQUIRE(engine.type() == ChatType::Custom);
    REQUIRE(engine.hasCompiledAst());
    REQUIRE(engine.apply(messages, false, {}, "SECOND") == "SECOND");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("ChatEngine empty message span produces generation prompt only for ChatML", "[token][chat][engine][edge][chatml]")
{
    ChatEngine engine{ChatType::ChatML};

    const std::array<ChatMessage, 0> messages{};

    REQUIRE(engine.apply(messages) == "<|im_start|>assistant\n");
    REQUIRE(engine.apply(messages, false).empty());
}

TEST_CASE("ChatEngine empty Raw conversation produces empty output", "[token][chat][engine][edge][raw]")
{
    ChatEngine engine{ChatType::Raw};

    const std::array<ChatMessage, 0> messages{};

    REQUIRE(engine.apply(messages).empty());
}

TEST_CASE("ChatEngine empty LLaMA3 conversation still carries begin token", "[token][chat][engine][edge][llama3]")
{
    ChatEngine engine{ChatType::LLaMA3};

    const std::array<ChatMessage, 0> messages{};

    REQUIRE(engine.apply(messages, false) == "<|begin_of_text|>");
}

TEST_CASE("ChatEngine Mistral ignores system-only conversation body", "[token][chat][engine][edge][mistral]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage system;
    system.setRole(ChatRole::System);
    system.setContent("Rules");

    const std::array<ChatMessage, 1> messages = {system};

    REQUIRE(engine.apply(messages, false).empty());
}

TEST_CASE("ChatEngine Mistral uses only first system message", "[token][chat][engine][edge][mistral][system]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage firstSystem;
    firstSystem.setRole(ChatRole::System);
    firstSystem.setContent("FIRST");

    ChatMessage secondSystem;
    secondSystem.setRole(ChatRole::System);
    secondSystem.setContent("SECOND");

    ChatMessage user;
    user.setRole(ChatRole::User);
    user.setContent("Hello");

    const std::array<ChatMessage, 3> messages = {firstSystem, secondSystem, user};
    const std::string result = engine.apply(messages, false);

    REQUIRE(result == "[INST] " "FIRST\n\n" "Hello" " [/INST]");
}

TEST_CASE("ChatEngine Mistral injects system prompt only into first user turn", "[token][chat][engine][edge][mistral][system]")
{
    ChatEngine engine{ChatType::Mistral};

    ChatMessage system;
    system.setRole(ChatRole::System);
    system.setContent("SYSTEM");

    ChatMessage firstUser;
    firstUser.setRole(ChatRole::User);
    firstUser.setContent("ONE");

    ChatMessage assistant;
    assistant.setRole(ChatRole::Assistant);
    assistant.setContent("ANSWER");

    ChatMessage secondUser;
    secondUser.setRole(ChatRole::User);
    secondUser.setContent("TWO");

    const std::array<ChatMessage, 4> messages = {system, firstUser, assistant, secondUser};

    REQUIRE(engine.apply(messages, false) ==
            "[INST] "
            "SYSTEM\n\n"
            "ONE"
            " [/INST]"
            " ANSWER"
            "</s>"
            "[INST] TWO [/INST]");
}

TEST_CASE("ChatEngine malformed template clears compiled AST and falls back by detected type", "[token][chat][engine][edge][jinja][fallback]")
{
    ChatEngine engine;

    engine.setTemplate("{{ definitely_unfinished");

    REQUIRE(engine.type() == ChatType::Custom);
    REQUIRE_FALSE(engine.hasCompiledAst());

    ChatMessage user;
    user.setContent("hello");

    const std::array<ChatMessage, 1> messages = {user};

    REQUIRE(engine.apply(messages, false) == "### user:\n" "hello\n\n");
}

TEST_CASE("ChatEngine malformed recognizable ChatML template falls back to ChatML", "[token][chat][engine][edge][jinja][fallback][chatml]")
{
    ChatEngine engine;

    engine.setTemplate("<|im_start|>{{ broken");

    REQUIRE(engine.type() == ChatType::ChatML);
    REQUIRE_FALSE(engine.hasCompiledAst());

    ChatMessage user;
    user.setContent("hello");

    const std::array<ChatMessage, 1> messages = {user};

    REQUIRE(engine.apply(messages, false) == "<|im_start|>user\n" "hello<|im_end|>\n");
}

TEST_CASE("ChatEngine empty template resets to ChatML without compiled AST", "[token][chat][engine][edge][template]")
{
    ChatEngine engine{std::string{"{{ bos_token }}"}};

    REQUIRE(engine.hasCompiledAst());

    engine.setTemplate({});

    REQUIRE(engine.type() == ChatType::ChatML);
    REQUIRE(engine.templateString().empty());
    REQUIRE_FALSE(engine.hasCompiledAst());
}

TEST_CASE("ChatEngine canonical formatter keeps empty message content", "[token][chat][engine][edge][content]")
{
    ChatEngine engine{ChatType::ChatML};

    ChatMessage message;
    message.setRole(ChatRole::User);
    message.setContent("");

    const std::array<ChatMessage, 1> messages = {message};

    REQUIRE(engine.apply(messages, false) == "<|im_start|>user\n" "<|im_end|>\n");
}

TEST_CASE("ChatEngine invalid ChatType falls back to Raw behavior", "[token][chat][engine][edge][type]")
{
    const auto invalidType = static_cast<ChatType>(255);

    ChatEngine engine{invalidType};

    ChatMessage message;
    message.setContent("hello");

    const std::array<ChatMessage, 1> messages = {message};

    REQUIRE(engine.apply(messages) == "hello\n");
}