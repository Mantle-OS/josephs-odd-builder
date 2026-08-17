#include "chat_engine.h"

#include <utility>

#include "jinja_lexer.h"
#include "jinja_parser.h"
#include "jinja_vm.h"

namespace job::token {

ChatEngine::ChatEngine(ChatType type) :
    m_type{type}
{
}

ChatEngine::ChatEngine(std::string customTemplate) :
    m_type{detectType(customTemplate)},
    m_templateString{std::move(customTemplate)}
{
    compileTemplate();
}

ChatType ChatEngine::detectType(std::string_view jinja) noexcept
{
    if (jinja.empty())
        return ChatType::ChatML;

    if (jinja.find("<|im_start|>") != std::string_view::npos)
        return ChatType::ChatML;

    if (jinja.find("<|start_header_id|>") != std::string_view::npos)
        return ChatType::LLaMA3;

    if (jinja.find("<start_of_turn>") != std::string_view::npos)
        return ChatType::Gemma;

    if (jinja.find("[INST]") != std::string_view::npos)
        return ChatType::Mistral;

    return ChatType::Custom;
}

void ChatEngine::setTemplate(std::string templateString)
{
    m_templateString = std::move(templateString);
    m_type = detectType(m_templateString);
    compileTemplate();
}

void ChatEngine::compileTemplate()
{
    m_compiledAst.reset();

    if (m_templateString.empty())
        return;

    try {
        JinjaLexer lexer{m_templateString};
        const auto tokens = lexer.tokenizeAll();

        JinjaParser parser{tokens};
        m_compiledAst = parser.parse();
    } catch (...) {
        // Compilation failure leaves the AST empty.
        // apply() will fall back to the canonical formatter.
        m_compiledAst.reset();
    }
}

std::string ChatEngine::apply(std::span<const ChatMessage> messages,
                              bool addGenerationPrompt,
                              std::string_view bosToken,
                              std::string_view eosToken) const
{
    if (m_compiledAst) {
        return formatJinja(messages,
                           addGenerationPrompt,
                           bosToken,
                           eosToken);
    }

    switch (m_type) {
    case ChatType::ChatML:
        return formatChatML(messages, addGenerationPrompt, bosToken);
    case ChatType::LLaMA3:
        return formatLLaMA3(messages, addGenerationPrompt, bosToken);
    case ChatType::Gemma:
        return formatGemma(messages, addGenerationPrompt, bosToken);
    case ChatType::Mistral:
        return formatMistral(messages, addGenerationPrompt, bosToken, eosToken);
    case ChatType::Custom:
        return formatGenericCustom(messages, addGenerationPrompt, bosToken, eosToken);

    case ChatType::Raw:
    default: {
        std::string result;

        for (const auto &message : messages) {
            result.append(message.content());
            result.push_back('\n');
        }

        return result;
    }
    }
}

std::string ChatEngine::formatJinja(std::span<const ChatMessage> messages,
                                    bool addGenerationPrompt,
                                    std::string_view bosToken,
                                    std::string_view eosToken) const
{
    if (!m_compiledAst) {
        return formatGenericCustom(messages,
                                   addGenerationPrompt,
                                   bosToken,
                                   eosToken);
    }

    ValueMap context;
    ValueList messageList;
    messageList.reserve(messages.size());

    for (const auto &message : messages) {
        ValueMap messageMap;
        messageMap["role"] = Value{std::string{message.roleName()}};
        messageMap["content"] = Value{message.content()};

        if (!message.name().empty())
            messageMap["name"] = Value{message.name()};

        messageList.emplace_back(std::move(messageMap));
    }

    context["messages"]                 = Value{std::move(messageList)};
    context["add_generation_prompt"]    = Value{addGenerationPrompt};
    context["bos_token"]                = Value{std::string{bosToken}};
    context["eos_token"]                = Value{std::string{eosToken}};

    JinjaVM vm;
    return vm.execute(*m_compiledAst, context);
}

std::string ChatEngine::formatChatML(std::span<const ChatMessage> messages,
                                     bool addGenerationPrompt,
                                     std::string_view bosToken) const
{
    std::string result;

    if (!bosToken.empty())
        result.append(bosToken);

    for (const auto &message : messages) {
        result.append("<|im_start|>");
        result.append(message.roleName());
        result.push_back('\n');
        result.append(message.content());
        result.append("<|im_end|>\n");
    }

    if (addGenerationPrompt)
        result.append("<|im_start|>assistant\n");

    return result;
}

std::string ChatEngine::formatLLaMA3(std::span<const ChatMessage> messages,
                                     bool addGenerationPrompt,
                                     std::string_view bosToken) const
{
    std::string result;

    if (!bosToken.empty())
        result.append(bosToken);
    else
        result.append("<|begin_of_text|>");

    for (const auto &message : messages) {
        result.append("<|start_header_id|>");
        result.append(message.roleName());
        result.append("<|end_header_id|>\n\n");
        result.append(message.content());
        result.append("<|eot_id|>");
    }

    if (addGenerationPrompt) {
        result.append(
            "<|start_header_id|>"
            "assistant"
            "<|end_header_id|>\n\n");
    }

    return result;
}

std::string ChatEngine::formatGemma(std::span<const ChatMessage> messages,
                                    bool addGenerationPrompt,
                                    std::string_view bosToken) const
{
    std::string result;

    if (!bosToken.empty())
        result.append(bosToken);

    for (const auto &message : messages) {
        result.append("<start_of_turn>");

        if (message.role() == ChatRole::Assistant)
            result.append("model");
        else
            result.append(message.roleName());

        result.push_back('\n');
        result.append(message.content());
        result.append("<end_of_turn>\n");
    }

    if (addGenerationPrompt)
        result.append("<start_of_turn>model\n");

    return result;
}

std::string ChatEngine::formatMistral(std::span<const ChatMessage> messages,
                                      bool addGenerationPrompt,
                                      std::string_view bosToken,
                                      std::string_view eosToken) const
{
    std::string result;
    std::string systemPrompt;

    for (const auto &message : messages) {
        if (message.role() == ChatRole::System) {
            systemPrompt = message.content();
            break;
        }
    }

    bool firstUser = true;

    for (const auto &message : messages) {
        if (message.role() == ChatRole::System)
            continue;

        if (message.role() == ChatRole::User) {
            if (!bosToken.empty())
                result.append(bosToken);

            result.append("[INST] ");

            if (firstUser && !systemPrompt.empty()) {
                result.append(systemPrompt);
                result.append("\n\n");
            }

            firstUser = false;

            result.append(message.content());
            result.append(" [/INST]");
        } else if (message.role() == ChatRole::Assistant) {
            result.push_back(' ');
            result.append(message.content());

            if (!eosToken.empty())
                result.append(eosToken);
            else
                result.append("</s>");
        }
    }

    if (addGenerationPrompt)
        result.push_back(' ');

    return result;
}

std::string ChatEngine::formatGenericCustom(std::span<const ChatMessage> messages,
                                            bool addGenerationPrompt,
                                            std::string_view bosToken,
                                            std::string_view /* eosToken */) const
{
    std::string result;
    if (!bosToken.empty())
        result.append(bosToken);

    for (const auto &message : messages) {
        result.append("### ");
        result.append(message.roleName());
        result.append(":\n");
        result.append(message.content());
        result.append("\n\n");
    }

    if (addGenerationPrompt)
        result.append("### assistant:\n");

    return result;
}

} // namespace job::token