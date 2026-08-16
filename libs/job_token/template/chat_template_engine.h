#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jobtoken_export.h"
#include "template/chat_message.h"

namespace job::token {

// Forward declarations
namespace ast {
struct BlockNode;
}

enum class ChatTemplateType : uint8_t {
    Raw = 0,
    ChatML,   // <|im_start|>role\ncontent<|im_end|>\n
    LLaMA3,   // <|start_header_id|>role<|end_header_id|>\n\ncontent<|eot_id|>
    Gemma,    // <start_of_turn>role\ncontent<end_of_turn>\n
    Mistral,  // [INST] prompt [/INST] response</s>
    Custom
};

class JOBTOKEN_EXPORT ChatTemplateEngine {
public:
    using Ptr  = std::shared_ptr<ChatTemplateEngine>;
    using UPtr = std::unique_ptr<ChatTemplateEngine>;

    ChatTemplateEngine() = default;
    explicit ChatTemplateEngine(ChatTemplateType type);
    explicit ChatTemplateEngine(std::string customTemplate);

    ~ChatTemplateEngine() = default;

    ChatTemplateEngine(const ChatTemplateEngine&) = default;
    ChatTemplateEngine& operator=(const ChatTemplateEngine&) = default;
    ChatTemplateEngine(ChatTemplateEngine&&) noexcept = default;
    ChatTemplateEngine& operator=(ChatTemplateEngine&&) noexcept = default;

    [[nodiscard]] static UPtr create(ChatTemplateType type = ChatTemplateType::ChatML)
    {
        return std::make_unique<ChatTemplateEngine>(type);
    }

    [[nodiscard]] static UPtr createCustom(std::string customTemplate)
    {
        return std::make_unique<ChatTemplateEngine>(std::move(customTemplate));
    }

    // Ingests Jinja template string (like GGUF tokenizer.chat_template) and detects format / compiles AST
    void setTemplateString(std::string templateStr);

    void setTemplateType(ChatTemplateType type) noexcept
    {
        m_type = type;
    }

    [[nodiscard]] ChatTemplateType templateType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& templateString() const noexcept { return m_templateStr; }
    [[nodiscard]] bool hasCompiledAst() const noexcept { return m_compiledAst != nullptr; }

    // Formats a sequence of chat messages into a single prompt string
    [[nodiscard]] std::string apply(
        std::span<const ChatMessage> messages,
        bool addGenerationPrompt = true,
        std::string_view bosToken = "",
        std::string_view eosToken = "") const;

    // Inspects Jinja template text and determines matching canonical archetype
    [[nodiscard]] static ChatTemplateType detectTemplateType(std::string_view jinjaStr) noexcept;

private:
    void compileTemplate();

    // AST-based evaluation
    [[nodiscard]] std::string formatJinja(
        std::span<const ChatMessage> messages,
        bool addGenPrompt,
        std::string_view bosToken,
        std::string_view eosToken) const;

    // Fast-path / canonical fallbacks
    [[nodiscard]] std::string formatChatML(std::span<const ChatMessage> messages, bool addGenPrompt, std::string_view bosToken) const;
    [[nodiscard]] std::string formatLLaMA3(std::span<const ChatMessage> messages, bool addGenPrompt, std::string_view bosToken) const;
    [[nodiscard]] std::string formatGemma(std::span<const ChatMessage> messages, bool addGenPrompt, std::string_view bosToken) const;
    [[nodiscard]] std::string formatMistral(std::span<const ChatMessage> messages, bool addGenPrompt, std::string_view bosToken, std::string_view eosToken) const;
    [[nodiscard]] std::string formatGenericCustom(std::span<const ChatMessage> messages, bool addGenPrompt, std::string_view bosToken, std::string_view eosToken) const;

private:
    ChatTemplateType m_type{ChatTemplateType::ChatML};
    std::string      m_templateStr;
    std::shared_ptr<const ast::BlockNode> m_compiledAst;
};

} // namespace job::token