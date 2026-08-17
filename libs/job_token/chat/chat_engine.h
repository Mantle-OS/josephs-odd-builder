#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "chat_message.h"
#include "job_token_enums.h"
#include "jinja_ast.h"

#include "jobtoken_export.h"

namespace job::token {

struct BodyNode;

class JOBTOKEN_EXPORT ChatEngine
{
public:
    using Ptr  = std::shared_ptr<ChatEngine>;
    using WPtr = std::weak_ptr<ChatEngine>;
    using UPtr = std::unique_ptr<ChatEngine>;

    ChatEngine() = default;
    explicit ChatEngine(ChatType type);
    explicit ChatEngine(std::string customTemplate);

    ~ChatEngine() = default;

    ChatEngine(const ChatEngine &) = delete;
    ChatEngine &operator=(const ChatEngine &) = delete;
    ChatEngine(ChatEngine &&) = delete;
    ChatEngine &operator=(ChatEngine &&) = delete;

    [[nodiscard]] static UPtr createUniq(ChatType type = ChatType::ChatML)
    {
        return std::make_unique<ChatEngine>(type);
    }

    [[nodiscard]] static Ptr createShared(ChatType type = ChatType::ChatML)
    {
        return std::make_shared<ChatEngine>(type);
    }

    [[nodiscard]] static UPtr createCustomUniq(std::string customTemplate)
    {
        return std::make_unique<ChatEngine>(std::move(customTemplate));
    }

    [[nodiscard]] static Ptr createCustomShared(std::string customTemplate)
    {
        return std::make_shared<ChatEngine>(std::move(customTemplate));
    }

    // Eats Jinja template text, detects its archetype, and compiles it.
    void setTemplate(std::string templateString);

    [[nodiscard]] ChatType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] const std::string &templateString() const noexcept
    {
        return m_templateString;
    }

    [[nodiscard]] bool hasCompiledAst() const noexcept
    {
        return m_compiledAst != nullptr;
    }

    [[nodiscard]] std::string apply(std::span<const ChatMessage> messages,
                                    bool addGenerationPrompt = true,
                                    std::string_view bosToken = {},
                                    std::string_view eosToken = {}) const;

    [[nodiscard]] static ChatType detectType(
        std::string_view jinja) noexcept;

private:
    void compileTemplate();

    [[nodiscard]] std::string formatJinja(std::span<const ChatMessage> messages,
                                          bool addGenerationPrompt,
                                          std::string_view bosToken,
                                          std::string_view eosToken) const;

    [[nodiscard]] std::string formatChatML(std::span<const ChatMessage> messages,
                                           bool addGenerationPrompt,
                                           std::string_view bosToken) const;

    [[nodiscard]] std::string formatLLaMA3(std::span<const ChatMessage> messages,
                                           bool addGenerationPrompt,
                                           std::string_view bosToken) const;

    [[nodiscard]] std::string formatGemma(std::span<const ChatMessage> messages,
                                          bool addGenerationPrompt,
                                          std::string_view bosToken) const;

    [[nodiscard]] std::string formatMistral(std::span<const ChatMessage> messages,
                                            bool addGenerationPrompt,
                                            std::string_view bosToken,
                                            std::string_view eosToken) const;

    [[nodiscard]] std::string formatGenericCustom(std::span<const ChatMessage> messages,
                                                  bool addGenerationPrompt,
                                                  std::string_view bosToken,
                                                  std::string_view eosToken) const;

private:
    ChatType                    m_type{ChatType::ChatML};
    std::string                 m_templateString;
    std::unique_ptr<BodyNode>   m_compiledAst;
};

} // namespace job::token